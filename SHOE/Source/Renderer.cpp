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
	InitRenderTargetViews();
}

bool Renderer::GetDrawColliderStatus() { return drawColliders; }
void Renderer::SetDrawColliderStatus(bool _newState) { drawColliders = _newState; }