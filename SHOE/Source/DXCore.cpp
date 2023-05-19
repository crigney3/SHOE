#include "../Headers/DXCore.h"

#include <WindowsX.h>
#include <sstream>
#include "../Headers/Time.h"
#include "../Headers/DX12Helper.h"

// Define the static instance variable so our OS-level 
// message handling function below can talk to our object
DXCore* DXCore::DXCoreInstance = 0;

// --------------------------------------------------------
// The global callback function for handling windows OS-level messages.
//
// This needs to be a global function (not part of a class), but we want
// to forward the parameters to our class to properly handle them.
// --------------------------------------------------------
LRESULT DXCore::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DXCoreInstance->ProcessMessage(hWnd, uMsg, wParam, lParam);
}

// --------------------------------------------------------
// Constructor - Set up fields and timer
//
// hInstance	- The application's OS-level handle (unique ID)
// titleBarText - Text for the window's title bar
// windowWidth	- Width of the window's client (internal) area
// windowHeight - Height of the window's client (internal) area
// debugTitleBarStats - Show debug stats in the title bar, like FPS?
// --------------------------------------------------------
DXCore::DXCore(
	HINSTANCE hInstance,		// The application's handle
	const char* titleBarText,	// Text for the window's title bar
	unsigned int windowWidth,	// Width of the window's client area
	unsigned int windowHeight,	// Height of the window's client area
	bool debugTitleBarStats,    // Show extra stats (fps) in title bar?
	DirectXVersion dxVersion)				// Which version of directX is this?
{
	// Save a static reference to this object.
	//  - Since the OS-level message function must be a non-member (global) function, 
	//    it won't be able to directly interact with our DXCore object otherwise.
	//  - (Yes, a singleton might be a safer choice here).
	DXCoreInstance = this;

	// Save params
	this->hInstance = hInstance;
	this->titleBarText = titleBarText;
	this->width = windowWidth;
	this->height = windowHeight;
	this->titleBarStats = debugTitleBarStats;

	// Initialize fields
	this->hasFocus = true; 
	
	this->fpsFrameCount = 0;
	this->fpsTimeElapsed = 0.0f;
	this->currentTime = 0;
	this->deltaTime = 0;
	this->startTime = 0;
	this->totalTime = 0;

	this->dxVersion = dxVersion;

	// Query performance counter for accurate timing information
	__int64 perfFreq;
	QueryPerformanceFrequency((LARGE_INTEGER*)&perfFreq);
	perfCounterSeconds = 1.0 / (double)perfFreq;
}

// --------------------------------------------------------
// Destructor - Clean up (release) all DirectX references
// --------------------------------------------------------
DXCore::~DXCore()
{
	// Note: Since we're using smart pointers (ComPtr),
	// we don't need to explicitly clean up those DirectX objects
	// - If we weren't using smart pointers, we'd need
	//   to call Release() on each DirectX object created in DXCore

	delete& DX12Helper::GetInstance();
}

