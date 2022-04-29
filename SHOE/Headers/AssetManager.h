#pragma once

// Used to show where out-parameter logic occurs
// Removed on compile
#define OUT

#pragma region saveLoadIdentifiers
// Saving and loading shorthand identifiers
// General:
#define VALID_SHOE_SCENE "ivs" // bool
#define SCENE_NAME "sN" // string

// Categories:
#define ENTITIES "en" // category - only used to fetch actual data
#define MESHES "m" // category - only used to fetch actual data
#define MATERIALS "a" // category - only used to fetch actual data
#define VERTEX_SHADERS "vS" // category - only used to fetch actual data
#define PIXEL_SHADERS "pS" // category - only used to fetch actual data
#define COMPUTE_SHADERS "cS" // category - only used to fetch actual data
#define FONTS "fn" // category - only used to fetch actual data
#define TEXTURE_SAMPLE_STATES "eS" // category - only used to fetch actual data
#define CAMERAS "ca" // category - only used to fetch actual data
#define SKIES "s" // category - only used to fetch actual data
#define SOUNDS "sO" // category - only used to fetch actual data
#define TERRAIN_MATERIALS "tM" // category - only used to fetch actual data
#define TERRAIN_ENTITIES "tE" // category - only used to fetch actual data

// Entities:
#define ENTITY_NAME "n" // string
#define ENTITY_ENABLED "e" // bool
#define ENTITY_HIERARCHY_ENABLED "hE" // bool
#define COMPONENT_TYPE "ct" // int
#define COMPONENTS "cm" // category - only used to fetch actual data

// Light Components:
#define LIGHT_TYPE "lT" // int
#define LIGHT_INTENSITY "i" // float
#define LIGHT_ENABLED "lE" // bool
#define LIGHT_RANGE "lR" // float
#define LIGHT_COLOR "lC" // float array 4
#define LIGHT_DIRECTION "lD" // float array 3

// Mesh Renderer Components:
#define MESH_OBJECT "mO" // category - only used to fetch actual data
#define MATERIAL_OBJECT "mA" // category - only used to fetch actual data
#define MESH_COMPONENT_INDEX "mCI" // int
#define MATERIAL_COMPONENT_INDEX "aCI" // int

// Mesh Data:

#define MESH_INDEX_COUNT "iC" // int
#define MESH_MATERIAL_INDEX "mI" // int
#define MESH_ENABLED "mE" // bool
#define MESH_NEEDS_DEPTH_PREPASS "nDP" // bool
#define MESH_NAME "mN" // string
#define MESH_FILENAME_KEY "mFK" // string

// Material Data:
#define MAT_UV_TILING "aUT" // float
#define MAT_NAME "aN" //string
#define MAT_ENABLED "aE" // bool
#define MAT_IS_TRANSPARENT "aIT" // bool
#define MAT_IS_REFRACTIVE "aIR" // bool
#define MAT_INDEX_OF_REFRACTION "aOR" // float
#define MAT_REFRACTION_SCALE "aRS" // float
#define MAT_COLOR_TINT "aCT" // float array 4
#define MAT_PIXEL_SHADER "aPS" // int
#define MAT_REFRACTION_PIXEL_SHADER "aRP" // int
#define MAT_VERTEX_SHADER "aVS" // int
#define MAT_TEXTURE_OR_ALBEDO_MAP "aAM" // string 
#define MAT_NORMAL_MAP "aNM" // string
#define MAT_METAL_MAP "aMM" // string
#define MAT_ROUGHNESS_MAP "aRM" // string
#define MAT_TEXTURE_SAMPLER_STATE "aTS" // int
#define MAT_CLAMP_SAMPLER_STATE "aCS" // int

// Texture Sampler data:
#define SAMPLER_ADDRESS_U "sAU" // int
#define SAMPLER_ADDRESS_V "sAV" // int
#define SAMPLER_ADDRESS_W "sAW" // int
#define SAMPLER_BORDER_COLOR "sBC" // float array 4
#define SAMPLER_COMPARISON_FUNCTION "sCF" // int?
#define SAMPLER_FILTER "sF" // int
#define SAMPLER_MAX_ANISOTROPY "sMA" // int
#define SAMPLER_MAX_LOD "sML" // float
#define SAMPLER_MIN_LOD "sIL" // float
#define SAMPLER_MIP_LOD_BIAS "sPB" // float

