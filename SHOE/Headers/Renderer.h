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
protected:
    AssetManager& globalAssets = AssetManager::GetInstance();

    virtual void InitRenderTargetViews() = 0;

    unsigned int windowHeight;
    unsigned int windowWidth;

    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;

    // Conditional Drawing
    static bool drawColliders;

public:
    Renderer(
        unsigned int windowHeight,
        unsigned int windowWidth, 
        Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain);
    virtual ~Renderer() = 0;

    virtual void PostResize() = 0;
    virtual void PreResize() = 0;
    virtual void InitShadows() = 0;

    virtual Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetRenderTargetSRV(RTVTypes type) = 0;
    virtual Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetMiscEffectSRV(MiscEffectSRVTypes type) = 0;

    virtual void DrawPointLights(std::shared_ptr<Camera> cam) = 0;
    virtual void Draw(std::shared_ptr<Camera> camera, EngineState engineState) = 0;
    virtual void RenderShadows() = 0;

    virtual void RenderDepths(std::shared_ptr<Camera> sourceCam, MiscEffectSRVTypes type) = 0;
    virtual void RenderColliders(std::shared_ptr<Camera> cam) = 0;
    virtual void RenderMeshBounds(std::shared_ptr<Camera> cam) = 0;
    virtual void RenderSelectedHighlight(std::shared_ptr<Camera> cam, EngineState engineState) = 0;

    static bool GetDrawColliderStatus();
    static void SetDrawColliderStatus(bool _newState);

    int selectedEntity;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> outlineSRV;
};