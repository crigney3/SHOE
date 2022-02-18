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
	delete& AudioHandler::GetInstance();
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
	emitterUIIndex = 0;
	camUIIndex = 0;
	terrainUIIndex = 0;
	skyUIIndex = 1;

	skies = globalAssets.GetSkyArray();

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

		std::shared_ptr<Sky> currentSky = globalAssets.GetSkyAtID(skyUIIndex);

		if (ImGui::ArrowButton("Previous Sky", ImGuiDir_Left)) {
			skyUIIndex--;
			if (skyUIIndex < 0) {
				skyUIIndex = globalAssets.GetSkyArraySize() - 1;
			}
			renderer->SetActiveSky(globalAssets.GetSkyAtID(skyUIIndex));
		};
		ImGui::SameLine();

		if (ImGui::ArrowButton("Next Sky", ImGuiDir_Right)) {
			skyUIIndex++;
			if (skyUIIndex > globalAssets.GetSkyArraySize() - 1) {
				skyUIIndex = 0;
			}
			renderer->SetActiveSky(globalAssets.GetSkyAtID(skyUIIndex));
		};

		std::string nameBuffer;
		static char nameBuf[64] = "";
		nameBuffer = currentSky->GetName();
		strcpy_s(nameBuf, nameBuffer.c_str());
		ImGui::InputText("Rename Sky ", nameBuf, sizeof(nameBuffer));

		currentSky->SetName(nameBuf);

		bool skyEnabled = currentSky->GetEnableDisable();
		ImGui::Checkbox("Enabled ", &skyEnabled);
		currentSky->SetEnableDisable(skyEnabled);

		if (skyEnabled && ImGui::CollapsingHeader("BRDF Lookup Texture")) {
			ImGui::Image((ImTextureID*)currentSky->GetBRDFLookupTexture().Get(), ImVec2(256, 256));
		}

		//ImGui::Image(globalAssets.GetEmitterAtID(0)->particleDataSRV.Get(), ImVec2(256, 256));
		ImGui::End();
	}

	if (lightWindowEnabled) {
		// Display the debug UI for lights
		Light* currentLight = globalAssets.GetLightAtID(lightUIIndex);

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

		if (ImGui::ArrowButton("Next Light", ImGuiDir_Right)) {
			lightUIIndex++;
			if (lightUIIndex > globalAssets.GetLightArraySize() - 1) {
				lightUIIndex = 0;
			}
		};

		bool lightEnabled = (bool)currentLight->enabled;
		ImGui::Checkbox("Enabled ", &lightEnabled);
		lightEnabled ? currentLight->enabled = 1.0f : currentLight->enabled = 0.0f;

		ImGui::ColorEdit3("Color ", &currentLight->color.x);
		ImGui::DragFloat("Intensity ", &currentLight->intensity, 1, 10.0f, 200.0f);
		ImGui::DragFloat("Range ", &currentLight->range, 1, 5.0f, 20.0f);
		ImGui::End();
	}

	if (objWindowEnabled) {
		// Display the debug UI for objects
		std::shared_ptr<GameEntity> currentEntity = Entities->at(entityUIIndex);
		std::string indexStr = std::to_string(entityUIIndex) + " - " + currentEntity->GetName();
		std::string node = "Editing object " + indexStr;
		ImGui::Begin("Object Editor");
		ImGui::Text(node.c_str());

		if (ImGui::ArrowButton("Previous Object", ImGuiDir_Left)) {
			entityUIIndex--;
			if (entityUIIndex < 0) entityUIIndex = globalAssets.GetGameEntityArraySize() - 1;
		};
		ImGui::SameLine();
		
		if (ImGui::ArrowButton("Next Object", ImGuiDir_Right)) {
			entityUIIndex++;
			if (entityUIIndex > globalAssets.GetGameEntityArraySize() - 1) entityUIIndex = 0;
		};

		std::string nameBuffer;
		static char nameBuf[64] = "";
		nameBuffer = currentEntity->GetName();
		strcpy_s(nameBuf, nameBuffer.c_str());
		ImGui::InputText("Rename GameObject", nameBuf, sizeof(nameBuffer));

		currentEntity->SetName(nameBuf);

		bool entityEnabled = currentEntity->GetEnableDisable();
		ImGui::Checkbox("Enabled: ", &entityEnabled);
		currentEntity->SetEnableDisable(entityEnabled);

		UIPositionEdit = currentEntity->GetTransform()->GetPosition();
		UIRotationEdit = currentEntity->GetTransform()->GetPitchYawRoll();
		UIScaleEdit = currentEntity->GetTransform()->GetScale();
		
		ImGui::DragFloat3("Position ", &UIPositionEdit.x, 0.5f);
		ImGui::DragFloat3("Rotation ", &UIRotationEdit.x, 0.5f, 0, 360);
		ImGui::InputFloat3("Scale ", &UIScaleEdit.x);

		currentEntity->GetTransform()->SetPosition(UIPositionEdit.x, UIPositionEdit.y, UIPositionEdit.z);
		currentEntity->GetTransform()->SetRotation(UIRotationEdit.x, UIRotationEdit.y, UIRotationEdit.z);
		currentEntity->GetTransform()->SetScale(UIScaleEdit.x, UIScaleEdit.y, UIScaleEdit.z);

		ImGui::End();
	}

	// Particle Menu
	if (particleWindowEnabled) {
		ImGui::Begin("Particle Editor");

		std::shared_ptr<Emitter> currentEmitter = globalAssets.GetEmitterAtID(emitterUIIndex);

		std::string indexStr = currentEmitter->GetName();
		std::string node = "Editing Particle Emitter " + indexStr;
		ImGui::Text(node.c_str());

		if (ImGui::ArrowButton("Previous Emitter", ImGuiDir_Left)) {
			emitterUIIndex--;
			if (emitterUIIndex < 0) {
				emitterUIIndex = globalAssets.GetEmitterArraySize() - 1;
			}
		};
		ImGui::SameLine();

		if (ImGui::ArrowButton("Next Emitter", ImGuiDir_Right)) {
			emitterUIIndex++;
			if (emitterUIIndex > globalAssets.GetEmitterArraySize() - 1) {
				emitterUIIndex = 0;
			}
		};

		std::string nameBuffer;
		static char nameBuf[64] = "";
		nameBuffer = currentEmitter->GetName();
		strcpy_s(nameBuf, nameBuffer.c_str());
		ImGui::InputText("Rename Emitter ", nameBuf, sizeof(nameBuffer));

		currentEmitter->SetName(nameBuf);

		bool emitterEnabled = currentEmitter->GetEnableDisable();
		ImGui::Checkbox("Enabled ", &emitterEnabled);
		currentEmitter->SetEnableDisable(emitterEnabled);

		DirectX::XMFLOAT4 currentTint = currentEmitter->GetColorTint();
		ImGui::ColorEdit3("Color ", &currentTint.x);
		currentEmitter->SetColorTint(currentTint);

		bool blendState = currentEmitter->GetBlendState();
		ImGui::Checkbox("Blend State ", &blendState);
		ImGui::SameLine();
		if (blendState) {
			ImGui::Text("Blend state is additive.");
		}
		else {
			ImGui::Text("Blend state is not additive.");
		}
		currentEmitter->SetBlendState(blendState);

		float scale = currentEmitter->GetScale();
		ImGui::SliderFloat("Scale with age ", &scale, 0.0f, 2.0f);
		currentEmitter->SetScale(scale);

		float particlesPerSecond = currentEmitter->GetParticlesPerSecond();
		ImGui::SliderFloat("Particles per Second ", &particlesPerSecond, 0.1f, 20.0f);
		ImGui::SameLine();
		ImGui::InputFloat("#ExtraEditor", &particlesPerSecond);
		currentEmitter->SetParticlesPerSecond(particlesPerSecond);

		float particlesLifetime = currentEmitter->GetParticleLifetime();
		ImGui::SliderFloat("Particles Lifetime ", &particlesLifetime, 0.1f, 20.0f);
		ImGui::SameLine();
		ImGui::InputFloat("#ExtraEditor2", &particlesLifetime);
		currentEmitter->SetParticleLifetime(particlesLifetime);

		float speed = currentEmitter->GetSpeed();
		ImGui::SliderFloat("Particle Speed ", &speed, 0.1f, 5.0f);
		currentEmitter->SetSpeed(speed);

		DirectX::XMFLOAT3 destination = currentEmitter->GetDestination();
		ImGui::InputFloat3("Particles Move Towards ", &destination.x);
		currentEmitter->SetDestination(destination);

		int maxParticles = currentEmitter->GetMaxParticles();
		ImGui::InputInt("Max Particles ", &maxParticles);
		currentEmitter->SetMaxParticles(maxParticles);

		if (ImGui::CollapsingHeader("Particle Data ")) {
			ImGui::Text("Sort List - Cannot be Edited live, Shader Only: ");
			if (ImGui::BeginListBox("SortList")) {
				DirectX::XMFLOAT2* list = (DirectX::XMFLOAT2*)currentEmitter->GetSortListSRV().Get();
				for (int i = 0; i < maxParticles; i++) {
					std::string listItem = "##Value at " + i;
					listItem += ": ";
					listItem += list[i].x;
					ImGui::Text(listItem.c_str());
				}
				ImGui::EndListBox();
			}
		}

		ImGui::End();
	}

	if (soundWindowEnabled) {
		ImGui::Begin("Sound Menu");

		for (int i = 0; i < globalAssets.GetSoundArraySize(); i++) {
			std::string buttonName = "Play Piano Sound ##" + std::to_string(i);
			if (ImGui::Button(buttonName.c_str())) {
				audioHandler.BasicPlaySound(globalAssets.GetSoundAtID(i));
			}
		}
		
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

		std::shared_ptr<GameEntity> currentEntity = globalAssets.GetTerrainAtID(terrainUIIndex);
		std::string indexStr = std::to_string(entityUIIndex) + " - " + currentEntity->GetName();
		std::string node = "Editing Terrain " + indexStr;
		ImGui::Text(node.c_str());

		if (ImGui::ArrowButton("Previous Object", ImGuiDir_Left)) {
			terrainUIIndex--;
			if (terrainUIIndex < 0) terrainUIIndex = globalAssets.GetTerrainEntityArraySize() - 1;
		};
		ImGui::SameLine();

		if (ImGui::ArrowButton("Next Object", ImGuiDir_Right)) {
			terrainUIIndex++;
			if (terrainUIIndex > globalAssets.GetTerrainEntityArraySize() - 1) terrainUIIndex = 0;
		};

		std::string nameBuffer;
		static char nameBuf[64] = "";
		nameBuffer = currentEntity->GetName();
		strcpy_s(nameBuf, nameBuffer.c_str());
		ImGui::InputText("Rename Terrain", nameBuf, sizeof(nameBuffer));

		// Like camera, terrain is currently fetched by name, so this is disabled.
		// currentEntity->SetName(nameBuf);

		bool entityEnabled = currentEntity->GetEnableDisable();
		ImGui::Checkbox("Enabled ", &entityEnabled);
		currentEntity->SetEnableDisable(entityEnabled);

		UIPositionEdit = currentEntity->GetTransform()->GetPosition();
		UIRotationEdit = currentEntity->GetTransform()->GetPitchYawRoll();
		UIScaleEdit = currentEntity->GetTransform()->GetScale();

		ImGui::DragFloat3("Position ", &UIPositionEdit.x, 0.5f);
		ImGui::DragFloat3("Rotation ", &UIRotationEdit.x, 0.5f, 0, 360);
		ImGui::InputFloat3("Scale ", &UIScaleEdit.x);

		currentEntity->GetTransform()->SetPosition(UIPositionEdit.x, UIPositionEdit.y, UIPositionEdit.z);
		currentEntity->GetTransform()->SetRotation(UIRotationEdit.x, UIRotationEdit.y, UIRotationEdit.z);
		currentEntity->GetTransform()->SetScale(UIScaleEdit.x, UIScaleEdit.y, UIScaleEdit.z);

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

	if (camWindowEnabled) {
		ImGui::Begin("Camera Editor");

		std::shared_ptr<Camera> currentCam = globalAssets.GetCameraAtID(camUIIndex);

		std::string indexStr = currentCam->GetName();
		std::string node = "Editing Camera " + indexStr;
		ImGui::Text(node.c_str());

		if (ImGui::ArrowButton("Previous Camera", ImGuiDir_Left)) {
			camUIIndex--;
			if (camUIIndex < 0) {
				camUIIndex = globalAssets.GetCameraArraySize() - 1;
			}
		};
		ImGui::SameLine();

		if (ImGui::ArrowButton("Next Camera", ImGuiDir_Right)) {
			camUIIndex++;
			if (camUIIndex > globalAssets.GetCameraArraySize() - 1) {
				camUIIndex = 0;
			}
		};

		std::string nameBuffer;
		static char nameBuf[64] = "";
		nameBuffer = currentCam->GetName();
		strcpy_s(nameBuf, nameBuffer.c_str());
		ImGui::InputText("Rename Camera (disabled) ", nameBuf, sizeof(nameBuffer));

		// Wait, isn't this a really bad idea?
		// 30 Minutes later, I have determined that this was, in fact a terrible idea.
		//currentCam->SetName(nameBuf);

		float fov = currentCam->GetFOV();
		ImGui::SliderFloat("FOV", &fov, 0, XM_PI - 0.01f);
		currentCam->SetFOV(fov);

		float nearDist = currentCam->GetNearDist();
		ImGui::SliderFloat("Near Distance", &nearDist, 0.001f, 1.0f);
		currentCam->SetNearDist(nearDist);

		float farDist = currentCam->GetFarDist();
		ImGui::SliderFloat("Far Distance", &farDist, 100.0f, 1000.0f);
		currentCam->SetFarDist(farDist);

		float lookSpeed = currentCam->GetLookSpeed();
		ImGui::SliderFloat("Look Speed", &lookSpeed, 0.5f, 10.0f);
		currentCam->SetLookSpeed(lookSpeed);

		float moveSpeed = currentCam->GetMoveSpeed();
		ImGui::SliderFloat("Move Speed", &moveSpeed, 1.0f, 20.0f);
		currentCam->SetMoveSpeed(moveSpeed);

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
			ImGui::MenuItem("Particles", "p", &particleWindowEnabled);
			ImGui::MenuItem("Terrain", "t", &terrainWindowEnabled);
			ImGui::MenuItem("Object Hierarchy", "h", &objHierarchyEnabled);
			ImGui::MenuItem("Skies", "", &skyWindowEnabled);
			ImGui::MenuItem("Sound", "", &soundWindowEnabled);
			ImGui::MenuItem("Camera", "c", &camWindowEnabled);

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
		skyUIIndex++;
		if (skyUIIndex > skies->size() - 1) {
			skyUIIndex = 0;
		}
	}
	else if (input.KeyPress(VK_LEFT)) {
		skyUIIndex--;
		if (skyUIIndex < 0) {
			skyUIIndex = skies->size() - 1;
		}
	}

	renderer->SetActiveSky(skies->at(skyUIIndex));
}

void Game::Flashlight() {
	Light* flashlight = globalAssets.GetFlashlight();
	if (input.TestKeyAction(KeyActions::ToggleFlashlight)) {
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
	if (input.TestKeyAction(KeyActions::ToggleFlashlightFlicker)) {
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

	renderer->PreResize();

	// Handle base-level DX resize stuff
	DXCore::OnResize();

	renderer->PostResize(this->height, this->width, this->backBufferRTV, this->depthStencilView);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	audioHandler.GetSoundSystem()->update();

	RenderUI(deltaTime);

	// To untie something from framerate, multiply by deltatime

	// Quit if the escape key is pressed
	if (input.TestKeyAction(KeyActions::QuitGame)) Quit();

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