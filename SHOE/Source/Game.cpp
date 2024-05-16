#include "../Headers/Game.h"
#include "../Headers/Time.h"
#include "..\Headers\ComponentManager.h"
#include "..\Headers\ShadowProjector.h"
#include "..\Headers\FlashlightController.h"
#include "..\Headers\NoclipMovement.h"
#include <d3dcompiler.h>

// Needed for a helper function to read compiled shader files from the hard drive
#pragma comment(lib, "d3dcompiler.lib")

#pragma warning( disable : 26495)

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// DirectX itself, and our window, are not ready yet!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance, LPSTR cmdLine)
	: DXCore(
		hInstance,		   // The application's handle
		"SHOE",	   // Text for the window's title bar
		1280,			   // Width of the window's client area
		720,			   // Height of the window's client area
		true)			   // Show extra stats (fps) in title bar?
{

#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif

	{
		// Process Command line input
		std::string startCmdString = cmdLine;
		std::string currentSubstring;
		std::string firstHalfOfParameter;
		std::string secondHalfOfParameter;
		std::string delimiter = " ";
		std::string paramDelimiter = ":";
		size_t innerEnd;
		size_t innerSecondStart;

		auto start = 0U;
		auto end = startCmdString.find(delimiter);
		while (true)
		{
			currentSubstring = startCmdString.substr(start, end - start);
			innerEnd = currentSubstring.find(paramDelimiter, 0);
			innerSecondStart = innerEnd + paramDelimiter.length();

			firstHalfOfParameter = currentSubstring.substr(0, innerEnd);

			secondHalfOfParameter = currentSubstring.substr(innerSecondStart, currentSubstring.length() - innerSecondStart);

			if (!firstHalfOfParameter.compare("/DXVersion")) {
				if (!secondHalfOfParameter.compare("DX11")) {
					this->dxVersion = DIRECT_X_11;
				}
				else if (!secondHalfOfParameter.compare("DX12")) {
					this->dxVersion = DIRECT_X_12;
				}
			}

			if (!firstHalfOfParameter.compare("/ProjectPath")) {
				SetProjectPath(secondHalfOfParameter);
			}

			if (!firstHalfOfParameter.compare("/EngineInstallPath")) {
				SetEngineInstallPath(secondHalfOfParameter);
			}

			if (!firstHalfOfParameter.compare("/StartupScene")) {
				SetStartupSceneName(secondHalfOfParameter);
				SetHasStartupScene(true);
			}

			if (end == std::string::npos) {
				// Loop's over, break
				// Can't just have this be the loop condition,
				// because it misses the last argument
				break;
			}

			start = end + delimiter.length();
			end = startCmdString.find(delimiter, start);
		}
	}

	// Check if the directory defines are correct
	// TODO: Remove in favor of launcher cmdline params
	// But also, keeping a run in visual studio would be nice
	// Could I teach VS to launch with parameters + copy critical assets?
	char pathBuf[1024];

	GetFullPathNameA("SHOE.exe", sizeof(pathBuf), pathBuf, NULL);
	std::string::size_type pos = std::string(pathBuf).find_last_of("\\/");

	std::string currentSubPath = std::string(pathBuf);
	
	// Need to figure out a way to detect if this is run from visual studio
	// Or turns out I can just run VS with command line params targeting my
	// local SHOE install
	//if (string.IsNullOrEmpty(Environment.GetEnvironmentVariable("VisualStudioEdition"))) {
	SetBuildAssetPaths();
	//}
	//else {
	//	SetVSAssetPaths();
	//}
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Release all DirectX objects created here
//  - Delete any objects to prevent memory leaks
// --------------------------------------------------------
Game::~Game()
{
	// Note: Since we're using smart pointers (ComPtr),
	// we don't need to explicitly clean up those DirectX objects
	// - If we weren't using smart pointers, we'd need
	//   to call Release() on each DirectX object created in Game

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	delete& Input::GetInstance();
	delete& AssetManager::GetInstance();
	delete& AudioHandler::GetInstance();
	delete& CollisionManager::GetInstance();
	delete& SceneManager::GetInstance();

	delete loadingSpriteBatch;
}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	engineState = EngineState::INIT;

	if (!IsDirectX12()) {
		loadingSpriteBatch = new SpriteBatch(context.Get());

#if defined(DEBUG) || defined(_DEBUG)
		printf("Took %3.4f seconds for pre-initialization. \n", this->GetTotalTime());
#endif

		sceneManager.Initialize(&engineState, std::bind(&Game::DrawLoadingScreen, this));

		globalAssets.Initialize(device, context, hWnd, &engineState, std::bind(&Game::DrawInitializingScreen, this, std::placeholders::_1));

#if defined(DEBUG) || defined(_DEBUG)
		printf("Took %3.4f seconds for main initialization. \n", this->GetDeltaTime());
#endif

		if (GetProjectPath() != "") {
			globalAssets.ScanProjectAssetsAndImport(GetProjectAssetPath(), std::bind(&Game::DrawInitializingScreen, this, std::placeholders::_1));
		}

		if (HasStartupScene()) {
			engineState = EngineState::LOAD_SCENE;
			sceneManager.LoadScene(GetStartupSceneName());

#if defined(DEBUG) || defined(_DEBUG)
			printf("Took %3.4f seconds for project-specific initialization. \n", this->GetDeltaTime());
#endif
		}
		else {
#if defined(DEBUG) || defined(_DEBUG)
			printf("No startup scene set, skipping project-specific initialization. \n");
#endif
		}

		// Tell the input assembler stage of the pipeline what kind of
		// geometric primitives (points, lines or triangles) we want to draw.  
		// Essentially: "What kind of shape should the GPU draw with our data?"
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	// Initialize the input manager with the window's handle
	Input::GetInstance().Initialize(this->hWnd);

	//With everything initialized, start the renderer

	// What graphics library are we using?
	if (dxVersion) {
		renderer = std::make_shared<DX12Renderer>(height,
			width,
			deviceDX12,
			swapChain,
			commandAllocator,
			commandQueue,
			commandList);
	}
	else {
		renderer = std::make_shared<DX11Renderer>(height,
			width,
			device,
			context,
			swapChain,
			backBufferRTV,
			depthStencilView);
	}

	// Start the UI now that IMGUI has initialized
	editUI = std::make_unique<EditingUI>(renderer);

#if defined(DEBUG) || defined(_DEBUG)
	printf("Took %3.4f seconds for  post-initialization. \n", this->GetDeltaTime());
	printf("Total Initialization time was %3.4f seconds. \n", this->GetTotalTime());
#endif
}

// --------------------------------------------------------
// Handle resizing DirectX "stuff" to match the new window size.
// For instance, updating our projection matrix's aspect ratio.
// --------------------------------------------------------
void Game::OnResize()
{
	globalAssets.GetEditingCamera()->SetAspectRatio((float)this->width / (float)this->height);
	if (globalAssets.GetMainCamera() != globalAssets.GetEditingCamera()) {
		globalAssets.GetMainCamera()->SetAspectRatio((float)this->width / (float)this->height);
	}

	

	if (dxVersion) {
		renderer->PreResize();

		// Handle base-level DX resize stuff
		DXCore::OnResize();

		(dynamic_cast<DX12Renderer*>(renderer.get()))->PostResize(this->height, this->width);
	}
	else {
		renderer->PreResize();

		DXCore::OnResize();

		(dynamic_cast<DX11Renderer*>(renderer.get()))->PostResize(this->height, this->width, this->backBufferRTV, this->depthStencilView);
	}
}

// The main logic loop for taking input and updating components.
// No direct rendering is performed here - see Game::Draw().
void Game::Update()
{
	audioHandler.GetSoundSystem()->update();

	switch (engineState) {
	case EngineState::EDITING:
		// Quit if the escape key is pressed
		if (input.TestKeyAction(KeyActions::QuitGame)) Quit();
		else {
			editUI->GenerateEditingUI();

			globalAssets.UpdateEditingCamera();
			globalAssets.BroadcastGlobalEntityEvent(EntityEventType::EditingUpdate);
		}
		break;
	case EngineState::PLAY:
		if (input.TestKeyAction(KeyActions::QuitGame)) {
			sceneManager.PostPlayLoad();
		}
		else {
			if (input.TestKeyAction(KeyActions::ToggleFlashlight)) {
				for (std::shared_ptr<FlashlightController> flashlight : ComponentManager::GetAll<FlashlightController>()) {
					flashlight->GetGameEntity()->SetEnabled(!flashlight->GetGameEntity()->GetLocallyEnabled());
				}
			}

			CollisionManager::GetInstance().Update();

			globalAssets.BroadcastGlobalEntityEvent(EntityEventType::Update);
		}
		break;
	}
}

void Game::DrawInitializingScreen(std::string category)
{
	// Screen clear color
	const float color[4] = { 0.0f, 0.0f, 0.1f, 0.0f };

	DirectX::XMFLOAT2 categoryOrigin;

	static std::shared_ptr<SpriteFont> categoryFont = globalAssets.GetFontByName("SmoochSans-Bold")->spritefont;

	std::string categoryString = "Loading Default " + category;
	DirectX::XMStoreFloat2(&categoryOrigin, categoryFont->MeasureString(categoryString.c_str()) / 2.0f);

	context->ClearRenderTargetView(backBufferRTV.Get(), color);
	context->ClearDepthStencilView(
		depthStencilView.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);

	loadingSpriteBatch->Begin();
	categoryFont->DrawString(loadingSpriteBatch, categoryString.c_str(), DirectX::XMFLOAT2(width / 2, height / 1.5), DirectX::Colors::White, 0.0f, categoryOrigin);
	loadingSpriteBatch->End();

	swapChain->Present(0, 0);

	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthStencilView.Get());
}

