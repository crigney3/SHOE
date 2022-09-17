#include "../Headers/DX12Renderer.h"

DX12Renderer::DX12Renderer(
    unsigned int windowHeight,
    unsigned int windowWidth,
    Microsoft::WRL::ComPtr<ID3D12Device> device,
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain,
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator,
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList
) : Renderer(windowHeight, windowWidth, swapChain) {
    this->windowHeight = windowHeight;
    this->windowWidth = windowWidth;
    this->swapChain = swapChain;
    this->device = device;
    this->commandAllocator = commandAllocator;
    this->commandQueue = commandQueue;
    this->commandList = commandList;

	InitRenderTargetViews();

	this->currentSwapBuffer = 0;
	this->currentBackBuffer = backBuffers[currentSwapBuffer];
}

DX12Renderer::~DX12Renderer() {

}

void DX12Renderer::InitRenderTargetViews() {
	// Create back buffers
	{
		// What is the increment size between RTV descriptors in a
		// descriptor heap?  This differs per GPU so we need to 
		// get it at applications start up
		rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// First create a descriptor heap for RTVs
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = numBackBuffers;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(rtvHeap.GetAddressOf()));

		// Now create the RTV handles for each buffer (buffers were created by the swap chain)
		for (unsigned int i = 0; i < numBackBuffers; i++)
		{
			// Grab this buffer from the swap chain
			swapChain->GetBuffer(i, IID_PPV_ARGS(backBuffers[i].GetAddressOf()));

			// Make a handle for it
			rtvHandles[i] = rtvHeap->GetCPUDescriptorHandleForHeapStart();
			rtvHandles[i].ptr += rtvDescriptorSize * i;

			// Create the render target view
			device->CreateRenderTargetView(backBuffers[i].Get(), 0, rtvHandles[i]);
		}

		currentBackBuffer = backBuffers[currentSwapBuffer];
	}

	// Create depth/stencil buffer
	{
		// Create a descriptor heap for DSV
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(dsvHeap.GetAddressOf()));

		// Describe the depth stencil buffer resource
		D3D12_RESOURCE_DESC depthBufferDesc = {};
		depthBufferDesc.Alignment = 0;
		depthBufferDesc.DepthOrArraySize = 1;
		depthBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthBufferDesc.Height = windowHeight;
		depthBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthBufferDesc.MipLevels = 1;
		depthBufferDesc.SampleDesc.Count = 1;
		depthBufferDesc.SampleDesc.Quality = 0;
		depthBufferDesc.Width = windowWidth;

		// Describe the clear value that will most often be used
		// for this buffer (which optimizes the clearing of the buffer)
		D3D12_CLEAR_VALUE clear = {};
		clear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		clear.DepthStencil.Depth = 1.0f;
		clear.DepthStencil.Stencil = 0;

		// Describe the memory heap that will house this resource
		D3D12_HEAP_PROPERTIES props = {};
		props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		props.CreationNodeMask = 1;
		props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		props.Type = D3D12_HEAP_TYPE_DEFAULT;
		props.VisibleNodeMask = 1;

		// Actually create the resource, and the heap in which it
		// will reside, and map the resource to that heap
		device->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&depthBufferDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clear,
			IID_PPV_ARGS(depthStencilBuffer.GetAddressOf()));

		// Get the handle to the Depth Stencil View that we'll
		// be using for the depth buffer.  The DSV is stored in
		// our DSV-specific descriptor Heap.
		dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();

		// Actually make the DSV
		device->CreateDepthStencilView(
			depthStencilBuffer.Get(),
			0,	// Default view (first mip)
			dsvHandle);
	}
}

