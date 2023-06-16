#include "../Headers/Game.h"

// --------------------------------------------------------
// Entry point for a graphical (non-console) Windows application
// --------------------------------------------------------
int WINAPI WinMain(
	_In_ HINSTANCE hInstance,			// The handle to this app's instance
	_In_opt_ HINSTANCE hPrevInstance,	// A handle to the previous instance of the app (always NULL)
	_In_ LPSTR lpCmdLine,				// Command line params
	_In_ int nCmdShow)					// How the window should be shown (we ignore this)
{
	DirectXVersion dxVersion = DIRECT_X_11;

#if defined(DEBUG) | defined(_DEBUG)
	// Enable memory leak detection as a quick and dirty
	// way of determining if we forgot to clean something up
	//  - You may want to use something more advanced, like Visual Leak Detector
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	printf(lpCmdLine);

	// Create the Game object using
	// the app handle we got from WinMain
	Game dxGame(hInstance, dxVersion);

	// Result variable for function calls below
	HRESULT hr = S_OK;

	// Attempt to create the window for our program, and
	// exit early if something failed
	hr = dxGame.InitWindow();
	if(FAILED(hr)) return hr;

	// Attempt to initialize DirectX, and exit
	// early if something failed
	if (dxVersion) {
		hr = dxGame.InitDirectX12();
	}
	else {
		hr = dxGame.InitDirectX11();
	}
	if(FAILED(hr)) return hr;

	// Begin the message and game loop, and then return
	// whatever we get back once the game loop is over
	return dxGame.Run();
}
