#include "../Headers/EditingUI.h"
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

EditingUI::EditingUI(std::shared_ptr<Renderer> renderer) {
	statsEnabled = true;
	movingEnabled = true;
	objWindowEnabled = false;
	skyWindowEnabled = false;
	objHierarchyEnabled = true;
	rtvWindowEnabled = false;

	entityUIIndex = 0;
	skyUIIndex = 0;
	materialUIIndex = 0;
	terrainMaterialUIIndex = 0;

	this->renderer = renderer;
}

EditingUI::~EditingUI() {

}

void EditingUI::ReInitializeEditingUI(std::shared_ptr<Renderer> renderer) {
	statsEnabled = true;
	movingEnabled = true;
	objWindowEnabled = false;
	skyWindowEnabled = false;
	objHierarchyEnabled = true;
	rtvWindowEnabled = false;

	entityUIIndex = 0;
	skyUIIndex = 0;
	materialUIIndex = 0;
	terrainMaterialUIIndex = 0;

	this->renderer = renderer;
}

void EditingUI::ResetUI() {
	// Reset the gui state to prevent tainted input
	input.SetGuiKeyboardCapture(false);
	input.SetGuiMouseCapture(false);

	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = Time::deltaTime;
	io.DisplaySize.x = (float)dxCore->width;
	io.DisplaySize.y = (float)dxCore->height;
	io.KeyCtrl = input.KeyDown(VK_CONTROL);
	io.KeyAlt = input.KeyDown(VK_SHIFT);
	io.MousePos.x = (float)input.GetMouseX();
	io.MousePos.y = (float)input.GetMouseY();
	io.MouseDown[0] = input.MouseLeftDown();
	io.MouseDown[1] = input.MouseRightDown();
	io.MouseDown[2] = input.MouseMiddleDown();
	io.MouseWheel = input.GetMouseWheel();
	input.GetKeyArray(io.KeysDown, 256);
}