// Font Data:
#define FONT_FILENAME_KEY "fFK" // string
#define FONT_NAME "fN" // string

// Transform Data:
#define TRANSFORM_LOCAL_POSITION "tLP" // float array 3
#define TRANSFORM_LOCAL_SCALE "tLS" // float array 3
#define TRANSFORM_LOCAL_ROTATION "tLR" // float array 3

// Generic Shader Data:
#define SHADER_NAME "sN" // string
#define SHADER_FILE_PATH "sK" // string

// Vertex Shader Data:
#define VERTEX_SHADER_OBJECT "vSO" // category - only used to fetch actual data

// Pixel Shader Data:
#define PIXEL_SHADER_OBJECT "pSO" // category - only used to fetch actual data

// Compute Shader Data:
#define COMPUTE_SHADER_OBJECT "cSO" // category - only used to fetch actual data

// Camera Data:
#define CAMERA_NAME "cN" // string
#define CAMERA_TRANSFORM "cT" // category - only used to fetch actual data
#define CAMERA_ASPECT_RATIO "cAR" // float
#define CAMERA_PROJECTION_MATRIX_TYPE "cPM" // int
#define CAMERA_TAG "cG" // int
#define CAMERA_LOOK_SPEED "cLS" // float
#define CAMERA_MOVE_SPEED "cMS" // float
#define CAMERA_ENABLED "cE" // bool
#define CAMERA_NEAR_DISTANCE "cND" // float
#define CAMERA_FAR_DISTANCE "cFD" // float
#define CAMERA_FIELD_OF_VIEW "cF" //float

// Sky Data:
#define SKY_NAME "sN" // string
#define SKY_FILENAME_KEY_TYPE "sFT" // bool
#define SKY_FILENAME_KEY "sFK" // string
#define SKY_FILENAME_EXTENSION "sFE" //string

// Collider Data:
#define COLLIDER_TYPE "cT" // bool
#define COLLIDER_ENABLED "cE" // bool
#define COLLIDER_IS_VISIBLE "cIV" // bool
#define COLLIDER_POSITION_OFFSET "cPO" // float array 3
#define COLLIDER_SCALE_OFFSET "cOS" // float array 3
#define COLLIDER_ROTATION_OFFSET "cRO" // float array 3

// Sound Data:
#define SOUND_FILENAME_KEY "oFK" // string
#define SOUND_NAME "oN" // string
#define SOUND_FMOD_MODE "oFM" // int

// Terrain Material Data:
#define TERRAIN_MATERIAL_NAME "tN" // string
#define TERRAIN_MATERIAL_ENABLED "tE" // bool
#define TERRAIN_MATERIAL_BLEND_MAP_PATH "tBP" // string
#define TERRAIN_MATERIAL_BLEND_MAP_ENABLED "tBE" // bool
#define TERRAIN_MATERIAL_MATERIAL_ARRAY "tMA" // array of Materials

// Terrain Data:
#define TERRAIN_HEIGHTMAP_FILENAME_KEY "hFK" // string
#define TERRAIN_INDEX_OF_TERRAIN_MATERIAL "hIM" // int

// Particle System Data:
#define PARTICLE_SYSTEM_MAX_PARTICLES "pMP" // int
#define PARTICLE_SYSTEM_FILENAME_KEY "pFK" // string
#define PARTICLE_SYSTEM_IS_MULTI_PARTICLE "pIP" // bool
#define PARTICLE_SYSTEM_ADDITIVE_BLEND "pAB" // bool
#define PARTICLE_SYSTEM_ENABLED "pEN" // bool
#define PARTICLE_SYSTEM_COLOR_TINT "pCT" // float array 4
#define PARTICLE_SYSTEM_SCALE "pS" // float
#define PARTICLE_SYSTEM_SPEED "pE" //float
#define PARTICLE_SYSTEM_DESTINATION "pD" //float array 3
#define PARTICLE_SYSTEM_PARTICLES_PER_SECOND "pPS" //float
#define PARTICLE_SYSTEM_PARTICLE_LIFETIME "pL" //float

