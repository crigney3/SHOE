#pragma once

// Used to show where out-parameter logic occurs
// Removed on compile
#define OUT

#include "Light.h"
#include "Sky.h"
#include "SimpleShader.h"
#include "GameEntity.h"
#include "ParticleSystem.h"
#include "Terrain.h"
#include "WICTextureLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/types.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <random>
#include "DXCore.h"
#include "experimental\filesystem"
#include <locale>
#include <codecvt>
#include "AudioHandler.h"
#include <exception>
#include "SpriteBatch.h"
#include "Collider.h"
#include "EngineState.h"
#include <tchar.h>

#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min

struct FMODUserData {
	std::shared_ptr<std::string> name;
	std::shared_ptr<std::string> filenameKey;
};

enum ComponentTypes {
	// While Transform is tracked here, it is often skipped or handled uniquely
	// when assessing all components, as it cannot be removed or doubled
	TRANSFORM,
	MESH_RENDERER,
	PARTICLE_SYSTEM,
	COLLIDER,
	TERRAIN,
	LIGHT,
	CAMERA,
	NOCLIP_CHAR_CONTROLLER,
	FLASHLIGHT_CONTROLLER,
	// Must always be the final enum
	COMPONENT_TYPE_COUNT
};

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
	AudioHandler& audioInstance = AudioHandler::GetInstance();
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
	EngineState* engineState;

	std::vector<Microsoft::WRL::ComPtr<ID3D11SamplerState>> textureSampleStates;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> textureState;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> clampState;

	DirectX::XMFLOAT4 whiteTint = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

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
	void ProcessComplexModel(aiNode* node, const aiScene* scene, std::string serializedFilenameKey, std::string name);
	std::shared_ptr<Mesh> ProcessComplexMesh(aiMesh* mesh, const aiScene* scene);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> LoadParticleTexture(std::string textureNameToLoad, bool isMultiParticle);

	void InitializeTextureSampleStates();
	void InitializeMeshes();
	void InitializeTextures();
	void InitializeMaterials();
	void InitializeShaders();
	void InitializeGameEntities();
	void InitializeColliders();
	void InitializeTerrainMaterials();
	void InitializeTerrainEntities();
	void InitializeCameras();
	void InitializeLights();
	void InitializeSkies();
	void InitializeEmitters();
	void InitializeAudio();
	void InitializeFonts();
	void InitializeIMGUI(HWND hwnd);

	std::vector<std::shared_ptr<SimplePixelShader>> pixelShaders;
	std::vector<std::shared_ptr<SimpleVertexShader>> vertexShaders;
	std::vector<std::shared_ptr<SimpleComputeShader>> computeShaders;
	std::vector<std::shared_ptr<Sky>> skies;
	std::vector<std::shared_ptr<Mesh>> globalMeshes;
	std::vector<std::shared_ptr<Texture>> globalTextures;
	std::vector<std::shared_ptr<Material>> globalMaterials;
	std::vector<std::shared_ptr<GameEntity>> globalEntities;
	std::vector<std::shared_ptr<TerrainMaterial>> globalTerrainMaterials;
	std::vector<FMOD::Sound*> globalSounds;
	std::vector<std::shared_ptr<SHOEFont>> globalFonts;

	std::shared_ptr<Camera> editingCamera;
	std::shared_ptr<Camera> mainCamera;

	friend class SceneManager;
