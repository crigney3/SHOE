#pragma once

#include <DirectXMath.h> 
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <Windows.h>
#include <chrono>
#include "EditingUI.h"

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
	
	// Asset Manager instance
	AssetManager& globalAssets = AssetManager::GetInstance();
	SceneManager& sceneManager = SceneManager::GetInstance();
	AudioHandler& audioHandler = AudioHandler::GetInstance();

	// GUI control tracking/UI toggles
	Input& input = Input::GetInstance();

	//std::unique_ptr<Renderer> Game::GetRenderer();
	std::shared_ptr<Renderer> renderer;
	std::unique_ptr<EditingUI> editUI;

private:
	// Rendering helper methods
	void DrawInitializingScreen(std::string category);
	void DrawLoadingScreen();

	// Loading screen info
	DirectX::SpriteBatch* loadingSpriteBatch;
	DirectX::SpriteFont* loadingFont;

	//DirectX::XMFLOAT3 UIPositionEdit;
	//DirectX::XMFLOAT3 UIRotationEdit;
	//DirectX::XMFLOAT3 UIScaleEdit;

	//float UILightType;
	//DirectX::XMFLOAT3 UILightDirectionEdit;
	//DirectX::XMFLOAT3 UILightColorEdit;
	//float UILightRange;
	//float UILightIntensity;

	//Assimp material pointers
	std::vector<std::shared_ptr<Material>> specialMaterials;

	EngineState engineState;
};

