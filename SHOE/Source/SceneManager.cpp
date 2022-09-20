#include "../Headers/SceneManager.h"
#include "..\Headers\NoclipMovement.h"
#include "..\Headers\FlashlightController.h"

SceneManager* SceneManager::instance;

/// <summary>
/// Gets what object category is currently being loaded in
/// </summary>
/// <returns>Name of the category</returns>
std::string SceneManager::GetLoadingCategory() {
	return currentLoadCategory;
}

/// <summary>
/// Gets what object is currently being loaded in
/// </summary>
/// <returns>Name of the object</returns>
std::string SceneManager::GetLoadingObjectName() {
	return currentLoadName;
}

/// <summary>
/// Gets the current loading exception, should there be one
/// </summary>
/// <returns></returns>
std::exception_ptr SceneManager::GetLoadingException() {
	return error;
}

/// <summary>
/// Loads a Float2 stored in a JSON block
/// </summary>
/// <param name="jsonBlock">The block to load from</param>
/// <param name="memberName">The key to search</param>
/// <returns>Float2 populated from the given array type</returns>
DirectX::XMFLOAT2 SceneManager::LoadFloat2(const rapidjson::Value& jsonBlock, const char* memberName)
{
	const rapidjson::Value& vecBlock = jsonBlock.FindMember(memberName)->value;
	return DirectX::XMFLOAT2(vecBlock[0].GetDouble(),
		vecBlock[1].GetDouble());
}

/// <summary>
/// Loads a Float3 stored in a JSON block
/// </summary>
/// <param name="jsonBlock">The block to load from</param>
/// <param name="memberName">The key to search</param>
/// <returns>Float3 populated from the given array type</returns>
DirectX::XMFLOAT3 SceneManager::LoadFloat3(const rapidjson::Value& jsonBlock, const char* memberName)
{
	const rapidjson::Value& vecBlock = jsonBlock.FindMember(memberName)->value;
	return DirectX::XMFLOAT3(vecBlock[0].GetDouble(), 
		vecBlock[1].GetDouble(), 
		vecBlock[2].GetDouble());
}

/// <summary>
/// Loads a Float3 stored in a JSON block
/// </summary>
/// <param name="jsonBlock">The block to load from</param>
/// <param name="memberName">The key to search</param>
/// <returns>Float3 populated from the given array type</returns>
DirectX::XMFLOAT4 SceneManager::LoadFloat4(const rapidjson::Value& jsonBlock, const char* memberName)
{
	const rapidjson::Value& vecBlock = jsonBlock.FindMember(memberName)->value;
	return DirectX::XMFLOAT4(vecBlock[0].GetDouble(),
		vecBlock[1].GetDouble(),
		vecBlock[2].GetDouble(),
		vecBlock[3].GetDouble());
}

/// <summary>
/// Stores a Float2 as a float array in JSON
/// </summary>
/// <param name="jsonObject">The JSON object to store the data into</param>
/// <param name="memberName">The key to store the data array under</param>
/// <param name="vec">The Float2 to store</param>
/// <param name="sceneDoc">The document the JSON is going into</param>
void SceneManager::SaveFloat2(rapidjson::Value& jsonObject, const char* memberName, DirectX::XMFLOAT2 vec, rapidjson::Document& sceneDoc)
{
	rapidjson::Value float2(rapidjson::kArrayType);
	float2.PushBack(vec.x, sceneDoc.GetAllocator());
	float2.PushBack(vec.y, sceneDoc.GetAllocator());

	jsonObject.AddMember(rapidjson::StringRef(memberName),
		float2,
		sceneDoc.GetAllocator());
}

/// <summary>
/// Stores a Float3 as a float array in JSON
/// </summary>
/// <param name="jsonObject">The JSON object to store the data into</param>
/// <param name="memberName">The key to store the data array under</param>
/// <param name="vec">The Float3 to store</param>
/// <param name="sceneDoc">The document the JSON is going into</param>
void SceneManager::SaveFloat3(rapidjson::Value& jsonObject, const char* memberName, DirectX::XMFLOAT3 vec, rapidjson::Document& sceneDoc)
{
	rapidjson::Value float3(rapidjson::kArrayType);
	float3.PushBack(vec.x, sceneDoc.GetAllocator());
	float3.PushBack(vec.y, sceneDoc.GetAllocator());
	float3.PushBack(vec.z, sceneDoc.GetAllocator());

	jsonObject.AddMember(rapidjson::StringRef(memberName), 
		float3, 
		sceneDoc.GetAllocator());
}

/// <summary>
/// Stores a Float4 as a float array in JSON
/// </summary>
/// <param name="jsonObject">The JSON object to store the data into</param>
/// <param name="memberName">The key to store the data array under</param>
/// <param name="vec">The Float4 to store</param>
/// <param name="sceneDoc">The document the JSON is going into</param>
void SceneManager::SaveFloat4(rapidjson::Value& jsonObject, const char* memberName, DirectX::XMFLOAT4 vec, rapidjson::Document& sceneDoc)
{
	rapidjson::Value float4(rapidjson::kArrayType);
	float4.PushBack(vec.x, sceneDoc.GetAllocator());
	float4.PushBack(vec.y, sceneDoc.GetAllocator());
	float4.PushBack(vec.z, sceneDoc.GetAllocator());
	float4.PushBack(vec.w, sceneDoc.GetAllocator());

	jsonObject.AddMember(rapidjson::StringRef(memberName),
		float4,
		sceneDoc.GetAllocator());
}

/// <summary>
/// Deserializes a file name in a JSON block
/// </summary>
/// <param name="jsonBlock">JSON block data is in</param>
/// <param name="memberName">Key of the value to deserialize</param>
/// <returns>The deserialized file name</returns>
std::string SceneManager::LoadDeserializedFileName(const rapidjson::Value& jsonBlock, const char* memberName)
{
	return assetManager.DeSerializeFileName(jsonBlock.FindMember(memberName)->value.GetString());
}

