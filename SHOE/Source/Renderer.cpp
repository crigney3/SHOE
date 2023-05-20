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

	// Whichever version of the renderer is running, initialize it
	
}

Renderer::~Renderer() {
	shadowDSVArray.clear();
	shadowProjMatArray.clear();
	shadowViewMatArray.clear();

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

HRESULT Renderer::RenderToVideoFile(std::shared_ptr<Camera> renderCam, FileRenderData RenderParameters) {
	// Process of this:
	// Call draw to get final composite frame. (possibly prevent it presenting to the screen?)
	// Have CPU accessible buffer of same type as final composite.
	// Copy final composite into this buffer.
	// Access buffer's data to get array of pixel colors.
	// Have media sink initialized (or maybe initialize in here? Could be laggy on first boot/scene load otherwise)
	// Use media sink tutorial on writing to video files
	// Make sure data is locked and released correctly, seems potentially unstable
	// May need to convert final composite data to YUV encoding, since RGBA is super uncompressed (high quality tho)
	// This method will needs either lots of parameters or lots of global data. Some stuff like width/height can be pulled from engine data.

	Microsoft::WRL::ComPtr<IMFSinkWriter> sinkWriter = NULL;
	DWORD streamIndex;
	long long int timeStamp = 0;

	RETURN_HRESULT_IF_FAILED(InitializeFileSinkWriter(&sinkWriter, &streamIndex, &RenderParameters));

	for (DWORD i = 0; i < RenderParameters.VideoFrameCount; ++i)
	{
		// Call draw in to generate the desired frame. Instead of presenting to the screen,
		// Draw will copy the final composite to a buffer (given the FILE_RENDER State.)
		Draw(renderCam, EngineState::FILE_RENDER);

		RETURN_HRESULT_IF_FAILED(WriteFrame(sinkWriter, streamIndex, timeStamp, &RenderParameters));

		timeStamp += RenderParameters.VideoFrameDuration;
	}

	sinkWriter->Finalize();

	sinkWriter->Release();

	return 0;
}

HRESULT Renderer::InitializeFileSinkWriter(Microsoft::WRL::ComPtr<IMFSinkWriter>* sinkWriterOut, DWORD* pStreamIndex, FileRenderData* RenderParameters) {
	*pStreamIndex = NULL;

	Microsoft::WRL::ComPtr<IMFSinkWriter> sinkWriter = NULL;
	Microsoft::WRL::ComPtr<IMFMediaType> pMediaTypeOut = NULL;
	Microsoft::WRL::ComPtr<IMFMediaType> pMediaTypeIn = NULL;
	DWORD streamIndex;

	RETURN_HRESULT_IF_FAILED(MFCreateDXGIDeviceManager(&deviceManagerResetToken, &deviceManager));
	RETURN_HRESULT_IF_FAILED(deviceManager->ResetDevice(device.Get(), deviceManagerResetToken));

	RETURN_HRESULT_IF_FAILED(MFCreateSinkWriterFromURL(RenderParameters->filePath.c_str(), NULL, NULL, &sinkWriter));

	// Set the output media type.
	RETURN_HRESULT_IF_FAILED(MFCreateMediaType(&pMediaTypeOut));
	
	RETURN_HRESULT_IF_FAILED(pMediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
	
	RETURN_HRESULT_IF_FAILED(pMediaTypeOut->SetGUID(MF_MT_SUBTYPE, RenderParameters->VideoEncodingFormat));
	
	RETURN_HRESULT_IF_FAILED(pMediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, RenderParameters->VideoBitRate));
	
	RETURN_HRESULT_IF_FAILED(pMediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));

	RETURN_HRESULT_IF_FAILED(MFSetAttributeSize(pMediaTypeOut.Get(), MF_MT_FRAME_SIZE, RenderParameters->VideoWidth, RenderParameters->VideoHeight));
	
	RETURN_HRESULT_IF_FAILED(MFSetAttributeRatio(pMediaTypeOut.Get(), MF_MT_FRAME_RATE, RenderParameters->VideoFPS, 1));
	
	RETURN_HRESULT_IF_FAILED(MFSetAttributeRatio(pMediaTypeOut.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
	
	RETURN_HRESULT_IF_FAILED(sinkWriter->AddStream(pMediaTypeOut.Get(), &streamIndex));
	
	// Set the input media type. May need to change to accept graphics buffer?
	RETURN_HRESULT_IF_FAILED(MFCreateMediaType(&pMediaTypeIn));
	
	RETURN_HRESULT_IF_FAILED(pMediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
	
	RETURN_HRESULT_IF_FAILED(pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, RenderParameters->VideoInputFormat));
	
	RETURN_HRESULT_IF_FAILED(pMediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
	
	RETURN_HRESULT_IF_FAILED(MFSetAttributeSize(pMediaTypeIn.Get(), MF_MT_FRAME_SIZE, RenderParameters->VideoWidth, RenderParameters->VideoHeight));
	
	RETURN_HRESULT_IF_FAILED(MFSetAttributeRatio(pMediaTypeIn.Get(), MF_MT_FRAME_RATE, RenderParameters->VideoFPS, 1));
	
	RETURN_HRESULT_IF_FAILED(MFSetAttributeRatio(pMediaTypeIn.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
	
	RETURN_HRESULT_IF_FAILED(sinkWriter->SetInputMediaType(streamIndex, pMediaTypeIn.Get(), NULL));
	
	// Tell the sink writer to start accepting data.
	RETURN_HRESULT_IF_FAILED(sinkWriter->BeginWriting());
	
	// Return the pointer to the caller.
	*sinkWriterOut = sinkWriter;
	(*sinkWriterOut)->AddRef();
	*pStreamIndex = streamIndex;

	/*sinkWriter->Release();
	pMediaTypeOut->Release();
	pMediaTypeIn->Release();*/
	return 0;
}

HRESULT Renderer::WriteFrame(Microsoft::WRL::ComPtr<IMFSinkWriter> sinkWriter, DWORD streamIndex, const long long int& timeStamp, FileRenderData* RenderParameters) {
	Microsoft::WRL::ComPtr<IMFSample> pSample = NULL;
	Microsoft::WRL::ComPtr<IMFMediaBuffer> pBuffer = NULL;

	D3D11_MAPPED_SUBRESOURCE msr;
	ZeroMemory(&msr, sizeof(D3D11_MAPPED_SUBRESOURCE));

	const LONG cbWidth = 4 * RenderParameters->VideoWidth;
	const DWORD cbBuffer = cbWidth * RenderParameters->VideoHeight;

	BYTE* pData = NULL;

	// Create a new memory buffer.
	HRESULT hr = MFCreateMemoryBuffer(cbBuffer, &pBuffer);

	// Lock the buffer and copy the video frame to the buffer.
	RETURN_HRESULT_IF_FAILED(pBuffer->Lock(&pData, NULL, NULL));

	context->Map(fileReadTexture.Get(), 0, D3D11_MAP_READ, 0, &msr);

	RETURN_HRESULT_IF_FAILED(MFCopyImage(
			pData,							// Destination buffer.
			cbWidth,						// Destination stride.
			(BYTE*)(msr.pData) + ((RenderParameters->VideoHeight - 1) * cbWidth),		// First row in rendered buffer. maybe use the resource with CPU access and a CopyResource??
			-cbWidth,						// Source stride
			cbWidth,						// Image width in bytes.
			RenderParameters->VideoHeight   // Image height in pixels.
		));

	context->Unmap(fileReadTexture.Get(), 0);

	if (pBuffer)
	{
		pBuffer->Unlock();
	}

	// Set the data length of the buffer.
	RETURN_HRESULT_IF_FAILED(pBuffer->SetCurrentLength(cbBuffer));

	// Create a media sample and add the buffer to the sample.
	RETURN_HRESULT_IF_FAILED(MFCreateSample(&pSample));

	RETURN_HRESULT_IF_FAILED(pSample->AddBuffer(pBuffer.Get()));

	// Set the time stamp and the duration.
	RETURN_HRESULT_IF_FAILED(pSample->SetSampleTime(timeStamp));
	
	RETURN_HRESULT_IF_FAILED(pSample->SetSampleDuration(RenderParameters->VideoFrameDuration));

	// Send the sample to the Sink Writer.
	RETURN_HRESULT_IF_FAILED(sinkWriter->WriteSample(streamIndex, pSample.Get()));

	/*pSample->Release();
	pBuffer->Release();*/
	return hr;
}

FileRenderData* Renderer::GetFileRenderData() {
	return &fileRenderData;
}