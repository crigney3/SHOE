#include "../Headers/Renderer.h"

class DX12Renderer : public Renderer {
private:

    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>			commandQueue;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	commandList;

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
};