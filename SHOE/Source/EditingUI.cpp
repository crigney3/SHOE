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

	entityUIIndex = -1;
	skyUIIndex = 0;

	objWindowEnabled = false;
	skyWindowEnabled = false;
	entityUIIndex = -1;

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

	entityUIIndex = -1;
	skyUIIndex = 0;

	objWindowEnabled = false;
	skyWindowEnabled = false;
	entityUIIndex = -1;

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
				sceneManager.SaveScene("FIX");
			}

			if (ImGui::MenuItem("Save Scene As", "ctrl++shift+s")) {
				sceneManager.SaveSceneAs();
			}

			if (ImGui::MenuItem("Load Scene", "ctrl+s")) {
				sceneManager.LoadScene("structureTest.json");
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

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit")) {
			ImGui::MenuItem("GameObjects", "g", GetObjWindowEnabled());
			ImGui::MenuItem("Object Hierarchy", "h", GetObjHierarchyEnabled());
			ImGui::MenuItem("Skies", "", GetSkyWindowEnabled());
			ImGui::MenuItem("Sound", "", GetSoundWindowEnabled());
			ImGui::MenuItem("Texture", "", GetTextureWindowEnabled());
			ImGui::MenuItem("Material", "", GetMaterialWindowEnabled());
			ImGui::MenuItem("Colliders", "", GetCollidersWindowEnabled());

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			ImGui::MenuItem("Render Target Views", 0, GetRtvWindowEnabled());

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Add")) {
			ImGui::Text("Add a new GameEntity, which can have components attached.");

			if (ImGui::Button("Add GameEntity")) {
				globalAssets.CreateGameEntity("GameEntity" + std::to_string(globalAssets.GetGameEntityArraySize()));

				SetEntityUIIndex(globalAssets.GetGameEntityArraySize() - 1);
				SetObjWindowEnabled(true);
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
		ImGui::Checkbox("Enabled ", &skyEnabled);
		currentSky->SetEnabled(skyEnabled);

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

		ImGui::Image((ImTextureID*)currentTexture->GetTexture().Get(), ImVec2(256, 256));

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
		ImGui::DragFloat3("Rotation ", &UIRotationEdit.x, 0.5f, 0, 360);
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

					if (ImGui::Button("Swap")) {
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

					if (ImGui::Button("Swap")) {
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
				ImGui::DragFloat("Type ", &UILightType, 1.0f, 0.0f, 2.0f);
				light->SetType(UILightType);

				//Directional Light
				if (light->GetType() == 0.0f || light->GetType() == 2.0f) {
					bool castsShadows = light->CastsShadows();
					ImGui::Checkbox("Casts Shadows ", &castsShadows);
					light->SetCastsShadows(castsShadows);
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

				if (ImGui::Button("Mark as main")) {
					globalAssets.SetMainCamera(light->GetShadowProjector());
				}
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
			static std::string typeArray[ComponentTypes::COMPONENT_TYPE_COUNT] = { "Mesh Renderer", "Particle System", "Collider", "Terrain", "Light", "Camera", "Noclip Character Controller", "Flashlight Controller" };

			if (ImGui::BeginListBox("Component Listbox")) {
				for (int i = 0; i < ComponentTypes::COMPONENT_TYPE_COUNT; i++) {
					const bool is_selected = (selectedComponent == i);
					if (ImGui::Selectable(typeArray[i].c_str(), is_selected))
						selectedComponent = (ComponentTypes)(i + 1);
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
			std::string buttonName = "Play Piano Sound ##" + std::to_string(i);
			if (ImGui::Button(buttonName.c_str())) {
				audioHandler.BasicPlaySound(globalAssets.GetSoundAtID(i));
			}
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
			for (std::shared_ptr<ShadowProjector> projector : ComponentManager::GetAll<ShadowProjector>()) {
				if (projector->IsEnabled()) {
					ImGui::Text((projector->GetGameEntity()->GetName() + " SRV").c_str());
					ImGui::Image(projector->GetSRV().Get(), ImVec2(500, 300));
				}
			}
		}

		if (ImGui::CollapsingHeader("Depth Prepass Views")) {
			ImGui::Text("Refraction Silhouette Depths");
			ImGui::Image(renderer->GetRenderTargetSRV(RTVTypes::REFRACTION_SILHOUETTE).Get(), ImVec2(500, 300));
			ImGui::Text("Transparency Depth Prepass");
			ImGui::Image(renderer->GetMiscEffectSRV(MiscEffectSRVTypes::TRANSPARENT_PREPASS_DEPTHS).Get(), ImVec2(500, 300));
			ImGui::Text("Render Depth Prepass (used for optimization)");
			ImGui::Image(renderer->GetMiscEffectSRV(MiscEffectSRVTypes::RENDER_PREPASS_DEPTHS).Get(), ImVec2(500, 300));
		}

		if (ImGui::CollapsingHeader("Selected Entity Filled View")) {
			ImGui::Text("Selected Entity");
			ImGui::Image(renderer->outlineSRV.Get(), ImVec2(500, 300));
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

	// TODO: Add Material Edit menu
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

int EditingUI::GetEntityUIIndex() {
	return entityUIIndex;
};
int EditingUI::GetSkyUIIndex() {
	return skyUIIndex;
};


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