#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <string>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include "../IMGUI/Headers/imgui.h"
#include "../IMGUI/Headers/imgui_impl_win32.h"
#include "../IMGUI/Headers/imgui_impl_dx11.h"
#include "Input.h"

#pragma region paths

enum AssetPathIndex {
	ASSET_MODEL_PATH,
	ASSET_SCENE_PATH,
	ASSET_HEIGHTMAP_PATH,
	ASSET_FONT_PATH,
	ASSET_PARTICLE_PATH,
	ASSET_SOUND_PATH,
	ASSET_TEXTURE_PATH_BASIC,
	ASSET_TEXTURE_PATH_SKIES,
	ASSET_TEXTURE_PATH_PBR,
	ASSET_TEXTURE_PATH_PBR_ALBEDO,
	ASSET_TEXTURE_PATH_PBR_NORMALS,
	ASSET_TEXTURE_PATH_PBR_METALNESS,
	ASSET_TEXTURE_PATH_PBR_ROUGHNESS,
	ASSET_SHADER_PATH,
	ASSET_PATH_COUNT
};

#pragma endregion

// We can include the correct library files here
// instead of in Visual Studio settings if we want
#pragma comment(lib, "d3d11.lib")

class DXCore
{
public:
	DXCore(
		HINSTANCE hInstance,		// The application's handle
		const char* titleBarText,	// Text for the window's title bar
		unsigned int windowWidth,	// Width of the window's client area
		unsigned int windowHeight,	// Height of the window's client area
		bool debugTitleBarStats);	// Show extra stats (fps) in title bar?
	~DXCore();

	// Static requirements for OS-level message processing
	static DXCore* DXCoreInstance;
	static LRESULT CALLBACK WindowProc(
		HWND hWnd,		// Window handle
		UINT uMsg,		// Message
		WPARAM wParam,	// Message's first parameter
		LPARAM lParam	// Message's second parameter
	);

	// Internal method for message handling
	LRESULT ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// Initialization and game-loop related methods
	HRESULT InitWindow();
	HRESULT InitDirectX();
	HRESULT Run();
	void Quit();
	virtual void OnResize();

	// Pure virtual methods for setup and game functionality
	virtual void Init() = 0;
	virtual void Update(float deltaTime, float totalTime) = 0;
	virtual void Draw(float deltaTime, float totalTime) = 0;

	std::string GetFullPathTo(std::string relativeFilePath);
	std::wstring GetFullPathTo_Wide(std::wstring relativeFilePath);

	void SetVSAssetPaths();
	void SetBuildAssetPaths();

	// Size of the window's client area
	unsigned int width;
	unsigned int height;

	std::string GetAssetPathString(AssetPathIndex index);

protected:
	HINSTANCE	hInstance;		// The handle to the application
	HWND		hWnd;			// The handle to the window itself
	std::string titleBarText;	// Custom text in window's title bar
	bool		titleBarStats;	// Show extra stats in title bar?

	// Does our window currently have focus?
	// Helpful if we want to pause while not the active window
	bool hasFocus;

	// DirectX related objects and variables
	D3D_FEATURE_LEVEL		dxFeatureLevel;
	Microsoft::WRL::ComPtr<IDXGISwapChain>		swapChain;
	Microsoft::WRL::ComPtr<ID3D11Device>		device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>	context;

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;

	// Helper function for allocating a console window
	void CreateConsoleWindow(int bufferLines, int bufferColumns, int windowLines, int windowColumns);

	// Helpers for determining the actual path to the executable
	std::string GetExePath();
	std::wstring GetExePath_Wide();

	float GetTotalTime();
	float GetDeltaTime();

	std::string assetPathStrings[ASSET_PATH_COUNT];

private:
	// Timing related data
	double perfCounterSeconds;
	float totalTime;
	float deltaTime;
	__int64 startTime;
	__int64 currentTime;
	__int64 previousTime;

	// FPS calculation
	int fpsFrameCount;
	float fpsTimeElapsed;

	void UpdateTimer();			// Updates the timer for this frame
	void UpdateTitleBarStats();	// Puts debug info in the title bar
};

