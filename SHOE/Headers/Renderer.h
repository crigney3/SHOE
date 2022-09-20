#pragma once

#include "AssetManager.h"
#include "CollisionManager.h"

// Effects that require multiple render target views
// are stored in the following order:
// 0 - Color minus ambient
// 1 - Only ambient
// 2 - Only normals
// 3 - Only depths
// 4 - Results of SSAO
// 5 - SSAO with blur fix
// 6 - Refraction Silhouette Render
// 7 - Render of pre-transparency composite
// 8 - Render of post-transparency composite
// 9 - Count: always the last one, tracks size
enum RTVTypes 
{
    COLORS_NO_AMBIENT,
    COLORS_AMBIENT,
    NORMALS,
    DEPTHS,
    SSAO_RAW,
    SSAO_BLUR,
    REFRACTION_SILHOUETTE,
    COMPOSITE,
    FINAL_COMPOSITE,

    RTV_TYPE_COUNT
};

enum MiscEffectSRVTypes
{
    REFRACTION_SILHOUETTE_DEPTHS,
    TRANSPARENT_PREPASS_DEPTHS,
    RENDER_PREPASS_DEPTHS,

    MISC_EFFECT_SRV_COUNT
};

// These need to match the expected per-frame/object/material vertex shader data
struct VSPerFrameData
{
    DirectX::XMFLOAT4X4 ViewMatrix;
    DirectX::XMFLOAT4X4 ProjectionMatrix;
    DirectX::XMFLOAT4X4 ShadowViewMatrices[MAX_LIGHTS];
    DirectX::XMFLOAT4X4 ShadowProjectionMatrices[MAX_LIGHTS];
};

struct VSPerMaterialData
{
    DirectX::XMFLOAT4 ColorTint;
};

struct VSPerObjectData
{
    DirectX::XMFLOAT4X4 world;
};

// These need to match the expected per-frame/object/material pixel shader data
struct PSPerFrameData
{
    LightData Lights[MAX_LIGHTS];
    DirectX::XMFLOAT3 CameraPosition;
    unsigned int LightCount;
    int SpecIBLMipLevel;
};

struct PSPerMaterialData
{
    DirectX::XMFLOAT3 AmbientColor;
    float UvMult;
};

class Renderer
{
private:
    AssetManager& globalAssets = AssetManager::GetInstance();

    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV;

    DirectX::XMFLOAT3 ambientColor;

    //General shaders
    std::shared_ptr<SimpleVertexShader> basicVS;
    std::shared_ptr<SimpleVertexShader> perFrameVS;
    std::shared_ptr<SimpleVertexShader> fullscreenVS;
    std::shared_ptr<SimplePixelShader> solidColorPS;
    std::shared_ptr<SimplePixelShader> perFramePS;
    std::shared_ptr<SimplePixelShader> textureSamplePS;
    std::shared_ptr<SimplePixelShader> outlinePS;

    //General meshes
    std::shared_ptr<Mesh> cubeMesh;
    std::shared_ptr<Mesh> sphereMesh;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> miscEffectSRVs[MiscEffectSRVTypes::MISC_EFFECT_SRV_COUNT];
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> miscEffectDepthBuffers[MiscEffectSRVTypes::MISC_EFFECT_SRV_COUNT];

    //components for shadows
    int shadowCount;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowDSVArraySRV;
    std::vector<Microsoft::WRL::ComPtr<ID3D11DepthStencilView>> shadowDSVArray;
    std::vector<DirectX::XMFLOAT4X4> shadowProjMatArray;
    std::vector<DirectX::XMFLOAT4X4> shadowViewMatArray;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRasterizer;
    std::shared_ptr<SimpleVertexShader> VSShadow;

    //components for colliders
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> wireframeRasterizer;

    // Offset and random values for SSAO blur and texture
    Microsoft::WRL::ComPtr<ID3D11Texture2D> ssaoRandomTex;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ssaoRandomSRV;

    Microsoft::WRL::ComPtr<ID3D11BlendState> particleBlendAdditive;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> particleDepthState;

    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>      renderTargetRTVs[RTVTypes::RTV_TYPE_COUNT];
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    renderTargetSRVs[RTVTypes::RTV_TYPE_COUNT];

    // Ambient Occlusion data
    std::shared_ptr<SimplePixelShader> ssaoPS;
    std::shared_ptr<SimplePixelShader> ssaoBlurPS;
    std::shared_ptr<SimplePixelShader> ssaoCombinePS;
    DirectX::XMFLOAT4 ssaoOffsets[64];
    const float ssaoRadius = 1.5f;
    const int ssaoSamples = 64;
    const int emptyRTV = 0;

    // Regardless of RTV count, SSAO needs 6 textures
    Microsoft::WRL::ComPtr<ID3D11Texture2D> ssaoTexture2D[6];

    // Composite and Silhouette also need textures
    Microsoft::WRL::ComPtr<ID3D11Texture2D> compositeTexture;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> silhouetteTexture;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> finalCompositeTexture;

    unsigned int windowHeight;
    unsigned int windowWidth;

    // Refraction data
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> refractionSilhouetteDepthState;

    // Depth pre-pass data
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> prePassDepthState;

    //Sky data
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> skyRasterizer;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> skyDepthState;

    //Selected Entity Outline targets
    Microsoft::WRL::ComPtr<ID3D11Texture2D> outlineTexture;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> outlineRTV;

	// Conditional Drawing
    static bool drawColliders;

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

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
    void PreResize();

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetRenderTargetSRV(RTVTypes type);
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetMiscEffectSRV(MiscEffectSRVTypes type);

    void DrawPointLights(std::shared_ptr<Camera> cam);
    void Draw(std::shared_ptr<Camera> camera, EngineState engineState);

    void InitShadows();
    void RenderShadows();
    void RenderDepths(std::shared_ptr<Camera> sourceCam, MiscEffectSRVTypes type);
    void RenderColliders(std::shared_ptr<Camera> cam);
    void RenderMeshBounds(std::shared_ptr<Camera> cam);
    void RenderSelectedHighlight(std::shared_ptr<Camera> cam, EngineState engineState);

    static bool GetDrawColliderStatus();
    static void SetDrawColliderStatus(bool _newState);

    int selectedEntity;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> outlineSRV;
};