// --------------------------------------------------------
// Created the actual window for our application
// --------------------------------------------------------
HRESULT DXCore::InitWindow()
{
	// Start window creation by filling out the
	// appropriate window class struct
	WNDCLASS wndClass		= {}; // Zero out the memory
	wndClass.style			= CS_HREDRAW | CS_VREDRAW;	// Redraw on horizontal or vertical movement/adjustment
	wndClass.lpfnWndProc	= DXCore::WindowProc;
	wndClass.cbClsExtra		= 0;
	wndClass.cbWndExtra		= 0;
	wndClass.hInstance		= hInstance;						// Our app's handle
	wndClass.hIcon			= LoadIcon(NULL, IDI_APPLICATION);	// Default icon
	wndClass.hCursor		= LoadCursor(NULL, IDC_ARROW);		// Default arrow cursor
	wndClass.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName	= NULL;
	wndClass.lpszClassName	= "Direct3DWindowClass";

	// Attempt to register the window class we've defined
	if (!RegisterClass(&wndClass))
	{
		// Get the most recent error
		DWORD error = GetLastError();

		// If the class exists, that's actually fine.  Otherwise,
		// we can't proceed with the next step.
		if (error != ERROR_CLASS_ALREADY_EXISTS)
			return HRESULT_FROM_WIN32(error);
	}

	// Adjust the width and height so the "client size" matches
	// the width and height given (the inner-area of the window)
	RECT clientRect;
	SetRect(&clientRect, 0, 0, width, height);
	AdjustWindowRect(
		&clientRect,
		WS_OVERLAPPEDWINDOW,	// Has a title bar, border, min and max buttons, etc.
		false);					// No menu bar

	// Center the window to the screen
	RECT desktopRect;
	GetClientRect(GetDesktopWindow(), &desktopRect);
	int centeredX = (desktopRect.right / 2) - (clientRect.right / 2);
	int centeredY = (desktopRect.bottom / 2) - (clientRect.bottom / 2);

	// Actually ask Windows to create the window itself
	// using our settings so far.  This will return the
	// handle of the window, which we'll keep around for later
	hWnd = CreateWindow(
		wndClass.lpszClassName,
		titleBarText.c_str(),
		WS_OVERLAPPEDWINDOW,
		centeredX,
		centeredY,
		clientRect.right - clientRect.left,	// Calculated width
		clientRect.bottom - clientRect.top,	// Calculated height
		0,			// No parent window
		0,			// No menu
		hInstance,	// The app's handle
		0);			// No other windows in our application

	// Ensure the window was created properly
	if (hWnd == NULL)
	{
		DWORD error = GetLastError();
		return HRESULT_FROM_WIN32(error);
	}

	// The window exists but is not visible yet
	// We need to tell Windows to show it, and how to show it
	ShowWindow(hWnd, SW_SHOW);

	// Return an "everything is ok" HRESULT value
	return S_OK;
}


