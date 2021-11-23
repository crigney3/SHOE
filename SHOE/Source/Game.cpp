#include "../Headers/Game.h"

// Needed for a helper function to read compiled shader files from the hard drive
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

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
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,		   // The application's handle
		"DirectX Game",	   // Text for the window's title bar
		1280,			   // Width of the window's client area
		720,			   // Height of the window's client area
		true)			   // Show extra stats (fps) in title bar?
{

#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
	printf("Use arrow keys to switch skyboxes. \n");
	printf("Use F to toggle flashlight. \n");
	printf("When the flashlight's on, use G to toggle flickering. \n");
#endif

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
}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Initialize everything from gameobjects to skies
	globalAssets.Initialize(device, context);

	mainCamera = globalAssets.GetCameraByName("mainCamera");
	mainShadowCamera = globalAssets.GetCameraByName("mainShadowCamera");
	flashShadowCamera = globalAssets.GetCameraByName("flashShadowCamera");

	Entities = globalAssets.GetActiveGameEntities();

	// Initialize the input manager with the window's handle
	Input::GetInstance().Initialize(this->hWnd);
	statsEnabled = true;
	movingEnabled = true;
	lightWindowEnabled = false;
	objWindowEnabled = false;
	terrainWindowEnabled = false;
	skyWindowEnabled = false;
	objHierarchyEnabled = true;
	rtvWindowEnabled = false;
	childIndices = std::vector<int>();

	flashMenuToggle = false;
	lightUIIndex = 0;

	skies = globalAssets.GetSkyArray();
	activeSky = 1;

	//Very important this is set accurately
	lightCount = globalAssets.GetLightArraySize();
	
	// Tell the input assembler stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our data?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Add ImGui components
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device.Get(), context.Get());

	//With everything initialized, start the renderer
	renderer = std::make_unique<Renderer>(height,
										  width,
										  device,
										  context,
										  swapChain,
										  backBufferRTV,
										  depthStencilView);


	
	renderer->InitShadows();
}

