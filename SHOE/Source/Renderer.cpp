#include "../Headers/Renderer.h"
#include "../Headers/MeshRenderer.h"
#include "../Headers/ParticleSystem.h"
#include "..\Headers\ShadowProjector.h"

using namespace DirectX;

// forward declaration for static members
bool Renderer::drawColliders;

Renderer::Renderer(
	unsigned int windowHeight,
	unsigned int windowWidth,
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain)
{
	this->windowHeight = windowHeight;
	this->windowWidth = windowWidth;
	this->swapChain = swapChain;

	MFStartup(MF_VERSION);

	// Initialize the basic File Renderer Data
	this->fileRenderData.filePath = L"C:\\output.mp4";
	this->fileRenderData.VideoBitRate = 800000;
	this->fileRenderData.VideoEncodingFormat = MFVideoFormat_H264;
	this->fileRenderData.VideoFPS = 30;
	this->fileRenderData.VideoFrameCount = 20 * this->fileRenderData.VideoFPS;
	this->fileRenderData.VideoFrameDuration = 10 * 1000 * 1000 / this->fileRenderData.VideoFPS;
	this->fileRenderData.VideoHeight = windowHeight;
	this->fileRenderData.VideoWidth = windowWidth;
	this->fileRenderData.VideoInputFormat = MFVideoFormat_RGB32;
	this->fileRenderData.VideoPels = this->fileRenderData.VideoWidth * this->fileRenderData.VideoHeight;
}

Renderer::~Renderer() {
	MFShutdown();
}

void Renderer::PreResize() {

}

void Renderer::InitRenderTargetViews() {

}

void Renderer::InitShadows() {

}

void Renderer::PostResize() {

}

bool Renderer::GetDrawColliderStatus() { return drawColliders; }

void Renderer::SetDrawColliderStatus(bool _newState) { drawColliders = _newState; }

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetDX11RenderTargetSRV(RTVTypes type) {
	return renderTargetSRVs[type];
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetDX11MiscEffectSRV(MiscEffectSRVTypes type) {
	return miscEffectSRVs[type];
}

HRESULT Renderer::RenderToVideoFile(std::shared_ptr<Camera> renderCam, FileRenderData RenderParameters) {
	return 1;
}

HRESULT Renderer::InitializeFileSinkWriter(Microsoft::WRL::ComPtr<IMFSinkWriter>* sinkWriterOut, DWORD* pStreamIndex, FileRenderData* RenderParameters) {
	return 1;
}

HRESULT Renderer::WriteFrame(Microsoft::WRL::ComPtr<IMFSinkWriter> sinkWriter, DWORD streamIndex, const long long int& timeStamp, FileRenderData* RenderParameters) {
	return 1;
}

FileRenderData* Renderer::GetFileRenderData() {
	return &fileRenderData;
}