/// <summary>
/// Loads the scene assets
/// </summary>
/// <param name="sceneDoc">JSON block to load from</param>
/// <param name="progressListener">Function to call when progressing to each new object load</param>
void SceneManager::LoadAssets(const rapidjson::Value& sceneDoc, std::function<void()> progressListener)
{
	// Load order:
	// Fonts
	// Texture Sample States
	// Shaders
	// Textures
	// Materials
	// Meshes
	// Terrain Materials
	// Skies
	// Audio

	// Fonts
	currentLoadCategory = "Fonts";
	const rapidjson::Value& fontBlock = sceneDoc[FONTS];
	assert(fontBlock.IsArray());
	for (rapidjson::SizeType i = 0; i < fontBlock.Size(); i++) {
		currentLoadName = fontBlock[i].FindMember(NAME)->value.GetString();
		//if(progressListener) progressListener(); NEEDS PRE-LOADED FONTS
		assetManager.CreateSHOEFont(currentLoadName, LoadDeserializedFileName(fontBlock[i], FILENAME_KEY));
	}

	// Texture Sampler States
	currentLoadCategory = "Texture Sampler States";
	currentLoadName = "";
	if(progressListener) progressListener();
	const rapidjson::Value& sampleStateBlock = sceneDoc[TEXTURE_SAMPLE_STATES];
	assert(sampleStateBlock.IsArray());
	for (rapidjson::SizeType i = 0; i < sampleStateBlock.Size(); i++) {
		Microsoft::WRL::ComPtr<ID3D11SamplerState> loadedSampler;

		D3D11_SAMPLER_DESC loadDesc;
		loadDesc.AddressU = (D3D11_TEXTURE_ADDRESS_MODE)(sampleStateBlock[i].FindMember(SAMPLER_ADDRESS_U)->value.GetInt());
		loadDesc.AddressV = (D3D11_TEXTURE_ADDRESS_MODE)(sampleStateBlock[i].FindMember(SAMPLER_ADDRESS_V)->value.GetInt());
		loadDesc.AddressW = (D3D11_TEXTURE_ADDRESS_MODE)(sampleStateBlock[i].FindMember(SAMPLER_ADDRESS_W)->value.GetInt());
		loadDesc.Filter = (D3D11_FILTER)(sampleStateBlock[i].FindMember(SAMPLER_FILTER)->value.GetInt());
		loadDesc.MaxAnisotropy = sampleStateBlock[i].FindMember(SAMPLER_MAX_ANISOTROPY)->value.GetInt();
		loadDesc.MinLOD = sampleStateBlock[i].FindMember(SAMPLER_MIN_LOD)->value.GetDouble();
		loadDesc.MaxLOD = sampleStateBlock[i].FindMember(SAMPLER_MAX_LOD)->value.GetDouble();
		loadDesc.MipLODBias = sampleStateBlock[i].FindMember(SAMPLER_MIP_LOD_BIAS)->value.GetDouble();
		loadDesc.ComparisonFunc = (D3D11_COMPARISON_FUNC)(sampleStateBlock[i].FindMember(SAMPLER_COMPARISON_FUNCTION)->value.GetInt());

		const rapidjson::Value& borderColorBlock = sampleStateBlock[i].FindMember(SAMPLER_BORDER_COLOR)->value;
		for (int j = 0; j < 4; j++) {
			loadDesc.BorderColor[j] = borderColorBlock[j].GetDouble();
		}

		assetManager.device->CreateSamplerState(&loadDesc, &loadedSampler);

		assetManager.textureSampleStates.push_back(loadedSampler);
	}

	assetManager.textureState = assetManager.textureSampleStates[0];
	assetManager.clampState = assetManager.textureSampleStates[1];

	// Pixel Shaders
	currentLoadCategory = "Pixel Shaders";
	const rapidjson::Value& pixelShaderBlock = sceneDoc[PIXEL_SHADERS];
	assert(pixelShaderBlock.IsArray());
	for (rapidjson::SizeType i = 0; i < pixelShaderBlock.Size(); i++) {
		currentLoadName = pixelShaderBlock[i].FindMember(NAME)->value.GetString();
		if(progressListener) progressListener();
		assetManager.CreatePixelShader(currentLoadName, LoadDeserializedFileName(pixelShaderBlock[i], SHADER_FILE_PATH));
	}

	// Vertex Shaders
	currentLoadCategory = "Vertex Shaders";
	const rapidjson::Value& vertexShaderBlock = sceneDoc[VERTEX_SHADERS];
	assert(vertexShaderBlock.IsArray());
	for (rapidjson::SizeType i = 0; i < vertexShaderBlock.Size(); i++) {
		currentLoadName = vertexShaderBlock[i].FindMember(NAME)->value.GetString();
		if(progressListener) progressListener();
		assetManager.CreateVertexShader(currentLoadName, LoadDeserializedFileName(vertexShaderBlock[i], SHADER_FILE_PATH));
	}

	// Compute Shaders
	currentLoadCategory = "Compute Shaders";
	const rapidjson::Value& computeShaderBlock = sceneDoc[COMPUTE_SHADERS];
	assert(computeShaderBlock.IsArray());
	for (rapidjson::SizeType i = 0; i < computeShaderBlock.Size(); i++) {
		currentLoadName = computeShaderBlock[i].FindMember(NAME)->value.GetString();
		if(progressListener) progressListener();
		assetManager.CreateComputeShader(currentLoadName, LoadDeserializedFileName(computeShaderBlock[i], SHADER_FILE_PATH));
	}

	currentLoadCategory = "Textures";
	const rapidjson::Value& textureBlock = sceneDoc[TEXTURES];
	assert(textureBlock.IsArray());
	for (rapidjson::SizeType i = 0; i < textureBlock.Size(); i++) {
		currentLoadName = textureBlock[i].FindMember(NAME)->value.GetString();
		if (progressListener) progressListener();

		// Textures require a check to determine which valid texture folder
		// they're in.
		AssetPathIndex assetPath;
		assetPath = (AssetPathIndex)(textureBlock[i].FindMember(TEXTURE_ASSET_PATH_INDEX)->value.GetInt());
		assetManager.CreateTexture(LoadDeserializedFileName(textureBlock[i], FILENAME_KEY), currentLoadName, assetPath);
	}

	currentLoadCategory = "Materials";
	const rapidjson::Value& materialBlock = sceneDoc[MATERIALS];
	assert(materialBlock.IsArray());
	for (rapidjson::SizeType i = 0; i < materialBlock.Size(); i++) {
		currentLoadName = LoadDeserializedFileName(materialBlock[i], NAME);
		if(progressListener) progressListener();

		std::shared_ptr<Material> newMaterial = assetManager.CreatePBRMaterial(
			currentLoadName,
			assetManager.globalTextures[materialBlock[i].FindMember(MAT_TEXTURE_OR_ALBEDO_MAP)->value.GetInt()],
			assetManager.globalTextures[materialBlock[i].FindMember(MAT_NORMAL_MAP)->value.GetInt()], 
			assetManager.globalTextures[materialBlock[i].FindMember(MAT_METAL_MAP)->value.GetInt()],
			assetManager.globalTextures[materialBlock[i].FindMember(MAT_ROUGHNESS_MAP)->value.GetInt()]);

		newMaterial->SetTransparent(materialBlock[i].FindMember(MAT_IS_TRANSPARENT)->value.GetBool());

		newMaterial->SetRefractive(materialBlock[i].FindMember(MAT_IS_REFRACTIVE)->value.GetBool());

		newMaterial->SetTiling(materialBlock[i].FindMember(MAT_UV_TILING)->value.GetDouble());

		newMaterial->SetIndexOfRefraction(materialBlock[i].FindMember(MAT_INDEX_OF_REFRACTION)->value.GetDouble());

		newMaterial->SetRefractionScale(materialBlock[i].FindMember(MAT_REFRACTION_SCALE)->value.GetDouble());

		newMaterial->SetSamplerState(assetManager.textureSampleStates[materialBlock[i].FindMember(MAT_TEXTURE_SAMPLER_STATE)->value.GetInt()]);

		newMaterial->SetClampSamplerState(assetManager.textureSampleStates[materialBlock[i].FindMember(MAT_CLAMP_SAMPLER_STATE)->value.GetInt()]);

		newMaterial->SetVertexShader(assetManager.GetVertexShaderAtID(materialBlock[i].FindMember(MAT_VERTEX_SHADER)->value.GetInt()));

		newMaterial->SetPixelShader(assetManager.GetPixelShaderAtID(materialBlock[i].FindMember(MAT_PIXEL_SHADER)->value.GetInt()));

		if (newMaterial->GetRefractive() || newMaterial->GetTransparent()) {
			int index = materialBlock[i].FindMember(MAT_REFRACTION_PIXEL_SHADER)->value.GetInt();
			newMaterial->SetRefractivePixelShader(assetManager.GetPixelShaderAtID(index));
		}

		newMaterial->SetTint(LoadFloat4(materialBlock[i], MAT_COLOR_TINT));
	}

	currentLoadCategory = "Meshes";
	const rapidjson::Value& meshBlock = sceneDoc[MESHES];
	for (rapidjson::SizeType i = 0; i < meshBlock.Size(); i++) {
		currentLoadName = meshBlock[i].FindMember(NAME)->value.GetString();
		if(progressListener) progressListener();

		std::shared_ptr<Mesh> newMesh = assetManager.CreateMesh(currentLoadName, LoadDeserializedFileName(meshBlock[i], FILENAME_KEY));
		newMesh->SetDepthPrePass(meshBlock[i].FindMember(MESH_NEEDS_DEPTH_PREPASS)->value.GetBool());
		newMesh->SetMaterialIndex(meshBlock[i].FindMember(MESH_MATERIAL_INDEX)->value.GetInt());

		// This is currently generated automatically. Would need to change
		// if storing meshes built through code arrays becomes supported.
		// newMesh->SetIndexCount(meshBlock[i].FindMember(MESH_INDEX_COUNT)->value.GetInt());
	}

	currentLoadCategory = "Terrain Materials";
	const rapidjson::Value& terrainMaterialBlock = sceneDoc[TERRAIN_MATERIALS];
	assert(terrainMaterialBlock.IsArray());
	for (rapidjson::SizeType i = 0; i < terrainMaterialBlock.Size(); i++) {
		const rapidjson::Value& tMatInternalBlock = terrainMaterialBlock[i].FindMember(TERRAIN_MATERIAL_MATERIAL_ARRAY)->value;
		currentLoadName = terrainMaterialBlock[i].FindMember(NAME)->value.GetString();
		if(progressListener) progressListener();

		std::vector<std::shared_ptr<Material>> internalMaterials;
		for (rapidjson::SizeType j = 0; j < tMatInternalBlock.Size(); j++) {
			// Material texture strings are being incorrectly serialized/loaded
			internalMaterials.push_back(assetManager.GetMaterialAtID(tMatInternalBlock[j].GetInt()));
		}

		if (terrainMaterialBlock[i].FindMember(TERRAIN_MATERIAL_BLEND_MAP_ENABLED)->value.GetBool()) {
			std::shared_ptr<TerrainMaterial> newTMat = assetManager.CreateTerrainMaterial(currentLoadName, internalMaterials, LoadDeserializedFileName(terrainMaterialBlock[i], TERRAIN_MATERIAL_BLEND_MAP_PATH));
		}
		else {
			std::shared_ptr<TerrainMaterial> newTMat = assetManager.CreateTerrainMaterial(currentLoadName, internalMaterials);
		}
	}

	currentLoadCategory = "Skies";
	const rapidjson::Value& skyBlock = sceneDoc[SKIES];
	assert(skyBlock.IsArray());
	for (rapidjson::SizeType i = 0; i < skyBlock.Size(); i++) {
		currentLoadName = skyBlock[i].FindMember(NAME)->value.GetString();
		if(progressListener) progressListener();
		bool keyType = skyBlock[i].FindMember(SKY_FILENAME_KEY_TYPE)->value.GetBool();
		std::string fileExt = keyType ? skyBlock[i].FindMember(SKY_FILENAME_EXTENSION)->value.GetString() : ".png";
		assetManager.CreateSky(LoadDeserializedFileName(skyBlock[i], FILENAME_KEY), keyType, currentLoadName, fileExt);
	}

	currentLoadCategory = "Sounds";
	const rapidjson::Value& soundBlock = sceneDoc[SOUNDS];
	assert(soundBlock.IsArray());
	for (rapidjson::SizeType i = 0; i < soundBlock.Size(); i++) {
		currentLoadName = soundBlock[i].FindMember(NAME)->value.GetString();
		if(progressListener) progressListener();
		int mode = soundBlock[i].FindMember(SOUND_FMOD_MODE)->value.GetInt();
		FMOD::Sound* newSound = assetManager.CreateSound(LoadDeserializedFileName(soundBlock[i], FILENAME_KEY), mode, currentLoadName);
	}
}