void DX12Renderer::PreResize() {
	// Wait for the GPU to finish all work, since we'll
	// be destroying and recreating resources
	dx12Helper.WaitForGPU();

	// Release the back buffers using ComPtr's Reset()
	for (unsigned int i = 0; i < numBackBuffers; i++)
		backBuffers[i].Reset();

	// Resize the swap chain (assuming a basic color format here)
	swapChain->ResizeBuffers(numBackBuffers, windowWidth, windowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

	// Go through the steps to setup the back buffers again
	// Note: This assumes the descriptor heap already exists
	// and that the rtvDescriptorSize was previously set
	for (unsigned int i = 0; i < numBackBuffers; i++)
	{
		// Grab this buffer from the swap chain
		swapChain->GetBuffer(i, IID_PPV_ARGS(backBuffers[i].GetAddressOf()));

		// Make a handle for it
		rtvHandles[i] = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandles[i].ptr += rtvDescriptorSize * i;

		// Create the render target view
		device->CreateRenderTargetView(backBuffers[i].Get(), 0, rtvHandles[i]);
	}

	// Reset back to the first back buffer
	currentSwapBuffer = 0;

	// Reset the depth buffer and create it again
	{
		depthStencilBuffer.Reset();

		// Describe the depth stencil buffer resource
		D3D12_RESOURCE_DESC depthBufferDesc = {};
		depthBufferDesc.Alignment = 0;
		depthBufferDesc.DepthOrArraySize = 1;
		depthBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthBufferDesc.Height = windowHeight;
		depthBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthBufferDesc.MipLevels = 1;
		depthBufferDesc.SampleDesc.Count = 1;
		depthBufferDesc.SampleDesc.Quality = 0;
		depthBufferDesc.Width = windowWidth;

		// Describe the clear value that will most often be used
		// for this buffer (which optimizes the clearing of the buffer)
		D3D12_CLEAR_VALUE clear = {};
		clear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		clear.DepthStencil.Depth = 1.0f;
		clear.DepthStencil.Stencil = 0;

		// Describe the memory heap that will house this resource
		D3D12_HEAP_PROPERTIES props = {};
		props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		props.CreationNodeMask = 1;
		props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		props.Type = D3D12_HEAP_TYPE_DEFAULT;
		props.VisibleNodeMask = 1;

		// Actually create the resource, and the heap in which it
		// will reside, and map the resource to that heap
		device->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&depthBufferDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clear,
			IID_PPV_ARGS(depthStencilBuffer.GetAddressOf()));

		// Now recreate the depth stencil view
		dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		device->CreateDepthStencilView(
			depthStencilBuffer.Get(),
			0,	// Default view (first mip)
			dsvHandle);
	}
}

void DX12Renderer::PostResize(
    unsigned int windowHeight,
    unsigned int windowWidth,
    Microsoft::WRL::ComPtr<ID3D12Resource> backBufferRTV) {

}

void DX12Renderer::InitShadows() {

}

void DX12Renderer::DrawPointLights(std::shared_ptr<Camera> cam) {

}

void DX12Renderer::Draw(std::shared_ptr<Camera> camera, EngineState engineState) {
	// Clearing the render target
	{
		// Transition the back buffer from present to render target
		D3D12_RESOURCE_BARRIER rb = {};
		rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		rb.Transition.pResource = currentBackBuffer.Get();
		rb.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		rb.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &rb);

		// Background color (Cornflower Blue in this case) for clearing
		float color[] = { 0.4f, 0.6f, 0.75f, 1.0f };

		// Clear the RTV
		commandList->ClearRenderTargetView(
			rtvHandles[currentSwapBuffer],
			color,
			0, 0); // No scissor rectangles

		// Clear the depth buffer, too
		commandList->ClearDepthStencilView(
			dsvHandle,
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f,	// Max depth = 1.0f
			0,		// Not clearing stencil, but need a value
			0, 0);	// No scissor rects
	}

	// Rendering here!
	{
		// Set overall pipeline state
		commandList->SetPipelineState(pipelineState.Get());

		// Root sig (must happen before root descriptor table)
		commandList->SetGraphicsRootSignature(rootSignature.Get());

		// Set up other commands for rendering
		commandList->OMSetRenderTargets(1, &rtvHandles[currentSwapBuffer], true, &dsvHandle);
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);
		commandList->IASetVertexBuffers(0, 1, &vbView);
		commandList->IASetIndexBuffer(&ibView);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Draw
		commandList->DrawIndexedInstanced(3, 1, 0, 0, 0);
	}

	// Present
	{
		// Transition back to present
		D3D12_RESOURCE_BARRIER rb = {};
		rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		rb.Transition.pResource = currentBackBuffer.Get();
		rb.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rb.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &rb);

		// Must occur BEFORE present
		DX12Helper::GetInstance().CloseExecuteAndResetCommandList();

		// Present the current back buffer
		swapChain->Present(vsync ? 1 : 0, 0);

		// Figure out which buffer is next
		currentSwapBuffer++;
		if (currentSwapBuffer >= numBackBuffers)
			currentSwapBuffer = 0;

	}
}

