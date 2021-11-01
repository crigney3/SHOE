#pragma once

#include "Lights.h"
#include "Sky.h"
#include "SimpleShader.h"
#include "GameEntity.h"
#include "packages/directxtk_desktop_2017.2021.8.2.1/include/WICTextureLoader.h"
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

	Microsoft::WRL::ComPtr<ID3D11Texture2D> ssaoRandomTex;

	// Ambient Occlusion data
	DirectX::XMFLOAT4 ssaoOffsets[64];

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
	void InitializeSSAO();

public:
	~AssetManager();

	Microsoft::WRL::ComPtr<ID3D11Device> GetDevice();
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> GetContext();

	void Initialize(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);

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