/// <summary>
/// Loads the scene entities
/// </summary>
/// <param name="sceneDoc">JSON block to load from</param>
/// <param name="progressListener">Function to call when progressing to each new entity load</param>
void SceneManager::LoadEntities(const rapidjson::Value& sceneDoc, std::function<void()> progressListener)
{
	currentLoadCategory = "Entities";
	const rapidjson::Value& entityBlock = sceneDoc[ENTITIES];
	assert(entityBlock.IsArray());
	for (rapidjson::SizeType i = 0; i < entityBlock.Size(); i++) {
		currentLoadName = entityBlock[i].FindMember(NAME)->value.GetString();
		if(progressListener) progressListener();

		std::shared_ptr<GameEntity> newEnt = assetManager.CreateGameEntity(currentLoadName);
		newEnt->SetEnabled(entityBlock[i].FindMember(ENABLED)->value.GetBool());

		newEnt->GetTransform()->SetPosition(LoadFloat3(entityBlock[i], TRANSFORM_LOCAL_POSITION));
		newEnt->GetTransform()->SetRotation(LoadFloat3(entityBlock[i], TRANSFORM_LOCAL_ROTATION));
		newEnt->GetTransform()->SetScale(LoadFloat3(entityBlock[i], TRANSFORM_LOCAL_SCALE));

		const rapidjson::Value& componentBlock = entityBlock[i].FindMember(COMPONENTS)->value;
		assert(componentBlock.IsArray());
		for (rapidjson::SizeType i = 0; i < componentBlock.Size(); i++) {
			int componentType = componentBlock[i].FindMember(COMPONENT_TYPE)->value.GetInt();
			if (componentType == ComponentTypes::COLLIDER) {
				std::shared_ptr<Collider> collider = newEnt->AddComponent<Collider>();

				collider->SetEnabled(componentBlock[i].FindMember(ENABLED)->value.GetBool());
				collider->SetVisible(componentBlock[i].FindMember(COLLIDER_IS_VISIBLE)->value.GetBool());
				collider->SetIsTrigger(componentBlock[i].FindMember(COLLIDER_TYPE)->value.GetBool());

				collider->SetPositionOffset(LoadFloat3(componentBlock[i], COLLIDER_POSITION_OFFSET));
				collider->SetRotationOffset(LoadFloat3(componentBlock[i], COLLIDER_ROTATION_OFFSET));
				collider->SetScale(LoadFloat3(componentBlock[i], COLLIDER_SCALE_OFFSET));
			}
			else if (componentType == ComponentTypes::TERRAIN) {
				std::shared_ptr<TerrainMaterial> tMat = assetManager.GetTerrainMaterialAtID(componentBlock[i].FindMember(TERRAIN_INDEX_OF_TERRAIN_MATERIAL)->value.GetInt());

				assetManager.CreateTerrainOnEntity(newEnt, LoadDeserializedFileName(componentBlock[i], FILENAME_KEY).c_str(), tMat)->SetEnabled(componentBlock[i].FindMember(ENABLED)->value.GetBool());
			}
			else if (componentType == ComponentTypes::PARTICLE_SYSTEM) {
				std::string filename = LoadDeserializedFileName(componentBlock[i], FILENAME_KEY);
				int maxParticles = componentBlock[i].FindMember(PARTICLE_SYSTEM_MAX_PARTICLES)->value.GetInt();
				bool blendState = componentBlock[i].FindMember(PARTICLE_SYSTEM_ADDITIVE_BLEND)->value.GetBool();
				bool isMultiParticle = componentBlock[i].FindMember(PARTICLE_SYSTEM_IS_MULTI_PARTICLE)->value.GetBool();
				float particlesPerSecond = componentBlock[i].FindMember(PARTICLE_SYSTEM_PARTICLES_PER_SECOND)->value.GetDouble();
				float particleLifetime = componentBlock[i].FindMember(PARTICLE_SYSTEM_PARTICLE_LIFETIME)->value.GetDouble();

				std::shared_ptr<ParticleSystem> newParticles = assetManager.CreateParticleEmitterOnEntity(newEnt, filename, maxParticles, particleLifetime, particlesPerSecond, isMultiParticle, blendState);

				newParticles->SetScale(componentBlock[i].FindMember(PARTICLE_SYSTEM_SCALE)->value.GetDouble());
				newParticles->SetSpeed(componentBlock[i].FindMember(PARTICLE_SYSTEM_SPEED)->value.GetDouble());
				newParticles->SetDestination(LoadFloat3(componentBlock[i], PARTICLE_SYSTEM_DESTINATION));
				bool enabled = componentBlock[i].FindMember(ENABLED)->value.GetBool();

				newParticles->SetColorTint(LoadFloat4(componentBlock[i], PARTICLE_SYSTEM_COLOR_TINT));

				// Particles are a bit of a mess, and need to be initially disabled for at least the first frame.
				newParticles->SetEnabled(false);
			}
			else if (componentType == ComponentTypes::LIGHT) {
				std::shared_ptr<Light> light;

				float type = componentBlock[i].FindMember(LIGHT_TYPE)->value.GetDouble();
				float intensity = componentBlock[i].FindMember(LIGHT_INTENSITY)->value.GetDouble();
				float range = componentBlock[i].FindMember(LIGHT_RANGE)->value.GetDouble();
				DirectX::XMFLOAT3 color = LoadFloat3(componentBlock[i], LIGHT_COLOR);

				if (type == 0.0f) {
					light = assetManager.CreateDirectionalLightOnEntity(newEnt, color, intensity);
				}
				else if (type == 1.0f) {
					light = assetManager.CreatePointLightOnEntity(newEnt, range, color, intensity);
				}
				else if (type == 2.0f) {
					light = assetManager.CreateSpotLightOnEntity(newEnt, range, color, intensity);
				}
				else {
					// Unrecognized light type, do nothing
				}
				light->SetEnabled(componentBlock[i].FindMember(ENABLED)->value.GetBool());
				light->SetCastsShadows(componentBlock[i].FindMember(LIGHT_CASTS_SHADOWS)->value.GetBool());
			}
			else if (componentType == ComponentTypes::MESH_RENDERER) {
				std::shared_ptr<MeshRenderer> mRenderer = newEnt->AddComponent<MeshRenderer>();
				mRenderer->SetMaterial(assetManager.GetMaterialAtID(componentBlock[i].FindMember(MATERIAL_COMPONENT_INDEX)->value.GetInt()));
				mRenderer->SetMesh(assetManager.GetMeshAtID(componentBlock[i].FindMember(MESH_COMPONENT_INDEX)->value.GetInt()));
				mRenderer->SetEnabled(componentBlock[i].FindMember(ENABLED)->value.GetBool());
			}
			else if (componentType == ComponentTypes::CAMERA) {
				std::shared_ptr<Camera> loadedCam = assetManager.CreateCameraOnEntity(newEnt, componentBlock[i].FindMember(CAMERA_ASPECT_RATIO)->value.GetDouble());
				loadedCam->SetIsPerspective(componentBlock[i].FindMember(CAMERA_PROJECTION_MATRIX_TYPE)->value.GetBool());
				loadedCam->SetNearDist(componentBlock[i].FindMember(CAMERA_NEAR_DISTANCE)->value.GetDouble());
				loadedCam->SetFarDist(componentBlock[i].FindMember(CAMERA_FAR_DISTANCE)->value.GetDouble());
				loadedCam->SetFOV(componentBlock[i].FindMember(CAMERA_FIELD_OF_VIEW)->value.GetDouble());
				if (componentBlock[i].FindMember(CAMERA_IS_MAIN)->value.GetBool())
					assetManager.SetMainCamera(loadedCam);
				loadedCam->SetEnabled(componentBlock[i].FindMember(ENABLED)->value.GetBool());
			}
			else if (componentType == ComponentTypes::NOCLIP_CHAR_CONTROLLER) {
				std::shared_ptr<NoclipMovement> ncMovement = newEnt->AddComponent<NoclipMovement>();
				ncMovement->moveSpeed = componentBlock[i].FindMember(NOCLIP_MOVE_SPEED)->value.GetDouble();
				ncMovement->lookSpeed = componentBlock[i].FindMember(NOCLIP_LOOK_SPEED)->value.GetDouble();
				ncMovement->SetEnabled(componentBlock[i].FindMember(ENABLED)->value.GetBool());
			}
			else if (componentType == ComponentTypes::FLASHLIGHT_CONTROLLER) {
				newEnt->AddComponent<FlashlightController>()->SetEnabled(componentBlock[i].FindMember(ENABLED)->value.GetBool());
			}
			else {
				// Unkown Component Type, do nothing
			}
		}
	}
}

