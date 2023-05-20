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
#define VALID_SHOE_SCENE "v" // bool
#define SCENE_NAME "sN" // string

//Shared
#define NAME "n"
#define FILENAME_KEY "fK" // string
#define ENABLED "e" // bool

// Categories:
#define ENTITIES "e" // category - only used to fetch actual data
#define MESHES "m" // category - only used to fetch actual data
#define TEXTURES "tE" // category - only used to fetch actual data
#define MATERIALS "a" // category - only used to fetch actual data
#define VERTEX_SHADERS "vS" // category - only used to fetch actual data
#define PIXEL_SHADERS "pS" // category - only used to fetch actual data
#define COMPUTE_SHADERS "cS" // category - only used to fetch actual data
#define FONTS "f" // category - only used to fetch actual data
#define TEXTURE_SAMPLE_STATES "tS" // category - only used to fetch actual data
#define SKIES "s" // category - only used to fetch actual data
#define SOUNDS "sO" // category - only used to fetch actual data
#define TERRAIN_MATERIALS "tM" // category - only used to fetch actual data

// Mesh Data:
#define MESH_INDEX_COUNT "iC" // int
#define MESH_MATERIAL_INDEX "mI" // int
#define MESH_NEEDS_DEPTH_PREPASS "nDP" // bool

// Texture Data:
#define TEXTURE_ASSET_PATH_INDEX "tAP" // int

// Material Data:
#define MAT_UV_TILING "uT" // float
#define MAT_IS_TRANSPARENT "t" // bool
#define MAT_IS_REFRACTIVE "r" // bool
#define MAT_INDEX_OF_REFRACTION "iOR" // float
#define MAT_REFRACTION_SCALE "rS" // float
#define MAT_COLOR_TINT "cT" // float array 4
#define MAT_PIXEL_SHADER "pS" // int
#define MAT_REFRACTION_PIXEL_SHADER "rPS" // int
#define MAT_VERTEX_SHADER "vS" // int
#define MAT_TEXTURE_OR_ALBEDO_MAP "aM" // int 
#define MAT_NORMAL_MAP "nM" // int
#define MAT_METAL_MAP "mM" // int
#define MAT_ROUGHNESS_MAP "rM" // int
#define MAT_TEXTURE_SAMPLER_STATE "tS" // int
#define MAT_CLAMP_SAMPLER_STATE "cS" // int

// Texture Sampler data:
#define SAMPLER_ADDRESS_U "u" // int
#define SAMPLER_ADDRESS_V "v" // int
#define SAMPLER_ADDRESS_W "w" // int
#define SAMPLER_BORDER_COLOR "bC" // float array 4
#define SAMPLER_COMPARISON_FUNCTION "cF" // int?
#define SAMPLER_FILTER "f" // int
#define SAMPLER_MAX_ANISOTROPY "mA" // int
#define SAMPLER_MAX_LOD "sML" // float
#define SAMPLER_MIN_LOD "sIL" // float
#define SAMPLER_MIP_LOD_BIAS "b" // float

// Generic Shader Data:
#define SHADER_FILE_PATH "p" // string

// Sky Data:
#define SKY_FILENAME_KEY_TYPE "fKT" // bool
#define SKY_FILENAME_EXTENSION "fE" //string

// Sound Data:
#define SOUND_FMOD_MODE "fM" // int

// Terrain Material Data:
#define TERRAIN_MATERIAL_BLEND_MAP_PATH "bP" // string
#define TERRAIN_MATERIAL_BLEND_MAP_ENABLED "bE" // bool
#define TERRAIN_MATERIAL_MATERIAL_ARRAY "mA" // array of Materials

// Entities:
#define COMPONENTS "c" // category - only used to fetch actual data
#define COMPONENT_TYPE "t" // int

// Transform Data:
#define TRANSFORM_LOCAL_POSITION "p" // float array 3
#define TRANSFORM_LOCAL_ROTATION "r" // float array 3
#define TRANSFORM_LOCAL_SCALE "s" // float array 3

// Light Components:
#define LIGHT_TYPE "lT" // int
#define LIGHT_INTENSITY "i" // float
#define LIGHT_RANGE "r" // float
#define LIGHT_COLOR "c" // float array 4
#define LIGHT_CASTS_SHADOWS "s" // bool

// Mesh Renderer Components:
#define MESH_COMPONENT_INDEX "mCI" // int
#define MATERIAL_COMPONENT_INDEX "aCI" // int

// Collider Data:
#define COLLIDER_TYPE "cT" // bool
#define COLLIDER_IS_VISIBLE "v" // bool
#define COLLIDER_POSITION_OFFSET "p" // float array 3
#define COLLIDER_ROTATION_OFFSET "r" // float array 3
#define COLLIDER_SCALE_OFFSET "s" // float array 3

// Terrain Data:
#define TERRAIN_INDEX_OF_TERRAIN_MATERIAL "hIM" // int

// Camera Data:
#define CAMERA_ASPECT_RATIO "aR" // float
#define CAMERA_PROJECTION_MATRIX_TYPE "pM" // int
#define CAMERA_NEAR_DISTANCE "nD" // float
#define CAMERA_FAR_DISTANCE "fD" // float
#define CAMERA_FIELD_OF_VIEW "f" //float
#define CAMERA_IS_MAIN "m" // bool

// Particle System Data:
#define PARTICLE_SYSTEM_MAX_PARTICLES "mP" // int
#define PARTICLE_SYSTEM_IS_MULTI_PARTICLE "iM" // bool
#define PARTICLE_SYSTEM_ADDITIVE_BLEND "aB" // bool
#define PARTICLE_SYSTEM_COLOR_TINT "c" // float array 4
#define PARTICLE_SYSTEM_SCALE "s" // float
#define PARTICLE_SYSTEM_SPEED "sP" //float
#define PARTICLE_SYSTEM_DESTINATION "d" //float array 3
#define PARTICLE_SYSTEM_PARTICLES_PER_SECOND "pPS" //float
#define PARTICLE_SYSTEM_PARTICLE_LIFETIME "pL" //float

// Noclip Movement Data:
#define NOCLIP_LOOK_SPEED "lS" // float
#define NOCLIP_MOVE_SPEED "mS" // float

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
	std::function<void()> progressListener;

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
	void Initialize(EngineState* engineState, std::function<void()> progressListener);

	void LoadScene(std::string filepath);
	void SaveScene(std::string filepath, std::string sceneName = "");
	void SaveSceneAs();

	void PrePlaySave();
	void PostPlayLoad();

	// Loading helper methods for other classes
	std::string GetLoadingSceneName();
	std::string GetCurrentSceneName();
	std::string GetLoadingCategory();
	std::string GetLoadingObjectName();

	std::exception_ptr GetLoadingException();
};

