#include "GameEntity.h"
#include "AssetManager.h"

class Renderer
{
private:
    AssetManager& globalAssets = AssetManager::GetInstance();

    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV;

    std::shared_ptr<Sky> currentSky;
    DirectX::XMFLOAT3 ambientColor;

    //components for shadows
    int shadowSize;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> envShadowSRV;
    std::vector< Microsoft::WRL::ComPtr<ID3D11DepthStencilView>> shadowDepthBuffers;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRasterizer;
    std::shared_ptr<SimpleVertexShader> VSShadow;

    unsigned int windowHeight;
    unsigned int windowWidth;

    //Camera pointer
    std::shared_ptr<Camera> mainCamera;

    //Camera pointers for shadows
    std::shared_ptr<Camera> flashShadowCamera;
    std::shared_ptr<Camera> mainShadowCamera;

public:
    Renderer(
        unsigned int windowHeight,
        unsigned int windowWidth,
        Microsoft::WRL::ComPtr<ID3D11Device> device,
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
        Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain,
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV
    );
    ~Renderer();

    void PostResize(
        unsigned int windowHeight,
        unsigned int windowWidth,
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV
    );

    void DrawPointLights();
    void Draw(std::shared_ptr<Camera> camera);

    void SetActiveSky(std::shared_ptr<Sky> sky);

    void InitShadows();
    void RenderShadows(std::shared_ptr<Camera> shadowCam, int depthBufferIndex);
};