/// <summary>
/// Saves the scene's assets
/// </summary>
/// <param name="sceneDocToSave">JSON document to save them into</param>
void SceneManager::SaveAssets(rapidjson::Document& sceneDocToSave)
{
	rapidjson::MemoryPoolAllocator<>& allocator = sceneDocToSave.GetAllocator();

	rapidjson::Value meshBlock(rapidjson::kArrayType);
	bool shouldBreak = false;
	for (auto me : assetManager.globalMeshes) {
		for (auto te : ComponentManager::GetAll<Terrain>()) {
			if (te->GetMesh() == me) shouldBreak = true;
		}
		if (shouldBreak) break;

		// Mesh
		rapidjson::Value meshValue(rapidjson::kObjectType);

		meshValue.AddMember(MESH_INDEX_COUNT, me->GetIndexCount(), allocator);
		meshValue.AddMember(MESH_MATERIAL_INDEX, me->GetMaterialIndex(), allocator);
		meshValue.AddMember(MESH_NEEDS_DEPTH_PREPASS, me->GetDepthPrePass(), allocator); 
		
		meshValue.AddMember(NAME, rapidjson::Value().SetString(me->GetName().c_str(), allocator), allocator);
		meshValue.AddMember(FILENAME_KEY, rapidjson::Value().SetString(me->GetFileNameKey().c_str(), allocator), allocator);

		meshBlock.PushBack(meshValue, allocator);
	}

	sceneDocToSave.AddMember(MESHES, meshBlock, allocator);

	rapidjson::Value textureBlock(rapidjson::kArrayType);
	for (auto tex : assetManager.globalTextures) {
		// Textures
		rapidjson::Value textureValue(rapidjson::kObjectType);

		textureValue.AddMember(NAME, rapidjson::Value().SetString(tex->GetName().c_str(), allocator), allocator);
		textureValue.AddMember(FILENAME_KEY, rapidjson::Value().SetString(tex->GetTextureFilenameKey().c_str(), allocator), allocator);
		textureValue.AddMember(TEXTURE_ASSET_PATH_INDEX, tex->GetAssetPathIndex(), allocator);

		textureBlock.PushBack(textureValue, allocator);
	}

	sceneDocToSave.AddMember(TEXTURES, textureBlock, allocator);

	rapidjson::Value materialBlock(rapidjson::kArrayType);
	for (auto mat : assetManager.globalMaterials) {
		// Material
		rapidjson::Value matValue(rapidjson::kObjectType);

		matValue.AddMember(MAT_UV_TILING, mat->GetTiling(), allocator);
		matValue.AddMember(MAT_IS_TRANSPARENT, mat->GetTransparent(), allocator);
		matValue.AddMember(MAT_IS_REFRACTIVE, mat->GetRefractive(), allocator);
		matValue.AddMember(MAT_INDEX_OF_REFRACTION, mat->GetIndexOfRefraction(), allocator);
		matValue.AddMember(MAT_REFRACTION_SCALE, mat->GetRefractionScale(), allocator);

		matValue.AddMember(MAT_PIXEL_SHADER, assetManager.GetPixelShaderIDByPointer(mat->GetPixShader()), allocator);
		matValue.AddMember(MAT_VERTEX_SHADER, assetManager.GetVertexShaderIDByPointer(mat->GetVertShader()), allocator);

		matValue.AddMember(NAME, rapidjson::Value().SetString(mat->GetName().c_str(), allocator), allocator);

		// Currently, refractivePixShader also covers transparency
		if (mat->GetRefractive() || mat->GetTransparent()) {
			matValue.AddMember(MAT_REFRACTION_PIXEL_SHADER, assetManager.GetPixelShaderIDByPointer(mat->GetRefractivePixelShader()), allocator);
		}

		SaveFloat4(matValue, MAT_COLOR_TINT, mat->GetTint(), sceneDocToSave);

		// Complex types - Sampler States
		// Store the index of the state being used.
		// States are stored in the scene file.
		// Index is determined by a pointer-match search
		for (int i = 0; i < assetManager.textureSampleStates.size(); i++) {
			if (mat->GetSamplerState() == assetManager.textureSampleStates[i]) {
				matValue.AddMember(MAT_TEXTURE_SAMPLER_STATE, i, allocator);
				break;
			}
		}

		for (int i = 0; i < assetManager.textureSampleStates.size(); i++) {
			if (mat->GetClampSamplerState() == assetManager.textureSampleStates[i]) {
				matValue.AddMember(MAT_CLAMP_SAMPLER_STATE, i, allocator);
				break;
			}
		}

		// Complex types - Textures
		// Store the index of the Texture being used.
		// Textures are stored in the scene file.
		// Index is determined by a pointer-match search

		for (int i = 0; i < assetManager.globalTextures.size(); i++) {
			if (mat->GetTexture() == assetManager.globalTextures[i]) {
				matValue.AddMember(MAT_TEXTURE_OR_ALBEDO_MAP, i, allocator);
				break;
			}
		}

		for (int i = 0; i < assetManager.globalTextures.size(); i++) {
			if (mat->GetNormalMap() == assetManager.globalTextures[i]) {
				matValue.AddMember(MAT_NORMAL_MAP, i, allocator);
				break;
			}
		}

		for (int i = 0; i < assetManager.globalTextures.size(); i++) {
			if (mat->GetRoughMap() == assetManager.globalTextures[i]) {
				matValue.AddMember(MAT_ROUGHNESS_MAP, i, allocator);
				break;
			}
		}

		for (int i = 0; i < assetManager.globalTextures.size(); i++) {
			if (mat->GetMetalMap() == assetManager.globalTextures[i]) {
				matValue.AddMember(MAT_METAL_MAP, i, allocator);
				break;
			}
		}

		// Add everything to the material
		materialBlock.PushBack(matValue, allocator);
	}

	sceneDocToSave.AddMember(MATERIALS, materialBlock, allocator);

	rapidjson::Value fontBlock(rapidjson::kArrayType);
	for (auto font : assetManager.globalFonts) {
		rapidjson::Value fontObject(rapidjson::kObjectType);

		fontObject.AddMember(FILENAME_KEY, rapidjson::Value().SetString(font->fileNameKey.c_str(), allocator), allocator);
		fontObject.AddMember(NAME, rapidjson::Value().SetString(font->name.c_str(), allocator), allocator);

		fontBlock.PushBack(fontObject, allocator);
	}

	sceneDocToSave.AddMember(FONTS, fontBlock, allocator);

	// Save all texture sample states
	rapidjson::Value texSampleStateBlock(rapidjson::kArrayType);
	for (auto tss : assetManager.textureSampleStates) {
		// Future optimization - If these are 0, don't store,
		// and in Load(), default all of these to 0 if not found
		rapidjson::Value sampleState(rapidjson::kObjectType);

		D3D11_SAMPLER_DESC texSamplerDesc;
		tss->GetDesc(&texSamplerDesc);

		// Read each value of the description and add it to the
		// JSON object
		sampleState.AddMember(SAMPLER_ADDRESS_U, texSamplerDesc.AddressU, allocator);
		sampleState.AddMember(SAMPLER_ADDRESS_V, texSamplerDesc.AddressV, allocator);
		sampleState.AddMember(SAMPLER_ADDRESS_W, texSamplerDesc.AddressW, allocator);
		sampleState.AddMember(SAMPLER_COMPARISON_FUNCTION, texSamplerDesc.ComparisonFunc, allocator);
		sampleState.AddMember(SAMPLER_FILTER, texSamplerDesc.Filter, allocator);
		sampleState.AddMember(SAMPLER_MAX_ANISOTROPY, texSamplerDesc.MaxAnisotropy, allocator);
		sampleState.AddMember(SAMPLER_MAX_LOD, texSamplerDesc.MaxLOD, allocator);
		sampleState.AddMember(SAMPLER_MIN_LOD, texSamplerDesc.MinLOD, allocator);
		sampleState.AddMember(SAMPLER_MIP_LOD_BIAS, texSamplerDesc.MipLODBias, allocator);

		SaveFloat4(sampleState, SAMPLER_BORDER_COLOR, DirectX::XMFLOAT4(texSamplerDesc.BorderColor), sceneDocToSave);

		// Add all that to array
		texSampleStateBlock.PushBack(sampleState, allocator);
	}

	sceneDocToSave.AddMember(TEXTURE_SAMPLE_STATES, texSampleStateBlock, allocator);

	// Save all shaders
	rapidjson::Value vertexShaderBlock(rapidjson::kArrayType);
	for (auto vs : assetManager.vertexShaders) {
		rapidjson::Value vsObject(rapidjson::kObjectType);

		vsObject.AddMember(NAME, rapidjson::Value().SetString(vs->GetName().c_str(), allocator), allocator);
		vsObject.AddMember(SHADER_FILE_PATH, rapidjson::Value().SetString(vs->GetFileNameKey().c_str(), allocator), allocator);

		vertexShaderBlock.PushBack(vsObject, allocator);
	}

	sceneDocToSave.AddMember(VERTEX_SHADERS, vertexShaderBlock, allocator);

	rapidjson::Value pixelShaderBlock(rapidjson::kArrayType);
	for (auto ps : assetManager.pixelShaders) {
		rapidjson::Value psObject(rapidjson::kObjectType);

		psObject.AddMember(NAME, rapidjson::Value().SetString(ps->GetName().c_str(), allocator), allocator);
		psObject.AddMember(SHADER_FILE_PATH, rapidjson::Value().SetString(ps->GetFileNameKey().c_str(), allocator), allocator);

		pixelShaderBlock.PushBack(psObject, allocator);
	}

	sceneDocToSave.AddMember(PIXEL_SHADERS, pixelShaderBlock, allocator);

	rapidjson::Value computeShaderBlock(rapidjson::kArrayType);
	for (auto cs : assetManager.computeShaders) {
		rapidjson::Value csObject(rapidjson::kObjectType);

		csObject.AddMember(NAME, rapidjson::Value().SetString(cs->GetName().c_str(), allocator), allocator);
		csObject.AddMember(SHADER_FILE_PATH, rapidjson::Value().SetString(cs->GetFileNameKey().c_str(), allocator), allocator);

		computeShaderBlock.PushBack(csObject, allocator);
	}

	sceneDocToSave.AddMember(COMPUTE_SHADERS, computeShaderBlock, allocator);

	rapidjson::Value skyBlock(rapidjson::kArrayType);
	for (auto sy : assetManager.skies) {
		rapidjson::Value skyObject(rapidjson::kObjectType);

		skyObject.AddMember(NAME, rapidjson::Value().SetString(sy->GetName().c_str(), allocator), allocator);
		skyObject.AddMember(SKY_FILENAME_KEY_TYPE, sy->GetFilenameKeyType(), allocator);
		skyObject.AddMember(FILENAME_KEY, rapidjson::Value().SetString(sy->GetFilenameKey().c_str(), allocator), allocator);
		skyObject.AddMember(SKY_FILENAME_EXTENSION, rapidjson::Value().SetString(sy->GetFileExtension().c_str(), allocator), allocator);

		skyBlock.PushBack(skyObject, allocator);
	}

	sceneDocToSave.AddMember(SKIES, skyBlock, allocator);

	rapidjson::Value soundBlock(rapidjson::kArrayType);
	for (int i = 0; i < assetManager.globalSounds.size(); i++) {
		rapidjson::Value soundObject(rapidjson::kObjectType);
		FMODUserData* uData;
		FMOD_MODE sMode;

		FMOD_RESULT uDataResult = assetManager.globalSounds[i]->getUserData((void**)&uData);

#if defined(DEBUG) || defined(_DEBUG)
		if (uDataResult != FMOD_OK) {
			printf("Failed to save sound with user data error!");
		}
#endif

		uDataResult = assetManager.globalSounds[i]->getMode(&sMode);

#if defined(DEBUG) || defined(_DEBUG)
		if (uDataResult != FMOD_OK) {
			printf("Failed to save sound with mode error!");
		}
#endif

		soundObject.AddMember(FILENAME_KEY, rapidjson::Value().SetString(uData->filenameKey->c_str(), allocator), allocator);
		soundObject.AddMember(NAME, rapidjson::Value().SetString(uData->name->c_str(), allocator), allocator);
		soundObject.AddMember(SOUND_FMOD_MODE, sMode, allocator);

		soundBlock.PushBack(soundObject, allocator);
	}

	sceneDocToSave.AddMember(SOUNDS, soundBlock, allocator);

	rapidjson::Value terrainMatBlock(rapidjson::kArrayType);
	for (auto tm : assetManager.globalTerrainMaterials) {
		rapidjson::Value terrainMatObj(rapidjson::kObjectType);

		terrainMatObj.AddMember(TERRAIN_MATERIAL_BLEND_MAP_ENABLED, tm->GetUsingBlendMap(), allocator);
		terrainMatObj.AddMember(NAME, rapidjson::Value().SetString(tm->GetName().c_str(), allocator), allocator);
		terrainMatObj.AddMember(TERRAIN_MATERIAL_BLEND_MAP_PATH, rapidjson::Value().SetString(tm->GetBlendMapFilenameKey().c_str(), allocator), allocator);

		// The internal materials are already tracked as regular materials,
		// So we just need an array of pointers to them
		rapidjson::Value terrainInternalMats(rapidjson::kArrayType);
		for (int i = 0; i < tm->GetMaterialCount(); i++) {
			// GUIDs aren't implemented yet, so store array indices for now
			int index;
			for (index = 0; index < assetManager.globalMaterials.size(); index++) {
				if (assetManager.globalMaterials[index] == tm->GetMaterialAtID(i)) break;
			}

			terrainInternalMats.PushBack(index, allocator);
		}
		terrainMatObj.AddMember(TERRAIN_MATERIAL_MATERIAL_ARRAY, terrainInternalMats, allocator);

		terrainMatBlock.PushBack(terrainMatObj, allocator);
	}

	sceneDocToSave.AddMember(TERRAIN_MATERIALS, terrainMatBlock, allocator);
}