public:
	~AssetManager();

	Microsoft::WRL::ComPtr<ID3D11Device> GetDevice();
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> GetContext();

	void Initialize(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, HWND hwnd, EngineState* engineState, std::function<void(std::string)> progressListener = {});

	void ImportSkyTexture();
	void ImportFont();
	void ImportSound();
	void ImportMesh();
	void ImportHeightMap();
	void ImportTexture();
	std::string GetImportedFileString(OPENFILENAME* file);

	/// <summary>
	/// Gets the full path to an asset that is inside the Assets/ dir.
	/// </summary>
	/// <param name="index"></param>
	/// <param name="filename"></param>
	/// <returns></returns>
	std::string GetFullPathToAssetFile(AssetPathIndex index, std::string filename);

	/// <summary>
	/// Gets the full path to an asset that is outside the Assets/ dir.
	/// </summary>
	/// <param name="index"></param>
	/// <param name="filename"></param>
	/// <returns></returns>
	std::string GetFullPathToExternalAssetFile(std::string filename);

	// Camera Tag Functions
	std::shared_ptr<Camera> GetEditingCamera();
	void UpdateEditingCamera();
	std::shared_ptr<Camera> GetMainCamera();
	void SetMainCamera(std::shared_ptr<Camera> newMain);

	// Methods to create new entities
	std::shared_ptr<GameEntity> CreateGameEntity(std::string name = "GameEntity");
	std::shared_ptr<GameEntity> CreateGameEntity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> mat, std::string name = "GameEntity");

	/// <summary>
	/// Creates a new skybox object with the given texture.
	/// </summary>
	/// <param name="filepath">If a .dds, this is the name of that file. If 6 textures, this is a directory path.</param>
	/// <param name="fileType">0 for .dds, 1 for directory path.</param>
	/// <param name="name"></param>
	/// <param name="fileExtension">Only needed if fileType is 1</param>
	/// <returns></returns>
	std::shared_ptr<Sky> CreateSky(std::string filepath, bool fileType, std::string name, std::string fileExtension = ".png");
	std::shared_ptr<SimpleVertexShader> CreateVertexShader(std::string id, std::string nameToLoad);
	std::shared_ptr<SimplePixelShader> CreatePixelShader(std::string id, std::string nameToLoad);
	std::shared_ptr<SimpleComputeShader> CreateComputeShader(std::string id, std::string nameToLoad);
	std::shared_ptr<Mesh> CreateMesh(std::string id, std::string nameToLoad, bool isNameFullPath = false);
	std::shared_ptr<Camera> CreateCamera(std::string name, float aspectRatio = 0);
	std::shared_ptr<Light> CreateDirectionalLight(std::string name, DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), float intensity = 1.0f);
	std::shared_ptr<Light> CreatePointLight(std::string name, float range, DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), float intensity = 1.0f);
	std::shared_ptr<Light> CreateSpotLight(std::string name, float range, DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), float intensity = 1.0f);
	std::shared_ptr<Texture> CreateTexture(std::string nameToLoad, std::string textureName = "newTexture", AssetPathIndex assetPath = ASSET_TEXTURE_PATH_BASIC, bool isNameFullPath = false);
	std::shared_ptr<Material> CreatePBRMaterial(std::string id,
											    std::string albedoNameToLoad,
											    std::string normalNameToLoad,
											    std::string metalnessNameToLoad,
											    std::string roughnessNameToLoad,
												bool addToGlobalList = true);
	std::shared_ptr<Material> CreatePBRMaterial(std::string id,
											    std::shared_ptr<Texture> albedoTexture,
											    std::shared_ptr<Texture> normalTexture,
											    std::shared_ptr<Texture> metalnessTexture,
											    std::shared_ptr<Texture> roughnessTexture,
												bool addToGlobalList = true);
	std::shared_ptr<Terrain> CreateTerrainEntity(std::string name = "Terrain");
	std::shared_ptr<Terrain> CreateTerrainEntity(const char* heightmap, 
												 std::shared_ptr<TerrainMaterial> material, 
												 std::string name = "Terrain", 
												 unsigned int mapWidth = 512, 
												 unsigned int mapHeight = 512, 
												 float heightScale = 25.0f);
	std::shared_ptr<Terrain> CreateTerrainEntity(std::shared_ptr<Mesh> terrainMesh, 
												 std::shared_ptr<TerrainMaterial> material, 
												 std::string name = "Terrain");
	std::shared_ptr<TerrainMaterial> CreateTerrainMaterial(std::string name, std::vector<std::shared_ptr<Material>> materials, std::string blendMapPath = "");
	std::shared_ptr<TerrainMaterial> CreateTerrainMaterial(std::string name,
														   std::vector<std::string> texturePaths,
														   std::vector<std::string> matNames,
														   bool isPBRMat = true,
														   std::string blendMapPath = "");
	std::shared_ptr<ParticleSystem> CreateParticleEmitter(std::string name,
													std::string textureNameToLoad,
													bool isMultiParticle);
	std::shared_ptr<ParticleSystem> CreateParticleEmitter(std::string name,
												   std::string textureNameToLoad,
												   int maxParticles,
												   float particleLifeTime,
												   float particlesPerSecond,
												   bool isMultiParticle = false,
												   bool additiveBlendState = true);
	FMOD::Sound* CreateSound(std::string filePath, FMOD_MODE mode = FMOD_DEFAULT, std::string name = "", bool isNameFullPath = false);
	std::shared_ptr<SHOEFont> CreateSHOEFont(std::string name, std::string filePath, bool preInitializing = false, bool isNameFullPath = false);

	// Create-On-Entity methods, for components and loading
	std::shared_ptr<Terrain> CreateTerrainOnEntity(std::shared_ptr<GameEntity> entityToEdit,
												   const char* heightmap, 
												   std::shared_ptr<TerrainMaterial> material, 
												   unsigned int mapWidth = 512, 
												   unsigned int mapHeight = 512, 
												   float heightScale = 25.0f);
	std::shared_ptr<Terrain> CreateTerrainOnEntity(std::shared_ptr<GameEntity> entityToEdit,
												   std::shared_ptr<Mesh> terrainMesh,
												   std::shared_ptr<TerrainMaterial> material);
	std::shared_ptr<ParticleSystem> CreateParticleEmitterOnEntity(std::shared_ptr<GameEntity> entityToEdit,
																  std::string textureNameToLoad,
																  int maxParticles,
																  float particleLifeTime,
																  float particlesPerSecond,
																  bool isMultiParticle = false,
																  bool additiveBlendState = true);
	std::shared_ptr<ParticleSystem> CreateParticleEmitterOnEntity(std::shared_ptr<GameEntity> entityToEdit, 
																  std::string textureNameToLoad,
																  bool isMultiParticle);
	std::shared_ptr<Light> CreateDirectionalLightOnEntity(std::shared_ptr<GameEntity> entityToEdit,
														  DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
														  float intensity = 1.0f);
	std::shared_ptr<Light> CreatePointLightOnEntity(std::shared_ptr<GameEntity> entityToEdit,
													float range,
													DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
													float intensity = 1.0f);
	std::shared_ptr<Light> CreateSpotLightOnEntity(std::shared_ptr<GameEntity> entityToEdit, 
												   float range,
												   DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
												   float intensity = 1.0f);
	std::shared_ptr<Camera> CreateCameraOnEntity(std::shared_ptr<GameEntity> entityToEdit, float aspectRatio = 0);
	std::shared_ptr<Collider> CreateColliderOnEntity(std::shared_ptr<GameEntity> entityToEdit);
	std::shared_ptr<Collider> CreateTriggerBoxOnEntity(std::shared_ptr<GameEntity> entityToEdit);

	// Creation Helper Methods
	HRESULT LoadPBRTexture(std::string nameToLoad, OUT Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* texture, PBRTextureTypes textureType);
	HRESULT LoadPBRTexture(std::string nameToLoad, OUT Texture* texture, PBRTextureTypes textureType);
	std::string GetTextureFileKey(std::string textureFilename);
	std::string SerializeFileName(std::string assetFolderPath, std::string fullPathToAsset);
	std::string DeSerializeFileName(std::string assetPath);

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
	void RemoveMaterial(std::string name);
	void RemoveMaterial(int id);
	void RemoveTerrainMaterial(std::string name);
	void RemoveTerrainMaterial(int id);

	void CleanAllEntities();
	void CleanAllVectors();

	// Asset search-by-name methods

	std::shared_ptr<GameEntity> GetGameEntityByName(std::string name);
	std::shared_ptr<Sky> GetSkyByName(std::string name);
	std::shared_ptr<SimpleVertexShader> GetVertexShaderByName(std::string name);
	std::shared_ptr<SimplePixelShader> GetPixelShaderByName(std::string name);
	std::shared_ptr<SimpleComputeShader> GetComputeShaderByName(std::string name);
	std::shared_ptr<Mesh> GetMeshByName(std::string name);
	std::shared_ptr<Texture> GetTextureByName(std::string name);
	std::shared_ptr<Material> GetMaterialByName(std::string name);
	std::shared_ptr<TerrainMaterial> GetTerrainMaterialByName(std::string name);
	FMOD::Sound* GetSoundByName();
	std::shared_ptr<SHOEFont> GetFontByName(std::string name);

	int GetGameEntityIDByName(std::string name);
	int GetSkyIDByName(std::string name);
	int GetVertexShaderIDByName(std::string name);
	int GetPixelShaderIDByName(std::string name);
	int GetComputeShaderIDByName(std::string name);
	int GetMeshIDByName(std::string name);
	int GetMaterialIDByName(std::string name);
	//int GetTerrainMaterialIDByName(std::string name);

	// Relevant Get methods
	
	Microsoft::WRL::ComPtr<ID3D11InputLayout> GetInputLayout();
	size_t GetPixelShaderArraySize();
	size_t GetVertexShaderArraySize();
	size_t GetComputeShaderArraySize();
	size_t GetSkyArraySize();
	size_t GetMeshArraySize();
	size_t GetTextureArraySize();
	size_t GetMaterialArraySize();
	size_t GetGameEntityArraySize();
	size_t GetTerrainMaterialArraySize();
	size_t GetSoundArraySize();

	FMOD::Sound* GetSoundAtID(int id);
	std::shared_ptr<Texture> GetTextureAtID(int id);
	std::shared_ptr<Material> GetMaterialAtID(int id);
	std::shared_ptr<Mesh> GetMeshAtID(int id);
	std::shared_ptr<SimpleVertexShader> GetVertexShaderAtID(int id);
	std::shared_ptr<SimplePixelShader> GetPixelShaderAtID(int id);
	std::shared_ptr<SimpleComputeShader> GetComputeShaderAtID(int id);
	std::shared_ptr<GameEntity> GetGameEntityAtID(int id);
	std::shared_ptr<Sky> GetSkyAtID(int id);
	std::shared_ptr<TerrainMaterial> GetTerrainMaterialAtID(int id);

	int GetPixelShaderIDByPointer(std::shared_ptr<SimplePixelShader> pixelPointer);
	int GetVertexShaderIDByPointer(std::shared_ptr<SimpleVertexShader> vertexPointer);

	void BroadcastGlobalEntityEvent(EntityEventType event, std::shared_ptr<void> message = nullptr);

	std::shared_ptr<Sky> currentSky;
};