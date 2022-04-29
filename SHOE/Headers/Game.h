#pragma once

#include <DirectXMath.h> 
#include <vector>
#include "GameEntity.h"
#include "Light.h"
#include "Sky.h"
#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <Windows.h>
#include "Input.h"
#include "Renderer.h"
#include "AssetManager.h"
#include <thread>
#include <chrono>

class Game : public DXCore
{
public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update();
	void Draw();

	// Loading screen info
	void DrawLoadingScreen(AMLoadState loadType);

private:
	// Asset Manager instance
	AssetManager& globalAssets = AssetManager::GetInstance();
	AudioHandler& audioHandler = AudioHandler::GetInstance();

	// Loading methods for initializing threads.
	// Mainly does pre- and post- loading, and creates
	// the threads that do the actual loading
	void LoadScene();
	void SaveScene();
	void SaveSceneAs();

	// Rendering helper methods
	void SelectSky();
	void GenerateEditingUI();
	std::unique_ptr<Renderer> renderer;

	//Camera pointer
	std::shared_ptr<Camera> mainCamera;

	//Camera pointers for shadows
	std::shared_ptr<Camera> flashShadowCamera;
	std::shared_ptr<Camera> mainShadowCamera;

	// Flashlight checking
	std::shared_ptr<Light> flashlight;
	bool flashMenuToggle;
	bool flickeringEnabled;
	bool hasFlickered;
	void Flashlight();

	// GUI control tracking/UI toggles
	Input& input = Input::GetInstance();
	bool statsEnabled;
	bool lightWindowEnabled;
	bool objWindowEnabled;
	bool objHierarchyEnabled;
	bool skyWindowEnabled;
	bool movingEnabled;
	bool rtvWindowEnabled;
	bool soundWindowEnabled;
	bool camWindowEnabled;
	bool collidersWindowEnabled;

	// Loading screen info
	std::condition_variable* notification;
	std::mutex* loadingMutex;
	DirectX::SpriteBatch* loadingSpriteBatch;
	DirectX::SpriteFont* loadingFont;

	// Transfer these to static locals
	// Then add helper functions for setting them?
	int entityUIIndex;
	int camUIIndex;
	int skyUIIndex;
	DirectX::XMFLOAT3 UIPositionEdit;
	DirectX::XMFLOAT3 UIRotationEdit;
	DirectX::XMFLOAT3 UIScaleEdit;

	float UILightType;
	DirectX::XMFLOAT3 UILightDirectionEdit;
	DirectX::XMFLOAT3 UILightColorEdit;
	float UILightRange;
	float UILightIntensity;

	//Assimp material pointers
	std::vector<std::shared_ptr<Material>> specialMaterials;

	// UI Helper/Recursive Functions
	void RenderChildObjectsInUI(std::shared_ptr<GameEntity> entity);

	//For selecting objects with a click
	std::shared_ptr<GameEntity> GetClickedEntity();
	std::shared_ptr<GameEntity> clickedEntityBuffer;
};

