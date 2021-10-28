#pragma once

#include <DirectXMath.h> 
#include <vector>
#include "GameEntity.h"
#include "Lights.h"
#include "Sky.h"
#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"
#include "Input.h"
#include "Renderer.h"
#include "AssetManager.h"

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
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:
	// Asset Manager instance
	AssetManager& globalAssets = AssetManager::GetInstance();

	// Initialization helper methods

	// Rendering helper methods
	void RenderSky();
	void RenderUI(float deltaTime);
	std::unique_ptr<Renderer> renderer;

	//Camera pointer
	std::shared_ptr<Camera> mainCamera;

	//Camera pointers for shadows
	std::shared_ptr<Camera> flashShadowCamera;
	std::shared_ptr<Camera> mainShadowCamera;

	// Flashlight checking
	bool flashEnabled;
	bool flashMenuToggle;
	bool flickeringEnabled;
	bool hasFlickered;
	void Flashlight();
	void FlickeringCheck();

	// Loading helper methods
	
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
	bool terrainWindowEnabled;
	bool movingEnabled;
	int entityUIIndex;
	int terrainUIIndex;
	std::vector<int> childIndices;
	DirectX::XMFLOAT3 UIPositionEdit;
	DirectX::XMFLOAT3 UIRotationEdit;
	DirectX::XMFLOAT3 UIScaleEdit;

	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	
	// Shaders and shader-related constructs

	//Texture pointers (old)
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cloverTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> brickTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> stoneTexture;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cushionTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rockTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalTexture;

	//Normal map pointers (old)
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cushionNormalMap;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rockNormalMap;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalNormalMap;

	std::shared_ptr<Mesh> FlashlightMesh;

	// Create some constant color values
	const DirectX::XMFLOAT4 red = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	const DirectX::XMFLOAT4 green = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	const DirectX::XMFLOAT4 blue = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	//Assimp material pointers
	std::vector<std::shared_ptr<Material>> specialMaterials;

	//Terrain PBR Mat pointers
	std::shared_ptr<Material> bogMat;
	std::shared_ptr<Material> forestMat;
	std::shared_ptr<Material> rockyMat;

	//Terrain pointers
	std::shared_ptr<GameEntity> terrainEntity;
	std::shared_ptr<Mesh> mainTerrain;
	std::shared_ptr<TerrainMats> mainTerrainMaterials;

	std::map<std::string, std::shared_ptr<GameEntity>> Entities;
	std::map<std::string, std::shared_ptr<GameEntity>>::iterator entIt;
	std::map<std::string, std::shared_ptr<GameEntity>>::iterator entUIIt;

	//Sky pointers
	int activeSky;
	std::vector<std::shared_ptr<Sky>> skies;
	std::shared_ptr<Sky> sunnySky;
	std::shared_ptr<Sky> spaceSky;
	std::shared_ptr<Sky> mountainSky;
	std::shared_ptr<Sky> niagaraSky;
	std::shared_ptr<Sky> starSky;

	//Lights
	std::vector<Light> lights;
	int lightUIIndex;
	unsigned int lightCount;
	DirectionalLight mainLight;
	DirectionalLight backLight;
	DirectionalLight bottomLight;
	SpotLight flashLight;
	PointLight centerLight;

	// UI Helper/Recursive Functions
	void RenderChildObjectsInUI(GameEntity* entity);
};

