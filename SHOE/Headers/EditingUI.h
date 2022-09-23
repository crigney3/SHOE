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

class EditingUI {
public:
	EditingUI(std::shared_ptr<Renderer> renderer);
	~EditingUI();

	DirectX::XMFLOAT3 UIPositionEdit;
	DirectX::XMFLOAT3 UIRotationEdit;
	DirectX::XMFLOAT3 UIScaleEdit;

	float UILightType;
	DirectX::XMFLOAT3 UILightDirectionEdit;
	DirectX::XMFLOAT3 UILightColorEdit;
	float UILightRange;
	float UILightIntensity;

	// Asset Manager instance
	AssetManager& globalAssets = AssetManager::GetInstance();
	SceneManager& sceneManager = SceneManager::GetInstance();
	AudioHandler& audioHandler = AudioHandler::GetInstance();

	// GUI control tracking/UI toggles
	Input& input = Input::GetInstance();

	DXCore* dxCore = DXCore::DXCoreInstance;

	void ReInitializeEditingUI(std::shared_ptr<Renderer> renderer);

	void GenerateEditingUI();
	void ResetUI();
	void DisplayMenu();
	void ResetFrame();
	void TrackHotkeys();

	// Setters
	void SetEntityUIIndex(int NewEntityUIIndex);
	void SetSkyUIIndex(int NewSkyUIIndex);
	void SetObjWindowEnabled(bool enabled);
	void EditingUI::SetMaterialUIIndex(int newIndex);
	void EditingUI::SetMaterialWindowEnabled(bool enabled);

	// Getters
	bool* GetObjWindowEnabled();
	bool* GetObjHierarchyEnabled();
	bool* GetSkyWindowEnabled();
	bool* GetSoundWindowEnabled();
	bool* GetTextureWindowEnabled();
	bool* GetMaterialWindowEnabled();
	bool* GetCollidersWindowEnabled();
	bool* GetRtvWindowEnabled();
	bool* GetRenderWindowEnabled();

	bool* GetStatsEnabled();
	bool* GetMovingEnabled();

	int GetEntityUIIndex();
	int GetSkyUIIndex();
	int GetMaterialUIIndex();

private:
	std::shared_ptr<Renderer> renderer;

	// GUI control tracking/UI toggles
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
	bool renderWindowEnabled;

	// Transfer these to static locals
	// Then add helper functions for setting them?
	int entityUIIndex;
	int skyUIIndex;
	int materialUIIndex;

	//For selecting objects with a click
	std::shared_ptr<GameEntity> GetClickedEntity();
	std::shared_ptr<GameEntity> clickedEntityBuffer;

	void RenderChildObjectsInUI(std::shared_ptr<GameEntity> entity);
};
