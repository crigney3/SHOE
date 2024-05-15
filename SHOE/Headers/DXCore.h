#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <d3d12.h>
#include <string>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include "../IMGUI/Headers/imgui.h"
#include "../IMGUI/Headers/imgui_impl_win32.h"
#include "../IMGUI/Headers/imgui_impl_dx11.h"
#include "Input.h"
#include <dxgidebug.h>

// We can include the correct library files here
// instead of in Visual Studio settings if we want
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

// This is used both to build the buffer pool that stores all data needed for
// the next X frames, as well as defining the point where the CPU must sync
// if it is too far ahead.
#define DX12_BUFFER_FRAME_COUNT 3

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
	ASSET_TEXTURE_BLENDMAP_PATH,
	ASSET_PATH_COUNT
};

enum AssetPathType {
	ENGINE_ASSET,
	PROJECT_ASSET,
	EXTERNAL_ASSET,
	ASSET_PATH_TYPE_COUNT
};

enum DirectXVersion {
	DIRECT_X_11,
	DIRECT_X_12,
	BAD_VERSION
};

//DX12 update TODO:
// Update API initialization and back buffer swapping
// Replace context with command list/queue/allocator
// Create fence for syncing
// Don't use SimpleShader (or try and update it to do some of the work automatically)
// Create root signatures for each shader (or at least each shader that has different inputs)
// Replace shaders + pipeline state with pipeline state objects
// Meshes should track views while dx12 is active
// Manual SRV creation for loaded textures
// Store groups of SRVs from materials into descriptor heap, then store resulting desc tables
// Drawing must set PSO, root sig, descriptor tables, any remaining DX12 stuff. Must also update
// appropriate constant buffers.
// 
//

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

	// Returns 0 for DirectX11 and 1 for DirectX12
	bool IsDirectX12();
	DirectXVersion GetDXVersion();

	// Initialization and game-loop related methods
	HRESULT InitWindow();
	HRESULT InitDirectX11();
	HRESULT InitDirectX12();
	HRESULT Run();
	void Quit();
	virtual void OnResize();

	// Pure virtual methods for setup and game functionality
	virtual void Init() = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;

	std::string GetFullPathTo(std::string relativeFilePath);
	std::wstring GetFullPathTo_Wide(std::wstring relativeFilePath);

	std::string GetProjectPath();
	std::string GetProjectAssetPath();
	std::string GetEngineAssetsPath();
	std::string GetEngineInstallPath();
	std::string GetStartupSceneName();
	bool HasStartupScene();
	void SetProjectPath(std::string projPath);
	void SetEngineInstallPath(std::string enginePath);
	void SetStartupSceneName(std::string sceneName);
	void SetHasStartupScene(bool hasScene);

	void SetVSAssetPaths();
	void SetBuildAssetPaths();

	// Syncing tools
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;

	// Size of the window's client area
	unsigned int width;
	unsigned int height;

	std::string GetAssetPathString(AssetPathIndex index, AssetPathType type);

	HWND		hWnd;			// The handle to the window itself

protected:
	HINSTANCE	hInstance;		// The handle to the application
	std::string titleBarText;	// Custom text in window's title bar
	bool		titleBarStats;	// Show extra stats in title bar?

	// Does our window currently have focus?
	// Helpful if we want to pause while not the active window
	bool hasFocus;

	// DirectX11 related objects and variables

	Microsoft::WRL::ComPtr<ID3D11DeviceContext>	context;

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
	Microsoft::WRL::ComPtr<ID3D11Device>		device;

	// DirectX12 related objects and variables
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>			commandQueue;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	commandList;

	Microsoft::WRL::ComPtr<ID3D12Device>		deviceDX12;

	static const unsigned int numBackBuffers = 2;

	D3D12_VIEWPORT			viewport;
	D3D12_RECT				scissorRect;

	// Both versions of DirectX use these
	D3D_FEATURE_LEVEL		dxFeatureLevel;
	Microsoft::WRL::ComPtr<IDXGISwapChain>		swapChain;

	// Helper function for allocating a console window
	void CreateConsoleWindow(int bufferLines, int bufferColumns, int windowLines, int windowColumns);

	// Helpers for determining the actual path to the executable
	std::string GetExePath();
	std::wstring GetExePath_Wide();

	float GetTotalTime();
	float GetDeltaTime();

	std::string criticalAssetPaths[ASSET_PATH_COUNT];
	std::string projectAssetPaths[ASSET_PATH_COUNT];

	// What version of DX is this?
	DirectXVersion dxVersion;

private:
	// Timing related data
	double perfCounterSeconds;
	float totalTime;
	float deltaTime;
	__int64 startTime;
	__int64 currentTime;
	__int64 previousTime;

	std::string projectPath;
	std::string mainAssetPath;
	std::string engineInstallPath;
	std::string engineAssetsPath;
	std::string startupSceneName;

	bool hasStartupScene;

	// FPS calculation
	int fpsFrameCount;
	float fpsTimeElapsed;

	void UpdateTimer();			// Updates the timer for this frame
	void UpdateTitleBarStats();	// Puts debug info in the title bar
};