// --------------------------------------------------------
// Initializes DirectX11, which requires a window.  This method
// also creates several DirectX objects we'll need to start
// drawing things to the screen.
// --------------------------------------------------------
HRESULT DXCore::InitDirectX11()
{
	// This will hold options for DirectX initialization
	unsigned int deviceFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
	// If we're in debug mode in visual studio, we also
	// want to make a "Debug DirectX Device" to see some
	// errors and warnings in Visual Studio's output window
	// when things go wrong!
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// Create a description of how our swap
	// chain should work
	DXGI_SWAP_CHAIN_DESC swapDesc = {};
	swapDesc.BufferCount = 2;
	swapDesc.BufferDesc.Width = width;
	swapDesc.BufferDesc.Height = height;
	swapDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.Flags = 0;
	swapDesc.OutputWindow = hWnd;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.SampleDesc.Quality = 0;
	swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapDesc.Windowed = true;

	// Result variable for below function calls
	HRESULT hr = S_OK;

	// Attempt to initialize DirectX
	hr = D3D11CreateDeviceAndSwapChain(
		0,							// Video adapter (physical GPU) to use, or null for default
		D3D_DRIVER_TYPE_HARDWARE,	// We want to use the hardware (GPU)
		0,							// Used when doing software rendering
		deviceFlags,				// Any special options
		0,							// Optional array of possible verisons we want as fallbacks
		0,							// The number of fallbacks in the above param
		D3D11_SDK_VERSION,			// Current version of the SDK
		&swapDesc,					// Address of swap chain options
		swapChain.GetAddressOf(),	// Pointer to our Swap Chain pointer
		device.GetAddressOf(),		// Pointer to our Device pointer
		&dxFeatureLevel,			// This will hold the actual feature level the app will use
		context.GetAddressOf());	// Pointer to our Device Context pointer
	if (FAILED(hr)) return hr;

	// The above function created the back buffer render target
	// for us, but we need a reference to it
	ID3D11Texture2D* backBufferTexture = 0;
	swapChain->GetBuffer(
		0,
		__uuidof(ID3D11Texture2D),
		(void**)&backBufferTexture);

	// Now that we have the texture, create a render target view
	// for the back buffer so we can render into it.  Then release
	// our local reference to the texture, since we have the view.
	if (backBufferTexture != 0)
	{
		device->CreateRenderTargetView(
			backBufferTexture,
			0,
			backBufferRTV.GetAddressOf());
		backBufferTexture->Release();
	}

	// Set up the description of the texture to use for the depth buffer
	D3D11_TEXTURE2D_DESC depthStencilDesc = {};
	depthStencilDesc.Width				= width;
	depthStencilDesc.Height				= height;
	depthStencilDesc.MipLevels			= 1;
	depthStencilDesc.ArraySize			= 1;
	depthStencilDesc.Format				= DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.Usage				= D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags			= D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags		= 0;
	depthStencilDesc.MiscFlags			= 0;
	depthStencilDesc.SampleDesc.Count	= 1;
	depthStencilDesc.SampleDesc.Quality = 0;

	// Create the depth buffer and its view, then 
	// release our reference to the texture
	ID3D11Texture2D* depthBufferTexture = 0;
	device->CreateTexture2D(&depthStencilDesc, 0, &depthBufferTexture);
	if (depthBufferTexture != 0)
	{
		device->CreateDepthStencilView(
			depthBufferTexture, 
			0, 
			depthStencilView.GetAddressOf());
		depthBufferTexture->Release();
	}

	// Bind the views to the pipeline, so rendering properly 
	// uses their underlying textures
	context->OMSetRenderTargets(
		1, 
		backBufferRTV.GetAddressOf(), 
		depthStencilView.Get());

	// Lastly, set up a viewport so we render into
	// to correct portion of the window
	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX	= 0;
	viewport.TopLeftY	= 0;
	viewport.Width		= (float)width;
	viewport.Height		= (float)height;
	viewport.MinDepth	= 0.0f;
	viewport.MaxDepth	= 1.0f;
	context->RSSetViewports(1, &viewport);

	// Return the "everything is ok" HRESULT value
	return S_OK;
}

