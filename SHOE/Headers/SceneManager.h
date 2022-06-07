#pragma once

#include <string>
#include <DirectXMath.h>
#include "rapidjson\document.h"
#include "rapidjson\filereadstream.h"
#include "rapidjson\filewritestream.h"
#include "rapidjson\writer.h"
#include "AssetManager.h"
#include "EngineState.h"

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
#define SKIES "s" // category - only used to fetch actual data
#define SOUNDS "sO" // category - only used to fetch actual data
#define TERRAIN_MATERIALS "tM" // category - only used to fetch actual data

// Mesh Data:
#define MESH_INDEX_COUNT "iC" // int
#define MESH_MATERIAL_INDEX "mI" // int
#define MESH_NEEDS_DEPTH_PREPASS "nDP" // bool
#define MESH_NAME "mN" // string
#define MESH_FILENAME_KEY "mFK" // string

// Material Data:
#define MAT_UV_TILING "aUT" // float
#define MAT_NAME "aN" //string
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

// Generic Shader Data:
#define SHADER_NAME "sN" // string
#define SHADER_FILE_PATH "sK" // string

// Vertex Shader Data:
#define VERTEX_SHADER_OBJECT "vSO" // category - only used to fetch actual data

// Pixel Shader Data:
#define PIXEL_SHADER_OBJECT "pSO" // category - only used to fetch actual data

// Compute Shader Data:
#define COMPUTE_SHADER_OBJECT "cSO" // category - only used to fetch actual data

// Sky Data:
#define SKY_NAME "sN" // string
#define SKY_FILENAME_KEY_TYPE "sFT" // bool
#define SKY_FILENAME_KEY "sFK" // string
#define SKY_FILENAME_EXTENSION "sFE" //string

// Sound Data:
#define SOUND_FILENAME_KEY "oFK" // string
#define SOUND_NAME "oN" // string
#define SOUND_FMOD_MODE "oFM" // int

// Terrain Material Data:
#define TERRAIN_MATERIAL_NAME "tN" // string
#define TERRAIN_MATERIAL_BLEND_MAP_PATH "tBP" // string
#define TERRAIN_MATERIAL_BLEND_MAP_ENABLED "tBE" // bool
#define TERRAIN_MATERIAL_MATERIAL_ARRAY "tMA" // array of Materials

// Entities:
#define ENTITY_NAME "n" // string
#define ENTITY_ENABLED "e" // bool
#define COMPONENT_TYPE "ct" // int
#define COMPONENTS "cm" // category - only used to fetch actual data

// Transform Data:
#define TRANSFORM_LOCAL_POSITION "tLP" // float array 3
#define TRANSFORM_LOCAL_SCALE "tLS" // float array 3
#define TRANSFORM_LOCAL_ROTATION "tLR" // float array 3

//All Components:
#define COMPONENT_ENABLED "cE" // bool

// Light Components:
#define LIGHT_TYPE "lT" // int
#define LIGHT_INTENSITY "i" // float
#define LIGHT_RANGE "lR" // float
#define LIGHT_COLOR "lC" // float array 4
#define LIGHT_CASTS_SHADOWS "lS" // bool

// Mesh Renderer Components:
#define MESH_OBJECT "mO" // category - only used to fetch actual data
#define MATERIAL_OBJECT "mA" // category - only used to fetch actual data
#define MESH_COMPONENT_INDEX "mCI" // int
#define MATERIAL_COMPONENT_INDEX "aCI" // int

// Collider Data:
#define COLLIDER_TYPE "cT" // bool
#define COLLIDER_IS_VISIBLE "cIV" // bool
#define COLLIDER_POSITION_OFFSET "cPO" // float array 3
#define COLLIDER_SCALE_OFFSET "cOS" // float array 3
#define COLLIDER_ROTATION_OFFSET "cRO" // float array 3

// Terrain Data:
#define TERRAIN_HEIGHTMAP_FILENAME_KEY "hFK" // string
#define TERRAIN_INDEX_OF_TERRAIN_MATERIAL "hIM" // int

