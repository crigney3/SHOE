#pragma once

#include "../Headers/Renderer.h"

class DX11Renderer : public Renderer {
private:
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

    // File Read/Write Video Buffers
    Microsoft::WRL::ComPtr<ID3D11Texture2D> fileWriteTexture;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> fileReadTexture;

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

    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    void InitRenderTargetViews();

public:
    DX11Renderer(
        unsigned int windowHeight,
        unsigned int windowWidth,
        Microsoft::WRL::ComPtr<ID3D11Device> device,
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
        Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain,
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV
    );
    ~DX11Renderer();
    void ReloadDefaultShaders();

    void PostResize(
        unsigned int windowHeight,
        unsigned int windowWidth,
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV
    );
    void PreResize();

    void DrawPointLights(std::shared_ptr<Camera> cam);
    void Draw(std::shared_ptr<Camera> camera, EngineState engineState);

    void InitShadows();
    void RenderShadows();
    void RenderDepths(std::shared_ptr<Camera> sourceCam, MiscEffectSRVTypes type);
    void RenderColliders(std::shared_ptr<Camera> cam);
    void RenderMeshBounds(std::shared_ptr<Camera> cam);
    void RenderSelectedHighlight(std::shared_ptr<Camera> cam, EngineState engineState);

    HRESULT RenderToVideoFile(std::shared_ptr<Camera> renderCam, FileRenderData RenderParameters);
    HRESULT WriteFrame(Microsoft::WRL::ComPtr<IMFSinkWriter> sinkWriter, DWORD streamIndex, const long long int& timeStamp, FileRenderData* RenderParameters);
    HRESULT InitializeFileSinkWriter(OUT Microsoft::WRL::ComPtr<IMFSinkWriter>* sinkWriterOut, DWORD* pStreamIndex, FileRenderData* RenderParameters);
};