// --------------------------------------------------------
// Initializes DirectX11, which requires a window.  This method
// also creates several DirectX objects we'll need to start
// drawing things to the screen.
// --------------------------------------------------------
HRESULT DXCore::InitDirectX12()
{
#if defined(DEBUG) || defined(_DEBUG)
	// If we're in debug mode in visual studio, we also
	// want to enable the DX12 debug layer to see some
	// errors and warnings in Visual Studio's output window
	// when things go wrong!
	ID3D12Debug* debugController;
	D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
	debugController->EnableDebugLayer();
#endif

	// Result variable for below function calls
	HRESULT hr = S_OK;

	// Create the DX 12 device and check which feature level
	// we can reliably use in our application
	{
		hr = D3D12CreateDevice(
			0,						// Not explicitly specifying which adapter (GPU)
			D3D_FEATURE_LEVEL_11_0,	// MINIMUM feature level - NOT the level we'll necessarily turn on
			IID_PPV_ARGS(deviceDX12.GetAddressOf()));	// Macro to grab necessary IDs of device
		if (FAILED(hr)) return hr;

		// Now that we have a device, determine the maximum
		// feature level supported by the device
		D3D_FEATURE_LEVEL levelsToCheck[] = {
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_12_1
		};
		D3D12_FEATURE_DATA_FEATURE_LEVELS levels = {};
		levels.pFeatureLevelsRequested = levelsToCheck;
		levels.NumFeatureLevels = ARRAYSIZE(levelsToCheck);
		deviceDX12->CheckFeatureSupport(
			D3D12_FEATURE_FEATURE_LEVELS,
			&levels,
			sizeof(D3D12_FEATURE_DATA_FEATURE_LEVELS));
		dxFeatureLevel = levels.MaxSupportedFeatureLevel;
	}

	// Set up DX12 command allocator / queue / list, 
	// which are necessary pieces for issuing standard API calls
	{
		// Set up allocator
		deviceDX12->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(commandAllocator.GetAddressOf()));

		// Command queue
		D3D12_COMMAND_QUEUE_DESC qDesc = {};
		qDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		qDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		deviceDX12->CreateCommandQueue(&qDesc, IID_PPV_ARGS(commandQueue.GetAddressOf()));

		// Command list
		deviceDX12->CreateCommandList(
			0,								// Which physical GPU will handle these tasks?  0 for single GPU setup
			D3D12_COMMAND_LIST_TYPE_DIRECT,	// Type of command list - direct is for standard API calls
			commandAllocator.Get(),			// The allocator for this list
			0,								// Initial pipeline state - none for now
			IID_PPV_ARGS(commandList.GetAddressOf()));
	}

	// Now that we have a device and a command list,
	// we can initialize the DX12 helper singleton, which will
	// also create a fence for synchronization
	{
		DX12Helper::GetInstance().Initialize(
			deviceDX12,
			commandList,
			commandQueue,
			commandAllocator);
	}

	// Swap chain creation
	{
		// Create a description of how our swap chain should work
		DXGI_SWAP_CHAIN_DESC swapDesc = {};
		swapDesc.BufferCount = numBackBuffers;
		swapDesc.BufferDesc.Width = width;
		swapDesc.BufferDesc.Height = height;
		swapDesc.BufferDesc.RefreshRate.Numerator = 60;
		swapDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapDesc.Flags = 0; // DX12: Do we need DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH?
		swapDesc.OutputWindow = hWnd;
		swapDesc.SampleDesc.Count = 1;
		swapDesc.SampleDesc.Quality = 0;
		swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapDesc.Windowed = true;

		// Create a DXGI factory, which is what we use to create a swap chain
		Microsoft::WRL::ComPtr<IDXGIFactory> dxgiFactory;
		CreateDXGIFactory(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));
		hr = dxgiFactory->CreateSwapChain(commandQueue.Get(), &swapDesc, swapChain.GetAddressOf());
	}

	// Initialize a fence event so manual GPU schronization is possible - already done in DXHelper?
	/*device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf()));
	fenceEvent = CreateEventEx(0, 0, 0, EVENT_ALL_ACCESS);*/

	// Set up the viewport so we render into the correct
	// portion of the render target
	viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = (float)width;
	viewport.Height = (float)height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// Define a scissor rectangle that defines a portion of
	// the render target for clipping.  This is different from
	// a viewport in that it is applied after the pixel shader.
	// We need at least one of these, but we're rendering to 
	// the entire window, so it'll be the same size.
	scissorRect = {};
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = width;
	scissorRect.bottom = height;

	// Wait for the GPU before we proceed
	DX12Helper::GetInstance().WaitForGPU();

	// Return the "everything is ok" HRESULT value
	return S_OK;
}

