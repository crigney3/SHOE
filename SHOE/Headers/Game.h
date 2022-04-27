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

class Game 
	: public DXCore
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
	void RenderSky();
	void RenderUI();
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
	
	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//    Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

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

	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	// Create some constant color values
	const DirectX::XMFLOAT4 red = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	const DirectX::XMFLOAT4 green = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	const DirectX::XMFLOAT4 blue = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	//Assimp material pointers
	std::vector<std::shared_ptr<Material>> specialMaterials;

	std::vector<std::shared_ptr<GameEntity>>* Entities;

	//Sky pointers
	int activeSky;
	std::vector<std::shared_ptr<Sky>>* skies;
	std::shared_ptr<Sky> sunnySky;
	std::shared_ptr<Sky> spaceSky;
	std::shared_ptr<Sky> mountainSky;
	std::shared_ptr<Sky> niagaraSky;
	std::shared_ptr<Sky> starSky;

	// UI Helper/Recursive Functions
	void RenderChildObjectsInUI(std::shared_ptr<GameEntity> entity);

	//For selecting objects with a click
	std::shared_ptr<GameEntity> GetClickedEntity();
	std::shared_ptr<GameEntity> clickedEntityBuffer;
};