// Camera Data:
#define CAMERA_ASPECT_RATIO "cAR" // float
#define CAMERA_PROJECTION_MATRIX_TYPE "cPM" // int
#define CAMERA_NEAR_DISTANCE "cND" // float
#define CAMERA_FAR_DISTANCE "cFD" // float
#define CAMERA_FIELD_OF_VIEW "cF" //float
#define CAMERA_IS_MAIN "cM" // bool

// Particle System Data:
#define PARTICLE_SYSTEM_MAX_PARTICLES "pMP" // int
#define PARTICLE_SYSTEM_FILENAME_KEY "pFK" // string
#define PARTICLE_SYSTEM_IS_MULTI_PARTICLE "pIP" // bool
#define PARTICLE_SYSTEM_ADDITIVE_BLEND "pAB" // bool
#define PARTICLE_SYSTEM_COLOR_TINT "pCT" // float array 4
#define PARTICLE_SYSTEM_SCALE "pS" // float
#define PARTICLE_SYSTEM_SPEED "pE" //float
#define PARTICLE_SYSTEM_DESTINATION "pD" //float array 3
#define PARTICLE_SYSTEM_PARTICLES_PER_SECOND "pPS" //float
#define PARTICLE_SYSTEM_PARTICLE_LIFETIME "pL" //float

// Noclip Movement Data:
#define NOCLIP_LOOK_SPEED "cLS" // float
#define NOCLIP_MOVE_SPEED "cMS" // float

#pragma endregion

#define FILE_BUFFER_SIZE 65536

class SceneManager
{
#pragma region Singleton
public:
	// Gets the one and only instance of this class
	static SceneManager& GetInstance()
	{
		if (!instance)
		{
			instance = new SceneManager();
		}

		return *instance;
	}

	// Remove these functions (C++ 11 version)
	SceneManager(SceneManager const&) = delete;
	void operator=(SceneManager const&) = delete;

private:
	static SceneManager* instance;
	SceneManager()
	{

	};
#pragma endregion
private:
	AssetManager& assetManager = AssetManager::GetInstance();

	EngineState* engineState;

	std::string currentSceneName;
	std::string loadingSceneName;

	std::string currentLoadCategory;
	std::string currentLoadName;
	std::exception_ptr error;

	DirectX::XMFLOAT2 LoadFloat2(const rapidjson::Value& jsonBlock, const char* memberName);
	DirectX::XMFLOAT3 LoadFloat3(const rapidjson::Value& jsonBlock, const char* memberName);
	DirectX::XMFLOAT4 LoadFloat4(const rapidjson::Value& jsonBlock, const char* memberName);
	void SaveFloat2(rapidjson::Value& jsonObject, const char* memberName, DirectX::XMFLOAT2 vec, rapidjson::Document& sceneDoc);
	void SaveFloat3(rapidjson::Value& jsonObject, const char* memberName, DirectX::XMFLOAT3 vec, rapidjson::Document& sceneDoc);
	void SaveFloat4(rapidjson::Value& jsonObject, const char* memberName, DirectX::XMFLOAT4 vec, rapidjson::Document& sceneDoc);

	std::string LoadDeserializedFileName(const rapidjson::Value& jsonBlock, const char* memberName);

	void LoadAssets(const rapidjson::Value& sceneDoc, std::function<void()> progressListener = {});
	void LoadEntities(const rapidjson::Value& sceneDoc, std::function<void()> progressListener = {});

	void SaveAssets(rapidjson::Document& sceneDocToSave);
	void SaveEntities(rapidjson::Document& sceneDocToSave);
public:
	void Initialize(EngineState* engineState);

	void LoadScene(std::string filepath, std::function<void()> progressListener = {});
	void SaveScene(std::string filepath, std::string sceneName = "");

	void PrePlaySave();
	void PostPlayLoad();

	// Loading helper methods for other classes
	std::string GetLoadingSceneName();
	std::string GetCurrentSceneName();
	std::string GetLoadingCategory();
	std::string GetLoadingObjectName();

	std::exception_ptr GetLoadingException();
};

