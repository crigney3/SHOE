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
    this->device = device;
    this->commandAllocator = commandAllocator;
    this->commandQueue = commandQueue;
    this->commandList = commandList;
}

DX12Renderer::~DX12Renderer() {

}