// --------------------------------------------------------
// When the window is resized, the underlying 
// buffers (textures) must also be resized to match.
//
// If we don't do this, the window size and our rendering
// resolution won't match up.  This can result in odd
// stretching/skewing.
// --------------------------------------------------------
void DXCore::OnResize()
{
	// Determine which resources need to be reset
	if (dxVersion) {
		// SHOE is running DX12, resize appropriately
		DX12Helper& dx12Helper = DX12Helper::GetInstance();

		// Recreate the viewport and scissor rects, too,
		// since the window size has changed
		{
			// Set up the viewport so we render into the correct
			// portion of the render target
			viewport = {};
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width = (float)width;
			viewport.Height = (float)height;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;

			// Define a scissor rectangle that defines a portion of
			// the render target for clipping.  This is different from
			// a viewport in that it is applied after the pixel shader.
			// We need at least one of these, but we're rendering to 
			// the entire window, so it'll be the same size.
			scissorRect = {};
			scissorRect.left = 0;
			scissorRect.top = 0;
			scissorRect.right = width;
			scissorRect.bottom = height;
		}

		// Wait for the GPU before we proceed
		dx12Helper.WaitForGPU();
	}
	else {
		// SHOE is running DX11, resize appropriately

		// Release the buffers before resizing the swap chain
		backBufferRTV.Reset();
		depthStencilView.Reset();

		// Resize the underlying swap chain buffers
		swapChain->ResizeBuffers(
			2,
			width,
			height,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			0);

		// Recreate the render target view for the back buffer
		// texture, then release our local texture reference
		ID3D11Texture2D* backBufferTexture = 0;
		swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBufferTexture));
		if (backBufferTexture != 0)
		{
			device->CreateRenderTargetView(
				backBufferTexture,
				0,
				backBufferRTV.ReleaseAndGetAddressOf()); // ReleaseAndGetAddressOf() cleans up the old object before giving us the pointer
			backBufferTexture->Release();
		}

		// Set up the description of the texture to use for the depth buffer
		D3D11_TEXTURE2D_DESC depthStencilDesc;
		depthStencilDesc.Width = width;
		depthStencilDesc.Height = height;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;

		// Create the depth buffer and its view, then 
		// release our reference to the texture
		ID3D11Texture2D* depthBufferTexture = 0;
		device->CreateTexture2D(&depthStencilDesc, 0, &depthBufferTexture);
		if (depthBufferTexture != 0)
		{
			device->CreateDepthStencilView(
				depthBufferTexture,
				0,
				depthStencilView.ReleaseAndGetAddressOf()); // ReleaseAndGetAddressOf() cleans up the old object before giving us the pointer
			depthBufferTexture->Release();
		}

		// Bind the views to the pipeline, so rendering properly 
		// uses their underlying textures
		context->OMSetRenderTargets(
			1,
			backBufferRTV.GetAddressOf(), // This requires a pointer to a pointer (an array of pointers), so we get the address of the pointer
			depthStencilView.Get());

		// Lastly, set up a viewport so we render into
		// to correct portion of the window
		D3D11_VIEWPORT viewport = {};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = (float)width;
		viewport.Height = (float)height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		context->RSSetViewports(1, &viewport);
	}
	
}


// --------------------------------------------------------
// This is the main game loop, handling the following:
//  - OS-level messages coming in from Windows itself
//  - Calling update & draw back and forth, forever
// --------------------------------------------------------
HRESULT DXCore::Run()
{
	// Grab the start time now that
	// the game loop is running
	__int64 now;
	QueryPerformanceCounter((LARGE_INTEGER*)&now);
	startTime = now;
	currentTime = now;
	previousTime = now;

	// Give subclass a chance to initialize
	Init();

	// Our overall game and message loop
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		// Determine if there is a message waiting
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			// Translate and dispatch the message
			// to our custom WindowProc function
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// Update timer and title bar (if necessary)
			UpdateTimer();
			if (titleBarStats)
				UpdateTitleBarStats();

			// Update the input manager
			Input::GetInstance().Update();

			// The game loop
			Update();
			Draw();

			// Frame is over, notify the input manager
			Input::GetInstance().EndOfFrame();
		}
	}

	// We'll end up here once we get a WM_QUIT message,
	// which usually comes from the user closing the window
	return (HRESULT)msg.wParam;
}


// --------------------------------------------------------
// Sends an OS-level window close message to our process, which
// will be handled by our message processing function
// --------------------------------------------------------
void DXCore::Quit()
{
	PostMessage(this->hWnd, WM_CLOSE, NULL, NULL);
}