void Game::RenderUI(float deltaTime) {
	// Reset the gui state to prevent tainted input
	input.SetGuiKeyboardCapture(false);
	input.SetGuiMouseCapture(false);

	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)this->width;
	io.DisplaySize.y = (float)this->height;
	io.KeyCtrl = input.KeyDown(VK_CONTROL);
	io.KeyAlt = input.KeyDown(VK_SHIFT);
	io.MousePos.x = (float)input.GetMouseX();
	io.MousePos.y = (float)input.GetMouseY();
	io.MouseDown[0] = input.MouseLeftDown();
	io.MouseDown[1] = input.MouseRightDown();
	io.MouseDown[2] = input.MouseMiddleDown();
	io.MouseWheel = input.GetMouseWheel();
	input.GetKeyArray(io.KeysDown, 256);

	// Reset the frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Determine new input capture
	input.SetGuiKeyboardCapture(io.WantCaptureKeyboard);
	input.SetGuiMouseCapture(io.WantCaptureMouse);

	if (statsEnabled) {
		// Display a UI element for stat tracking
		ImGui::Begin("Stats - Debug Mode");

		std::string infoStr = std::to_string(io.Framerate);
		std::string node = "Current Framerate: " + infoStr;

		ImGui::Text(node.c_str());

		infoStr = std::to_string(width);
		std::string infoStrTwo = std::to_string(height);
		node = "Window Width: " + infoStr + ", Window Height: " + infoStrTwo;

		ImGui::Text(node.c_str());

		infoStr = std::to_string(globalAssets.GetLightArraySize());
		node = "Light count: " + infoStr;

		ImGui::Text(node.c_str());

		infoStr = std::to_string(Entities->size());
		node = "Game Entity count: " + infoStr;

		ImGui::Text(node.c_str());

		ImGui::End();
	}

	if (skyWindowEnabled) {
		ImGui::Begin("Sky Editor");
		ImGui::Image(globalAssets.GetEmitterAtID(0)->particleDataSRV.Get(), ImVec2(256, 256));
		ImGui::End();
	}

	if (lightWindowEnabled) {
		// Display the debug UI for lights
		std::string indexStr = std::to_string(lightUIIndex);
		std::string node = "Editing light " + indexStr;
		ImGui::Begin("Light Editor");
		ImGui::Text(node.c_str());
		if (lightUIIndex == 4) ImGui::Text("Caution: Editing the flashlight");

		if (ImGui::ArrowButton("Previous Light", ImGuiDir_Left)) {
			lightUIIndex--;
			if (lightUIIndex < 0) {
				lightUIIndex = globalAssets.GetLightArraySize() - 1;
			}
		};
		ImGui::SameLine();
		ImGui::Text("Previous Light");

		if (ImGui::ArrowButton("Next Light", ImGuiDir_Right)) {
			lightUIIndex++;
			if (lightUIIndex > globalAssets.GetLightArraySize() - 1) {
				lightUIIndex = 0;
			}
		};
		ImGui::SameLine();
		ImGui::Text("Next Light");

		ImGui::ColorEdit3("Color: ", &globalAssets.GetLightAtID(lightUIIndex)->color.x);
		ImGui::DragFloat("Intensity: ", &globalAssets.GetLightAtID(lightUIIndex)->intensity, 1, 10.0f, 200.0f);
		ImGui::DragFloat("Range: ", &globalAssets.GetLightAtID(lightUIIndex)->range, 1, 5.0f, 20.0f);
		ImGui::End();
	}

	if (objWindowEnabled) {
		// Display the debug UI for objects
		std::string indexStr = std::to_string(entityUIIndex) + " - " + Entities->at(entityUIIndex)->GetName();
		std::string node = "Editing object " + indexStr;
		ImGui::Begin("Object Editor");
		ImGui::Text(node.c_str());

		if (ImGui::ArrowButton("Previous Object", ImGuiDir_Left)) {
			entityUIIndex--;
			if (entityUIIndex < 0) entityUIIndex = globalAssets.GetGameEntityArraySize() - 1;
		};
		ImGui::SameLine();
		ImGui::Text("Previous Object");
		
		if (ImGui::ArrowButton("Next Object", ImGuiDir_Right)) {
			entityUIIndex++;
			if (entityUIIndex > globalAssets.GetGameEntityArraySize()) entityUIIndex = 0;
		};
		ImGui::SameLine();
		ImGui::Text("Next Object");

		UIPositionEdit = Entities->at(entityUIIndex)->GetTransform()->GetPosition();
		UIRotationEdit = Entities->at(entityUIIndex)->GetTransform()->GetPitchYawRoll();
		UIScaleEdit = Entities->at(entityUIIndex)->GetTransform()->GetScale();
		
		ImGui::DragFloat3("Position: ", &UIPositionEdit.x, 0.5f);
		ImGui::DragFloat3("Rotation: ", &UIRotationEdit.x, 0.5f, 0, 360);
		ImGui::InputFloat3("Scale: ", &UIScaleEdit.x);

		Entities->at(entityUIIndex)->GetTransform()->SetPosition(UIPositionEdit.x, UIPositionEdit.y, UIPositionEdit.z);
		Entities->at(entityUIIndex)->GetTransform()->SetRotation(UIRotationEdit.x, UIRotationEdit.y, UIRotationEdit.z);
		Entities->at(entityUIIndex)->GetTransform()->SetScale(UIScaleEdit.x, UIScaleEdit.y, UIScaleEdit.z);

		std::string nameBuffer;
		static char nameBuf[64] = "";
		nameBuffer = Entities->at(entityUIIndex)->GetName();
		strcpy_s(nameBuf, nameBuffer.c_str());
		ImGui::InputText("Rename GameObject", nameBuf, sizeof(nameBuffer));

		Entities->at(entityUIIndex)->SetName(nameBuf);

		ImGui::End();
	}

	if (objHierarchyEnabled) {
		// Display the UI for setting parents
		if (ImGui::TreeNodeEx("GameObjects", 
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_FramePadding)) {
			ImGui::TextWrapped("Ran out of time to do drag-and-drop parenting, but hey, it shows which ones are parented!");
			for (int i = 0; i < globalAssets.GetGameEntityArraySize(); i++) {
				if (Entities->at(i)->GetTransform()->GetParent() == NULL) {
					RenderChildObjectsInUI(Entities->at(i).get());
				}
			}
			ImGui::TreePop();
		}
		if (ImGui::TreeNodeEx("Lights", 
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_FramePadding)) {
			ImGui::Text("Lights can't be parented (yet)");
			for (int i = 0; i < globalAssets.GetLightArraySize(); i++) {
				if (ImGui::TreeNode((void*)(intptr_t)i, "Light %d", i)) {
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
	}

	if (terrainWindowEnabled) {
		// Display the debug UI for terrain
		ImGui::Begin("Terrain Editor");

		ImGui::Text("Currently unimplemented");
		
		ImGui::End();
	}

	// TODO: Make MRT menu toggleable
	if (rtvWindowEnabled) {
		ImGui::Begin("Multiple Render Target Viewer");

		ImGui::Text("Color Without Ambient");
		ImGui::Image(renderer->GetRenderTargetSRV(0).Get(), ImVec2(500, 300));
		ImGui::Text("Ambient Color");
		ImGui::Image(renderer->GetRenderTargetSRV(1).Get(), ImVec2(500, 300));
		ImGui::Text("Normals");
		ImGui::Image(renderer->GetRenderTargetSRV(2).Get(), ImVec2(500, 300));
		ImGui::Text("Depths");
		ImGui::Image(renderer->GetRenderTargetSRV(3).Get(), ImVec2(500, 300));
		ImGui::Text("SSAO");
		ImGui::Image(renderer->GetRenderTargetSRV(4).Get(), ImVec2(500, 300));
		ImGui::Text("SSAO Post Blur");
		ImGui::Image(renderer->GetRenderTargetSRV(5).Get(), ImVec2(500, 300));

		ImGui::End();
	}

	// TODO: Add skybox menu

	// Display a menu at the top
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			ImGui::Text("This menu will eventually contain a saving and loading system, designed for swapping between feature test scenes.");
			//ImGui::MenuItem("Toggle Flashlight", "f", &flashMenuToggle);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit")) {
			ImGui::MenuItem("Lights", "l", &lightWindowEnabled);
			ImGui::MenuItem("GameObjects", "g", &objWindowEnabled);
			ImGui::MenuItem("Terrain", "t", &terrainWindowEnabled);
			ImGui::MenuItem("Object Hierarchy", "h", &objHierarchyEnabled);
			ImGui::MenuItem("Skies", "", &skyWindowEnabled);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			ImGui::MenuItem("Render Target Views", 0, &rtvWindowEnabled);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Add")) {
			//ImGui::MenuItem("Toggle Flashlight", "f", &flashMenuToggle);
			ImGui::Text("This menu will allow easily adding more objects and lights.");

			if (ImGui::Button("Add GameObject")) {
				globalAssets.CreateGameEntity(globalAssets.GetMeshByName("Cube"), globalAssets.GetMaterialByName("bronzeMat"), "GameEntity" + std::to_string(globalAssets.GetGameEntityArraySize()));
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Extra")) {
			ImGui::Text("Spare dropdown");

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Toggleables")) {
			ImGui::MenuItem("Toggle Flashlight", "f", &flashMenuToggle);
			ImGui::MenuItem("Toggle Flashlight Flickering", "v", &flickeringEnabled);
			ImGui::MenuItem("Toggle Stats Menu", ".", &statsEnabled);
			ImGui::MenuItem("Toggle movement", "m", &movingEnabled);

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}

void Game::RenderChildObjectsInUI(GameEntity* entity) {
	std::string nodeName = entity->GetName();
	if (ImGui::TreeNodeEx(nodeName.c_str(), 
		ImGuiTreeNodeFlags_DefaultOpen |
		ImGuiTreeNodeFlags_FramePadding)) {
		int childCount = entity->GetTransform()->GetChildCount();
		if (childCount > 0) {
			std::vector<GameEntity*> children = entity->GetTransform()->GetChildrenAsGameEntities();
			for (int i = 0; i < childCount; i++) {
				RenderChildObjectsInUI(children[i]);
			}
		}
		ImGui::TreePop();
	}
}

void Game::RenderSky() {
	if (input.KeyPress(VK_RIGHT)) {
		activeSky++;
		if (activeSky > skies->size() - 1) {
			activeSky = 0;
		}
	}
	else if (input.KeyPress(VK_LEFT)) {
		activeSky--;
		if (activeSky < 0) {
			activeSky = skies->size() - 1;
		}
	}

	renderer->SetActiveSky(skies->at(activeSky));
}

void Game::Flashlight() {
	Light* flashlight = globalAssets.GetFlashlight();
	if (input.KeyPress(0x46)) {
		flashMenuToggle = !flashMenuToggle;
	}

	if (flashMenuToggle) {
		flashlight->enabled = true;
	}
	else {
		flashlight->enabled = false;
	}

	if (flashMenuToggle) {
		XMFLOAT3 camPos = mainCamera->GetTransform()->GetPosition();
		flashlight->position = XMFLOAT3(camPos.x + 0.5f, camPos.y, camPos.z + 0.5f);
		flashlight->direction = mainCamera->GetTransform()->GetForward();
		flashShadowCamera->GetTransform()->SetPosition(flashlight->position.x, flashlight->position.y, flashlight->position.z);
		flashShadowCamera->GetTransform()->SetRotation(mainCamera->GetTransform()->GetPitchYawRoll().x, mainCamera->GetTransform()->GetPitchYawRoll().y, mainCamera->GetTransform()->GetPitchYawRoll().z);
		flashShadowCamera->UpdateViewMatrix();
		FlickeringCheck();
	}
}

void Game::FlickeringCheck() {
	if (input.KeyPress(0x47)) {
		flickeringEnabled = !flickeringEnabled;
	}
}

// --------------------------------------------------------
// Handle resizing DirectX "stuff" to match the new window size.
// For instance, updating our projection matrix's aspect ratio.
// --------------------------------------------------------
void Game::OnResize()
{
	if (mainCamera != 0) {
		mainCamera->UpdateProjectionMatrix((float)this->width / (float)this->height, 1);
	}

	// Handle base-level DX resize stuff
	DXCore::OnResize();

	renderer->PostResize(this->height, this->width, this->backBufferRTV, this->depthStencilView);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	RenderUI(deltaTime);

	// To untie something from framerate, multiply by deltatime

	// Quit if the escape key is pressed
	if (input.KeyDown(VK_ESCAPE)) Quit();

	if (movingEnabled) {
		globalAssets.GetGameEntityByName("Bronze Cube")->GetTransform()->SetPosition(+2.0f, -(float)sin(totalTime), +0.0f);
		globalAssets.GetGameEntityByName("Stone Cylinder")->GetTransform()->SetPosition(-2.0f, (float)sin(totalTime), +0.0f);

		globalAssets.GetGameEntityByName("Floor Helix")->GetTransform()->SetPosition((float)sin(totalTime), +2.0f, +0.0f);
		globalAssets.GetGameEntityByName("Paint Sphere")->GetTransform()->SetPosition(-(float)sin(totalTime), -2.0f, +0.0f);

		globalAssets.GetGameEntityByName("Rough Torus")->GetTransform()->Rotate(+0.0f, +0.0f, +1.0f * deltaTime);
	}

	Flashlight();
	RenderSky();

	for (int i = 0; i < globalAssets.GetEmitterArraySize(); i++) {
		globalAssets.GetEmitterAtID(i)->Update(deltaTime, totalTime);
	}

	// Flickering is currently broken
	/*if (flickeringEnabled) {
		srand((unsigned int)(deltaTime * totalTime));
		float prevIntensity = globalAssets.globalLights.at(4).intensity;
		int flickerRand = rand() % 6 + 1;
		if (flickerRand == 5 && !hasFlickered) {
			printf("flicker \n");
			globalAssets.globalLights.at(4).intensity = 5.0f;
			hasFlickered = true;
		}
		else {
			globalAssets.globalLights.at(4).intensity = prevIntensity;
			hasFlickered = false;
		}
	}*/

	mainCamera->Update(deltaTime, this->hWnd);
	//flashShadowCamera->Update(deltaTime, this->hWnd);
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	//Render shadows before anything else
	if (flashMenuToggle) {
		renderer->RenderShadows(flashShadowCamera, 0);
	}

	renderer->RenderShadows(mainShadowCamera, 1);

	renderer->Draw(mainCamera, totalTime);
}