#pragma endregion

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
#include <map>
#include <random>
#include "DXCore.h"
#include "experimental\filesystem"
#include <locale>
#include <codecvt>
#include "AudioHandler.h"
#include <thread>
#include <mutex>
#include <exception>
#include "SpriteBatch.h"
#include "CollisionManager.h"
#include "rapidjson\document.h"
#include "rapidjson\filereadstream.h"
#include "rapidjson\filewritestream.h"
#include "rapidjson\writer.h"

#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min
#define FILE_BUFFER_SIZE 65536

struct LoadingNotifications {
	std::string category;
	std::string object;
	std::exception_ptr errorCode;
};

struct FMODUserData {
	std::shared_ptr<std::string> name;
	std::shared_ptr<std::string> filenameKey;
};

enum ComponentTypes {
	// While Transform is tracked here, it is often skipped or handled uniquely
	// when assessing all components, as it cannot be removed or doubled
	TRANSFORM,
	COLLIDER,
	TERRAIN,
	PARTICLE_SYSTEM,
	LIGHT,
	MESH_RENDERER,
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
	std::vector<std::shared_ptr<Camera>> globalCameras;
	std::vector<std::shared_ptr<Mesh>> globalMeshes;
	std::vector<std::shared_ptr<Material>> globalMaterials;
	std::vector<std::shared_ptr<GameEntity>> globalEntities;
	std::vector<std::shared_ptr<TerrainMaterial>> globalTerrainMaterials;
	std::vector<FMOD::Sound*> globalSounds;
	std::vector<std::shared_ptr<SHOEFont>> globalFonts;

	std::condition_variable* threadNotifier;
	std::mutex* threadLock;

	// Most recently loaded object from category
	LoadingNotifications loaded;
	// Helper functions for threads
	void SetLoadedAndWait(std::string category, std::string object, std::exception_ptr error = NULL);
	void SetLoadingAndWait(std::string category, std::string object);
	AMLoadState assetManagerLoadState;
	bool singleLoadComplete;

	std::string currentSceneName;
	std::string loadingSceneName;

public:
	static bool materialSortDirty;

	~AssetManager();

	Microsoft::WRL::ComPtr<ID3D11Device> GetDevice();
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> GetContext();

	void Initialize(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::condition_variable* threadNotifier, std::mutex* threadLock, HWND hwnd);

	// Loading helper methods for other classes
	std::string GetLastLoadedCategory();
	std::string GetLastLoadedObject();

	std::exception_ptr GetLoadingException();
	AMLoadState GetAMLoadState();

	bool GetSingleLoadComplete();
	void SetAMLoadState(AMLoadState state);
	void SetSingleLoadComplete(bool loadComplete);

	std::string GetLoadingSceneName();
	// Pass file pointer?
	void LoadScene(std::string filepath, std::condition_variable* threadNotifier, std::mutex* threadLock);
	void LoadScene(FILE* file, std::condition_variable* threadNotifier, std::mutex* threadLock);
	void SaveScene(std::string filepath, std::string sceneName = "");
	void SaveScene(FILE* file, std::string sceneName = "");

	std::string GetCurrentSceneName();

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
	std::shared_ptr<Camera> GetMainCamera();
	std::shared_ptr<Camera> GetPlayCamera();
	std::vector<std::shared_ptr<Camera>> GetCamerasByTag(CameraType type);
	void SetCameraTag(std::shared_ptr<Camera> cam, CameraType tag);