// This is so cursed. Visual Studio why
void DXCore::SetBuildAssetPaths() {
	assetPathStrings[0] = "..\\..\\..\\Assets\\Models\\";
	assetPathStrings[1] = "..\\..\\..\\Assets\\Scenes\\";
	assetPathStrings[2] = "..\\..\\..\\Assets\\HeightMaps\\";
	assetPathStrings[3] = "..\\..\\..\\Assets\\Fonts\\";
	assetPathStrings[4] = "..\\..\\..\\Assets\\Particles\\";
	assetPathStrings[5] = "..\\..\\..\\Assets\\Sounds\\";
	assetPathStrings[6] = "..\\..\\..\\Assets\\Textures\\";
	assetPathStrings[7] = "..\\..\\..\\Assets\\Textures\\Skies\\";
	assetPathStrings[8] = "..\\..\\..\\Assets\\PBR\\";
	assetPathStrings[9] = "..\\..\\..\\Assets\\PBR\\Albedo\\";
	assetPathStrings[10] = "..\\..\\..\\Assets\\PBR\\Normals\\";
	assetPathStrings[11] = "..\\..\\..\\Assets\\PBR\\Metalness\\";
	assetPathStrings[12] = "..\\..\\..\\Assets\\PBR\\Roughness\\";
	assetPathStrings[13] = "..\\..\\..\\Assets\\Shaders\\";
}

void DXCore::SetVSAssetPaths() {
	assetPathStrings[0] = "..\\Assets\\Models\\";
	assetPathStrings[1] = "..\\Assets\\Scenes\\";
	assetPathStrings[2] = "..\\Assets\\HeightMaps\\";
	assetPathStrings[3] = "..\\Assets\\Fonts\\";
	assetPathStrings[4] = "..\\Assets\\Particles\\";
	assetPathStrings[5] = "..\\Assets\\Sounds\\";
	assetPathStrings[6] = "..\\Assets\\Textures\\";
	assetPathStrings[7] = "..\\Assets\\Textures\\Skies\\";
	assetPathStrings[8] = "..\\Assets\\PBR\\";
	assetPathStrings[9] = "..\\Assets\\PBR\\Albedo\\";
	assetPathStrings[10] = "..\\Assets\\PBR\\Normals\\";
	assetPathStrings[11] = "..\\Assets\\PBR\\Metalness\\";
	assetPathStrings[12] = "..\\Assets\\PBR\\Roughness\\";
	assetPathStrings[13] = "..\\Assets\\Shaders\\";
}

std::string DXCore::GetAssetPathString(AssetPathIndex index) {
	return assetPathStrings[index];
}

// --------------------------------------------------------
// Uses high resolution time stamps to get very accurate
// timing information, and calculates useful time stats
// --------------------------------------------------------
void DXCore::UpdateTimer()
{
	// Grab the current time
	__int64 now;
	QueryPerformanceCounter((LARGE_INTEGER*)&now);
	currentTime = now;

	// Calculate delta time and clamp to zero
	//  - Could go negative if CPU goes into power save mode 
	//    or the process itself gets moved to another core
	deltaTime = max((float)((currentTime - previousTime) * perfCounterSeconds), 0.0f);

	// Calculate the total time from start to now
	totalTime = (float)((currentTime - startTime) * perfCounterSeconds);

	Time::currentTime = currentTime;
	Time::deltaTime = deltaTime;
	Time::totalTime = totalTime;

	// Save current time for next frame
	previousTime = currentTime;
}