/// <summary>
/// Saves the scene's entities
/// </summary>
/// <param name="sceneDocToSave">JSON document to save them into</param>
void SceneManager::SaveEntities(rapidjson::Document& sceneDocToSave)
{
	rapidjson::MemoryPoolAllocator<>& allocator = sceneDocToSave.GetAllocator();

	rapidjson::Value gameEntityBlock(rapidjson::kArrayType);
	for (auto ge : assetManager.globalEntities) {
		rapidjson::Value geValue(rapidjson::kObjectType);

		// First add entity-level types
		rapidjson::Value eName;
		eName.SetString(ge->GetName().c_str(), allocator);
		geValue.AddMember(NAME, eName, allocator);
		geValue.AddMember(ENABLED, ge->GetEnabled(), allocator);

		rapidjson::Value geComponents(rapidjson::kArrayType);
		rapidjson::Value coValue(rapidjson::kObjectType);
		for (auto co : ge->GetAllComponents()) {
			coValue.AddMember(ENABLED, co->IsLocallyEnabled(), allocator);

			// Is it a Light?
			if (std::shared_ptr<Light> light = std::dynamic_pointer_cast<Light>(co)) {
				coValue.AddMember(COMPONENT_TYPE, ComponentTypes::LIGHT, allocator);

				coValue.AddMember(LIGHT_TYPE, light->GetType(), allocator);
				coValue.AddMember(LIGHT_INTENSITY, light->GetIntensity(), allocator);
				coValue.AddMember(LIGHT_RANGE, light->GetRange(), allocator);
				coValue.AddMember(LIGHT_CASTS_SHADOWS, light->CastsShadows(), allocator);

				SaveFloat3(coValue, LIGHT_COLOR, light->GetColor(), sceneDocToSave);
			}

			// Is it a Collider?
			else if (std::shared_ptr<Collider> collider = std::dynamic_pointer_cast<Collider>(co)) {
				coValue.AddMember(COMPONENT_TYPE, ComponentTypes::COLLIDER, allocator);

				coValue.AddMember(COLLIDER_TYPE, collider->IsTrigger(), allocator);
				coValue.AddMember(COLLIDER_IS_VISIBLE, collider->IsVisible(), allocator);

				SaveFloat3(coValue, COLLIDER_POSITION_OFFSET, collider->GetPositionOffset(), sceneDocToSave);
				SaveFloat3(coValue, COLLIDER_ROTATION_OFFSET, collider->GetRotationOffset(), sceneDocToSave);
				SaveFloat3(coValue, COLLIDER_SCALE_OFFSET, collider->GetScale(), sceneDocToSave);
			}

			// Is it Terrain?
			else if (std::shared_ptr<Terrain> terrain = std::dynamic_pointer_cast<Terrain>(co)) {
				coValue.AddMember(COMPONENT_TYPE, ComponentTypes::TERRAIN, allocator);

				coValue.AddMember(FILENAME_KEY, rapidjson::Value().SetString(terrain->GetMesh()->GetFileNameKey().c_str(), allocator), allocator);

				int index;
				for (index = 0; index < assetManager.globalTerrainMaterials.size(); index++) {
					if (assetManager.globalTerrainMaterials[index] == terrain->GetMaterial()) break;
				}
				coValue.AddMember(TERRAIN_INDEX_OF_TERRAIN_MATERIAL, index, allocator);
			}

			// Is it a Particle System?
			else if (std::shared_ptr<ParticleSystem> ps = std::dynamic_pointer_cast<ParticleSystem>(co)) {
				coValue.AddMember(COMPONENT_TYPE, ComponentTypes::PARTICLE_SYSTEM, allocator);

				coValue.AddMember(PARTICLE_SYSTEM_MAX_PARTICLES, ps->GetMaxParticles(), allocator);
				coValue.AddMember(PARTICLE_SYSTEM_IS_MULTI_PARTICLE, ps->IsMultiParticle(), allocator);
				coValue.AddMember(PARTICLE_SYSTEM_ADDITIVE_BLEND, ps->GetBlendState(), allocator);
				coValue.AddMember(PARTICLE_SYSTEM_SCALE, ps->GetScale(), allocator);
				coValue.AddMember(PARTICLE_SYSTEM_SPEED, ps->GetSpeed(), allocator);
				coValue.AddMember(PARTICLE_SYSTEM_PARTICLES_PER_SECOND, ps->GetParticlesPerSecond(), allocator);
				coValue.AddMember(PARTICLE_SYSTEM_PARTICLE_LIFETIME, ps->GetParticleLifetime(), allocator);

				SaveFloat3(coValue, PARTICLE_SYSTEM_DESTINATION, ps->GetDestination(), sceneDocToSave);
				SaveFloat4(coValue, PARTICLE_SYSTEM_COLOR_TINT, ps->GetColorTint(), sceneDocToSave);

				coValue.AddMember(FILENAME_KEY, rapidjson::Value().SetString(ps->GetFilenameKey().c_str(), allocator), allocator);
			}

			// Is it a MeshRenderer?
			else if (std::shared_ptr<MeshRenderer> meshRenderer = std::dynamic_pointer_cast<MeshRenderer>(co)) {
				coValue.AddMember(COMPONENT_TYPE, ComponentTypes::MESH_RENDERER, allocator);

				// Mesh Renderers are really just storage for a Mesh
				// and a Material, so get the indices for those in the 
				// stored list

				rapidjson::Value meshIndex;
				for (int i = 0; i < assetManager.globalMeshes.size(); i++) {
					if (assetManager.globalMeshes[i] == meshRenderer->GetMesh()) {
						meshIndex.SetInt(i);
						break;
					}
				}
				coValue.AddMember(MESH_COMPONENT_INDEX, meshIndex, allocator);

				rapidjson::Value materialIndex;
				for (int i = 0; i < assetManager.globalMaterials.size(); i++) {
					if (assetManager.globalMaterials[i] == meshRenderer->GetMaterial()) {
						materialIndex.SetInt(i);
						break;
					}
				}
				coValue.AddMember(MATERIAL_COMPONENT_INDEX, materialIndex, allocator);
			}

			// Is it a Camera?
			else if (std::shared_ptr<Camera> camera = std::dynamic_pointer_cast<Camera>(co)) {
				coValue.AddMember(COMPONENT_TYPE, ComponentTypes::CAMERA, allocator);
				coValue.AddMember(CAMERA_ASPECT_RATIO, camera->GetAspectRatio(), allocator);
				coValue.AddMember(CAMERA_PROJECTION_MATRIX_TYPE, camera->IsPerspective(), allocator);
				coValue.AddMember(CAMERA_NEAR_DISTANCE, camera->GetNearDist(), allocator);
				coValue.AddMember(CAMERA_FAR_DISTANCE, camera->GetFarDist(), allocator);
				coValue.AddMember(CAMERA_FIELD_OF_VIEW, camera->GetFOV(), allocator);
				coValue.AddMember(CAMERA_IS_MAIN, camera == assetManager.mainCamera, allocator);
			}

			// Is it a Noclip Movement Controller?
			else if (std::shared_ptr<NoclipMovement> noclip = std::dynamic_pointer_cast<NoclipMovement>(co)) {
				coValue.AddMember(COMPONENT_TYPE, ComponentTypes::NOCLIP_CHAR_CONTROLLER, allocator);
				coValue.AddMember(NOCLIP_MOVE_SPEED, noclip->moveSpeed, allocator);
				coValue.AddMember(NOCLIP_LOOK_SPEED, noclip->lookSpeed, allocator);
			}

			// Is it a Flashlight Controller?
			else if (std::shared_ptr<FlashlightController> flashlight = std::dynamic_pointer_cast<FlashlightController>(co)) {
				coValue.AddMember(COMPONENT_TYPE, ComponentTypes::FLASHLIGHT_CONTROLLER, allocator);
			}

			geComponents.PushBack(coValue, allocator);
			coValue.SetObject();
		}

		// Transforms are treated and stored differently
		SaveFloat3(geValue, TRANSFORM_LOCAL_POSITION, ge->GetTransform()->GetLocalPosition(), sceneDocToSave);
		SaveFloat3(geValue, TRANSFORM_LOCAL_ROTATION, ge->GetTransform()->GetLocalPitchYawRoll(), sceneDocToSave);
		SaveFloat3(geValue, TRANSFORM_LOCAL_SCALE, ge->GetTransform()->GetLocalScale(), sceneDocToSave);

		// I have no idea how to serialize this
		// I'd need to essentially create a unique id system - GUIDs?
		rapidjson::Value parent;
		rapidjson::Value children;

		// Push all the components to the game entity
		geValue.AddMember(COMPONENTS, geComponents, allocator);

		// Push everything to the game entity array
		gameEntityBlock.PushBack(geValue, allocator);
	}

	// Add the game entity array to the doc
	sceneDocToSave.AddMember(ENTITIES, gameEntityBlock, allocator);
}