void DX12Renderer::CreateRootSignatureAndPipelineState() {
	// Root Signature
	{
		// Define a table of CBV's (constant buffer views)
		D3D12_DESCRIPTOR_RANGE cbvTable = {};
		cbvTable.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvTable.NumDescriptors = 1;
		cbvTable.BaseShaderRegister = 0;
		cbvTable.RegisterSpace = 0;
		cbvTable.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Define the root parameter
		D3D12_ROOT_PARAMETER rootParam = {};
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParam.DescriptorTable.NumDescriptorRanges = 1;
		rootParam.DescriptorTable.pDescriptorRanges = &cbvTable;

		// Describe and serialize the root signature
		D3D12_ROOT_SIGNATURE_DESC rootSig = {};
		rootSig.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSig.NumParameters = 1;
		rootSig.pParameters = &rootParam;
		rootSig.NumStaticSamplers = 0;
		rootSig.pStaticSamplers = 0;

		ID3DBlob* serializedRootSig = 0;
		ID3DBlob* errors = 0;

		D3D12SerializeRootSignature(
			&rootSig,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&serializedRootSig,
			&errors);

		// Check for errors during serialization
		if (errors != 0)
		{
			OutputDebugString((char*)errors->GetBufferPointer());
		}

		// Actually create the root sig
		device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(rootSignature.GetAddressOf()));
	}

	// Pipeline state
	{
		// Describe the pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		// -- Input assembler related ---
		psoDesc.InputLayout.NumElements = inputElementCount;
		psoDesc.InputLayout.pInputElementDescs = inputElements;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		// Overall primitive topology type (triangle, line, etc.) is set here 
		// IASetPrimTop() is still used to set list/strip/adj options
		// See: https://docs.microsoft.com/en-us/windows/desktop/direct3d12/managing-graphics-pipeline-state-in-direct3d-12

		// Root sig
		psoDesc.pRootSignature = rootSignature.Get();

		// -- Shaders (VS/PS) --- 
		psoDesc.VS.pShaderBytecode = vertexShaderByteCode->GetBufferPointer();
		psoDesc.VS.BytecodeLength = vertexShaderByteCode->GetBufferSize();
		psoDesc.PS.pShaderBytecode = pixelShaderByteCode->GetBufferPointer();
		psoDesc.PS.BytecodeLength = pixelShaderByteCode->GetBufferSize();

		// -- Render targets ---
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		// -- States ---
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.DepthClipEnable = true;

		psoDesc.DepthStencilState.DepthEnable = true;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

		psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// -- Misc ---
		psoDesc.SampleMask = 0xffffffff;

		// Create the pipe state object
		device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(pipelineState.GetAddressOf()));
	}
}

void DX12Renderer::RenderShadows() {

}

void DX12Renderer::RenderDepths(std::shared_ptr<Camera> sourceCam, MiscEffectSRVTypes type) {

}

void DX12Renderer::RenderColliders(std::shared_ptr<Camera> cam) {

}

void DX12Renderer::RenderMeshBounds(std::shared_ptr<Camera> cam) {

}

void DX12Renderer::RenderSelectedHighlight(std::shared_ptr<Camera> cam, EngineState engineState) {

}