// --------------------------------------------------------
// Updates the window's title bar with several stats once
// per second, including:
//  - The window's width & height
//  - The current FPS and ms/frame
//  - The version of DirectX actually being used (usually 11)
// --------------------------------------------------------
void DXCore::UpdateTitleBarStats()
{
	fpsFrameCount++;

	// Only calc FPS and update title bar once per second
	float timeDiff = totalTime - fpsTimeElapsed;
	if (timeDiff < 1.0f)
		return;

	// How long did each frame take?  (Approx)
	float mspf = 1000.0f / (float)fpsFrameCount;

	// Quick and dirty title bar text (mostly for debugging)
	std::ostringstream output;
	output.precision(6);
	output << titleBarText <<
		"    Width: "		<< width <<
		"    Height: "		<< height <<
		"    FPS: "			<< fpsFrameCount <<
		"    Frame Time: "	<< mspf << "ms";

	// Append the version of DirectX the app is using
	switch (dxFeatureLevel)
	{
	case D3D_FEATURE_LEVEL_12_1: output << "    DX 12.1"; break;
	case D3D_FEATURE_LEVEL_12_0: output << "    DX 12.0"; break;
	case D3D_FEATURE_LEVEL_11_1: output << "    DX 11.1"; break;
	case D3D_FEATURE_LEVEL_11_0: output << "    DX 11.0"; break;
	case D3D_FEATURE_LEVEL_10_1: output << "    DX 10.1"; break;
	case D3D_FEATURE_LEVEL_10_0: output << "    DX 10.0"; break;
	case D3D_FEATURE_LEVEL_9_3:  output << "    DX 9.3";  break;
	case D3D_FEATURE_LEVEL_9_2:  output << "    DX 9.2";  break;
	case D3D_FEATURE_LEVEL_9_1:  output << "    DX 9.1";  break;
	default:                     output << "    DX ???";  break;
	}

	// Actually update the title bar and reset fps data
	SetWindowText(hWnd, output.str().c_str());
	fpsFrameCount = 0;
	fpsTimeElapsed += 1.0f;
}

// --------------------------------------------------------
// Allocates a console window we can print to for debugging
// 
// bufferLines   - Number of lines in the overall console buffer
// bufferColumns - Numbers of columns in the overall console buffer
// windowLines   - Number of lines visible at once in the window
// windowColumns - Number of columns visible at once in the window
// --------------------------------------------------------
void DXCore::CreateConsoleWindow(int bufferLines, int bufferColumns, int windowLines, int windowColumns)
{
	// Our temp console info struct
	CONSOLE_SCREEN_BUFFER_INFO coninfo;

	// Get the console info and set the number of lines
	AllocConsole();
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = bufferLines;
	coninfo.dwSize.X = bufferColumns;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

	SMALL_RECT rect;
	rect.Left = 0;
	rect.Top = 0;
	rect.Right = windowColumns;
	rect.Bottom = windowLines;
	SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &rect);

	FILE *stream;
	freopen_s(&stream, "CONIN$", "r", stdin);
	freopen_s(&stream, "CONOUT$", "w", stdout);
	freopen_s(&stream, "CONOUT$", "w", stderr);

	// Prevent accidental console window close
	HWND consoleHandle = GetConsoleWindow();
	HMENU hmenu = GetSystemMenu(consoleHandle, FALSE);
	EnableMenuItem(hmenu, SC_CLOSE, MF_GRAYED);
}

// --------------------------------------------------------------------------
// Gets the actual path to this executable
//
// - As it turns out, the relative path for a program is different when 
//    running through VS and when running the .exe directly, which makes 
//    it a pain to properly load files external files (like textures)
//    - Running through VS: Current Dir is the *project folder*
//    - Running from .exe:  Current Dir is the .exe's folder
// - This has nothing to do with DEBUG and RELEASE modes - it's purely a 
//    Visual Studio "thing", and isn't obvious unless you know to look 
//    for it.  In fact, it could be fixed by changing a setting in VS, but
//    the option is stored in a user file (.suo), which is ignored by most
//    version control packages by default.  Meaning: the option must be
//    changed every on every PC.  Ugh.  So instead, here's a helper.
// --------------------------------------------------------------------------
std::string DXCore::GetExePath()
{
	// Assume the path is just the "current directory" for now
	std::string path = ".\\";

	// Get the real, full path to this executable
	char currentDir[1024] = {};
	GetModuleFileName(0, currentDir, 1024);

	// Find the location of the last slash charaacter
	char* lastSlash = strrchr(currentDir, '\\');
	if (lastSlash)
	{
		// End the string at the last slash character, essentially
		// chopping off the exe's file name.  Remember, c-strings
		// are null-terminated, so putting a "zero" character in 
		// there simply denotes the end of the string.
		*lastSlash = 0;
		
		// Set the remainder as the path
		path = currentDir;
	}

	// Toss back whatever we've found
	return path;
}


