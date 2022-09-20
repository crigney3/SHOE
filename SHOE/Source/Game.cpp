#include "../Headers/Game.h"
#include "../Headers/Time.h"
#include "..\Headers\ComponentManager.h"
#include "..\Headers\ShadowProjector.h"
#include "..\Headers\FlashlightController.h"
#include "..\Headers\NoclipMovement.h"
#include <d3dcompiler.h>

#include "../Headers/EdditingUI.h"

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

	loadingSpriteBatch = new SpriteBatch(context.Get());

#if defined(DEBUG) || defined(_DEBUG)
	printf("Took %3.4f seconds for pre-initialization. \n", this->GetTotalTime());
#endif

	sceneManager.Initialize(&engineState);
	globalAssets.Initialize(device, context, hWnd, &engineState, std::bind(&Game::DrawInitializingScreen, this, std::placeholders::_1));

#if defined(DEBUG) || defined(_DEBUG)
	printf("Took %3.4f seconds for main initialization. \n", this->GetDeltaTime());
#endif

	// Initialize the input manager with the window's handle
	Input::GetInstance().Initialize(this->hWnd);
	statsEnabled = true;
	movingEnabled = true;
	objWindowEnabled = false;
	skyWindowEnabled = false;
	objHierarchyEnabled = true;
	rtvWindowEnabled = false;

	entityUIIndex = -1;
	skyUIIndex = 0;

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
	engineState = EngineState::LOAD_SCENE;

#if defined(DEBUG) || defined(_DEBUG)
	//printf("Took %3.4f seconds for pre-initialization. \n", this->GetTotalTime());
#endif

	objWindowEnabled = false;
	skyWindowEnabled = false;
	entityUIIndex = -1;

	sceneManager.LoadScene("structureTest.json", std::bind(&Game::DrawLoadingScreen, this));

#if defined(DEBUG) || defined(_DEBUG)
	printf("Took %3.4f seconds for main initialization. \n", this->GetDeltaTime());
#endif

	renderer.reset();

	context->Flush();

	//With everything initialized, start the renderer
	renderer = std::make_unique<Renderer>(height,
		width,
		device,
		context,
		swapChain,
		backBufferRTV,
		depthStencilView);
}

void Game::SaveScene() {
	sceneManager.SaveScene("structureTest.json", "Default Scene");
}

void Game::SaveSceneAs() {
	// Handle file browser popup before anything else



	sceneManager.SaveScene("structureTest.json");
}

void Game::GenerateEditingUI() {
	EdditingUI::GenerateEditingUI(this);
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

std::shared_ptr<GameEntity> Game::GetClickedEntity()
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

	switch (engineState) {
	case EngineState::EDITING:
		// Quit if the escape key is pressed
		if (input.TestKeyAction(KeyActions::QuitGame)) Quit();
		else {
			GenerateEditingUI();

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

			if (input.KeyPress('P')) {
				ImGui::Render();
				ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
				ImGui_ImplDX11_NewFrame();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();
				sceneManager.PrePlaySave();
			}

			globalAssets.UpdateEditingCamera();
			globalAssets.BroadcastGlobalEntityEvent(EntityEventType::EditingUpdate);
		}
		break;
	case EngineState::PLAY:
		if (input.TestKeyAction(KeyActions::QuitGame)) {
			sceneManager.PostPlayLoad();
		}
		else {
			if (movingEnabled) {
				globalAssets.GetGameEntityByName("Bronze Cube")->GetTransform()->SetPosition(+1.5f, (float)sin(Time::totalTime) + 2.5f, +0.0f);
				globalAssets.GetGameEntityByName("Bronze Cube")->GetTransform()->Rotate(0.0f, (float)sin(Time::deltaTime), -(float)sin(Time::deltaTime));
				globalAssets.GetGameEntityByName("Scratched Cube")->GetTransform()->Rotate(-(float)sin(Time::deltaTime), 0.0f, 0.0f);
				globalAssets.GetGameEntityByName("Stone Cylinder")->GetTransform()->SetPosition(-2.0f, (float)sin(Time::totalTime), +0.0f);
				globalAssets.GetGameEntityByName("Paint Sphere")->GetTransform()->SetPosition(-(float)sin(Time::totalTime), -2.0f, +0.0f);
				globalAssets.GetGameEntityByName("Rough Torus")->GetTransform()->Rotate(+0.0f, +0.0f, +1.0f * Time::deltaTime);
			}

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
	if (engineState == EngineState::EDITING) {
		renderer->Draw(globalAssets.GetEditingCamera(), engineState);
	}
	else if (engineState == EngineState::PLAY) {
		renderer->Draw(globalAssets.GetMainCamera(), engineState);
	}
}

// Getters
bool* Game::GetObjWindowEnabled() {
	return &objWindowEnabled;
}

bool* Game::GetObjHierarchyEnabled() {
	return &objHierarchyEnabled;
}

bool* Game::GetSkyWindowEnabled() {
	return &skyWindowEnabled;
}

bool* Game::GetSoundWindowEnabled() {
	return &soundWindowEnabled;
}

bool* Game::GetTextureWindowEnabled() {
	return &textureWindowEnabled;
}

bool* Game::GetMaterialWindowEnabled() {
	return &materialWindowEnabled;
}

bool* Game::GetCollidersWindowEnabled() {
	return &collidersWindowEnabled;
}

bool* Game::GetRtvWindowEnabled() {
	return &rtvWindowEnabled;
}

bool* Game::GetStatsEnabled() {
	return &statsEnabled;
}

bool* Game::GetMovingEnabled() {
	return &movingEnabled;
}

int Game::GetEntityUIIndex() {
	return entityUIIndex;
};
int Game::GetSkyUIIndex() {
	return skyUIIndex;
};


// Setters
void Game::SetEntityUIIndex(int NewEntityUIIndex) {
	entityUIIndex = NewEntityUIIndex;
};
void Game::SetSkyUIIndex(int NewSkyUIIndex) {
	skyUIIndex = NewSkyUIIndex;
};

void Game::SetObjWindowEnabled(bool enabled) {
	objWindowEnabled = enabled;
}