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
#include "Emitter.h"
#include "experimental\filesystem"
#include <locale>
#include <codecvt>

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
	void InitializeEmitters();

	std::vector<std::shared_ptr<SimplePixelShader>> pixelShaders;
	std::vector<std::shared_ptr<SimpleVertexShader>> vertexShaders;
	std::vector<std::shared_ptr<Sky>> skies;
	std::vector<std::shared_ptr<Camera>> globalCameras;
	std::vector<std::shared_ptr<Mesh>> globalMeshes;
	std::vector<std::shared_ptr<Material>> globalMaterials;
	std::vector<std::shared_ptr<GameEntity>> globalEntities;
	std::vector<std::shared_ptr<Light>> globalLights;
	std::vector<std::shared_ptr<TerrainMats>> globalTerrainMaterials;
	std::vector<std::shared_ptr<GameEntity>> globalTerrainEntities;
	std::vector<std::shared_ptr<Emitter>> globalParticleEmitters;

public:
	~AssetManager();

	Microsoft::WRL::ComPtr<ID3D11Device> GetDevice();
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> GetContext();

	void Initialize(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);

	// Methods to create new assets

	std::shared_ptr<GameEntity> CreateGameEntity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> mat, std::string name = "GameEntity");
	std::shared_ptr<Sky> CreateSky(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skyTexture, std::string name);
	std::shared_ptr<SimpleVertexShader> CreateVertexShader(std::string id, std::wstring nameToLoad);
	std::shared_ptr<SimplePixelShader> CreatePixelShader(std::string id, std::wstring nameToLoad);
	std::shared_ptr<Mesh> CreateMesh(std::string id, std::string nameToLoad);
	std::shared_ptr<Camera> CreateCamera(std::string id, DirectX::XMFLOAT3 pos, float aspectRatio, int type);
	std::shared_ptr<Material> CreatePBRMaterial(std::string id,
											    std::wstring albedoNameToLoad,
											    std::wstring normalNameToLoad,
											    std::wstring metalnessNameToLoad,
											    std::wstring roughnessNameToLoad);
	std::shared_ptr<GameEntity> CreateTerrainEntity(std::shared_ptr<Mesh> mesh, std::string name = "Terrain");
	std::shared_ptr<Emitter> CreateParticleEmitter(int maxParticles,
												   float particleLifeTime,
												   float particlesPerSecond,
												   DirectX::XMFLOAT3 position, 
												   std::wstring textureNameToLoad,
												   std::string name,
												   bool isMultiParticle = false,
												   bool additiveBlendState = true);

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
	void RemoveEmitter(std::string name);
	void RemoveEmitter(int id);

	// Methods to disable and enable assets for rendering
	// Currently not implemented except for lights

	void EnableDisableGameEntity(std::string name, bool value);
	void EnableDisableGameEntity(int id, bool value);
	void EnableDisableSky(std::string name, bool value);
	void EnableDisableSky(int id, bool value);
	/*void EnableDisableVertexShader(std::string name, bool value);
	void EnableDisableVertexShader(int id, bool value);
	void EnableDisablePixelShader(std::string name, bool value);
	void EnableDisablePixelShader(int id, bool value);*/
	void EnableDisableMesh(std::string name, bool value);
	void EnableDisableMesh(int id, bool value);
	void EnableDisableCamera(std::string name, bool value);
	void EnableDisableCamera(int id, bool value);
	void EnableDisableTerrain(std::string name, bool value);
	void EnableDisableTerrain(int id, bool value);
	void EnableDisableMaterial(std::string name, bool value);
	void EnableDisableMaterial(int id, bool value);
	/*void EnableDisableTerrainMaterial(std::string name, bool value);
	void EnableDisableTerrainMaterial(int id, bool value);
	void EnableDisableLight(std::string name, bool value);*/
	void EnableDisableLight(int id, bool value);

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

	int GetGameEntityIDByName(std::string name);
	int GetSkyIDByName(std::string name);
	int GetVertexShaderIDByName(std::string name);
	int GetPixelShaderIDByName(std::string name);
	int GetMeshIDByName(std::string name);
	int GetCameraIDByName(std::string name);
	int GetTerrainIDByName(std::string name);
	int GetMaterialIDByName(std::string name);
	/*int GetTerrainMaterialIDByName(std::string name);
	int GetLightIDByName(std::string name);*/

	// Relevant Get methods
	
	Microsoft::WRL::ComPtr<ID3D11InputLayout> GetInputLayout();
	size_t GetPixelShaderArraySize();
	size_t GetVertexShaderArraySize();
	size_t GetSkyArraySize();
	size_t GetCameraArraySize();
	size_t GetMeshArraySize();
	size_t GetMaterialArraySize();
	size_t GetGameEntityArraySize();
	size_t GetLightArraySize();
	size_t GetTerrainMaterialArraySize();
	size_t GetTerrainEntityArraySize();
	size_t GetEmitterArraySize();
	Light* GetLightArray();
	std::vector<std::shared_ptr<GameEntity>>* GetActiveGameEntities();
	std::vector<std::shared_ptr<Sky>>* GetSkyArray();
	Light* GetFlashlight();
	Light* GetLightAtID(int id);
	std::shared_ptr<Emitter> GetEmitterAtID(int id);

	inline std::wstring ConvertToWide(const std::string& as);

	std::shared_ptr<Sky> currentSky;
	int lightCount;
};