// ---------------------------------------------------
//  Same as GetExePath(), except it returns a wide character
//  string, which most of the Windows API requires.
// ---------------------------------------------------
std::wstring DXCore::GetExePath_Wide()
{
	// Grab the path as a standard string
	std::string path = GetExePath();

	// Convert to a wide string
	wchar_t widePath[1024] = {};
	mbstowcs_s(0, widePath, path.c_str(), 1024);

	// Create a wstring for it and return
	return std::wstring(widePath);
}


// ---------------------------------------------------
// Gets the internal total time. Should not be used for
// game updates. Currently used during initialization
// for debug purposes.
// ---------------------------------------------------
float DXCore::GetTotalTime() {
	UpdateTimer();
	return this->totalTime;
}


// ---------------------------------------------------
// Gets the internal delta time. Should not be used for
// game updates. Currently used during initialization
// for debug purposes.
// ---------------------------------------------------
float DXCore::GetDeltaTime() {
	UpdateTimer();
	return this->deltaTime;
}


// ----------------------------------------------------
//  Gets the full path to a given file.  NOTE: This does 
//  NOT "find" the file, it simply concatenates the given
//  relative file path onto the executable's path
// ----------------------------------------------------
std::string DXCore::GetFullPathTo(std::string relativeFilePath)
{
	return GetExePath() + "\\" + relativeFilePath;
}



// ----------------------------------------------------
//  Same as GetFullPathTo, but with wide char strings.
// 
//  Gets the full path to a given file.  NOTE: This does 
//  NOT "find" the file, it simply concatenates the given
//  relative file path onto the executable's path
// ----------------------------------------------------
std::wstring DXCore::GetFullPathTo_Wide(std::wstring relativeFilePath)
{
	return GetExePath_Wide() + L"\\" + relativeFilePath;
}





// --------------------------------------------------------
// Handles messages that are sent to our window by the
// operating system.  Ignoring these messages would cause
// our program to hang and Windows would think it was
// unresponsive.
// --------------------------------------------------------
LRESULT DXCore::ProcessMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Check the incoming message and handle any we care about
	switch (uMsg)
	{
	// This is the message that signifies the window closing
	case WM_DESTROY:
		PostQuitMessage(0); // Send a quit message to our own program
		return 0;

	// Prevent beeping when we "alt-enter" into fullscreen
	case WM_MENUCHAR: 
		return MAKELRESULT(0, MNC_CLOSE);

	// Prevent the overall window from becoming too small
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	// Sent when the window size changes
	case WM_SIZE:
		// Don't adjust anything when minimizing,
		// since we end up with a width/height of zero
		// and that doesn't play well with the GPU
		if (wParam == SIZE_MINIMIZED)
			return 0;
		
		// Save the new client area dimensions.
		width = LOWORD(lParam);
		height = HIWORD(lParam);

		// If DX is initialized, resize 
		// our required buffers
		if (device) 
			OnResize();

		return 0;

	// Has the mouse wheel been scrolled?
	case WM_MOUSEWHEEL:
		Input::GetInstance().SetWheelDelta(GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
		return 0;

	// Sent when a key is pressed
	case WM_CHAR:
		ImGui::GetIO().AddInputCharacter((char)wParam);
		return 0;
	
	// Is our focus state changing?
	case WM_SETFOCUS:	hasFocus = true;	return 0;
	case WM_KILLFOCUS:	hasFocus = false;	return 0;
	case WM_ACTIVATE:	hasFocus = (LOWORD(wParam) != WA_INACTIVE); return 0;
	}

	// Let Windows handle any messages we're not touching
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


bool DXCore::IsDirectX12() {
	if (dxVersion == DirectXVersion::DIRECT_X_11) {
		return 0;
	}
	else {
		return 1;
	}
}

