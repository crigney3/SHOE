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

    // SSAO and MRT render target views
    // Stored in the following order:
    // 0 - Color minus ambient
    // 1 - Only ambient
    // 2 - Only normals
    // 3 - Only depths
    // 4 - Results of SSAO
    // 5 - SSAO with blur fix
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>      ssaoRTVs[6] = {};
    Microsoft::WRL::ComPtr<ID3D11Texture2D>             ssaoTexture2D[6] = {};
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    ssaoSRV[6] = {};

    // Offset and random values for SSAO blur and texture
    Microsoft::WRL::ComPtr<ID3D11Texture2D> ssaoRandomTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ssaoRandomSRV;

    // Ambient Occlusion data
    DirectX::XMFLOAT4 ssaoOffsets[64];

    unsigned int windowHeight;
    unsigned int windowWidth;

    //Camera pointer
    std::shared_ptr<Camera> mainCamera;

    //Camera pointers for shadows
    std::shared_ptr<Camera> flashShadowCamera;
    std::shared_ptr<Camera> mainShadowCamera;

    void InitRenderTargetViews();

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