/// <summary>
/// Allows for tracking of the engine state
/// </summary>
/// <param name="engineState">Pointer to the engine state's storage</param>
void SceneManager::Initialize(EngineState* engineState, std::function<void()> progressListener)
{
	this->engineState = engineState;
	this->progressListener = progressListener;
}

/// <summary>
/// Gets the name of the scene that is currently loading
/// </summary>
std::string SceneManager::GetLoadingSceneName() {
	return loadingSceneName;
}

/// <summary>
/// Gets the name of the scene that is currently loaded
/// </summary>
std::string SceneManager::GetCurrentSceneName() {
	return currentSceneName;
}

/// <summary>
/// Loads a scene from a JSON file
/// </summary>
/// <param name="filepath">Path to the file</param>
/// <param name="progressListener">Function to call when progressing to each new object load</param>
void SceneManager::LoadScene(std::string filepath) {
	HRESULT hr = CoInitialize(NULL);

	*engineState = EngineState::LOAD_SCENE;

	try {
		rapidjson::Document sceneDoc;

		std::string namePath = assetManager.GetFullPathToAssetFile(AssetPathIndex::ASSET_SCENE_PATH, filepath);

		FILE* file;
		fopen_s(&file, namePath.c_str(), "rb");

		if (file == NULL) {
			return;
		}

		char readBuffer[FILE_BUFFER_SIZE];
		rapidjson::FileReadStream sceneFileStream(file, readBuffer, sizeof(readBuffer));

		sceneDoc.ParseStream(sceneFileStream);

		// Check if this is a valid SHOE scene
		assert(sceneDoc[VALID_SHOE_SCENE].GetBool());

		// Get the scene name for loading purposes
		loadingSceneName = sceneDoc[NAME].GetString();

		// Remove the current scene from memory
		assetManager.CleanAllVectors();

//#if defined(DEBUG) || defined(_DEBUG)
//		printf("Took %3.4f seconds for pre-initialization. \n", DXCore::GetTotalTime());
//#endif

		LoadAssets(sceneDoc, progressListener);
		LoadEntities(sceneDoc, progressListener);

		currentLoadCategory = "Post-Initialization";
		currentLoadName = "Renderer and Final Setup";
		if(progressListener) progressListener();

		fclose(file);

//#if defined(DEBUG) || defined(_DEBUG)
//		printf("Took %3.4f seconds for pre-initialization. \n", this->GetTotalTime());
//#endif

		currentSceneName = loadingSceneName;
		loadingSceneName = "";
		*engineState = EngineState::EDITING;
		if (progressListener) progressListener();
	}
	catch (...) {

	}
}