void Game::DrawLoadingScreen() {
	if (engineState == EngineState::EDITING) {
		// If loading is done, reinitialize the important references.

		renderer.reset();

		context->Flush();

		// What graphics library are we using?
		if (dxVersion) {
			renderer = std::make_shared<DX12Renderer>(height,
				width,
				deviceDX12,
				swapChain,
				commandAllocator,
				commandQueue,
				commandList);
		}
		else {
			renderer = std::make_shared<DX11Renderer>(height,
				width,
				device,
				context,
				swapChain,
				backBufferRTV,
				depthStencilView);
		}

		// Start the UI now that IMGUI has initialized
		editUI->ReInitializeEditingUI(renderer);

		return;
	}

	// Screen clear color
	const float color[4] = { 0.0f, 0.0f, 0.1f, 0.0f };

	DirectX::XMFLOAT2 titleOrigin;
	DirectX::XMFLOAT2 categoryOrigin;
	DirectX::XMFLOAT2 objectOrigin;

	static std::shared_ptr<SpriteFont> titleFont = globalAssets.GetFontByName("Roboto-Bold-72pt")->spritefont;
	static std::shared_ptr<SpriteFont> categoryFont = globalAssets.GetFontByName("SmoochSans-Bold")->spritefont;
	static std::shared_ptr<SpriteFont> objectFont = globalAssets.GetFontByName("SmoochSans-Italic")->spritefont;

	std::string titleString = "Loading '" + sceneManager.GetLoadingSceneName() + "'";
	std::string loadedCategoryString = "Loading " + sceneManager.GetLoadingCategory();
	std::string loadedObjectString;

	if (sceneManager.GetLoadingException()) {
		try {
			std::rethrow_exception(sceneManager.GetLoadingException());
		}
		catch (const std::exception& e) {
			loadedObjectString = sceneManager.GetLoadingObjectName() + " Failed to Load! Error is printed to DBG console.";
#if defined(DEBUG) || defined(_DEBUG)
			printf(e.what());
#endif
		}
	}
	else {
		loadedObjectString = "Loading Object: " + sceneManager.GetLoadingObjectName();
	}

	context->ClearRenderTargetView(backBufferRTV.Get(), color);
	context->ClearDepthStencilView(
		depthStencilView.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);

	loadingSpriteBatch->Begin();

	// Certified conversion moment
	DirectX::XMStoreFloat2(&titleOrigin, titleFont->MeasureString(titleString.c_str()) / 2.0f);
	DirectX::XMStoreFloat2(&categoryOrigin, categoryFont->MeasureString(loadedCategoryString.c_str()) / 2.0f);
	DirectX::XMStoreFloat2(&objectOrigin, objectFont->MeasureString(loadedObjectString.c_str()) / 2.0f);

	titleFont->DrawString(loadingSpriteBatch, titleString.c_str(), DirectX::XMFLOAT2(width / 2, height / 5), DirectX::Colors::Gold, 0.0f, titleOrigin);
	categoryFont->DrawString(loadingSpriteBatch, loadedCategoryString.c_str(), DirectX::XMFLOAT2(width / 2, height / 1.5), DirectX::Colors::White, 0.0f, categoryOrigin);
	objectFont->DrawString(loadingSpriteBatch, loadedObjectString.c_str(), DirectX::XMFLOAT2(width / 2, height / 1.2), DirectX::Colors::LightGray, 0.0f, objectOrigin);

	loadingSpriteBatch->End();

	swapChain->Present(0, 0);

	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthStencilView.Get());
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw()
{
	switch (dxVersion) {
	case DIRECT_X_11:
		if (engineState == EngineState::EDITING) {
			renderer->Draw(globalAssets.GetEditingCamera(), engineState);
		}
		else if (engineState == EngineState::PLAY) {
			renderer->Draw(globalAssets.GetMainCamera(), engineState);
		}
		break;
	case DIRECT_X_12:
		if (engineState == EngineState::EDITING) {
			renderer->Draw(globalAssets.GetEditingCamera(), engineState);
		}
		else if (engineState == EngineState::PLAY) {
			renderer->Draw(globalAssets.GetMainCamera(), engineState);
		}
	}
	
}