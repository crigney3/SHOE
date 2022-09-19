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
#include <chrono>
#include "SceneManager.h"

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

private:
	// Asset Manager instance
	AssetManager& globalAssets = AssetManager::GetInstance();
	SceneManager& sceneManager = SceneManager::GetInstance();
	AudioHandler& audioHandler = AudioHandler::GetInstance();

	// Loading methods for initializing threads.
	// Mainly does pre- and post- loading, and creates
	// the threads that do the actual loading
	void LoadScene();
	void SaveScene();
	void SaveSceneAs();

	// Rendering helper methods
	void DrawInitializingScreen(std::string category);
	void DrawLoadingScreen();
	void GenerateEditingUI();
	void RenderChildObjectsInUI(std::shared_ptr<GameEntity> entity);
	std::unique_ptr<Renderer> renderer;

	// GUI control tracking/UI toggles
	Input& input = Input::GetInstance();
	bool statsEnabled;
	bool objWindowEnabled;
	bool objHierarchyEnabled;
	bool skyWindowEnabled;
	bool textureWindowEnabled;
	bool materialWindowEnabled;
	bool movingEnabled;
	bool rtvWindowEnabled;
	bool soundWindowEnabled;
	bool collidersWindowEnabled;

	// Loading screen info
	DirectX::SpriteBatch* loadingSpriteBatch;
	DirectX::SpriteFont* loadingFont;

	// Transfer these to static locals
	// Then add helper functions for setting them?
	int entityUIIndex;
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

	//For selecting objects with a click
	std::shared_ptr<GameEntity> GetClickedEntity();
	std::shared_ptr<GameEntity> clickedEntityBuffer;

	EngineState engineState;
};

