#pragma once

#include "AssetManager.h"
#include "CollisionManager.h"
#include <windows.media.mediaproperties.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mfapi.h>

#pragma comment(lib, "Mfreadwrite.lib")

#define RETURN_HRESULT_IF_FAILED(x) do { HRESULT status = (x); if (FAILED(status)) return status; } while(0)

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
// 9 - Render for writing to a file
// 10 - Count: always the last one, tracks size
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
    FILE_WRITE_COMPOSITE,

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

/// <summary>
/// Preset data that needs to be fully initialized before
/// passing it to Renderer::RenderToFile.
/// </summary>
struct FileRenderData
{
    unsigned int VideoWidth;
    unsigned int VideoHeight;
    unsigned int VideoFPS;

    // Should usually be Width * Height
    unsigned int VideoPels;
    unsigned int VideoBitRate;
    unsigned int VideoFrameCount;
    unsigned long VideoFrameDuration; 

    // Formats from MFVideoFormat
    GUID VideoEncodingFormat;
    GUID VideoInputFormat;

    std::wstring filePath;
};

class Renderer
{
protected:
    AssetManager& globalAssets = AssetManager::GetInstance();

    virtual void InitRenderTargetViews();

    FileRenderData fileRenderData;
    Microsoft::WRL::ComPtr<IMFDXGIDeviceManager> deviceManager;
    UINT deviceManagerResetToken = 0;

    unsigned int windowHeight;
    unsigned int windowWidth;

    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;

    // These are related to functions that can't be made covariant
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>      renderTargetRTVs[RTVTypes::RTV_TYPE_COUNT];
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    renderTargetSRVs[RTVTypes::RTV_TYPE_COUNT];

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> miscEffectSRVs[MiscEffectSRVTypes::MISC_EFFECT_SRV_COUNT];
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> miscEffectDepthBuffers[MiscEffectSRVTypes::MISC_EFFECT_SRV_COUNT];

    // Conditional Drawing
    static bool drawColliders;

public:
    Renderer(
        unsigned int windowHeight,
        unsigned int windowWidth, 
        Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain);
    virtual ~Renderer();

    virtual void ReloadDefaultShaders();

    virtual void PostResize();
    virtual void PreResize();
    virtual void InitShadows();

    // These can't be made covariant, so they have to go here
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetDX11RenderTargetSRV(RTVTypes type);
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetDX11MiscEffectSRV(MiscEffectSRVTypes type);

    virtual void DrawPointLights(std::shared_ptr<Camera> cam) = 0;
    virtual void Draw(std::shared_ptr<Camera> camera, EngineState engineState) = 0;
    virtual void RenderShadows() = 0;

    virtual void RenderDepths(std::shared_ptr<Camera> sourceCam, MiscEffectSRVTypes type) = 0;
    virtual void RenderColliders(std::shared_ptr<Camera> cam) = 0;
    virtual void RenderMeshBounds(std::shared_ptr<Camera> cam) = 0;
    virtual void RenderSelectedHighlight(std::shared_ptr<Camera> cam, EngineState engineState) = 0;

    static bool GetDrawColliderStatus();
    static void SetDrawColliderStatus(bool _newState);

    virtual HRESULT RenderToVideoFile(std::shared_ptr<Camera> renderCam, FileRenderData RenderParameters);
    virtual HRESULT WriteFrame(Microsoft::WRL::ComPtr<IMFSinkWriter> sinkWriter, DWORD streamIndex, const long long int& timeStamp, FileRenderData* RenderParameters);
    virtual HRESULT InitializeFileSinkWriter(OUT Microsoft::WRL::ComPtr<IMFSinkWriter>* sinkWriterOut, DWORD* pStreamIndex, FileRenderData* RenderParameters);

    FileRenderData* GetFileRenderData();

    int selectedEntity;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> outlineSRV;
};