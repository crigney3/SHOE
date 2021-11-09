#pragma once

#include "Lights.h"
#include "Sky.h"
#include "SimpleShader.h"
#include "GameEntity.h"
#include "WICTextureLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/types.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <map>
#include <random>
#include "DXCore.h"

#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min

class AssetManager
{
#pragma region Singleton
public:
	// Gets the one and only instance of this class
	static AssetManager& GetInstance()
	{
		if (!instance)
		{
			instance = new AssetManager();
		}

		return *instance;
	}

	// Remove these functions (C++ 11 version)
	AssetManager(AssetManager const&) = delete;
	void operator=(AssetManager const&) = delete;

private:
	static AssetManager* instance;
	AssetManager() 
	{
		
	};
#pragma endregion

private:
	DXCore* dxInstance;
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> textureState;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> clampState;

	DirectX::XMFLOAT4 redTint = DirectX::XMFLOAT4(0.2f, 0.0f, 0.0f, 1.0f);
	DirectX::XMFLOAT4 greenTint = DirectX::XMFLOAT4(0.0f, 0.2f, 0.0f, 1.0f);
	DirectX::XMFLOAT4 blueTint = DirectX::XMFLOAT4(0.0f, 0.0f, 0.2f, 1.0f);
	DirectX::XMFLOAT4 whiteTint = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	DirectX::XMFLOAT4 grayTint = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	DirectX::XMFLOAT4 brownTint = DirectX::XMFLOAT4(1.0f, 0.4f, 0.0f, 1.0f);

	// Helper for creating a cubemap from 6 individual textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCubemap(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);
	std::shared_ptr<Mesh> LoadTerrain(const char* filename, unsigned int mapWidth, unsigned int mapHeight, float heightScale);

	void CreateComplexGeometry();
	void ProcessComplexModel(aiNode* node, const aiScene* scene);
	std::shared_ptr<Mesh> ProcessComplexMesh(aiMesh* mesh, const aiScene* scene);

	void InitializeMeshes();
	void InitializeMaterials();
	void InitializeShaders();
	void InitializeGameEntities();
	void InitializeLights();
	void InitializeTerrainMaterials();
	void InitializeCameras();
	void InitializeSkies();

public:
	~AssetManager();

	Microsoft::WRL::ComPtr<ID3D11Device> GetDevice();
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> GetContext();

	void Initialize(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);

	// Methods to create new assets

	std::shared_ptr<GameEntity> CreateGameEntity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> mat, std::string name = "GameEntity");
	std::shared_ptr<Sky> CreateSky(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skyTexture);
	std::shared_ptr<SimpleVertexShader> CreateVertexShader(std::string id, std::wstring nameToLoad);
	std::shared_ptr<SimplePixelShader> CreatePixelShader(std::string id, std::wstring nameToLoad);
	std::shared_ptr<Mesh> CreateMesh(std::string id, std::string nameToLoad);
	std::shared_ptr<Camera> CreateCamera(std::string id, DirectX::XMFLOAT3 pos, float aspectRatio, int type);
	std::shared_ptr<Material> CreatePBRMaterial(std::string id,
											    std::wstring albedoNameToLoad,
											    std::wstring normalNameToLoad,
											    std::wstring metalnessNameToLoad,
											    std::wstring roughnessNameToLoad);

	// Methods to remove assets

	void RemoveGameEntity(std::string name);
	void RemoveGameEntity(int id);
	void RemoveSky(std::string name);
	void RemoveSky(int id);
	void RemoveVertexShader(std::string name);
	void RemoveVertexShader(int id);
	void RemovePixelShader(std::string name);
	void RemovePixelShader(int id);
	void RemoveMesh(std::string name);
	void RemoveMesh(int id);
	void RemoveCamera(std::string name);
	void RemoveCamera(int id);
	void RemoveTerrain(std::string name);
	void RemoveTerrain(int id);
	void RemoveMaterial(std::string name);
	void RemoveMaterial(int id);
	void RemoveTerrainMaterial(std::string name);
	void RemoveTerrainMaterial(int id);
	void RemoveLight(std::string name);
	void RemoveLight(int id);

	// Methods to disable and enable assets for rendering
	// Currently not implemented except for lights

	void DisableGameEntity(std::string name);
	void DisableGameEntity(int id);
	void DisableSky(std::string name);
	void DisableSky(int id);
	void DisableVertexShader(std::string name);
	void DisableVertexShader(int id);
	void DisablePixelShader(std::string name);
	void DisablePixelShader(int id);
	void DisableMesh(std::string name);
	void DisableMesh(int id);
	void DisableCamera(std::string name);
	void DisableCamera(int id);
	void DisableTerrain(std::string name);
	void DisableTerrain(int id);
	void DisableMaterial(std::string name);
	void DisableMaterial(int id);
	void DisableTerrainMaterial(std::string name);
	void DisableTerrainMaterial(int id);
	void DisableLight(std::string name);
	void DisableLight(int id);

	void EnableGameEntity(std::string name);
	void EnableGameEntity(int id);
	void EnableSky(std::string name);
	void EnableSky(int id);
	void EnableVertexShader(std::string name);
	void EnableVertexShader(int id);
	void EnablePixelShader(std::string name);
	void EnablePixelShader(int id);
	void EnableMesh(std::string name);
	void EnableMesh(int id);
	void EnableCamera(std::string name);
	void EnableCamera(int id);
	void EnableTerrain(std::string name);
	void EnableTerrain(int id);
	void EnableMaterial(std::string name);
	void EnableMaterial(int id);
	void EnableTerrainMaterial(std::string name);
	void EnableTerrainMaterial(int id);
	void EnableLight(std::string name);
	void EnableLight(int id);

	// Asset search-by-name methods

	std::shared_ptr<GameEntity> GetGameEntityByName(std::string name);
	std::shared_ptr<Sky> GetSkyByName(std::string name);
	std::shared_ptr<SimpleVertexShader> GetVertexShaderByName(std::string name);
	std::shared_ptr<SimplePixelShader> GetPixelShaderByName(std::string name);
	std::shared_ptr<Mesh> GetMeshByName(std::string name);
	std::shared_ptr<Camera> GetCameraByName(std::string name);
	std::shared_ptr<GameEntity> GetTerrainByName(std::string name);
	std::shared_ptr<Material> GetMaterialByName(std::string name);
	std::shared_ptr<TerrainMats> GetTerrainMaterialByName(std::string name);
	std::shared_ptr<Light> GetLightByName(std::string name);

	Microsoft::WRL::ComPtr<ID3D11InputLayout> GetInputLayout();

	std::shared_ptr<Sky> currentSky;
	int lightCount;

	std::map<std::string, std::shared_ptr<SimplePixelShader>> pixelShaders;
	std::map<std::string, std::shared_ptr<SimpleVertexShader>> vertexShaders;
	std::vector<std::shared_ptr<Sky>> skies;
	std::map<std::string, std::shared_ptr<Camera>> globalCameras;
	std::map<std::string, std::shared_ptr<Mesh>> globalMeshes;
	std::map<std::string, std::shared_ptr<Material>> globalMaterials;
	std::map<std::string, std::shared_ptr<GameEntity>> globalEntities;
	std::vector<Light> globalLights;
	std::map<std::string, std::shared_ptr<TerrainMats>> globalTerrainMaterials;
	std::map<std::string, std::shared_ptr<GameEntity>> globalTerrainEntities;
};