/// <summary>
/// Saves a scene to a JSON file
/// </summary>
/// <param name="filepath">Path to the file</param>
/// <param name="sceneName">Name to store the scene under</param>
void SceneManager::SaveScene(std::string filepath, std::string sceneName) {
	//Cannot save scene during play
	if (*engineState == EngineState::PLAY)
		return;

	try {
		char cbuf[4096];
		rapidjson::MemoryPoolAllocator<> allocator(cbuf, sizeof(cbuf));

		rapidjson::Document sceneDocToSave(&allocator, 256);

		sceneDocToSave.SetObject();

		// RapidJSON doesn't know about our file types directly, so they
		// have to be reconstructed and stored as individual values.
		sceneDocToSave.AddMember(VALID_SHOE_SCENE, true, allocator);
		sceneDocToSave.AddMember(NAME, rapidjson::Value().SetString(sceneName.c_str(), allocator), allocator);

		//
		// In all rapidjson saving and loading instances, defines are used to 
		// create shorthand strings to optimize memory while keeping the code readable.
		//
		SaveAssets(sceneDocToSave);
		SaveEntities(sceneDocToSave);

		// At the end of gathering data, write it all
		// to the appropriate file
		std::string namePath = assetManager.GetFullPathToAssetFile(AssetPathIndex::ASSET_SCENE_PATH, filepath);

		FILE* file;
		fopen_s(&file, namePath.c_str(), "w");

		char writeBuffer[FILE_BUFFER_SIZE];
		rapidjson::FileWriteStream sceneFileStream(file, writeBuffer, sizeof(writeBuffer));

		rapidjson::Writer<rapidjson::FileWriteStream> writer(sceneFileStream);
		sceneDocToSave.Accept(writer);

		fclose(file);
	}
	catch (...) {
#if defined(DEBUG) || defined(_DEBUG)
		printf("Failed to save scene with error:\n %s \n", std::current_exception());
#endif
	}
}