	// Methods to create new assets

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
	std::shared_ptr<Mesh> CreateMesh(std::string id, std::string nameToLoad);
	std::shared_ptr<Camera> CreateCamera(std::string id, DirectX::XMFLOAT3 pos, float aspectRatio, int type);
	std::shared_ptr<Light> CreateDirectionalLight(std::string name, DirectX::XMFLOAT3 direction, DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), float intensity = 1.0f);
	std::shared_ptr<Light> CreatePointLight(std::string name, float range, DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), float intensity = 1.0f);
	std::shared_ptr<Light> CreateSpotLight(std::string name, DirectX::XMFLOAT3 direction, float range, DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), float intensity = 1.0f);
	std::shared_ptr<Material> CreatePBRMaterial(std::string id,
											    std::string albedoNameToLoad,
											    std::string normalNameToLoad,
											    std::string metalnessNameToLoad,
											    std::string roughnessNameToLoad,
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
	FMOD::Sound* CreateSound(std::string filePath, FMOD_MODE mode, std::string name = "");
	std::shared_ptr<SHOEFont> CreateSHOEFont(std::string name, std::string filePath, bool preInitializing = false);

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
														  DirectX::XMFLOAT3 direction,
														  DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
														  float intensity = 1.0f);
	std::shared_ptr<Light> CreatePointLightOnEntity(std::shared_ptr<GameEntity> entityToEdit,
													float range,
													DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
													float intensity = 1.0f);
	std::shared_ptr<Light> CreateSpotLightOnEntity(std::shared_ptr<GameEntity> entityToEdit, 
												   DirectX::XMFLOAT3 direction,
												   float range,
												   DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
												   float intensity = 1.0f);

	// Creation Helper Methods
	HRESULT LoadPBRTexture(std::string nameToLoad, OUT Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* texture, PBRTextureTypes textureType);
	void SetMaterialTextureFileKey(std::string textureFilename, std::shared_ptr<Material> mat, PBRTextureTypes textureType);
	std::string SerializeFileName(std::string assetFolderPath, std::string fullPathToAsset);
	std::string DeSerializeFileName(std::string assetPath);

	// Helper methods to add components to objects

	std::shared_ptr<Collider> AddColliderToGameEntity(OUT std::shared_ptr<GameEntity> entity);
	std::shared_ptr<Collider> AddTriggerBoxToGameEntity(OUT std::shared_ptr<GameEntity> entity);

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
	void RemoveMaterial(std::string name);
	void RemoveMaterial(int id);
	void RemoveTerrainMaterial(std::string name);
	void RemoveTerrainMaterial(int id);

	void CleanAllVectors();

	// Methods to disable and enable assets for rendering
	// Currently not implemented except for lights
	void EnableDisableCamera(std::string name, bool value);
	void EnableDisableCamera(int id, bool value);

	// Asset search-by-name methods

	std::shared_ptr<GameEntity> GetGameEntityByName(std::string name);
	std::shared_ptr<Sky> GetSkyByName(std::string name);
	std::shared_ptr<SimpleVertexShader> GetVertexShaderByName(std::string name);
	std::shared_ptr<SimplePixelShader> GetPixelShaderByName(std::string name);
	std::shared_ptr<SimpleComputeShader> GetComputeShaderByName(std::string name);
	std::shared_ptr<Mesh> GetMeshByName(std::string name);
	std::shared_ptr<Camera> GetCameraByName(std::string name);
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
	int GetCameraIDByName(std::string name);
	int GetMaterialIDByName(std::string name);
	//int GetTerrainMaterialIDByName(std::string name);

	// Relevant Get methods
	
	Microsoft::WRL::ComPtr<ID3D11InputLayout> GetInputLayout();
	size_t GetPixelShaderArraySize();
	size_t GetVertexShaderArraySize();
	size_t GetComputeShaderArraySize();
	size_t GetSkyArraySize();
	size_t GetCameraArraySize();
	size_t GetMeshArraySize();
	size_t GetMaterialArraySize();
	size_t GetGameEntityArraySize();
	size_t GetTerrainMaterialArraySize();
	size_t GetSoundArraySize();

	FMOD::Sound* GetSoundAtID(int id);
	std::shared_ptr<Camera> GetCameraAtID(int id);
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