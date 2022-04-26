#include "../Headers/Game.h"
#include "../Headers/Time.h"

// Needed for a helper function to read compiled shader files from the hard drive
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>
#include "..\Headers\ComponentManager.h"

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
#endif

	// Check if the directory defines are correct
	char pathBuf[1024];

	GetFullPathNameA("SHOE.exe", sizeof(pathBuf), pathBuf, NULL);
	std::string::size_type pos = std::string(pathBuf).find_last_of("\\/");

	std::string currentSubPath = std::string(pathBuf);// .substr(40, pos);

	if (currentSubPath.find("SHOE\\x64\\") != std::string::npos) {
		SetBuildAssetPaths();
	}
	else {
		SetVSAssetPaths();
	}
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

	delete loadingSpriteBatch;
}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	loadingMutex = new std::mutex();
	notification = new std::condition_variable();

	// Set up the multithreading for the loading screen
	globalAssets.SetAMLoadState(AMLoadState::INITIALIZING);

	loadingSpriteBatch = new SpriteBatch(context.Get());

#if defined(DEBUG) || defined(_DEBUG)
	printf("Took %3.4f seconds for pre-initialization. \n", this->GetTotalTime());
#endif

	// Start the loading thread and the loading screen thread
	std::thread loadingThread = std::thread( [this] { globalAssets.Initialize(device, context, notification, loadingMutex, hWnd); });
	std::thread screenThread = std::thread([this] { this->DrawLoadingScreen(globalAssets.GetAMLoadState()); });

	// Once they've stopped passing control back and forth, join them
	// to the main thread
	screenThread.join();
	loadingThread.join();

#if defined(DEBUG) || defined(_DEBUG)
	printf("Took %3.4f seconds for main initialization. \n", this->GetDeltaTime());
#endif

	mainCamera = globalAssets.GetMainCamera();
	mainShadowCamera = globalAssets.GetCameraByName("mainShadowCamera");
	flashShadowCamera = globalAssets.GetCameraByName("flashShadowCamera");
	flashlight = globalAssets.GetGameEntityByName("Flashlight")->GetComponent<Light>();

	Entities = globalAssets.GetActiveGameEntities();

	// Initialize the input manager with the window's handle
	Input::GetInstance().Initialize(this->hWnd);
	statsEnabled = true;
	movingEnabled = true;
	lightWindowEnabled = false;
	objWindowEnabled = false;
	skyWindowEnabled = false;
	objHierarchyEnabled = true;
	rtvWindowEnabled = false;

	flashMenuToggle = false;
	camUIIndex = 0;
	skyUIIndex = 0;

	skies = globalAssets.GetSkyArray();
	
	// Tell the input assembler stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our data?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//With everything initialized, start the renderer
	renderer = std::make_unique<Renderer>(height,
										  width,
										  device,
										  context,
										  swapChain,
										  backBufferRTV,
										  depthStencilView);

#if defined(DEBUG) || defined(_DEBUG)
	printf("Took %3.4f seconds for  post-initialization. \n", this->GetDeltaTime());
	printf("Total Initialization time was %3.4f seconds. \n", this->GetTotalTime());
#endif
}

void Game::LoadScene() {
	globalAssets.LoadScene("structureTest.json");
}

void Game::SaveScene() {
	globalAssets.SaveScene("structureTest.json");
}

void Game::SaveSceneAs() {
	// Handle file browser popup before anything else



	globalAssets.SaveScene("structureTest.json");
}

