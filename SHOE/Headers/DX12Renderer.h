#pragma once

#include "../Headers/Renderer.h"
#include "../Headers/DX12Helper.h"

class DX12Renderer : public Renderer {
private:
    DX12Helper& dx12Helper = DX12Helper::GetInstance();
    DXCore* dxInstance;

    bool vsync;

    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>			commandQueue;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	commandList;

    static const unsigned int numBackBuffers = 2;
    unsigned int currentSwapBuffer;
    unsigned int rtvDescriptorSize;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap; // Pointers into the RTV desc heap
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;

    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer;

    Microsoft::WRL::ComPtr<ID3D12Resource> backBuffers[numBackBuffers];
    Microsoft::WRL::ComPtr<ID3D12Resource> currentBackBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[numBackBuffers];

    // Overall pipeline and rendering requirements
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;

    void InitRenderTargetViews();

    void CreateRootSignatureAndPipelineState();

public:
    DX12Renderer(
        unsigned int windowHeight,
        unsigned int windowWidth,
        Microsoft::WRL::ComPtr<ID3D12Device> device,
        Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain,
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator,
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList
    );

    ~DX12Renderer();

    void PostResize(
        unsigned int windowHeight,
        unsigned int windowWidth
    );
    void PreResize();
    void InitShadows();

    void DrawPointLights(std::shared_ptr<Camera> cam);
    void Draw(std::shared_ptr<Camera> camera, EngineState engineState);
    void RenderShadows();

    void RenderDepths(std::shared_ptr<Camera> sourceCam, MiscEffectSRVTypes type);
    void RenderColliders(std::shared_ptr<Camera> cam);
    void RenderMeshBounds(std::shared_ptr<Camera> cam);
    void RenderSelectedHighlight(std::shared_ptr<Camera> cam, EngineState engineState);

    HRESULT RenderToVideoFile(std::shared_ptr<Camera> renderCam, FileRenderData RenderParameters);
    HRESULT WriteFrame(Microsoft::WRL::ComPtr<IMFSinkWriter> sinkWriter, DWORD streamIndex, const long long int& timeStamp, FileRenderData* RenderParameters);
    HRESULT InitializeFileSinkWriter(OUT Microsoft::WRL::ComPtr<IMFSinkWriter>* sinkWriterOut, DWORD* pStreamIndex, FileRenderData* RenderParameters);
};