void EditingUI::ResetFrame() {
	// Reset the frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void EditingUI::DisplayMenu() {
	// Display a menu at the top
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			ImGui::Text("This menu will eventually contain a saving and loading system, designed for swapping between feature test scenes.");
			if (ImGui::MenuItem("Save Scene", "ctrl+s")) {
				sceneManager.SaveScene();
			}

			if (ImGui::MenuItem("Save Scene As", "ctrl++shift+s")) {
				sceneManager.SaveSceneAs();
			}

			if (ImGui::MenuItem("Load Scene", "ctrl+s")) {
				char filename[MAX_PATH];
				OPENFILENAME ofn;

				ZeroMemory(&ofn, sizeof(ofn));
				ZeroMemory(&filename, sizeof(filename));
				ofn.lpstrFilter = _T("Scene JSON files (.json)\0*.json;\0Any File\0*.*\0");
				ofn.lpstrTitle = _T("Select a scene file:");
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = dxCore->hWnd;
				ofn.lpstrFile = filename;
				ofn.nMaxFile = MAX_PATH;
				ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

				if (GetOpenFileName(&ofn)) {
					sceneManager.LoadScene(ofn.lpstrFile, true);

					// Reset the shaders used as defaults for DX11 rendering
					// This is a virtual function so DX12 can also use it if it
					// ends up needing it
					renderer->ReloadDefaultShaders();
				}
			}

			ImGui::Separator();

			if (ImGui::BeginMenu("Import New Asset")) {
				if (ImGui::MenuItem("Texture")) {
					globalAssets.ImportTexture();
				}

				if (ImGui::MenuItem("Sky")) {
					globalAssets.ImportSkyTexture();
				}

				if (ImGui::MenuItem("Model/Mesh")) {
					globalAssets.ImportMesh();
				}

				if (ImGui::MenuItem("Audio")) {
					globalAssets.ImportSound();
				}

				if (ImGui::MenuItem("HeightMap")) {
					globalAssets.ImportHeightMap();
				}

				if (ImGui::MenuItem("Font")) {
					globalAssets.ImportFont();
				}

				ImGui::EndMenu();
			}

			ImGui::Separator();

			ImGui::MenuItem("Render", "", GetRenderWindowEnabled());

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit")) {
			ImGui::MenuItem("GameObjects", "g", GetObjWindowEnabled());
			ImGui::MenuItem("Object Hierarchy", "h", GetObjHierarchyEnabled());
			ImGui::MenuItem("Skies", "", GetSkyWindowEnabled());
			ImGui::MenuItem("Sound", "", GetSoundWindowEnabled());
			ImGui::MenuItem("Texture", "", GetTextureWindowEnabled());
			ImGui::MenuItem("Material", "", GetMaterialWindowEnabled());
			ImGui::MenuItem("Terrain Materials", "", GetTerrainMaterialWindowEnabled());
			ImGui::MenuItem("Colliders", "", GetCollidersWindowEnabled());

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			ImGui::MenuItem("Render Target Views", 0, GetRtvWindowEnabled());

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Add")) {
			//ImGui::Text("Add a new GameEntity, which can have components attached.");


			if (ImGui::MenuItem("Add GameEntity", 0, GetObjWindowEnabled())) {
				globalAssets.CreateGameEntity("GameEntity" + std::to_string(globalAssets.GetGameEntityArraySize()));

				SetEntityUIIndex(globalAssets.GetGameEntityArraySize() - 1);
				SetObjWindowEnabled(true);
			}

			if (ImGui::MenuItem("Add Material", 0, GetMaterialWindowEnabled())) {
				std::shared_ptr<Material> new_material = globalAssets.GetMaterialAtID(0);
				globalAssets.CreatePBRMaterial("new material",
												new_material->GetTexture(),
												new_material->GetNormalMap(),
												new_material->GetMetalMap(),
												new_material->GetRoughMap());

				SetMaterialUIIndex(globalAssets.GetMaterialArraySize() - 1);
				SetMaterialWindowEnabled(true);
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Extra")) {
			ImGui::Text("Spare dropdown");

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Toggleables")) {
			//ImGui::MenuItem("Toggle Flashlight Flickering", "v", &flickeringEnabled);
			ImGui::MenuItem("Toggle Stats Menu", ".", GetStatsEnabled());
			ImGui::MenuItem("Toggle movement", "m", GetMovingEnabled());

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}

void EditingUI::RenderChildObjectsInUI(std::shared_ptr<GameEntity> entity) {
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
				std::shared_ptr<GameEntity> sourceEntity = globalAssets.GetGameEntityAtID(payload_n);

				sourceEntity->GetTransform()->SetParent(entity->GetTransform());

				// Re-render children list
				for (int i = 0; i < globalAssets.GetGameEntityArraySize(); i++) {
					if (globalAssets.GetGameEntityAtID(i)->GetTransform()->GetParent() == NULL) {
						RenderChildObjectsInUI(globalAssets.GetGameEntityAtID(i));
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

/// <summary>
/// Determine if any action should be taken based on specific keypresses.
/// </summary>
void EditingUI::TrackHotkeys() {
	if (input.KeyPress(VK_RIGHT)) {
		skyUIIndex++;
		if (skyUIIndex > globalAssets.GetSkyArraySize() - 1) {
			skyUIIndex = 0;
		}
		globalAssets.GetSkyAtID(skyUIIndex);
	}
	else if (input.KeyPress(VK_LEFT)) {
		skyUIIndex--;
		if (skyUIIndex < 0) {
			skyUIIndex = globalAssets.GetSkyArraySize() - 1;
		}
		globalAssets.GetSkyAtID(skyUIIndex);
	}

	if (input.KeyPress('P')) {
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		sceneManager.PrePlaySave();
	}

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
		renderer->selectedEntity = entityUIIndex;
	}
}

std::shared_ptr<GameEntity> EditingUI::GetClickedEntity()
{
	//Load necessary vectors and matrices
	XMMATRIX projectionMatrix = XMLoadFloat4x4(&globalAssets.GetEditingCamera()->GetProjectionMatrix());
	XMMATRIX viewMatrix = XMLoadFloat4x4(&globalAssets.GetEditingCamera()->GetViewMatrix());

	//Convert screen position to ray
	//Based on https://stackoverflow.com/questions/39376687/mouse-picking-with-ray-casting-in-directx
	XMVECTOR origin = XMVector3Unproject(
		XMLoadFloat3(&XMFLOAT3(input.GetMouseX(), input.GetMouseY(), 0)),
		0,
		0,
		dxCore->width,
		dxCore->height,
		0,
		1,
		projectionMatrix,
		viewMatrix,
		XMMatrixIdentity());

	XMVECTOR destination = XMVector3Unproject(
		XMLoadFloat3(&XMFLOAT3(input.GetMouseX(), input.GetMouseY(), 1)),
		0,
		0,
		dxCore->width,
		dxCore->height,
		0,
		1,
		projectionMatrix,
		viewMatrix,
		XMMatrixIdentity());

	XMVECTOR direction = XMVector3Normalize(destination - origin);

	//Raycast against MeshRenderer bounds
	std::shared_ptr<GameEntity> closestHitEntity = nullptr;
	float distToHit = globalAssets.GetEditingCamera()->GetFarDist();
	float rayLength = globalAssets.GetEditingCamera()->GetFarDist();

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

void EditingUI::GenerateEditingUI() {	
	ResetUI();
	ResetFrame();

	// Determine new input capture
	ImGuiIO& io = ImGui::GetIO();

	input.SetGuiKeyboardCapture(io.WantCaptureKeyboard);
	input.SetGuiMouseCapture(io.WantCaptureMouse);

	DisplayMenu();
	TrackHotkeys();

	if (*(GetStatsEnabled())) {
		// Display a UI element for stat tracking
		ImGui::Begin("Stats - Debug Mode");

		std::string infoStr = std::to_string(io.Framerate);
		std::string node = "Current Framerate: " + infoStr;

		ImGui::Text(node.c_str());

		infoStr = std::to_string(dxCore->width);
		std::string infoStrTwo = std::to_string(dxCore->height);
		node = "Window Width: " + infoStr + ", Window Height: " + infoStrTwo;

		ImGui::Text(node.c_str());

		infoStr = std::to_string(Light::GetLightArrayCount());
		node = "Light count: " + infoStr;

		ImGui::Text(node.c_str());

		infoStr = std::to_string(globalAssets.GetGameEntityArraySize());
		node = "Game Entity count: " + infoStr;

		ImGui::Text(node.c_str());

		ImGui::End();
	}

	if (*(GetSkyWindowEnabled())) {
		ImGui::Begin("Sky Editor");

		int skyUIIndex = GetSkyUIIndex();
		std::shared_ptr<Sky> currentSky = globalAssets.GetSkyAtID(skyUIIndex);

		if (ImGui::ArrowButton("Previous Sky", ImGuiDir_Left)) {
			skyUIIndex--;
			if (skyUIIndex < 0) {
				skyUIIndex = globalAssets.GetSkyArraySize() - 1;
			}
			globalAssets.currentSky = globalAssets.GetSkyAtID(skyUIIndex);
		};
		ImGui::SameLine();

		if (ImGui::ArrowButton("Next Sky", ImGuiDir_Right)) {
			skyUIIndex++;
			if (skyUIIndex > globalAssets.GetSkyArraySize() - 1) {
				skyUIIndex = 0;
			}
			globalAssets.currentSky = globalAssets.GetSkyAtID(skyUIIndex);
		};

		std::string nameBuffer;
		static char nameBuf[64] = "";
		nameBuffer = currentSky->GetName();
		strcpy_s(nameBuf, nameBuffer.c_str());
		ImGui::InputText("Rename Sky ", nameBuf, sizeof(nameBuffer));

		currentSky->SetName(nameBuf);

		bool skyEnabled = currentSky->IsEnabled();
		ImGui::Checkbox("Enabled ##SkyEnabled", &skyEnabled);
		currentSky->SetEnabled(skyEnabled);

		bool skyIBLEnabled = currentSky->IsIBLEnabled();
		ImGui::Checkbox("IBL Enabled ##SkyIBLEnabled", &skyIBLEnabled);
		currentSky->SetIBLEnabled(skyIBLEnabled);

		float skyIBLIntensity = currentSky->GetIBLIntensity();
		ImGui::InputFloat("IBL Intensity ##SkyIBLIntensity", &skyIBLIntensity);
		currentSky->SetIBLIntensity(skyIBLIntensity);

		if (skyEnabled && ImGui::CollapsingHeader("BRDF Lookup Texture")) {
			ImGui::Image((ImTextureID*)currentSky->GetBRDFLookupTexture().Get(), ImVec2(256, 256));
		}

		SetSkyUIIndex(skyUIIndex);
		//ImGui::Image(globalAssets.GetEmitterAtID(0)->particleDataSRV.Get(), ImVec2(256, 256));
		ImGui::End();
	}

	if (*(GetTextureWindowEnabled())) {
		static int textureUIIndex = 0;
			std::shared_ptr<Texture> currentTexture = globalAssets.GetTextureAtID(textureUIIndex);
			std::string indexStr = std::to_string(textureUIIndex) + " - " + currentTexture->GetName();
			std::string node = "Viewing texture " + indexStr;
			ImGui::Begin("Texture Editor");
			ImGui::Text(node.c_str());

			if (ImGui::ArrowButton("Previous Texture", ImGuiDir_Left)) {
				textureUIIndex--;
					if (textureUIIndex < 0) textureUIIndex = globalAssets.GetTextureArraySize() - 1;
			};
		ImGui::SameLine();

		if (ImGui::ArrowButton("Next Texture", ImGuiDir_Right)) {
			textureUIIndex++;
			if (textureUIIndex > globalAssets.GetTextureArraySize() - 1) textureUIIndex = 0;
		};

		std::string nameBuffer;
		static char nameBuf[64] = "";
		nameBuffer = currentTexture->GetName();
		strcpy_s(nameBuf, nameBuffer.c_str());
		ImGui::InputText("Rename Texture", nameBuf, sizeof(nameBuffer));

		currentTexture->SetName(nameBuf);

		ImGui::Separator();

		ImTextureID* currentTexDisplay;

		if (dxCore->IsDirectX12()) {
			// Temporary
			currentTexDisplay = NULL;
		}
		else {
			currentTexDisplay = (ImTextureID*)currentTexture->GetDX11Texture().Get();
		}
		ImGui::Image(currentTexDisplay, ImVec2(256, 256));

		ImGui::End();
	}

	if (*(GetObjWindowEnabled())) {
		// Display the debug UI for objects
		int entityUIIndex = GetEntityUIIndex();
		std::shared_ptr<GameEntity> currentEntity = globalAssets.GetGameEntityAtID(entityUIIndex);
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

		bool entityEnabled = currentEntity->GetEnabled();
		ImGui::Checkbox("Enabled: ", &entityEnabled);
		currentEntity->SetEnabled(entityEnabled);

		//Displays all components on the object
		std::vector<std::shared_ptr<IComponent>> componentList = currentEntity->GetAllComponents();

		// Transform is a special case, as it cannot be fetched by dynamic_pointer_cast
		ImGui::Separator();

		UIPositionEdit = currentEntity->GetTransform()->GetLocalPosition();
		UIRotationEdit = currentEntity->GetTransform()->GetLocalPitchYawRoll();
		UIScaleEdit = currentEntity->GetTransform()->GetLocalScale();

		ImGui::DragFloat3("Position ", &UIPositionEdit.x, 0.5f);
		ImGui::DragFloat3("Rotation ", &UIRotationEdit.x, 0.01f, 0, XM_2PI);
		ImGui::InputFloat3("Scale ", &UIScaleEdit.x);

		currentEntity->GetTransform()->SetPosition(UIPositionEdit.x, UIPositionEdit.y, UIPositionEdit.z);
		currentEntity->GetTransform()->SetRotation(UIRotationEdit.x, UIRotationEdit.y, UIRotationEdit.z);
		currentEntity->GetTransform()->SetScale(UIScaleEdit.x, UIScaleEdit.y, UIScaleEdit.z);

		for (int c = 0; c < componentList.size(); c++)
		{
			ImGui::Separator();
			ImGui::PushID(101 + c);

			if (std::shared_ptr<MeshRenderer> meshRenderer = std::dynamic_pointer_cast<MeshRenderer>(componentList[c]))
			{
				ImGui::Text("MeshRenderer");

				bool meshEnabled = meshRenderer->IsLocallyEnabled();
				ImGui::Checkbox("Enabled ", &meshEnabled);
				if (meshEnabled != meshRenderer->IsLocallyEnabled())
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

					if (ImGui::Button("Swap##Material")) {
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

					if (ImGui::Button("Swap##Mesh")) {
						meshRenderer->SetMesh(globalAssets.GetMeshAtID(meshIndex));
					}
				}
			}

			else if (std::shared_ptr<ParticleSystem> particleSystem = std::dynamic_pointer_cast<ParticleSystem>(componentList[c]))
			{
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
				//ImGui::SameLine();
				//ImGui::InputFloat("#ExtraEditor", &particlesPerSecond);
				particleSystem->SetParticlesPerSecond(particlesPerSecond);

				float particlesLifetime = particleSystem->GetParticleLifetime();
				ImGui::SliderFloat("Particles Lifetime ", &particlesLifetime, 0.1f, 20.0f);
				//ImGui::SameLine();
				//ImGui::InputFloat("#ExtraEditor2", &particlesLifetime);
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

				if (ImGui::CollapsingHeader("Particle Texture Swapping")) {
					Texture* currentTexture = particleSystem->GetParticleTexture().get();
					ImTextureID* currentTexDisplay;
					if (dxCore->IsDirectX12()) {
						// Temporary
						currentTexDisplay = NULL;
					}
					else {
						currentTexDisplay = (ImTextureID*)currentTexture->GetDX11Texture().Get();
					}
					ImGui::Image(currentTexDisplay, ImVec2(256, 256));
					ImGui::SameLine();
				}

			}

			else if (std::shared_ptr<Terrain> terrain = std::dynamic_pointer_cast<Terrain>(componentList[c]))
			{
				ImGui::Text("Terrain");

				bool terrainEnabled = terrain->IsLocallyEnabled();
				ImGui::Checkbox("Enabled ", &terrainEnabled);
				if (terrainEnabled != terrain->IsLocallyEnabled())
					terrain->SetEnabled(terrainEnabled);

				ImGui::Checkbox("Render Bounds ", &terrain->DrawBounds);

				// Material changes
				if (ImGui::CollapsingHeader("Terrain Material Swapping")) {
					static int materialIndex = 0;

					std::string nameBuffer;
					static char nameBuf[64] = "";
					nameBuffer = terrain->GetMaterial()->GetName();
					strcpy_s(nameBuf, nameBuffer.c_str());

					ImGui::Text(nameBuf);
					if (ImGui::BeginListBox("TMaterialList")) {
						for (int i = 0; i < globalAssets.GetTerrainMaterialArraySize(); i++) {
							const bool is_selected = (materialIndex == i);
							if (ImGui::Selectable(globalAssets.GetTerrainMaterialAtID(i)->GetName().c_str(), is_selected)) {
								materialIndex = i;
							}

							if (is_selected) ImGui::SetItemDefaultFocus();
						}

						ImGui::EndListBox();
					}

					if (ImGui::Button("Swap ##Terrain Material Swap Button")) {
						terrain->SetMaterial(globalAssets.GetTerrainMaterialAtID(materialIndex));
					}
				}

				// Mesh Swapping
				if (ImGui::CollapsingHeader("Mesh Swapping")) {
					static int meshIndex = 0;

					std::string nameBuffer;
					static char nameBuf[64] = "";
					nameBuffer = terrain->GetMesh()->GetName();
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

					if (ImGui::Button("Swap ##Terrain Mesh Swap Button")) {
						terrain->SetMesh(globalAssets.GetMeshAtID(meshIndex));
					}
				}
			}

			else if (std::shared_ptr<Collider> currentCollider = std::dynamic_pointer_cast<Collider>(componentList[c]))
			{
				ImGui::Text(currentCollider->IsTrigger() ? "Trigger" : "Collider");

				bool colliderEnabled = currentCollider->IsLocallyEnabled();
				ImGui::Checkbox("Enabled ", &colliderEnabled);
				if (colliderEnabled != currentCollider->IsLocallyEnabled())
					currentCollider->SetEnabled(colliderEnabled);

				bool UIDrawCollider = currentCollider->IsVisible();
				ImGui::Checkbox("Render Collider", &UIDrawCollider);
				currentCollider->SetVisible(UIDrawCollider);

				bool UITriggerSwitch = currentCollider->IsTrigger();
				ImGui::Checkbox("Is Trigger", &UITriggerSwitch);
				currentCollider->SetIsTrigger(UITriggerSwitch);

				XMFLOAT3 offsetPos = currentCollider->GetPositionOffset();
				XMFLOAT3 offsetRot = currentCollider->GetRotationOffset();
				XMFLOAT3 offsetScale = currentCollider->GetScale();

				ImGui::DragFloat3("Position Offset ", &offsetPos.x, 0.5f);
				ImGui::DragFloat3("Rotation Offset ", &offsetRot.x, 0.5f, 0, 360);
				ImGui::InputFloat3("Scale ", &offsetScale.x);

				currentCollider->SetPositionOffset(offsetPos);
				currentCollider->SetRotationOffset(offsetRot);
				currentCollider->SetScale(offsetScale);
			}

			else if (std::shared_ptr<Light> light = std::dynamic_pointer_cast<Light>(componentList[c]))
			{
				ImGui::Text("Light");

				bool lightEnabled = light->IsLocallyEnabled();
				ImGui::Checkbox("Enabled ", &lightEnabled);
				if (lightEnabled != light->IsLocallyEnabled())
					light->SetEnabled(lightEnabled);

				UILightType = light->GetType();
				ImGui::Text("Light Type: ");
				ImGui::SameLine();
				ImGui::InputFloat("##LightTypeSelector", &UILightType); //ImGui::SetFloat("Type ", &UILightType, 1.0f, 0.0f, 2.0f);
				ImGui::SameLine();
				if (ImGui::ArrowButton("PrevLightType", ImGuiDir_Left)) {
					if (UILightType != 0) {
						UILightType -= 1;
					}
				}
				ImGui::SameLine();
				if (ImGui::ArrowButton("NextLightType", ImGuiDir_Right)) {
					if (UILightType < 2) {
						UILightType += 1;
					}
				}
				light->SetType(UILightType);

				//Directional Light/Spot Light
				if (light->GetType() == 0.0f || light->GetType() == 2.0f) {
					bool castsShadows = light->CastsShadows();
					ImGui::Checkbox("Casts Shadows ", &castsShadows);
					if ((castsShadows && !light->CastsShadows()) || (!castsShadows && light->CastsShadows())) {
						light->SetCastsShadows(castsShadows);
					}			
				}
				//Point Light/Spot Light
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

				// What?
				//if (ImGui::Button("Mark as main")) {
				//	globalAssets.SetMainCamera(light->GetShadowProjector());
				//}
			}

			else if (std::shared_ptr<Camera> camera = std::dynamic_pointer_cast<Camera>(componentList[c]))
			{
				ImGui::Text("Camera");

				bool camEnabled = camera->IsLocallyEnabled();
				ImGui::Checkbox("Enabled ", &camEnabled);
				if (camEnabled != camera->IsLocallyEnabled())
					camera->SetEnabled(camEnabled);

				bool isPerspective = camera->IsPerspective();
				ImGui::Checkbox("Is Perspective ", &isPerspective);
				camera->SetIsPerspective(isPerspective);

				if (camera->IsPerspective()) {
					float fov = camera->GetFOV();
					ImGui::SliderFloat("FOV", &fov, 0, XM_PI - 0.01f);
					camera->SetFOV(fov);
				}

				float nearDist = camera->GetNearDist();
				ImGui::SliderFloat("Near Distance", &nearDist, 0.001f, 1.0f);
				camera->SetNearDist(nearDist);

				float farDist = camera->GetFarDist();
				ImGui::SliderFloat("Far Distance", &farDist, 100.0f, 1000.0f);
				camera->SetFarDist(farDist);

				if (ImGui::Button("Mark as main")) {
					globalAssets.SetMainCamera(camera);
				}
			}

			else if (std::shared_ptr<NoclipMovement> noclip = std::dynamic_pointer_cast<NoclipMovement>(componentList[c]))
			{
				ImGui::Text("Noclip Movement");

				bool ncEnabled = noclip->IsLocallyEnabled();
				ImGui::Checkbox("Enabled ", &ncEnabled);
				if (ncEnabled != noclip->IsLocallyEnabled())
					noclip->SetEnabled(ncEnabled);

				ImGui::SliderFloat("Move Speed", &noclip->moveSpeed, 1.0f, 20.0f);

				ImGui::SliderFloat("Look Speed", &noclip->lookSpeed, 0.5f, 10.0f);
			}

			else if (std::shared_ptr<FlashlightController> flashlight = std::dynamic_pointer_cast<FlashlightController>(componentList[c]))
			{
				ImGui::Text("Flashlight Controller");

				bool flEnabled = flashlight->IsLocallyEnabled();
				ImGui::Checkbox("Enabled ", &flEnabled);
				if (flEnabled != flashlight->IsLocallyEnabled())
					flashlight->SetEnabled(flEnabled);
			}

			else if (std::shared_ptr<AudioResponse> audioResponder = std::dynamic_pointer_cast<AudioResponse>(componentList[c]))
			{
				ImGui::Text("Audio Response Device");
				ImGui::Text("Use this to link an audio trigger to a graphical effect.");

				bool arEnabled = audioResponder->IsLocallyEnabled();
				ImGui::Checkbox("Enabled ", &arEnabled);
				if (arEnabled != audioResponder->IsLocallyEnabled())
					audioResponder->SetEnabled(arEnabled);

				static int selectedChannel = 0;
				// Still need to initialize sounds correctly through the channel system
				if (ImGui::BeginListBox("Sound to link")) {
					for (int i = 0; i < audioHandler.GetChannelVectorLength(); i++) {
						const bool is_selected = ((int)selectedChannel == i);
						FMOD::Sound* listSound;
						FMOD::Channel* listChannel;
						FMODUserData* uData;

						listChannel = audioHandler.GetChannelByIndex(i);
						listChannel->getCurrentSound(&listSound);
						listSound->getUserData((void**)& uData);

						if (ImGui::Selectable(uData->name->c_str(), is_selected)) {
							selectedChannel = i;
							audioResponder->SetLinkedSound(listChannel);
						}
					}

					ImGui::EndListBox();
				}

				ImGui::Separator();

				static AudioEventTrigger selectedTrigger = AudioEventTrigger::FrequencyAbove;
				static std::string triggerTypeArray[(int)AudioEventTrigger::AudioEventTriggerCount] = { "Frequency Above", "Frequency Below", "Pitch Above", "Pitch Below" };

				if (ImGui::BeginListBox("Audio Event Trigger Listbox")) {
					for (int i = 0; i < (int)AudioEventTrigger::AudioEventTriggerCount; i++) {
						const bool is_selected = ((int)selectedTrigger == i);
						if (ImGui::Selectable(triggerTypeArray[i].c_str(), is_selected)) {
							selectedTrigger = (AudioEventTrigger)(i);
							audioResponder->trigger = selectedTrigger;
						}	
					}

					ImGui::EndListBox();
				}

				ImGui::InputFloat("Trigger Data: ", &audioResponder->triggerComparison);

				ImGui::Separator();

				static AudioEventResponse selectedResponse = AudioEventResponse::Move;
				static std::string responseTypeArray[(int)AudioEventResponse::AudioEventResponseCount] = { "Move", "Rotate", "Scale", "Modify Light Intensity", "Change Light Color" };

				if (ImGui::BeginListBox("Audio Event Response Listbox")) {
					for (int i = 0; i < (int)AudioEventResponse::AudioEventResponseCount; i++) {
						const bool is_selected = ((int)selectedResponse == i);
						if (ImGui::Selectable(responseTypeArray[i].c_str(), is_selected)) {
							selectedResponse = (AudioEventResponse)(i);
							audioResponder->response = selectedResponse;
						}	
					}

					ImGui::EndListBox();
				}

				ImGui::Text("Set the data for the linked graphical response.");
				
				if (selectedResponse == AudioEventResponse::Move ||
					selectedResponse == AudioEventResponse::Rotate ||
					selectedResponse == AudioEventResponse::Scale)
				{
					ImGui::InputFloat3("Input XYZ Data", &audioResponder->data.x);
				}
				else if (selectedResponse == AudioEventResponse::ChangeLightIntensity) {
					ImGui::InputFloat("Input Intensity Data", &audioResponder->data.x);
				}
				else if (selectedResponse == AudioEventResponse::ChangeLightColor) {
					ImGui::InputFloat3("Input RGB Color Data", &audioResponder->data.x);
				}

				ImGui::Separator();

				static AudioResponseContinuityMode selectedContinuity = AudioResponseContinuityMode::Once;
				static std::string continuityTypeArray[(int)AudioResponseContinuityMode::AudioResponseContinuityModeCount] = { "Once", "Loop X Times", "Until Interval Stop", "Until Audio Stop", "Infinite" };

				if (ImGui::BeginListBox("Choose Repetition Level")) {
					for (int i = 0; i < (int)AudioResponseContinuityMode::AudioResponseContinuityModeCount; i++) {
						const bool is_selected = ((int)selectedContinuity == i);
						if (ImGui::Selectable(continuityTypeArray[i].c_str(), is_selected)) {
							selectedContinuity = (AudioResponseContinuityMode)(i);
							audioResponder->continuityType = selectedContinuity;
						}
					}

					ImGui::EndListBox();
				}

				ImGui::Separator();

				static AudioResponseMathModifier selectedOperator = AudioResponseMathModifier::Additive;
				static std::string operatorTypeArray[(int)AudioResponseMathModifier::AudioResponseMathModifierCount] = { "Add", "Subtract", "Multiply", "Divide" };

				if (ImGui::BeginListBox("Choose Operator For Response Data")) {
					for (int i = 0; i < (int)AudioResponseMathModifier::AudioResponseMathModifierCount; i++) {
						const bool is_selected = ((int)selectedOperator == i);
						if (ImGui::Selectable(operatorTypeArray[i].c_str(), is_selected)) {
							selectedOperator = (AudioResponseMathModifier)(i);
							audioResponder->operatorType = selectedOperator;
						}
					}

					ImGui::EndListBox();
				}
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
			static std::string typeArray[ComponentTypes::COMPONENT_TYPE_COUNT] = { "Mesh Renderer", "Particle System", "Collider", "Terrain", "Light", "Camera", "Noclip Character Controller", "Flashlight Controller", "Audio Response Device"};

			if (ImGui::BeginListBox("Component Listbox")) {
				for (int i = 1; i < ComponentTypes::COMPONENT_TYPE_COUNT; i++) {
					const bool is_selected = (selectedComponent == i);
					if (ImGui::Selectable(typeArray[i - 1].c_str(), is_selected))
						selectedComponent = (ComponentTypes)(i);
				}

				ImGui::EndListBox();
			}

			ImGui::PushID(100);
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.3f, 1.0f, 0.56f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.3f, 1.0f, 0.87f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.65f, 1.0f, 0.5f));
			if (ImGui::Button("Add Selected Component")) {
				switch (selectedComponent) {
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
				case ComponentTypes::CAMERA:
					currentEntity->AddComponent<Camera>();
					break;
				case ComponentTypes::NOCLIP_CHAR_CONTROLLER:
					currentEntity->AddComponent<NoclipMovement>();
					break;
				case ComponentTypes::FLASHLIGHT_CONTROLLER:
					currentEntity->AddComponent<FlashlightController>();
					break;
				case ComponentTypes::AUDIO_RESPONSE_DEVICE:
					currentEntity->AddComponent<AudioResponse>();
					break;
				}
			}
			ImGui::PopStyleColor(3);
			ImGui::PopID();
		}
		SetEntityUIIndex(entityUIIndex);
		ImGui::End();
	}

	if (*(GetSoundWindowEnabled())) {
		ImGui::Begin("Sound Menu");

		for (int i = 0; i < globalAssets.GetSoundArraySize(); i++) {
			FMOD::Channel* channel;
			FMOD::Sound* sound;
			FMODUserData* uData;

			sound = globalAssets.GetSoundAtID(i);
			channel = audioHandler.GetChannelBySound(sound);
			sound->getUserData((void**)&uData);
			std::string infoStr = uData->name.get()->c_str();
			std::string node = "Viewing Sound: " + infoStr;

			ImGui::Text(node.c_str());

			std::string playButtonName = "Play Sound ##" + std::to_string(i);
			std::string pauseButtonName = "Pause Sound ##" + std::to_string(i);
			if (ImGui::Button(playButtonName.c_str())) {
				audioHandler.BasicPlaySound(channel, false);
			}
			ImGui::SameLine();
			if (ImGui::Button(pauseButtonName.c_str())) {
				audioHandler.BasicPlaySound(channel, true);
			}

			char timeBuf[50];
			unsigned int currentPosition;
			unsigned int audioLength;
			float posFloat;
			float lenFloat;
			channel->getPosition(&currentPosition, FMOD_TIMEUNIT_MS);
			sound->getLength(&audioLength, FMOD_TIMEUNIT_MS);

			posFloat = currentPosition / 1000.0f;
			lenFloat = audioLength / 1000.0f;

			sprintf_s(timeBuf, "%4.2f / %4.2f", posFloat, lenFloat);
			ImGui::Text(timeBuf);
			ImGui::Separator();
		}

		ImGui::End();
	}

	if (*(GetObjHierarchyEnabled())) {
		// Display the UI for setting parents
		if (ImGui::TreeNodeEx("GameObjects",
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_FramePadding)) {
			for (int i = 0; i < globalAssets.GetGameEntityArraySize(); i++) {
				if (globalAssets.GetGameEntityAtID(i)->GetTransform()->GetParent() == nullptr) {
					RenderChildObjectsInUI(globalAssets.GetGameEntityAtID(i));
				}
			}

			ImGui::TreePop();
		}
	}

	if (*(GetRtvWindowEnabled())) {
		ImGui::Begin("Multiple Render Target Viewer");

		if (ImGui::CollapsingHeader("MRT Effects")) {
			ImGui::Text("Color Without Ambient");
			if (dxCore->IsDirectX12()) {

			}
			else {
				ImGui::Image(renderer->GetDX11RenderTargetSRV(RTVTypes::COLORS_NO_AMBIENT).Get(), ImVec2(500, 300));
			}
			
			ImGui::Text("Ambient Color");
			if (dxCore->IsDirectX12()) {

			}
			else {
				ImGui::Image(renderer->GetDX11RenderTargetSRV(RTVTypes::COLORS_AMBIENT).Get(), ImVec2(500, 300));
			}

			ImGui::Text("Normals");
			if (dxCore->IsDirectX12()) {

			}
			else {
				ImGui::Image(renderer->GetDX11RenderTargetSRV(RTVTypes::NORMALS).Get(), ImVec2(500, 300));
			}

			ImGui::Text("Depths");
			if (dxCore->IsDirectX12()) {

			}
			else {
				ImGui::Image(renderer->GetDX11RenderTargetSRV(RTVTypes::DEPTHS).Get(), ImVec2(500, 300));
			}

			ImGui::Text("SSAO");
			if (dxCore->IsDirectX12()) {

			}
			else {
				ImGui::Image(renderer->GetDX11RenderTargetSRV(RTVTypes::SSAO_RAW).Get(), ImVec2(500, 300));
			}

			ImGui::Text("SSAO Post Blur");
			if (dxCore->IsDirectX12()) {

			}
			else {
				ImGui::Image(renderer->GetDX11RenderTargetSRV(RTVTypes::SSAO_BLUR).Get(), ImVec2(500, 300));
			}

			ImGui::Text("Composite");
			if (dxCore->IsDirectX12()) {

			}
			else {
				ImGui::Image(renderer->GetDX11RenderTargetSRV(RTVTypes::FINAL_COMPOSITE).Get(), ImVec2(500, 300));
			}

		}

		if (ImGui::CollapsingHeader("Shadow Depth Views")) {
			for (std::shared_ptr<ShadowProjector> projector : ComponentManager::GetAll<ShadowProjector>()) {
				if (projector->IsEnabled()) {
					ImGui::Text((projector->GetGameEntity()->GetName() + " SRV").c_str());
					ImGui::Image(projector->GetSRV().Get(), ImVec2(500, 300));
				}
			}
		}

		if (ImGui::CollapsingHeader("Depth Prepass Views")) {
			ImGui::Text("Refraction Silhouette Depths");
			if (dxCore->IsDirectX12()) {

			}
			else {
				ImGui::Image(renderer->GetDX11RenderTargetSRV(RTVTypes::REFRACTION_SILHOUETTE).Get(), ImVec2(500, 300));
			}

			ImGui::Text("Transparency Depth Prepass");
			if (dxCore->IsDirectX12()) {

			}
			else {
				ImGui::Image(renderer->GetDX11MiscEffectSRV(MiscEffectSRVTypes::TRANSPARENT_PREPASS_DEPTHS).Get(), ImVec2(500, 300));
			}

			ImGui::Text("Render Depth Prepass (used for optimization)");
			if (dxCore->IsDirectX12()) {

			}
			else {
				ImGui::Image(renderer->GetDX11MiscEffectSRV(MiscEffectSRVTypes::RENDER_PREPASS_DEPTHS).Get(), ImVec2(500, 300));
			}
		}

		if (ImGui::CollapsingHeader("Selected Entity Filled View")) {
			ImGui::Text("Selected Entity");
			ImGui::Image(renderer->outlineSRV.Get(), ImVec2(500, 300));
		}

		ImGui::End();
	}

	if (*(GetMaterialWindowEnabled()))
	{
		ImGui::Begin("Material Editor");

		int materialUIIndex = GetMaterialUIIndex();
		std::shared_ptr<Material> currentMaterial = globalAssets.GetMaterialAtID(materialUIIndex);

		std::string infoStr = currentMaterial->GetName();
		std::string node = "Edting Material: " + infoStr;

		ImGui::Text(node.c_str());

		if (ImGui::ArrowButton("Previous Material", ImGuiDir_Left)) {
			materialUIIndex--;
			if (materialUIIndex < 0) {
				materialUIIndex = globalAssets.GetMaterialArraySize() - 1;
			}
			this->materialUIIndex = materialUIIndex;
		};
		ImGui::SameLine();

		if (ImGui::ArrowButton("Next Material", ImGuiDir_Right)) {
			materialUIIndex++;
			if (materialUIIndex > globalAssets.GetMaterialArraySize() - 1) {
				materialUIIndex = 0;
			}
			this->materialUIIndex = materialUIIndex;
		};

		std::string nameBuffer;
		static char nameBuf[64] = "";
		nameBuffer = currentMaterial->GetName();
		strcpy_s(nameBuf, nameBuffer.c_str());
		ImGui::InputText("Rename Material ", nameBuf, sizeof(nameBuffer));

		currentMaterial->SetName(nameBuf);

		float currTiling = currentMaterial->GetTiling();
		ImGui::InputFloat("Tiling ", &currTiling);
		currentMaterial->SetTiling(currTiling);

		XMFLOAT4 currColorTint = currentMaterial->GetTint();
		ImGui::DragFloat3("Color Tint ", &currColorTint.x, 1.0f, 0.0f, 256.0f);
		currentMaterial->SetTint(currColorTint);

		bool currentTransparencyEnabled = currentMaterial->GetTransparent();
		ImGui::Checkbox("Transparancy", &currentTransparencyEnabled);
		currentMaterial->SetTransparent(currentTransparencyEnabled);
		if (currentTransparencyEnabled && currentMaterial->GetRefractivePixelShader() == nullptr) {
			currentMaterial->SetRefractivePixelShader(globalAssets.GetPixelShaderByName("RefractivePS"));
		}

		bool currentRefractionEnabled = currentMaterial->GetRefractive();
		ImGui::Checkbox("Refraction (Enabling this will also enable transparency)", &currentRefractionEnabled);
		currentMaterial->SetRefractive(currentRefractionEnabled);
		if (currentRefractionEnabled && currentMaterial->GetRefractivePixelShader() == nullptr) {
			currentMaterial->SetRefractivePixelShader(globalAssets.GetPixelShaderByName("RefractivePS"));
		}

		float currIndexOfRefraction = currentMaterial->GetIndexOfRefraction();
		ImGui::DragFloat("Index of Refraction ", &currIndexOfRefraction, 0.05f, 0.0f, 1.0f);
		currentMaterial->SetIndexOfRefraction(currIndexOfRefraction);

		float currRefractionScale = currentMaterial->GetRefractionScale();
		ImGui::DragFloat("Refraction Scale ", &currRefractionScale, 0.05f, 0.0f, 1.0f);
		currentMaterial->SetRefractionScale(currRefractionScale);

		ImGui::Separator();
		std::shared_ptr<Texture> currentTexture = currentMaterial->GetTexture();
		std::shared_ptr<Texture> currentNormalMap = currentMaterial->GetNormalMap();
		std::shared_ptr<Texture> currentMetalMap = currentMaterial->GetMetalMap();
		std::shared_ptr<Texture> currentRoughMap = currentMaterial->GetRoughMap();

		infoStr = currentMaterial->GetName();
		node = "Current Material View: " + infoStr;
		ImGui::Text(node.c_str());

		infoStr = currentTexture->GetName();
		node = "Texture: \t" + infoStr;
		ImGui::Text(node.c_str());
		ImGui::SameLine();

		infoStr = currentNormalMap->GetName();
		node = "Normal Map: " + infoStr;
		ImGui::Text(node.c_str());
		ImGui::SameLine();

		infoStr = currentMetalMap->GetName();
		node = "Metal Map: " + infoStr;
		ImGui::Text(node.c_str());
		ImGui::SameLine();

		infoStr = currentRoughMap->GetName();
		node = "Rough Map: " + infoStr;
		ImGui::Text(node.c_str());

		ImTextureID* currentTexDisplay;

		if (dxCore->IsDirectX12()) {
			// Temporary
			currentTexDisplay = NULL;
		}
		else {
			currentTexDisplay = (ImTextureID*)currentTexture->GetDX11Texture().Get();
		}
		ImGui::Image(currentTexDisplay, ImVec2(256, 256));
		ImGui::SameLine();

		if (dxCore->IsDirectX12()) {
			// Temporary
			currentTexDisplay = NULL;
		}
		else {
			currentTexDisplay = (ImTextureID*)currentNormalMap->GetDX11Texture().Get();
		}
		ImGui::Image(currentTexDisplay, ImVec2(256, 256));
		ImGui::SameLine();

		if (dxCore->IsDirectX12()) {
			// Temporary
			currentTexDisplay = NULL;
		}
		else {
			currentTexDisplay = (ImTextureID*)currentMetalMap->GetDX11Texture().Get();
		}
		ImGui::Image(currentTexDisplay, ImVec2(256, 256));
		ImGui::SameLine();

		if (dxCore->IsDirectX12()) {
			// Temporary
			currentTexDisplay = NULL;
		}
		else {
			currentTexDisplay = (ImTextureID*)currentRoughMap->GetDX11Texture().Get();
		}
		ImGui::Image(currentTexDisplay, ImVec2(256, 256));

		// Texture Swapping
		if (ImGui::CollapsingHeader("Texture Swapping")) {
			static int textureIndex = 0;

			std::string nameBuffer;
			static char nameBuf[64] = "";
			nameBuffer = currentTexture->GetName();
			strcpy_s(nameBuf, nameBuffer.c_str());

			ImGui::Text(nameBuf);
			if (ImGui::BeginListBox("TextureList")) {
				for (int i = 0; i < globalAssets.GetTextureArraySize(); i++) {
					const bool is_selected = (textureIndex == i);
					if (ImGui::Selectable(globalAssets.GetTextureAtID(i)->GetName().c_str(), is_selected)) {
						textureIndex = i;
					}

					if (is_selected) ImGui::SetItemDefaultFocus();
				}

				ImGui::EndListBox();
				ImGui::SameLine();

				if (dxCore->IsDirectX12()) {
					// Temporary
					currentTexDisplay = NULL;
				}
				else {
					currentTexDisplay = (ImTextureID*)globalAssets.GetTextureAtID(textureIndex)->GetDX11Texture().Get();
				}
				ImGui::Image(currentTexDisplay, ImVec2(256, 256));

			}

			if (ImGui::Button("Swap Texture")) {
				currentMaterial->SetTexture(globalAssets.GetTextureAtID(textureIndex));
			}
		}

		// Normal Map Swapping
		if (ImGui::CollapsingHeader("Normal Map Swapping")) {
			static int normalIndex = 0;

			std::string nameBuffer;
			static char nameBuf[64] = "";
			nameBuffer = currentNormalMap->GetName();
			strcpy_s(nameBuf, nameBuffer.c_str());

			ImGui::Text(nameBuf);
			if (ImGui::BeginListBox("NormalList")) {
				for (int i = 0; i < globalAssets.GetTextureArraySize(); i++) {
					const bool is_selected = (normalIndex == i);
					if (ImGui::Selectable(globalAssets.GetTextureAtID(i)->GetName().c_str(), is_selected)) {
						normalIndex = i;
					}

					if (is_selected) ImGui::SetItemDefaultFocus();
				}

				ImGui::EndListBox();
				ImGui::SameLine();

				if (dxCore->IsDirectX12()) {
					// Temporary
					currentTexDisplay = NULL;
				}
				else {
					currentTexDisplay = (ImTextureID*)globalAssets.GetTextureAtID(normalIndex)->GetDX11Texture().Get();
				}
				ImGui::Image(currentTexDisplay, ImVec2(256, 256));
			}

			if (ImGui::Button("Swap Normal Map")) {
				currentMaterial->SetNormalMap(globalAssets.GetTextureAtID(normalIndex));
			}
		}

		// Metal Map Swapping
		if (ImGui::CollapsingHeader("Metal Map Swapping")) {
			static int metalIndex = 0;

			std::string nameBuffer;
			static char nameBuf[64] = "";
			nameBuffer = currentMetalMap->GetName();
			strcpy_s(nameBuf, nameBuffer.c_str());

			ImGui::Text(nameBuf);
			if (ImGui::BeginListBox("MetalList")) {
				for (int i = 0; i < globalAssets.GetTextureArraySize(); i++) {
					const bool is_selected = (metalIndex == i);
					if (ImGui::Selectable(globalAssets.GetTextureAtID(i)->GetName().c_str(), is_selected)) {
						metalIndex = i;
					}

					if (is_selected) ImGui::SetItemDefaultFocus();
				}

				ImGui::EndListBox();
				ImGui::SameLine();

				if (dxCore->IsDirectX12()) {
					// Temporary
					currentTexDisplay = NULL;
				}
				else {
					currentTexDisplay = (ImTextureID*)globalAssets.GetTextureAtID(metalIndex)->GetDX11Texture().Get();
				}
				ImGui::Image(currentTexDisplay, ImVec2(256, 256));
			}

			if (ImGui::Button("Swap Metal Map")) {
				currentMaterial->SetMetalMap(globalAssets.GetTextureAtID(metalIndex));
			}
		}

		// Rough Map Swapping
		if (ImGui::CollapsingHeader("Rough Map Swapping")) {
			static int roughIndex = 0;

			std::string nameBuffer;
			static char nameBuf[64] = "";
			nameBuffer = currentRoughMap->GetName();
			strcpy_s(nameBuf, nameBuffer.c_str());

			ImGui::Text(nameBuf);
			if (ImGui::BeginListBox("RoughList")) {
				for (int i = 0; i < globalAssets.GetTextureArraySize(); i++) {
					const bool is_selected = (roughIndex == i);
					if (ImGui::Selectable(globalAssets.GetTextureAtID(i)->GetName().c_str(), is_selected)) {
						roughIndex = i;
					}

					if (is_selected) ImGui::SetItemDefaultFocus();
				}

				ImGui::EndListBox();
				ImGui::SameLine();

				if (dxCore->IsDirectX12()) {
					// Temporary
					currentTexDisplay = NULL;
				}
				else {
					currentTexDisplay = (ImTextureID*)globalAssets.GetTextureAtID(roughIndex)->GetDX11Texture().Get();
				}
				ImGui::Image(currentTexDisplay, ImVec2(256, 256));
			}

			if (ImGui::Button("Swap Rough Map")) {
				currentMaterial->SetRoughMap(globalAssets.GetTextureAtID(roughIndex));
			}
		}

		ImGui::End();
	}

	if (*(GetTerrainMaterialWindowEnabled())) {
		ImGui::Begin("Terrain Material Editor");

		std::shared_ptr<TerrainMaterial> currentTMat = globalAssets.GetTerrainMaterialAtID(GetTerrainMaterialUIIndex());

		std::string infoStr = currentTMat->GetName();
		std::string node = "Edting Terrain Material: " + infoStr;

		ImGui::Text(node.c_str());

		if (ImGui::ArrowButton("Previous Terrain Material", ImGuiDir_Left)) {
			terrainMaterialUIIndex--;
			if (terrainMaterialUIIndex < 0) {
				terrainMaterialUIIndex = globalAssets.GetTerrainMaterialArraySize() - 1;
			}
		};
		ImGui::SameLine();

		if (ImGui::ArrowButton("Next Terrain Material", ImGuiDir_Right)) {
			terrainMaterialUIIndex++;
			if (terrainMaterialUIIndex > globalAssets.GetTerrainMaterialArraySize() - 1) {
				terrainMaterialUIIndex = 0;
			}
		};

		std::string nameBuffer;
		static char nameBuf[64] = "";
		nameBuffer = currentTMat->GetName();
		strcpy_s(nameBuf, nameBuffer.c_str());
		ImGui::InputText("Rename Terrain Material ", nameBuf, sizeof(nameBuffer));

		currentTMat->SetName(nameBuf);

		ImGui::Text("Current Materials in TMat List:");
		for (int i = 0; i < currentTMat->GetMaterialCount(); i++) {
			ImGui::Text(currentTMat->GetMaterialAtID(i)->GetName().c_str());
		}

		if (ImGui::CollapsingHeader("Swap Blend Map")) {
			static int textureIndex = 0;
			ImTextureID* currentTexDisplay;
			std::shared_ptr<Texture> currentBlendMap;

			currentBlendMap = currentTMat->GetBlendMapTexture();

			std::string nameBuffer;
			static char nameBuf[64] = "";
			if (currentBlendMap != nullptr) {
				nameBuffer = currentBlendMap->GetName();							
			}
			else {
				nameBuffer = "No blendmap assigned!";
			}
			strcpy_s(nameBuf, nameBuffer.c_str());
			ImGui::Text(nameBuf);

			if (ImGui::BeginListBox("TextureBlendMapList")) {
				for (int i = 0; i < globalAssets.GetTextureArraySize(); i++) {
					const bool is_selected = (textureIndex == i);
					if (ImGui::Selectable(globalAssets.GetTextureAtID(i)->GetName().c_str(), is_selected)) {
						textureIndex = i;
					}

					if (is_selected) ImGui::SetItemDefaultFocus();
				}

				ImGui::EndListBox();
				ImGui::SameLine();

				if (dxCore->IsDirectX12()) {
					// Temporary
					currentTexDisplay = NULL;
				}
				else {
					currentTexDisplay = (ImTextureID*)globalAssets.GetTextureAtID(textureIndex)->GetDX11Texture().Get();
				}
				ImGui::Image(currentTexDisplay, ImVec2(256, 256));

			}

			if (ImGui::Button("Swap Texture###TMatBlendSwap")) {
				currentTMat->SetBlendMapFromTexture(globalAssets.GetTextureAtID(textureIndex));
				currentTMat->SetBlendMapFilenameKey(globalAssets.GetTextureAtID(textureIndex)->GetTextureFilenameKey());
			}
		}

		static int allMatsIndex = 0;
		if (ImGui::CollapsingHeader("Change Individual Materials")) {
			static int currentMatIndex = 0;
			
			std::string infoStr = currentTMat->GetMaterialAtID(currentMatIndex)->GetName();
			std::string node = "Swapping Material: " + infoStr;

			ImGui::Text(node.c_str());

			if (ImGui::ArrowButton("Previous Material###TMatListPrev", ImGuiDir_Left)) {
				currentMatIndex--;
				if (currentMatIndex < 0) {
					currentMatIndex = currentTMat->GetMaterialCount() - 1;
				}
			};
			ImGui::SameLine();

			if (ImGui::ArrowButton("Next Terrain Material###TMatListNext", ImGuiDir_Right)) {
				currentMatIndex++;
				if (currentMatIndex > currentTMat->GetMaterialCount() - 1) {
					currentMatIndex = 0;
				}
			};

			if (ImGui::BeginListBox("TMaterialList")) {
				for (int i = 0; i < globalAssets.GetMaterialArraySize(); i++) {
					const bool is_selected = (allMatsIndex == i);
					if (ImGui::Selectable(globalAssets.GetMaterialAtID(i)->GetName().c_str(), is_selected)) {
						allMatsIndex = i;
					}

					if (is_selected) ImGui::SetItemDefaultFocus();
				}

				ImGui::EndListBox();
				ImGui::SameLine();
			}

			if (ImGui::Button("Swap Material###TMaterialSwap")) {
				currentTMat->SetMaterialAtID(globalAssets.GetMaterialAtID(allMatsIndex), currentMatIndex);
			}
		}

		if (ImGui::CollapsingHeader("Add Material to Terrain Material")) {
			if (ImGui::BeginListBox("TMaterialAddList")) {
				for (int i = 0; i < globalAssets.GetMaterialArraySize(); i++) {
					const bool is_selected = (allMatsIndex == i);
					if (ImGui::Selectable(globalAssets.GetMaterialAtID(i)->GetName().c_str(), is_selected)) {
						allMatsIndex = i;
					}

					if (is_selected) ImGui::SetItemDefaultFocus();
				}

				ImGui::EndListBox();
				ImGui::SameLine();
			}

			if (ImGui::Button("Add Material###TMaterialAdd")) {
				currentTMat->AddMaterial(globalAssets.GetMaterialAtID(allMatsIndex));
			}
		}

		ImGui::End();
	}

	if (*(GetCollidersWindowEnabled()))
	{
		ImGui::Begin("Collider Bulk Operations");

		bool UIDrawColliders = Renderer::GetDrawColliderStatus();
		ImGui::Checkbox("Draw Colliders: ", &UIDrawColliders);
		Renderer::SetDrawColliderStatus(UIDrawColliders);

		ImGui::End();
	}

	if (*(GetRenderWindowEnabled())) {
		ImGui::Begin("Render to Video File");

		static int videoDuration = 20;
		FileRenderData* renderData = renderer->GetFileRenderData();

		static char nameBuf[64] = "C:\\output.mp4";
		if (ImGui::InputText("Output file: ", nameBuf, sizeof(nameBuf))) {
			std::wstring renderDataString;
			ISimpleShader::ConvertToWide(nameBuf, renderDataString);
			renderData->filePath = renderDataString;
		}

		if (ImGui::InputScalar("FPS", ImGuiDataType_U32, &renderData->VideoFPS)) {
			renderData->VideoFrameCount = videoDuration * renderData->VideoFPS;
			renderData->VideoFrameDuration = 10 * 1000 * 1000 / renderData->VideoFPS;
		}

		ImGui::InputScalar("Bitrate", ImGuiDataType_U32, &renderData->VideoBitRate);

		if (ImGui::InputScalar("Duration (s)", ImGuiDataType_U32, &videoDuration)) {
			renderData->VideoFrameCount = videoDuration * renderData->VideoFPS;
		}

		if (ImGui::Button("Render")) {
			renderer->RenderToVideoFile(globalAssets.GetEditingCamera(), *renderData);
		}

		ImGui::End();
	}
}

// Getters
bool* EditingUI::GetObjWindowEnabled() {
	return &objWindowEnabled;
}

bool* EditingUI::GetObjHierarchyEnabled() {
	return &objHierarchyEnabled;
}

bool* EditingUI::GetSkyWindowEnabled() {
	return &skyWindowEnabled;
}

bool* EditingUI::GetSoundWindowEnabled() {
	return &soundWindowEnabled;
}

bool* EditingUI::GetTextureWindowEnabled() {
	return &textureWindowEnabled;
}

bool* EditingUI::GetMaterialWindowEnabled() {
	return &materialWindowEnabled;
}

bool* EditingUI::GetTerrainMaterialWindowEnabled() {
	return &terrainMaterialWindowEnabled;
}

bool* EditingUI::GetCollidersWindowEnabled() {
	return &collidersWindowEnabled;
}

bool* EditingUI::GetRtvWindowEnabled() {
	return &rtvWindowEnabled;
}

bool* EditingUI::GetStatsEnabled() {
	return &statsEnabled;
}

bool* EditingUI::GetMovingEnabled() {
	return &movingEnabled;
}

bool* EditingUI::GetRenderWindowEnabled() {
	return &renderWindowEnabled;
}

int EditingUI::GetEntityUIIndex() {
	return entityUIIndex;
};
int EditingUI::GetSkyUIIndex() {
	return skyUIIndex;
};
int EditingUI::GetMaterialUIIndex() {
	return materialUIIndex;
};

int EditingUI::GetTerrainMaterialUIIndex() {
	return terrainMaterialUIIndex;
}


// Setters
void EditingUI::SetEntityUIIndex(int NewEntityUIIndex) {
	entityUIIndex = NewEntityUIIndex;
};
void EditingUI::SetSkyUIIndex(int NewSkyUIIndex) {
	skyUIIndex = NewSkyUIIndex;
};

void EditingUI::SetObjWindowEnabled(bool enabled) {
	objWindowEnabled = enabled;
}

void EditingUI::SetMaterialUIIndex(int newIndex) {
	materialUIIndex = newIndex;
};

void EditingUI::SetMaterialWindowEnabled(bool enabled) {
	materialWindowEnabled = enabled;
}