void Game::RenderUI() {
	// Reset the gui state to prevent tainted input
	input.SetGuiKeyboardCapture(false);
	input.SetGuiMouseCapture(false);

	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = Time::deltaTime;
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

		infoStr = std::to_string(Light::GetLightArrayCount());
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

		//Displays all components on the object
		std::vector<std::shared_ptr<IComponent>> componentList = currentEntity->GetAllComponents();

		// Transform is a special case, as it cannot be fetched by dynamic_pointer_cast
		ImGui::Separator();

		UIPositionEdit = currentEntity->GetTransform()->GetLocalPosition();
		UIRotationEdit = currentEntity->GetTransform()->GetLocalPitchYawRoll();
		UIScaleEdit = currentEntity->GetTransform()->GetLocalScale();

		ImGui::DragFloat3("Position ", &UIPositionEdit.x, 0.5f);
		ImGui::DragFloat3("Rotation ", &UIRotationEdit.x, 0.5f, 0, 360);
		ImGui::InputFloat3("Scale ", &UIScaleEdit.x);

		currentEntity->GetTransform()->SetPosition(UIPositionEdit.x, UIPositionEdit.y, UIPositionEdit.z);
		currentEntity->GetTransform()->SetRotation(UIRotationEdit.x, UIRotationEdit.y, UIRotationEdit.z);
		currentEntity->GetTransform()->SetScale(UIScaleEdit.x, UIScaleEdit.y, UIScaleEdit.z);

		for(int c = 0; c < componentList.size(); c++)
		{
			ImGui::Separator();
			ImGui::PushID(101 + c);

			if(std::dynamic_pointer_cast<MeshRenderer>(componentList[c]) != nullptr)
			{
				std::shared_ptr<MeshRenderer> meshRenderer = std::dynamic_pointer_cast<MeshRenderer>(componentList[c]);
				ImGui::Text("MeshRenderer");

				bool meshEnabled = meshRenderer->IsLocallyEnabled();
				ImGui::Checkbox("Enabled ", &meshEnabled);
				if(meshEnabled != meshRenderer->IsLocallyEnabled())
					meshRenderer->SetEnabled(meshEnabled);

				ImGui::Checkbox("Render Bounds ", &meshRenderer->DrawBounds);

				// Material changes
				if (ImGui::CollapsingHeader("Material Swapping")) {
					static int materialIndex = 0;

					std::string nameBuffer;
					static char nameBuf[64] = "";
					nameBuffer = meshRenderer->GetMaterial()->GetName();
					strcpy_s(nameBuf, nameBuffer.c_str());

					ImGui::Text(nameBuf);
					if (ImGui::BeginListBox("MaterialList")) {
						for (int i = 0; i < globalAssets.GetMaterialArraySize(); i++) {
							const bool is_selected = (materialIndex == i);
							if (ImGui::Selectable(globalAssets.GetMaterialAtID(i)->GetName().c_str(), is_selected)) {
								materialIndex = i;
							}

							if (is_selected) ImGui::SetItemDefaultFocus();
						}

						ImGui::EndListBox();
					}

					/*ImGui::LabelText("Switch to Selected Material: ", 0);
					ImGui::SameLine();*/
					if (ImGui::Button("Swap")) {
						meshRenderer->SetMaterial(globalAssets.GetMaterialAtID(materialIndex));
					}

					float currentTiling = meshRenderer->GetMaterial()->GetTiling();
					ImGui::InputFloat("Change UV Tiling", &currentTiling);
					meshRenderer->GetMaterial()->SetTiling(currentTiling);
				}

				// Mesh Swapping
				if (ImGui::CollapsingHeader("Mesh Swapping")) {
					static int meshIndex = 0;

					std::string nameBuffer;
					static char nameBuf[64] = "";
					nameBuffer = meshRenderer->GetMesh()->GetName();
					strcpy_s(nameBuf, nameBuffer.c_str());

					ImGui::Text(nameBuf);
					if (ImGui::BeginListBox("MeshList")) {
						for (int i = 0; i < globalAssets.GetMeshArraySize(); i++) {
							const bool is_selected = (meshIndex == i);
							if (ImGui::Selectable(globalAssets.GetMeshAtID(i)->GetName().c_str(), is_selected)) {
								meshIndex = i;
							}

							if (is_selected) ImGui::SetItemDefaultFocus();
						}

						ImGui::EndListBox();
					}

					if (ImGui::Button("Swap")) {
						meshRenderer->SetMesh(globalAssets.GetMeshAtID(meshIndex));
					}
				}
			}

			if (std::dynamic_pointer_cast<ParticleSystem>(componentList[c]) != nullptr)
			{
				std::shared_ptr<ParticleSystem> particleSystem = std::dynamic_pointer_cast<ParticleSystem>(componentList[c]);
				ImGui::Text("ParticleSystem");

				bool emitterEnabled = particleSystem->IsLocallyEnabled();
				ImGui::Checkbox("Enabled ", &emitterEnabled);
				if (emitterEnabled != particleSystem->IsLocallyEnabled())
					particleSystem->SetEnabled(emitterEnabled);

				XMFLOAT4 currentTint = particleSystem->GetColorTint();
				ImGui::ColorEdit3("Color ", &currentTint.x);
				particleSystem->SetColorTint(currentTint);

				bool blendState = particleSystem->GetBlendState();
				ImGui::Checkbox("Blend State ", &blendState);
				ImGui::SameLine();
				if (blendState) {
					ImGui::Text("Blend state is additive.");
				}
				else {
					ImGui::Text("Blend state is not additive.");
				}
				particleSystem->SetBlendState(blendState);

				float scale = particleSystem->GetScale();
				ImGui::SliderFloat("Scale with age ", &scale, 0.0f, 2.0f);
				particleSystem->SetScale(scale);

				float particlesPerSecond = particleSystem->GetParticlesPerSecond();
				ImGui::SliderFloat("Particles per Second ", &particlesPerSecond, 0.1f, 20.0f);
				ImGui::SameLine();
				ImGui::InputFloat("#ExtraEditor", &particlesPerSecond);
				particleSystem->SetParticlesPerSecond(particlesPerSecond);

				float particlesLifetime = particleSystem->GetParticleLifetime();
				ImGui::SliderFloat("Particles Lifetime ", &particlesLifetime, 0.1f, 20.0f);
				ImGui::SameLine();
				ImGui::InputFloat("#ExtraEditor2", &particlesLifetime);
				particleSystem->SetParticleLifetime(particlesLifetime);

				float speed = particleSystem->GetSpeed();
				ImGui::SliderFloat("Particle Speed ", &speed, 0.1f, 5.0f);
				particleSystem->SetSpeed(speed);

				XMFLOAT3 destination = particleSystem->GetDestination();
				ImGui::InputFloat3("Particles Move Towards ", &destination.x);
				particleSystem->SetDestination(destination);

				int maxParticles = particleSystem->GetMaxParticles();
				ImGui::InputInt("Max Particles ", &maxParticles);
				particleSystem->SetMaxParticles(maxParticles);
			}

			if (std::dynamic_pointer_cast<Terrain>(componentList[c]) != nullptr)
			{
				std::shared_ptr<Terrain> terrain = std::dynamic_pointer_cast<Terrain>(componentList[c]);
				ImGui::Text("Terrain");

				bool terrainEnabled = terrain->IsLocallyEnabled();
				ImGui::Checkbox("Enabled ", &terrainEnabled);
				if (terrainEnabled != terrain->IsLocallyEnabled())
					terrain->SetEnabled(terrainEnabled);
			}

			if (std::dynamic_pointer_cast<Collider>(componentList[c]) != nullptr)
			{
				std::shared_ptr<Collider> currentCollider = std::dynamic_pointer_cast<Collider>(componentList[c]);
				currentCollider->GetTriggerStatus() ? ImGui::Text("TriggerBox") : ImGui::Text("Collider");

				bool colliderEnabled = currentCollider->IsLocallyEnabled();
				ImGui::Checkbox("Enabled ", &colliderEnabled);
				if (colliderEnabled != currentCollider->IsLocallyEnabled())
					currentCollider->SetEnabled(colliderEnabled);

				bool UIDrawCollider = currentCollider->GetVisibilityStatus();
				ImGui::Checkbox("Draw Collider?", &UIDrawCollider);
				UIDrawCollider ? currentCollider->SetVisibilityStatus(true) : currentCollider->SetVisibilityStatus(false);

				bool UIDrawColliderTransform = currentCollider->GetTransformVisibilityStatus();
				ImGui::Checkbox("Draw Transform?", &UIDrawColliderTransform);
				UIDrawColliderTransform ? currentCollider->SetTransformVisibilityStatus(true) : currentCollider->SetTransformVisibilityStatus(false);

				bool UITriggerSwitch = currentCollider->GetTriggerStatus();
				ImGui::Checkbox("Is this a TriggerBox?", &UITriggerSwitch);
				UITriggerSwitch ? currentCollider->SetTriggerStatus(true) : currentCollider->SetTriggerStatus(false);

				ImGui::NewLine();

				// Don't edit collider transforms from the UI - they use the main entity transform
			}

			if (std::dynamic_pointer_cast<Light>(componentList[c]) != nullptr)
			{
				std::shared_ptr<Light> light = std::dynamic_pointer_cast<Light>(componentList[c]);
				ImGui::Text("Light");

				bool lightEnabled = light->IsLocallyEnabled();
				ImGui::Checkbox("Enabled ", &lightEnabled);
				if (lightEnabled != light->IsLocallyEnabled())
					light->SetEnabled(lightEnabled);

				UILightType = light->GetType();
				ImGui::DragFloat("Type ", &UILightType, 1.0f, 0.0f, 2.0f);
				light->SetType(UILightType);

				//Directional Light
				if (light->GetType() == 0.0f || light->GetType() == 2.0f) {
					UILightDirectionEdit = light->GetDirection();
					ImGui::InputFloat3("Direction ", &UILightDirectionEdit.x);
					light->SetDirection(UILightDirectionEdit);
				}
				//Point Light
				if (light->GetType() == 1.0f || light->GetType() == 2.0f) {
					UILightRange = light->GetRange();
					ImGui::DragFloat("Range ", &UILightRange, 1, 5.0f, 20.0f);
					light->SetRange(UILightRange);
				}
				UILightColorEdit = light->GetColor();
				ImGui::ColorEdit3("Color ", &UILightColorEdit.x);
				light->SetColor(UILightColorEdit);
				UILightIntensity = light->GetIntensity();
				ImGui::DragFloat("Intensity ", &UILightIntensity, 0.1f, 0.01f, 1.0f);
				light->SetIntensity(UILightIntensity);
			}

			// Remove Component Button
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 1.0f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(1.0f, 1.0f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.35f, 1.0f, 0.6f));
			if (ImGui::Button("Remove Component")) {
				currentEntity->RemoveComponent(componentList[c]);
			}
			ImGui::PopStyleColor(3);
			ImGui::PopID();
		}

		ImGui::Separator();

		// Dropdown and Collapsible Header to add components
		if (ImGui::CollapsingHeader("Add Component")) {
			static ComponentTypes selectedComponent = ComponentTypes::MESH_RENDERER;
			static std::string typeArray[ComponentTypes::COMPONENT_TYPE_COUNT] = { "Transform", "Mesh Renderer", "Particle System", "Collider", "Terrain", "Light"};

			if (ImGui::BeginListBox("Component Listbox")) {
				for (int i = 0; i < ComponentTypes::COMPONENT_TYPE_COUNT; i++) {
					const bool is_selected = (selectedComponent == i);
					if (ImGui::Selectable(typeArray[i].c_str(), is_selected))
						selectedComponent = (ComponentTypes)i;
				}

				ImGui::EndListBox();
			}

			ImGui::SameLine();

			ImGui::PushID(100);
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.3f, 1.0f, 0.56f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.3f, 1.0f, 0.87f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.65f, 1.0f, 0.5f));
			if (ImGui::Button("Add Selected Component")) {
				switch (selectedComponent) {
					case ComponentTypes::TRANSFORM:
						// No.
						break;
					case ComponentTypes::MESH_RENDERER:
						currentEntity->AddComponent<MeshRenderer>();
						break;
					case ComponentTypes::PARTICLE_SYSTEM:
						currentEntity->AddComponent<ParticleSystem>();
						break;
					case ComponentTypes::COLLIDER:
						currentEntity->AddComponent<Collider>();
						break;
					case ComponentTypes::TERRAIN:
						currentEntity->AddComponent<Terrain>();
						break;
					case ComponentTypes::LIGHT:
						currentEntity->AddComponent<Light>();
						break;
				}
			}
			ImGui::PopStyleColor(3);
			ImGui::PopID();
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
			for (int i = 0; i < globalAssets.GetGameEntityArraySize(); i++) {
				if (Entities->at(i)->GetTransform()->GetParent() == nullptr) {
					RenderChildObjectsInUI(Entities->at(i));
				}
			}

			ImGui::TreePop();
		}
	}

	if (rtvWindowEnabled) {
		ImGui::Begin("Multiple Render Target Viewer");

		if (ImGui::CollapsingHeader("MRT Effects")) {
			ImGui::Text("Color Without Ambient");
			ImGui::Image(renderer->GetRenderTargetSRV(RTVTypes::COLORS_NO_AMBIENT).Get(), ImVec2(500, 300));
			ImGui::Text("Ambient Color");
			ImGui::Image(renderer->GetRenderTargetSRV(RTVTypes::COLORS_AMBIENT).Get(), ImVec2(500, 300));
			ImGui::Text("Normals");
			ImGui::Image(renderer->GetRenderTargetSRV(RTVTypes::NORMALS).Get(), ImVec2(500, 300));
			ImGui::Text("Depths");
			ImGui::Image(renderer->GetRenderTargetSRV(RTVTypes::DEPTHS).Get(), ImVec2(500, 300));
			ImGui::Text("SSAO");
			ImGui::Image(renderer->GetRenderTargetSRV(RTVTypes::SSAO_RAW).Get(), ImVec2(500, 300));
			ImGui::Text("SSAO Post Blur");
			ImGui::Image(renderer->GetRenderTargetSRV(RTVTypes::SSAO_BLUR).Get(), ImVec2(500, 300));
			ImGui::Text("Composite");
			ImGui::Image(renderer->GetRenderTargetSRV(RTVTypes::COMPOSITE).Get(), ImVec2(500, 300));
		}

		if (ImGui::CollapsingHeader("Shadow Depth Views")) {
			ImGui::Text("Environmental Shadows");
			ImGui::Image(renderer->GetMiscEffectSRV(MiscEffectSRVTypes::ENV_SHADOW).Get(), ImVec2(500, 300));
			ImGui::Text("Flashlight Shadows");
			ImGui::Image(renderer->GetMiscEffectSRV(MiscEffectSRVTypes::FLASHLIGHT_SHADOW).Get(), ImVec2(500, 300));
		}

		if (ImGui::CollapsingHeader("Depth Prepass Views")) {
			ImGui::Text("Refraction Silhouette Depths");
			ImGui::Image(renderer->GetRenderTargetSRV(RTVTypes::REFRACTION_SILHOUETTE).Get(), ImVec2(500, 300));
			ImGui::Text("Transparency Depth Prepass");
			ImGui::Image(renderer->GetMiscEffectSRV(MiscEffectSRVTypes::TRANSPARENT_PREPASS_DEPTHS).Get(), ImVec2(500, 300));
			ImGui::Text("Render Depth Prepass (used for optimization)");
			ImGui::Image(renderer->GetMiscEffectSRV(MiscEffectSRVTypes::RENDER_PREPASS_DEPTHS).Get(), ImVec2(500, 300));
		}

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
	
	if (collidersWindowEnabled)
	{
		ImGui::Begin("Collider Inspector");
		
		// Currently commented as it's not needed for the bulk operations,
		// and has been moved to the components UI
		/*static int colliderUIIndex = 0;
		std::shared_ptr<Collider> currentCollider = ComponentManager::GetAll<Collider>()[colliderUIIndex];
		std::string indexStr = std::to_string(colliderUIIndex) + " - " + currentCollider->GetOwner()->GetName();
		std::string node = "Editing collider " + indexStr;
		ImGui::Text(node.c_str());
		
		if (ImGui::ArrowButton("Previous Collider", ImGuiDir_Left)) {
			colliderUIIndex--;
			if (colliderUIIndex < 0) colliderUIIndex = ComponentManager::GetAll<Collider>().size() - 1;
		};
		ImGui::SameLine();
		
		if (ImGui::ArrowButton("Next Collider", ImGuiDir_Right)) {
			colliderUIIndex++;
			if (colliderUIIndex > ComponentManager::GetAll<Collider>().size() - 1) colliderUIIndex = 0;
		};*/

		ImGui::Text("Collider bulk operations:");

		bool UIDrawColliders = Renderer::GetDrawColliderStatus();
		ImGui::Checkbox("Draw Colliders?", &UIDrawColliders);
		UIDrawColliders ? Renderer::SetDrawColliderStatus(true) : Renderer::SetDrawColliderStatus(false);
		
		bool UIDrawColliderTransforms = Renderer::GetDrawColliderTransformsStatus() & UIDrawColliders;
		ImGui::Checkbox("Draw Colliders' Transforms?", &UIDrawColliderTransforms);
		UIDrawColliderTransforms ? Renderer::SetDrawColliderTransformsStatus(true) : Renderer::SetDrawColliderTransformsStatus(false);

		// Don't edit collider transforms from the UI - they use the main entity transform
		
		ImGui::End();
	}

	// TODO: Add Material Edit menu

	// Display a menu at the top
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			ImGui::Text("This menu will eventually contain a saving and loading system, designed for swapping between feature test scenes.");
			if (ImGui::MenuItem("Save Scene", "ctrl+s")) {
				SaveScene();
			}

			if (ImGui::MenuItem("Save Scene As", "ctrl+s")) {
				SaveSceneAs();
			}

			if (ImGui::MenuItem("Load Scene", "ctrl+s")) {
				LoadScene();
			}
			//ImGui::MenuItem("Toggle Flashlight", "f", &flashMenuToggle);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit")) {
			ImGui::MenuItem("Lights", "l", &lightWindowEnabled);
			ImGui::MenuItem("GameObjects", "g", &objWindowEnabled);
			ImGui::MenuItem("Object Hierarchy", "h", &objHierarchyEnabled);
			ImGui::MenuItem("Skies", "", &skyWindowEnabled);
			ImGui::MenuItem("Sound", "", &soundWindowEnabled);
			ImGui::MenuItem("Camera", "c", &camWindowEnabled);
			ImGui::MenuItem("Colliders", "", &collidersWindowEnabled);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			ImGui::MenuItem("Render Target Views", 0, &rtvWindowEnabled);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Add")) {
			ImGui::Text("This menu will allow easily adding more objects and lights.");

			if (ImGui::Button("Add GameObject")) {
				globalAssets.CreateGameEntity("GameEntity" + std::to_string(globalAssets.GetGameEntityArraySize()));

				entityUIIndex = globalAssets.GetGameEntityArraySize() - 1;
				objWindowEnabled = true;
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

void Game::RenderChildObjectsInUI(std::shared_ptr<GameEntity> entity) {
	std::string nodeName = entity->GetName();
	if (ImGui::TreeNodeEx(nodeName.c_str(), 
		ImGuiTreeNodeFlags_DefaultOpen |
		ImGuiTreeNodeFlags_FramePadding)) {
		int childCount = entity->GetTransform()->GetChildCount();
		if (childCount > 0) {
			std::vector<std::shared_ptr<GameEntity>> children = entity->GetTransform()->GetChildrenEntities();
			for (int i = 0; i < childCount; i++) {
				RenderChildObjectsInUI(children[i]);
			}
		}

		if (ImGui::IsItemClicked()) {
			entityUIIndex = globalAssets.GetGameEntityIDByName(entity->GetName());
			objWindowEnabled = true;
		}

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PARENTING_CELL")) 
			{
				IM_ASSERT(payload->DataSize == sizeof(int));
				int payload_n = *(const int*)payload->Data;

				// Logic to parent objects and reorder list
				std::shared_ptr<GameEntity> sourceEntity = globalAssets.GetGameEntityByID(payload_n);

				sourceEntity->GetTransform()->SetParent(entity->GetTransform());

				// Re-render children list
				for (int i = 0; i < globalAssets.GetGameEntityArraySize(); i++) {
					if (Entities->at(i)->GetTransform()->GetParent() == NULL) {
						RenderChildObjectsInUI(Entities->at(i));
					}
				}
			}

			ImGui::EndDragDropTarget();
		}

		if (ImGui::BeginDragDropSource()) {
			ImGui::SetDragDropPayload("PARENTING_CELL", &entityUIIndex, sizeof(int));

			ImGui::EndDragDropSource();
		}

		ImGui::TreePop();
	}
}

std::shared_ptr<GameEntity> Game::GetClickedEntity()
{
	//Load necessary vectors and matrices
	XMMATRIX projectionMatrix = XMLoadFloat4x4(&mainCamera->GetProjectionMatrix());
	XMMATRIX viewMatrix = XMLoadFloat4x4(&mainCamera->GetViewMatrix());

	//Convert screen position to ray
	//Based on https://stackoverflow.com/questions/39376687/mouse-picking-with-ray-casting-in-directx
	XMVECTOR origin = XMVector3Unproject(
		XMLoadFloat3(&XMFLOAT3(input.GetMouseX(), input.GetMouseY(), 0)),
		0,
		0,
		width,
		height,
		0,
		1,
		projectionMatrix,
		viewMatrix,
		XMMatrixIdentity());

	XMVECTOR destination = XMVector3Unproject(
		XMLoadFloat3(&XMFLOAT3(input.GetMouseX(), input.GetMouseY(), 1)),
		0,
		0,
		width,
		height,
		0,
		1,
		projectionMatrix,
		viewMatrix,
		XMMatrixIdentity());

	XMVECTOR direction = XMVector3Normalize(destination - origin);

	//Raycast against MeshRenderer bounds
	std::shared_ptr<GameEntity> closestHitEntity = nullptr;
	float distToHit = mainCamera->GetFarDist();
	float rayLength = mainCamera->GetFarDist();

	for (std::shared_ptr<MeshRenderer> meshRenderer : ComponentManager::GetAllEnabled<MeshRenderer>()) 
	{
		if (meshRenderer->GetBounds().Intersects(origin, direction, rayLength)) {
			std::shared_ptr<Mesh> mesh = meshRenderer->GetMesh();
			XMMATRIX worldMatrix = XMLoadFloat4x4(&meshRenderer->GetTransform()->GetWorldMatrix());
			Vertex* vertices = mesh->GetVertexArray();
			unsigned int* indices = mesh->GetIndexArray();
			float distToTri;
			
			for (int i = 0; i < mesh->GetIndexCount(); i += 3) {
				XMVECTOR vertex0 = XMVector3Transform(XMLoadFloat3(&vertices[indices[i]].Position), worldMatrix);
				XMVECTOR vertex1 = XMVector3Transform(XMLoadFloat3(&vertices[indices[i + 1]].Position), worldMatrix);
				XMVECTOR vertex2 = XMVector3Transform(XMLoadFloat3(&vertices[indices[i + 2]].Position), worldMatrix);
				if (DirectX::TriangleTests::Intersects(origin, direction, vertex0, vertex1, vertex2, distToTri) && distToTri < distToHit)
				{
					distToHit = distToTri;
					closestHitEntity = meshRenderer->GetGameEntity();
				}
			}
		}
	}

	return closestHitEntity;
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
	if (input.TestKeyAction(KeyActions::ToggleFlashlight)) {
		flashMenuToggle = !flashMenuToggle;
		flashlight->SetEnabled(flashMenuToggle);
	}

	if (flashMenuToggle) {
		XMFLOAT3 camPos = mainCamera->GetTransform()->GetGlobalPosition();
		flashlight->GetTransform()->SetPosition(XMFLOAT3(camPos.x + 0.5f, camPos.y, camPos.z + 0.5f));
		flashlight->SetDirection(mainCamera->GetTransform()->GetForward());
		flashShadowCamera->GetTransform()->SetPosition(flashlight->GetTransform()->GetGlobalPosition());
		flashShadowCamera->GetTransform()->SetRotation(mainCamera->GetTransform()->GetLocalPitchYawRoll());
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
void Game::Update()
{
	audioHandler.GetSoundSystem()->update();

	RenderUI();

	// To untie something from framerate, multiply by Time::deltaTime

	// Quit if the escape key is pressed
	if (input.TestKeyAction(KeyActions::QuitGame)) Quit();

	globalAssets.BroadcastGlobalEntityEvent(EntityEventType::Update);

	if (movingEnabled) {
		//globalAssets.GetGameEntityByName("Floor Helix")->GetTransform()->SetPosition((float)sin(Time::totalTime), +2.0f, +0.0f);
		globalAssets.GetGameEntityByName("Bronze Cube")->GetTransform()->SetPosition(+1.5f, (float)sin(Time::totalTime) + 2.5f, +0.0f);
		globalAssets.GetGameEntityByName("Bronze Cube")->GetTransform()->Rotate( 0.0f, 0.0f, -(float)sin(Time::deltaTime));
		//globalAssets.GetGameEntityByName("Floor Cube")->GetTransform()->SetPosition( (float)sin(Time::deltaTime) + 2.5f, +1.0f, +0.0f);
		globalAssets.GetGameEntityByName("Bronze Cube")->GetTransform()->Rotate( 0.0f, (float)sin(Time::deltaTime), 0.0f);
		globalAssets.GetGameEntityByName("Scratched Cube")->GetTransform()->Rotate(-(float)sin(Time::deltaTime), 0.0f, 0.0f);

		globalAssets.GetGameEntityByName("Stone Cylinder")->GetTransform()->SetPosition(-2.0f, (float)sin(Time::totalTime), +0.0f);

		globalAssets.GetGameEntityByName("Paint Sphere")->GetTransform()->SetPosition(-(float)sin(Time::totalTime), -2.0f, +0.0f);

		globalAssets.GetGameEntityByName("Rough Torus")->GetTransform()->Rotate(+0.0f, +0.0f, +1.0f * Time::deltaTime);
	}

	Flashlight();
	RenderSky();

	CollisionManager::Update();

	//Click to select an object
	if (input.MouseRightPress()) {
		clickedEntityBuffer = GetClickedEntity();
	}
	if (input.MouseRightRelease()) {
		if (clickedEntityBuffer != nullptr && clickedEntityBuffer == GetClickedEntity()) {
			objWindowEnabled = true;
			entityUIIndex = globalAssets.GetGameEntityIDByName(clickedEntityBuffer->GetName());
		}
		else {
			objWindowEnabled = false;
			entityUIIndex = -1;
		}
		clickedEntityBuffer = nullptr;
	}

	// Flickering is currently broken
	/*if (flickeringEnabled) {
		srand((unsigned int)(Time::deltaTime * Time::totalTime));
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

	mainCamera->Update(this->hWnd);
	//flashShadowCamera->Update(this->hWnd);
}

void Game::DrawLoadingScreen(AMLoadState loadType) {
	while (globalAssets.GetAMLoadState() != AMLoadState::NOT_LOADING) {
		std::unique_lock<std::mutex> lock(*loadingMutex);
		using time = std::chrono::duration<int, std::milli>;
		if (notification->wait_for(lock, time(3000), [&] {return globalAssets.GetSingleLoadComplete(); })) {
			// Super generic draw code for now
			// Background color (Cornflower Blue in this case) for clearing
			const float color[4] = { 0.0f, 0.0f, 0.1f, 0.0f };

			std::string loadedCategoryString = "Loading " + globalAssets.GetLastLoadedCategory();
			std::string loadedObjectString;

			if (globalAssets.GetLoadingException()) {
				try {
					std::rethrow_exception(globalAssets.GetLoadingException());
				}
				catch (const std::exception& e) {
					loadedObjectString = "Last Object: " + globalAssets.GetLastLoadedObject() + " Failed to Load! Error is printed to DBG console.";
#if defined(DEBUG) || defined(_DEBUG)
					printf(e.what());
#endif
				}
			}
			else {
				loadedObjectString = "Last Object Loaded: " + globalAssets.GetLastLoadedObject();
			}

			context->ClearRenderTargetView(backBufferRTV.Get(), color);
			context->ClearDepthStencilView(
				depthStencilView.Get(),
				D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
				1.0f,
				0);

			// Get fonts
			static std::shared_ptr<SpriteFont> titleFont = globalAssets.GetFontByName("Roboto-Bold-72pt")->spritefont;
			static std::shared_ptr<SpriteFont> categoryFont = globalAssets.GetFontByName("SmoochSans-Bold")->spritefont;
			static std::shared_ptr<SpriteFont> objectFont = globalAssets.GetFontByName("SmoochSans-Italic")->spritefont;

			loadingSpriteBatch->Begin();

			DirectX::XMFLOAT2 titleOrigin;
			DirectX::XMFLOAT2 categoryOrigin;
			DirectX::XMFLOAT2 objectOrigin;

			std::string titleString = "";

			if (loadType == AMLoadState::INITIALIZING) {
				titleString = "SHOE";
				DirectX::XMStoreFloat2(&titleOrigin, titleFont->MeasureString(titleString.c_str()) / 2.0f);
			}
			else if (loadType == AMLoadState::SCENE_LOAD) {
				titleString = "Loading '";
				titleString += globalAssets.GetLoadingSceneName();
				titleString += "'";
				DirectX::XMStoreFloat2(&titleOrigin, titleFont->MeasureString(titleString.c_str()) / 2.0f);
			}
			
			// Certified conversion moment
			DirectX::XMStoreFloat2(&categoryOrigin, categoryFont->MeasureString(loadedCategoryString.c_str()) / 2.0f);
			DirectX::XMStoreFloat2(&objectOrigin, objectFont->MeasureString(loadedObjectString.c_str()) / 2.0f);

			titleFont->DrawString(loadingSpriteBatch, titleString.c_str(), DirectX::XMFLOAT2(width / 2, height / 5), DirectX::Colors::Gold, 0.0f, titleOrigin);
			categoryFont->DrawString(loadingSpriteBatch, loadedCategoryString.c_str(), DirectX::XMFLOAT2(width / 2, height / 1.5), DirectX::Colors::White, 0.0f, categoryOrigin);
			objectFont->DrawString(loadingSpriteBatch, loadedObjectString.c_str(), DirectX::XMFLOAT2(width / 2, height / 1.2), DirectX::Colors::LightGray, 0.0f, objectOrigin);

			loadingSpriteBatch->End();

			swapChain->Present(0, 0);

			context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthStencilView.Get());
		}
		else {
#if defined(DEBUG) || defined(_DEBUG)
			printf("Took too long to load. \n");
#endif
		}	

		globalAssets.SetSingleLoadComplete(false);
		lock.unlock();
		notification->notify_all();
	}
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw()
{
	if (globalAssets.GetAMLoadState() != INITIALIZING || globalAssets.GetAMLoadState() != SCENE_LOAD) {
		//Render shadows before anything else
		if (flashMenuToggle) {
			renderer->RenderShadows(flashShadowCamera, MiscEffectSRVTypes::FLASHLIGHT_SHADOW);
		}

		renderer->RenderShadows(mainShadowCamera, MiscEffectSRVTypes::ENV_SHADOW);

	renderer->Draw(mainCamera);
}