/// <summary>
/// UNIMPLEMENTED - this function allows the user to choose a filpath and scene name
/// to save a scene as.
/// </summary>
void SceneManager::SaveSceneAs() {

}

/// <summary>
/// Saves the state of the entities in the scene before moving to play state
/// </summary>
void SceneManager::PrePlaySave()
{
	if (*engineState != EngineState::EDITING)
		return;

	try {
		char cbuf[4096];
		rapidjson::MemoryPoolAllocator<> allocator(cbuf, sizeof(cbuf));

		rapidjson::Document sceneDocToSave(&allocator, 256);

		sceneDocToSave.SetObject();

		// RapidJSON doesn't know about our file types directly, so they
		// have to be reconstructed and stored as individual values.
		sceneDocToSave.AddMember(VALID_SHOE_SCENE, true, allocator);

		SaveEntities(sceneDocToSave);

		// At the end of gathering data, write it all
		// to the appropriate file
		std::string namePath = assetManager.GetFullPathToAssetFile(AssetPathIndex::ASSET_SCENE_PATH, ".temp_play_save.json");

		FILE* file;
		fopen_s(&file, namePath.c_str(), "w");

		char writeBuffer[FILE_BUFFER_SIZE];
		rapidjson::FileWriteStream sceneFileStream(file, writeBuffer, sizeof(writeBuffer));

		rapidjson::Writer<rapidjson::FileWriteStream> writer(sceneFileStream);
		sceneDocToSave.Accept(writer);

		fclose(file);

		*engineState = EngineState::PLAY;
	}
	catch (...) {
#if defined(DEBUG) || defined(_DEBUG)
		printf("Failed to save pre-play scene with error:\n %s \n", std::current_exception());
#endif
	}
}

/// <summary>
/// Loads the state of the scene how it was just before moving to play state
/// </summary>
void SceneManager::PostPlayLoad()
{
	if (*engineState != EngineState::PLAY)
		return;

//	try {
		*engineState = EngineState::UNLOAD_PLAY;

		rapidjson::Document sceneDoc;

		std::string namePath = assetManager.GetFullPathToAssetFile(AssetPathIndex::ASSET_SCENE_PATH, ".temp_play_save.json");

		FILE* file;
		fopen_s(&file, namePath.c_str(), "rb");

		if (file == NULL) {
			return;
		}

		char readBuffer[FILE_BUFFER_SIZE];
		rapidjson::FileReadStream sceneFileStream(file, readBuffer, sizeof(readBuffer));

		sceneDoc.ParseStream(sceneFileStream);

		// Check if this is a valid SHOE scene
		assert(sceneDoc[VALID_SHOE_SCENE].GetBool());

		assetManager.CleanAllEntities();

		LoadEntities(sceneDoc);

		fclose(file);

		*engineState = EngineState::EDITING;
//	}
//	catch (...) {

//	}
}
