#include "../Headers/SceneManager.h"
#include "../Headers/AssetManager.h"


bool SceneManager::GetSingleLoadComplete() {
	return this->singleLoadComplete;
}

std::string SceneManager::GetLoadingCategory() {
	return currentLoadCategory;
}

std::string SceneManager::GetLoadingObjectName() {
	return currentLoadName;
}

std::exception_ptr SceneManager::GetLoadingException() {
	return error;
}

void SceneManager::SetSingleLoadComplete(bool loadComplete) {
	this->singleLoadComplete = loadComplete;
}

void SceneManager::CaughtLoadError(std::exception_ptr error) {
	this->error = error;

	SetSingleLoadComplete(true);
	threadNotifier->notify_all();

	std::unique_lock<std::mutex> lock(*threadLock);
	threadNotifier->wait(lock, [&] {return singleLoadComplete == 0; });
}

void SceneManager::SetLoadingAndWait(std::string category, std::string object) {
	currentLoadCategory = category;
	currentLoadName = object;
	error = NULL;

	SetSingleLoadComplete(true);
	threadNotifier->notify_all();

	std::unique_lock<std::mutex> lock(*threadLock);
	threadNotifier->wait(lock, [&] {return singleLoadComplete == 0; });
}

DirectX::XMFLOAT2 SceneManager::LoadFloat2(const rapidjson::Value& jsonBlock, const char* memberName)
{
	const rapidjson::Value& vecBlock = jsonBlock.FindMember(memberName)->value;
	return DirectX::XMFLOAT2(vecBlock[0].GetDouble(),
		vecBlock[1].GetDouble());
}

DirectX::XMFLOAT3 SceneManager::LoadFloat3(const rapidjson::Value& jsonBlock, const char* memberName)
{
	const rapidjson::Value& vecBlock = jsonBlock.FindMember(memberName)->value;
	return DirectX::XMFLOAT3(vecBlock[0].GetDouble(), 
		vecBlock[1].GetDouble(), 
		vecBlock[2].GetDouble());
}

DirectX::XMFLOAT4 SceneManager::LoadFloat4(const rapidjson::Value& jsonBlock, const char* memberName)
{
	const rapidjson::Value& vecBlock = jsonBlock.FindMember(memberName)->value;
	return DirectX::XMFLOAT4(vecBlock[0].GetDouble(),
		vecBlock[1].GetDouble(),
		vecBlock[2].GetDouble(),
		vecBlock[3].GetDouble());
}

void SceneManager::SaveFloat2(rapidjson::Value jsonObject, const char* memberName, DirectX::XMFLOAT2 vec, rapidjson::Document sceneDoc)
{
	rapidjson::Value float2(rapidjson::kArrayType);
	float2.PushBack(vec.x, sceneDoc.GetAllocator());
	float2.PushBack(vec.y, sceneDoc.GetAllocator());

	jsonObject.AddMember(rapidjson::StringRef(memberName),
		float2,
		sceneDoc.GetAllocator());
}

void SceneManager::SaveFloat3(rapidjson::Value jsonObject, const char* memberName, DirectX::XMFLOAT3 vec, rapidjson::Document sceneDoc)
{
	rapidjson::Value float3(rapidjson::kArrayType);
	float3.PushBack(vec.x, sceneDoc.GetAllocator());
	float3.PushBack(vec.y, sceneDoc.GetAllocator());
	float3.PushBack(vec.z, sceneDoc.GetAllocator());

	jsonObject.AddMember(rapidjson::StringRef(memberName), 
		float3, 
		sceneDoc.GetAllocator());
}

void SceneManager::SaveFloat4(rapidjson::Value jsonObject, const char* memberName, DirectX::XMFLOAT4 vec, rapidjson::Document sceneDoc)
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

void SceneManager::Initialize(std::condition_variable* threadNotifier, std::mutex* threadLock)
{
	assetManager = AssetManager::GetInstance();
	this->threadNotifier = threadNotifier;
	this->threadLock = threadLock;
}

std::string SceneManager::GetLoadingSceneName() {
	return loadingSceneName;
}

std::string SceneManager::GetCurrentSceneName() {
	return currentSceneName;
}

void SceneManager::LoadScene(std::string filepath, std::condition_variable* threadNotifier, std::mutex* threadLock) {
	HRESULT hr = CoInitialize(NULL);

	this->threadNotifier = threadNotifier;
	this->threadLock = threadLock;

	try {
		rapidjson::Document sceneDoc;

		std::string namePath = GetFullPathToAssetFile(AssetPathIndex::ASSET_SCENE_PATH, filepath);

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
		loadingSceneName = sceneDoc[SCENE_NAME].GetString();

		// Remove the current scene from memory
		CleanAllVectors();

		// Load order:
		// Fonts
		// Texture Sample States
		// Shaders
		// Cameras
		// Materials
		// Meshes
		// Terrain Materials
		// Entities
		// Terrain Entities (entity component)
		// Emitters (entity component)
		// Lights (entity component)
		// Skies
		// Audio
		// IMGUI?

		// Fonts
		const rapidjson::Value& fontBlock = sceneDoc[FONTS];
		assert(fontBlock.IsArray());
		for (rapidjson::SizeType i = 0; i < fontBlock.Size(); i++) {
			std::string fileKey = DeSerializeFileName(fontBlock[i].FindMember(FONT_FILENAME_KEY)->value.GetString());
			CreateSHOEFont(fontBlock[i].FindMember(FONT_NAME)->value.GetString(), fileKey);
		}

		// Texture Sampler States
		const rapidjson::Value& sampleStateBlock = sceneDoc[TEXTURE_SAMPLE_STATES];
		assert(sampleStateBlock.IsArray());
		for (rapidjson::SizeType i = 0; i < sampleStateBlock.Size(); i++) {
			Microsoft::WRL::ComPtr<ID3D11SamplerState> loadedSampler;

			D3D11_SAMPLER_DESC loadDesc;
			loadDesc.AddressU = (D3D11_TEXTURE_ADDRESS_MODE)(sampleStateBlock[i].FindMember(SAMPLER_ADDRESS_U)->value.GetInt());
			loadDesc.AddressV = (D3D11_TEXTURE_ADDRESS_MODE)(sampleStateBlock[i].FindMember(SAMPLER_ADDRESS_V)->value.GetInt());
			loadDesc.AddressW = (D3D11_TEXTURE_ADDRESS_MODE)(sampleStateBlock[i].FindMember(SAMPLER_ADDRESS_W)->value.GetInt());
			loadDesc.Filter = (D3D11_FILTER)(sampleStateBlock[i].FindMember(SAMPLER_FILTER)->value.GetInt());
			loadDesc.MaxAnisotropy = (sampleStateBlock[i].FindMember(SAMPLER_MAX_ANISOTROPY)->value.GetInt());
			loadDesc.MinLOD = (sampleStateBlock[i].FindMember(SAMPLER_MIN_LOD)->value.GetDouble());
			loadDesc.MaxLOD = (sampleStateBlock[i].FindMember(SAMPLER_MAX_LOD)->value.GetDouble());
			loadDesc.MipLODBias = (sampleStateBlock[i].FindMember(SAMPLER_MIP_LOD_BIAS)->value.GetDouble());
			loadDesc.ComparisonFunc = (D3D11_COMPARISON_FUNC)(sampleStateBlock[i].FindMember(SAMPLER_COMPARISON_FUNCTION)->value.GetInt());

			const rapidjson::Value& borderColorBlock = sampleStateBlock[i].FindMember(SAMPLER_BORDER_COLOR)->value;
			for (int j = 0; j < 4; j++) {
				loadDesc.BorderColor[j] = borderColorBlock[j].GetDouble();
			}

			device->CreateSamplerState(&loadDesc, &loadedSampler);

			textureSampleStates.push_back(loadedSampler);
		}

		textureState = textureSampleStates[0];
		clampState = textureSampleStates[1];

		// Pixel Shaders
		const rapidjson::Value& pixelShaderBlock = sceneDoc[PIXEL_SHADERS];
		assert(pixelShaderBlock.IsArray());
		for (rapidjson::SizeType i = 0; i < pixelShaderBlock.Size(); i++) {
			std::string fileKey = DeSerializeFileName(pixelShaderBlock[i].FindMember(SHADER_FILE_PATH)->value.GetString());
			CreatePixelShader(pixelShaderBlock[i].FindMember(SHADER_NAME)->value.GetString(), fileKey);
		}

		// Vertex Shaders
		const rapidjson::Value& vertexShaderBlock = sceneDoc[VERTEX_SHADERS];
		assert(vertexShaderBlock.IsArray());
		for (rapidjson::SizeType i = 0; i < vertexShaderBlock.Size(); i++) {
			std::string fileKey = DeSerializeFileName(vertexShaderBlock[i].FindMember(SHADER_FILE_PATH)->value.GetString());
			CreateVertexShader(vertexShaderBlock[i].FindMember(SHADER_NAME)->value.GetString(), fileKey);
		}

		// Compute Shaders
		const rapidjson::Value& computeShaderBlock = sceneDoc[COMPUTE_SHADERS];
		assert(computeShaderBlock.IsArray());
		for (rapidjson::SizeType i = 0; i < computeShaderBlock.Size(); i++) {
			std::string fileKey = DeSerializeFileName(computeShaderBlock[i].FindMember(SHADER_FILE_PATH)->value.GetString());
			CreateComputeShader(computeShaderBlock[i].FindMember(SHADER_NAME)->value.GetString(), fileKey);
		}

		const rapidjson::Value& materialBlock = sceneDoc[MATERIALS];
		assert(materialBlock.IsArray());
		for (rapidjson::SizeType i = 0; i < materialBlock.Size(); i++) {
			std::string name = DeSerializeFileName(materialBlock[i].FindMember(MAT_NAME)->value.GetString());
			std::string albedo = DeSerializeFileName(materialBlock[i].FindMember(MAT_TEXTURE_OR_ALBEDO_MAP)->value.GetString());
			std::string normal = DeSerializeFileName(materialBlock[i].FindMember(MAT_NORMAL_MAP)->value.GetString());
			std::string metal = DeSerializeFileName(materialBlock[i].FindMember(MAT_METAL_MAP)->value.GetString());
			std::string rough = DeSerializeFileName(materialBlock[i].FindMember(MAT_ROUGHNESS_MAP)->value.GetString());

			std::shared_ptr<Material> newMaterial = CreatePBRMaterial(name, albedo, normal, metal, rough);

			newMaterial->SetTransparent(materialBlock[i].FindMember(MAT_IS_TRANSPARENT)->value.GetBool());

			newMaterial->SetRefractive(materialBlock[i].FindMember(MAT_IS_REFRACTIVE)->value.GetBool());

			newMaterial->SetTiling(materialBlock[i].FindMember(MAT_UV_TILING)->value.GetDouble());

			newMaterial->SetIndexOfRefraction(materialBlock[i].FindMember(MAT_INDEX_OF_REFRACTION)->value.GetDouble());

			newMaterial->SetRefractionScale(materialBlock[i].FindMember(MAT_REFRACTION_SCALE)->value.GetDouble());

			newMaterial->SetSamplerState(textureSampleStates[materialBlock[i].FindMember(MAT_TEXTURE_SAMPLER_STATE)->value.GetInt()]);

			newMaterial->SetClampSamplerState(textureSampleStates[materialBlock[i].FindMember(MAT_CLAMP_SAMPLER_STATE)->value.GetInt()]);

			newMaterial->SetVertexShader(vertexShaders[materialBlock[i].FindMember(MAT_VERTEX_SHADER)->value.GetInt()]);

			newMaterial->SetPixelShader(pixelShaders[materialBlock[i].FindMember(MAT_PIXEL_SHADER)->value.GetInt()]);

			if (newMaterial->GetRefractive() || newMaterial->GetTransparent()) {
				int index = materialBlock[i].FindMember(MAT_REFRACTION_PIXEL_SHADER)->value.GetInt();
				newMaterial->SetRefractivePixelShader(pixelShaders[index]);
			}

			DirectX::XMFLOAT4 tint;
			tint.x = materialBlock[i].FindMember(MAT_COLOR_TINT)->value[0].GetDouble();
			tint.y = materialBlock[i].FindMember(MAT_COLOR_TINT)->value[1].GetDouble();
			tint.z = materialBlock[i].FindMember(MAT_COLOR_TINT)->value[2].GetDouble();
			tint.w = materialBlock[i].FindMember(MAT_COLOR_TINT)->value[3].GetDouble();
			newMaterial->SetTint(tint);
		}

		const rapidjson::Value& meshBlock = sceneDoc[MESHES];
		for (rapidjson::SizeType i = 0; i < meshBlock.Size(); i++) {
			std::string mFilename = DeSerializeFileName(meshBlock[i].FindMember(MESH_FILENAME_KEY)->value.GetString());
			std::shared_ptr<Mesh> newMesh = CreateMesh(meshBlock[i].FindMember(MESH_NAME)->value.GetString(), mFilename);

			newMesh->SetDepthPrePass(meshBlock[i].FindMember(MESH_NEEDS_DEPTH_PREPASS)->value.GetBool());

			newMesh->SetMaterialIndex(meshBlock[i].FindMember(MESH_MATERIAL_INDEX)->value.GetInt());

			// This is currently generated automatically. Would need to change
			// if storing meshes built through code arrays becomes supported.
			// newMesh->SetIndexCount(meshBlock[i].FindMember(MESH_INDEX_COUNT)->value.GetInt());
		}

		const rapidjson::Value& terrainMaterialBlock = sceneDoc[TERRAIN_MATERIALS];
		assert(terrainMaterialBlock.IsArray());
		for (rapidjson::SizeType i = 0; i < terrainMaterialBlock.Size(); i++) {
			const rapidjson::Value& tMatInternalBlock = terrainMaterialBlock[i].FindMember(TERRAIN_MATERIAL_MATERIAL_ARRAY)->value;
			std::vector<std::shared_ptr<Material>> internalMaterials;

			std::string tMatName = terrainMaterialBlock[i].FindMember(TERRAIN_MATERIAL_NAME)->value.GetString();
			std::string tMatBlendMapPath = DeSerializeFileName(terrainMaterialBlock[i].FindMember(TERRAIN_MATERIAL_BLEND_MAP_PATH)->value.GetString());

			for (rapidjson::SizeType j = 0; j < tMatInternalBlock.Size(); j++) {
				// Material texture strings are being incorrectly serialized/loaded
				internalMaterials.push_back(GetMaterialAtID(tMatInternalBlock[j].GetInt()));
			}

			if (terrainMaterialBlock[i].FindMember(TERRAIN_MATERIAL_BLEND_MAP_ENABLED)->value.GetBool()) {
				std::shared_ptr<TerrainMaterial> newTMat = CreateTerrainMaterial(tMatName, internalMaterials, tMatBlendMapPath);
			}
			else {
				std::shared_ptr<TerrainMaterial> newTMat = CreateTerrainMaterial(tMatName, internalMaterials);
			}
		}

		const rapidjson::Value& entityBlock = sceneDoc[ENTITIES];
		assert(entityBlock.IsArray());
		for (rapidjson::SizeType i = 0; i < entityBlock.Size(); i++) {
			std::string name = entityBlock[i].FindMember(ENTITY_NAME)->value.GetString();

			std::shared_ptr<GameEntity> newEnt = CreateGameEntity(name);
			newEnt->SetEnabled(entityBlock[i].FindMember(ENTITY_ENABLED)->value.GetBool());
			newEnt->UpdateHierarchyIsEnabled(entityBlock[i].FindMember(ENTITY_HIERARCHY_ENABLED)->value.GetBool());

			const rapidjson::Value& componentBlock = entityBlock[i].FindMember(COMPONENTS)->value;
			assert(componentBlock.IsArray());
			for (rapidjson::SizeType i = 0; i < componentBlock.Size(); i++) {
				int componentType = componentBlock[i].FindMember(COMPONENT_TYPE)->value.GetInt();
				if (componentType == ComponentTypes::TRANSFORM) {
					std::shared_ptr<Transform> trans = newEnt->GetTransform();

					trans->SetPosition(LoadFloat3(componentBlock[i], TRANSFORM_LOCAL_POSITION));
					trans->SetRotation(LoadFloat3(componentBlock[i], TRANSFORM_LOCAL_ROTATION));
					trans->SetScale(LoadFloat3(componentBlock[i], TRANSFORM_LOCAL_SCALE));
				}
				else if (componentType == ComponentTypes::COLLIDER) {
					std::shared_ptr<Collider> collider = newEnt->AddComponent<Collider>();

					collider->SetEnabled(componentBlock[i].FindMember(COLLIDER_ENABLED)->value.GetBool());
					collider->SetVisible(componentBlock[i].FindMember(COLLIDER_IS_VISIBLE)->value.GetBool());
					collider->SetIsTrigger(componentBlock[i].FindMember(COLLIDER_TYPE)->value.GetBool());

					collider->SetPositionOffset(LoadFloat3(componentBlock[i], COLLIDER_POSITION_OFFSET));
					collider->SetRotationOffset(LoadFloat3(componentBlock[i], COLLIDER_ROTATION_OFFSET));
					collider->SetScale(LoadFloat3(componentBlock[i], COLLIDER_SCALE_OFFSET));
				}
				else if (componentType == ComponentTypes::TERRAIN) {
					std::string heightmapKey = DeSerializeFileName(componentBlock[i].FindMember(TERRAIN_HEIGHTMAP_FILENAME_KEY)->value.GetString());
					std::shared_ptr<TerrainMaterial> tMat = GetTerrainMaterialAtID(componentBlock[i].FindMember(TERRAIN_INDEX_OF_TERRAIN_MATERIAL)->value.GetInt());

					CreateTerrainOnEntity(newEnt, heightmapKey.c_str(), tMat);
				}
				else if (componentType == ComponentTypes::PARTICLE_SYSTEM) {
					std::string filename = DeSerializeFileName(componentBlock[i].FindMember(PARTICLE_SYSTEM_FILENAME_KEY)->value.GetString());
					int maxParticles = componentBlock[i].FindMember(PARTICLE_SYSTEM_MAX_PARTICLES)->value.GetInt();
					bool blendState = componentBlock[i].FindMember(PARTICLE_SYSTEM_ADDITIVE_BLEND)->value.GetBool();
					bool isMultiParticle = componentBlock[i].FindMember(PARTICLE_SYSTEM_IS_MULTI_PARTICLE)->value.GetBool();
					float particlesPerSecond = componentBlock[i].FindMember(PARTICLE_SYSTEM_PARTICLES_PER_SECOND)->value.GetDouble();
					float particleLifetime = componentBlock[i].FindMember(PARTICLE_SYSTEM_PARTICLE_LIFETIME)->value.GetDouble();

					std::shared_ptr<ParticleSystem> newParticles = CreateParticleEmitterOnEntity(newEnt, filename, maxParticles, particleLifetime, particlesPerSecond, isMultiParticle, blendState);

					newParticles->SetScale(componentBlock[i].FindMember(PARTICLE_SYSTEM_SCALE)->value.GetDouble());
					newParticles->SetSpeed(componentBlock[i].FindMember(PARTICLE_SYSTEM_SPEED)->value.GetDouble());
					newParticles->SetDestination(LoadFloat3(componentBlock[i], PARTICLE_SYSTEM_DESTINATION));
					bool enabled = componentBlock[i].FindMember(PARTICLE_SYSTEM_ENABLED)->value.GetBool();

					DirectX::XMFLOAT4 color;

					const rapidjson::Value& colorBlock = componentBlock[i].FindMember(PARTICLE_SYSTEM_COLOR_TINT)->value;

					color.x = colorBlock[0].GetDouble();
					color.y = colorBlock[1].GetDouble();
					color.z = colorBlock[2].GetDouble();
					color.w = colorBlock[3].GetDouble();

					newParticles->SetColorTint(color);

					// Particles are a bit of a mess, and need to be initially disabled for at least the first frame.
					newParticles->SetEnabled(false);
				}
				else if (componentType == ComponentTypes::LIGHT) {
					std::shared_ptr<Light> light;

					float type = componentBlock[i].FindMember(LIGHT_TYPE)->value.GetDouble();
					float intensity = componentBlock[i].FindMember(LIGHT_INTENSITY)->value.GetDouble();
					float range = componentBlock[i].FindMember(LIGHT_RANGE)->value.GetDouble();
					bool enabled = componentBlock[i].FindMember(LIGHT_ENABLED)->value.GetBool();
					DirectX::XMFLOAT3 color = LoadFloat3(componentBlock[i], LIGHT_COLOR);

					if (type == 0.0f) {
						light = CreateDirectionalLightOnEntity(newEnt, color, intensity);
					}
					else if (type == 1.0f) {
						light = CreatePointLightOnEntity(newEnt, range, color, intensity);
					}
					else if (type == 2.0f) {
						light = CreateSpotLightOnEntity(newEnt, range, color, intensity);
					}
					else {
						// Unrecognized light type, do nothing
					}
					light->SetCastsShadows(componentBlock[i].FindMember(LIGHT_CASTS_SHADOWS)->value.GetBool());
				}
				else if (componentType == ComponentTypes::MESH_RENDERER) {
					std::shared_ptr<MeshRenderer> mRenderer = newEnt->AddComponent<MeshRenderer>();
					mRenderer->SetMaterial(GetMaterialAtID(componentBlock[i].FindMember(MATERIAL_COMPONENT_INDEX)->value.GetInt()));
					mRenderer->SetMesh(GetMeshAtID(componentBlock[i].FindMember(MESH_COMPONENT_INDEX)->value.GetInt()));
				}
				else if (componentType == ComponentTypes::CAMERA) {
					std::shared_ptr<Camera> loadedCam = CreateCameraOnEntity(newEnt, componentBlock[i].FindMember(CAMERA_ASPECT_RATIO)->value.GetDouble());
					loadedCam->SetIsPerspective(componentBlock[i].FindMember(CAMERA_PROJECTION_MATRIX_TYPE)->value.GetBool());
					loadedCam->SetNearDist(componentBlock[i].FindMember(CAMERA_NEAR_DISTANCE)->value.GetDouble());
					loadedCam->SetFarDist(componentBlock[i].FindMember(CAMERA_FAR_DISTANCE)->value.GetDouble());
					loadedCam->SetFOV(componentBlock[i].FindMember(CAMERA_FIELD_OF_VIEW)->value.GetDouble());
					if (componentBlock[i].FindMember(CAMERA_IS_MAIN)->value.GetBool())
						mainCamera = loadedCam;
				}
				else if (componentType == ComponentTypes::NOCLIP_CHAR_CONTROLLER) {
					std::shared_ptr<NoclipMovement> ncMovement = newEnt->AddComponent<NoclipMovement>();
					ncMovement->moveSpeed = componentBlock[i].FindMember(NOCLIP_MOVE_SPEED)->value.GetDouble();
					ncMovement->lookSpeed = componentBlock[i].FindMember(NOCLIP_LOOK_SPEED)->value.GetDouble();
				}
				else if (componentType == ComponentTypes::FLASHLIGHT_CONTROLLER) {
					newEnt->AddComponent<FlashlightController>();
				}
				else {
					// Unkown Component Type, do nothing
				}
			}
		}

		const rapidjson::Value& skyBlock = sceneDoc[SKIES];
		assert(skyBlock.IsArray());
		for (rapidjson::SizeType i = 0; i < skyBlock.Size(); i++) {
			std::string name = skyBlock[i].FindMember(SKY_NAME)->value.GetString();
			std::string filename = DeSerializeFileName(skyBlock[i].FindMember(SKY_FILENAME_KEY)->value.GetString());
			bool keyType = skyBlock[i].FindMember(SKY_FILENAME_KEY_TYPE)->value.GetBool();

			if (keyType) {
				std::string filenameExtension = skyBlock[i].FindMember(SKY_FILENAME_EXTENSION)->value.GetString();
				CreateSky(filename, keyType, name, filenameExtension);
			}
			else {
				CreateSky(filename, keyType, name);
			}

			// Skies are such large objects that Flush is needed to prevent
			// the Device from timing out
			//context->Flush();
		}

		const rapidjson::Value& soundBlock = sceneDoc[SOUNDS];
		assert(soundBlock.IsArray());
		for (rapidjson::SizeType i = 0; i < soundBlock.Size(); i++) {
			std::string filename = DeSerializeFileName(soundBlock[i].FindMember(SOUND_FILENAME_KEY)->value.GetString());
			std::string name = soundBlock[i].FindMember(SOUND_NAME)->value.GetString();
			int mode = soundBlock[i].FindMember(SOUND_FMOD_MODE)->value.GetInt();

			FMOD::Sound* newSound = CreateSound(filename, mode, name);
		}

		SetLoadingAndWait("Post-Initialization", "Renderer and Final Setup");

		fclose(file);

		this->assetManagerLoadState = AMLoadState::NOT_LOADING;
		this->singleLoadComplete = true;
		threadNotifier->notify_all();
	}
	catch (...) {

	}
}

void SceneManager::SaveScene(std::string filepath, std::string sceneName) {
	try {
		char cbuf[4096];
		rapidjson::MemoryPoolAllocator<> allocator(cbuf, sizeof(cbuf));

		rapidjson::Document sceneDocToSave(&allocator, 256);

		sceneDocToSave.SetObject();

		// RapidJSON doesn't know about our file types directly, so they
		// have to be reconstructed and stored as individual values.
		rapidjson::Value validSceneCheck;
		validSceneCheck.SetBool(true);
		sceneDocToSave.AddMember(VALID_SHOE_SCENE, validSceneCheck, allocator);

		rapidjson::Value sceneNameValue;
		sceneNameValue.SetString("Test");
		sceneDocToSave.AddMember(SCENE_NAME, sceneNameValue, allocator);

		//
		// In all rapidjson saving and loading instances, defines are used to 
		// create shorthand strings to optimize memory while keeping the code readable.
		//

		rapidjson::Value meshBlock(rapidjson::kArrayType);
		bool shouldBreak = false; assetManager.
		for (auto me : this->globalMeshes) {
			for (auto te : ComponentManager::GetAll<Terrain>()) {
				if (te->GetMesh() == me) shouldBreak = true;
			}

			if (shouldBreak) break;

			// Mesh
			rapidjson::Value meshValue(rapidjson::kObjectType);

			// Simple types first
			meshValue.AddMember(MESH_INDEX_COUNT, me->GetIndexCount(), allocator);
			meshValue.AddMember(MESH_MATERIAL_INDEX, me->GetMaterialIndex(), allocator);
			//meshValue.AddMember(MESH_ENABLED, me->GetEnableDisable(), allocator);
			meshValue.AddMember(MESH_NEEDS_DEPTH_PREPASS, me->GetDepthPrePass(), allocator);

			// Strings
			rapidjson::Value meshName;
			meshName.SetString(me->GetName().c_str(), allocator);
			meshValue.AddMember(MESH_NAME, meshName, allocator);

			// Complex data - filename string key
			rapidjson::Value fileKeyValue;
			fileKeyValue.SetString(me->GetFileNameKey().c_str(), allocator);

			meshValue.AddMember(MESH_FILENAME_KEY, rapidjson::StringRef(me->GetFileNameKey().c_str()), allocator);

			meshBlock.PushBack(meshValue, allocator);
		}

		sceneDocToSave.AddMember(MESHES, meshBlock, allocator);

		rapidjson::Value materialBlock(rapidjson::kArrayType);
		for (auto mat : this->globalMaterials) {
			// Material
			rapidjson::Value matValue(rapidjson::kObjectType);

			// Simple types first
			matValue.AddMember(MAT_UV_TILING, mat->GetTiling(), allocator);
			matValue.AddMember(MAT_IS_TRANSPARENT, mat->GetTransparent(), allocator);
			matValue.AddMember(MAT_IS_REFRACTIVE, mat->GetRefractive(), allocator);
			matValue.AddMember(MAT_INDEX_OF_REFRACTION, mat->GetIndexOfRefraction(), allocator);
			matValue.AddMember(MAT_REFRACTION_SCALE, mat->GetRefractionScale(), allocator);

			// String types
			rapidjson::Value matName;
			rapidjson::Value pixShader;
			rapidjson::Value vertShader;
			rapidjson::Value refractivePixShader;
			rapidjson::Value albedoMap;
			rapidjson::Value normalMap;
			rapidjson::Value metalMap;
			rapidjson::Value roughnessMap;

			matName.SetString(mat->GetName().c_str(), allocator);
			pixShader.SetInt(GetPixelShaderIDByPointer(mat->GetPixShader()));
			vertShader.SetInt(GetVertexShaderIDByPointer(mat->GetVertShader()));

			albedoMap.SetString(mat->GetTextureFilenameKey(PBRTextureTypes::ALBEDO).c_str(), allocator);
			normalMap.SetString(mat->GetTextureFilenameKey(PBRTextureTypes::NORMAL).c_str(), allocator);
			metalMap.SetString(mat->GetTextureFilenameKey(PBRTextureTypes::METAL).c_str(), allocator);
			roughnessMap.SetString(mat->GetTextureFilenameKey(PBRTextureTypes::ROUGH).c_str(), allocator);

			matValue.AddMember(MAT_NAME, matName, allocator);
			matValue.AddMember(MAT_PIXEL_SHADER, pixShader, allocator);
			matValue.AddMember(MAT_VERTEX_SHADER, vertShader, allocator);

			matValue.AddMember(MAT_TEXTURE_OR_ALBEDO_MAP, albedoMap, allocator);
			matValue.AddMember(MAT_NORMAL_MAP, normalMap, allocator);
			matValue.AddMember(MAT_METAL_MAP, metalMap, allocator);
			matValue.AddMember(MAT_ROUGHNESS_MAP, roughnessMap, allocator);

			// Currently, refractivePixShader also covers transparency
			if (mat->GetRefractive() || mat->GetTransparent()) {
				refractivePixShader.SetInt(GetPixelShaderIDByPointer(mat->GetRefractivePixelShader()));
				matValue.AddMember(MAT_REFRACTION_PIXEL_SHADER, refractivePixShader, allocator);
			}

			// Array and Color types
			rapidjson::Value colorTintValue(rapidjson::kArrayType);

			colorTintValue.PushBack(mat->GetTint().x, allocator);
			colorTintValue.PushBack(mat->GetTint().y, allocator);
			colorTintValue.PushBack(mat->GetTint().z, allocator);
			colorTintValue.PushBack(mat->GetTint().w, allocator);

			matValue.AddMember(MAT_COLOR_TINT, colorTintValue, allocator);

			// Complex types - Sampler States
			// Store the index of the state being used.
			// States are stored in the scene file.
			// Index is determined by a pointer-match search
			for (int i = 0; i < textureSampleStates.size(); i++) {
				if (mat->GetSamplerState() == textureSampleStates[i]) {
					matValue.AddMember(MAT_TEXTURE_SAMPLER_STATE, i, allocator);
					break;
				}
			}

			for (int i = 0; i < textureSampleStates.size(); i++) {
				if (mat->GetClampSamplerState() == textureSampleStates[i]) {
					matValue.AddMember(MAT_CLAMP_SAMPLER_STATE, i, allocator);
					break;
				}
			}

			// Add everything to the material
			materialBlock.PushBack(matValue, allocator);
		}

		sceneDocToSave.AddMember(MATERIALS, materialBlock, allocator);

		rapidjson::Value gameEntityBlock(rapidjson::kArrayType);
		for (auto ge : this->globalEntities) {
			rapidjson::Value geValue(rapidjson::kObjectType);

			// First add entity-level types
			rapidjson::Value geName;
			geName.SetString(ge->GetName().c_str(), allocator);
			geValue.AddMember(ENTITY_NAME, geName, allocator);
			geValue.AddMember(ENTITY_ENABLED, ge->GetEnabled(), allocator);
			geValue.AddMember(ENTITY_HIERARCHY_ENABLED, ge->GetHierarchyIsEnabled(), allocator);

			rapidjson::Value geComponents(rapidjson::kArrayType);
			rapidjson::Value coValue(rapidjson::kObjectType);
			for (auto co : ge->GetAllComponents()) {
				// Is it a Light?
				if (std::shared_ptr<Light> light = std::dynamic_pointer_cast<Light>(co)) {
					coValue.AddMember(COMPONENT_TYPE, ComponentTypes::LIGHT, allocator);

					// Basic types
					coValue.AddMember(LIGHT_TYPE, light->GetType(), allocator);
					coValue.AddMember(LIGHT_INTENSITY, light->GetIntensity(), allocator);
					coValue.AddMember(LIGHT_ENABLED, light->IsEnabled(), allocator);
					coValue.AddMember(LIGHT_RANGE, light->GetRange(), allocator);
					coValue.AddMember(LIGHT_CASTS_SHADOWS, light->CastsShadows(), allocator);

					// Treat FLOATX as float array[x]
					rapidjson::Value color(rapidjson::kArrayType);
					DirectX::XMFLOAT3 lightColor = light->GetColor();
					color.PushBack(lightColor.x, allocator);
					color.PushBack(lightColor.y, allocator);
					color.PushBack(lightColor.z, allocator);

					coValue.AddMember(LIGHT_COLOR, color, allocator);

					// No need to store position, as it's pulled from ge's transform
					// Padding is always empty
				}

				// Is it a Collider?
				else if (std::shared_ptr<Collider> collider = std::dynamic_pointer_cast<Collider>(co)) {
					coValue.AddMember(COMPONENT_TYPE, ComponentTypes::COLLIDER, allocator);


					coValue.AddMember(COLLIDER_TYPE, collider->IsTrigger(), allocator);
					coValue.AddMember(COLLIDER_ENABLED, collider->IsEnabled(), allocator);
					coValue.AddMember(COLLIDER_IS_VISIBLE, collider->IsVisible(), allocator);

					// Treat FLOATX as float array[x]
					rapidjson::Value pos(rapidjson::kArrayType);
					rapidjson::Value rot(rapidjson::kArrayType);
					rapidjson::Value scale(rapidjson::kArrayType);

					pos.PushBack(collider->GetPositionOffset().x, allocator);
					pos.PushBack(collider->GetPositionOffset().y, allocator);
					pos.PushBack(collider->GetPositionOffset().z, allocator);

					scale.PushBack(collider->GetScale().x, allocator);
					scale.PushBack(collider->GetScale().y, allocator);
					scale.PushBack(collider->GetScale().z, allocator);

					rot.PushBack(collider->GetRotationOffset().x, allocator);
					rot.PushBack(collider->GetRotationOffset().y, allocator);
					rot.PushBack(collider->GetRotationOffset().z, allocator);

					coValue.AddMember(COLLIDER_POSITION_OFFSET, pos, allocator);
					coValue.AddMember(COLLIDER_SCALE_OFFSET, scale, allocator);
					coValue.AddMember(COLLIDER_ROTATION_OFFSET, rot, allocator);
				}

				// Is it Terrain?
				else if (std::shared_ptr<Terrain> terrain = std::dynamic_pointer_cast<Terrain>(co)) {
					coValue.AddMember(COMPONENT_TYPE, ComponentTypes::TERRAIN, allocator);

					rapidjson::Value hmKey;
					hmKey.SetString(terrain->GetMesh()->GetFileNameKey().c_str(), allocator);

					coValue.AddMember(TERRAIN_HEIGHTMAP_FILENAME_KEY, hmKey, allocator);

					int index;
					for (index = 0; index < globalTerrainMaterials.size(); index++) {
						if (globalTerrainMaterials[index] == terrain->GetMaterial()) break;
					}
					coValue.AddMember(TERRAIN_INDEX_OF_TERRAIN_MATERIAL, index, allocator);
				}

				// Is it a Particle System?
				else if (std::shared_ptr<ParticleSystem> ps = std::dynamic_pointer_cast<ParticleSystem>(co)) {
					coValue.AddMember(COMPONENT_TYPE, ComponentTypes::PARTICLE_SYSTEM, allocator);

					coValue.AddMember(PARTICLE_SYSTEM_MAX_PARTICLES, ps->GetMaxParticles(), allocator);
					coValue.AddMember(PARTICLE_SYSTEM_IS_MULTI_PARTICLE, ps->IsMultiParticle(), allocator);
					coValue.AddMember(PARTICLE_SYSTEM_ENABLED, ps->IsLocallyEnabled(), allocator);
					coValue.AddMember(PARTICLE_SYSTEM_ADDITIVE_BLEND, ps->GetBlendState(), allocator);
					coValue.AddMember(PARTICLE_SYSTEM_SCALE, ps->GetScale(), allocator);
					coValue.AddMember(PARTICLE_SYSTEM_SPEED, ps->GetSpeed(), allocator);
					coValue.AddMember(PARTICLE_SYSTEM_PARTICLES_PER_SECOND, ps->GetParticlesPerSecond(), allocator);
					coValue.AddMember(PARTICLE_SYSTEM_PARTICLE_LIFETIME, ps->GetParticleLifetime(), allocator);

					// Float arrays
					rapidjson::Value psDestination(rapidjson::kArrayType);
					psDestination.PushBack(ps->GetDestination().x, allocator);
					psDestination.PushBack(ps->GetDestination().y, allocator);
					psDestination.PushBack(ps->GetDestination().z, allocator);
					coValue.AddMember(PARTICLE_SYSTEM_DESTINATION, psDestination, allocator);

					rapidjson::Value psColorTint(rapidjson::kArrayType);
					psColorTint.PushBack(ps->GetColorTint().x, allocator);
					psColorTint.PushBack(ps->GetColorTint().y, allocator);
					psColorTint.PushBack(ps->GetColorTint().z, allocator);
					psColorTint.PushBack(ps->GetColorTint().w, allocator);
					coValue.AddMember(PARTICLE_SYSTEM_COLOR_TINT, psColorTint, allocator);

					// Strings
					rapidjson::Value psFileKey;
					psFileKey.SetString(ps->GetFilenameKey().c_str(), allocator);
					coValue.AddMember(PARTICLE_SYSTEM_FILENAME_KEY, psFileKey, allocator);
				}

				// Is it a MeshRenderer?
				else if (std::shared_ptr<MeshRenderer> meshRenderer = std::dynamic_pointer_cast<MeshRenderer>(co)) {
					coValue.AddMember(COMPONENT_TYPE, ComponentTypes::MESH_RENDERER, allocator);

					// Mesh Renderers are really just storage for a Mesh
					// and a Material, so get the indices for those in the 
					// stored list

					rapidjson::Value meshIndex;
					for (int i = 0; i < globalMeshes.size(); i++) {
						if (globalMeshes[i] == meshRenderer->GetMesh()) {
							meshIndex.SetInt(i);
							break;
						}
					}
					coValue.AddMember(MESH_COMPONENT_INDEX, meshIndex, allocator);

					rapidjson::Value materialIndex;
					for (int i = 0; i < globalMaterials.size(); i++) {
						if (globalMaterials[i] == meshRenderer->GetMaterial()) {
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
					coValue.AddMember(CAMERA_IS_MAIN, camera == mainCamera, allocator);
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
			coValue.AddMember(COMPONENT_TYPE, ComponentTypes::TRANSFORM, allocator);
			std::shared_ptr<Transform> transform = ge->GetTransform();

			// Treat FLOATX as float array[x]
			rapidjson::Value pos(rapidjson::kArrayType);
			rapidjson::Value rot(rapidjson::kArrayType);
			rapidjson::Value scale(rapidjson::kArrayType);

			// I have no idea how to serialize this
			// I'd need to essentially create a unique id system - GUIDs?
			rapidjson::Value parent;
			rapidjson::Value children;

			pos.PushBack(transform->GetLocalPosition().x, allocator);
			pos.PushBack(transform->GetLocalPosition().y, allocator);
			pos.PushBack(transform->GetLocalPosition().z, allocator);

			scale.PushBack(transform->GetLocalScale().x, allocator);
			scale.PushBack(transform->GetLocalScale().y, allocator);
			scale.PushBack(transform->GetLocalScale().z, allocator);

			rot.PushBack(transform->GetLocalPitchYawRoll().x, allocator);
			rot.PushBack(transform->GetLocalPitchYawRoll().y, allocator);
			rot.PushBack(transform->GetLocalPitchYawRoll().z, allocator);

			coValue.AddMember(TRANSFORM_LOCAL_POSITION, pos, allocator);
			coValue.AddMember(TRANSFORM_LOCAL_SCALE, scale, allocator);
			coValue.AddMember(TRANSFORM_LOCAL_ROTATION, rot, allocator);

			// Push transform to the components list
			geComponents.PushBack(coValue, allocator);

			// Push all the components to the game entity
			geValue.AddMember(COMPONENTS, geComponents, allocator);

			// Push everything to the game entity array
			gameEntityBlock.PushBack(geValue, allocator);
		}

		// Add the game entity array to the doc
		sceneDocToSave.AddMember(ENTITIES, gameEntityBlock, allocator);

		rapidjson::Value fontBlock(rapidjson::kArrayType);
		for (auto font : globalFonts) {
			rapidjson::Value fontObject(rapidjson::kObjectType);

			fontObject.AddMember(FONT_FILENAME_KEY, rapidjson::Value().SetString(font->fileNameKey.c_str(), allocator), allocator);
			fontObject.AddMember(FONT_NAME, rapidjson::Value().SetString(font->name.c_str(), allocator), allocator);

			fontBlock.PushBack(fontObject, allocator);
		}

		sceneDocToSave.AddMember(FONTS, fontBlock, allocator);

		// Save all texture sample states
		rapidjson::Value texSampleStateBlock(rapidjson::kArrayType);
		for (auto tss : textureSampleStates) {
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

			// Some of these are complex types
			rapidjson::Value borderColorValue(rapidjson::kArrayType);

			borderColorValue.PushBack(texSamplerDesc.BorderColor[0], allocator);
			borderColorValue.PushBack(texSamplerDesc.BorderColor[1], allocator);
			borderColorValue.PushBack(texSamplerDesc.BorderColor[2], allocator);
			borderColorValue.PushBack(texSamplerDesc.BorderColor[3], allocator);

			sampleState.AddMember(SAMPLER_BORDER_COLOR, borderColorValue, allocator);

			// Add all that to array
			texSampleStateBlock.PushBack(sampleState, allocator);
		}

		sceneDocToSave.AddMember(TEXTURE_SAMPLE_STATES, texSampleStateBlock, allocator);

		// Save all shaders
		rapidjson::Value vertexShaderBlock(rapidjson::kArrayType);
		for (auto vs : vertexShaders) {
			rapidjson::Value vsObject(rapidjson::kObjectType);

			rapidjson::Value vsName;
			rapidjson::Value vsKey;

			vsName.SetString(vs->GetName().c_str(), allocator);
			vsKey.SetString(vs->GetFileNameKey().c_str(), allocator);

			vsObject.AddMember(SHADER_NAME, vsName, allocator);
			vsObject.AddMember(SHADER_FILE_PATH, vsKey, allocator);

			vertexShaderBlock.PushBack(vsObject, allocator);
		}

		sceneDocToSave.AddMember(VERTEX_SHADERS, vertexShaderBlock, allocator);

		rapidjson::Value pixelShaderBlock(rapidjson::kArrayType);
		for (auto ps : pixelShaders) {
			rapidjson::Value psObject(rapidjson::kObjectType);

			rapidjson::Value psName;
			rapidjson::Value psKey;

			psName.SetString(ps->GetName().c_str(), allocator);
			psKey.SetString(ps->GetFileNameKey().c_str(), allocator);

			psObject.AddMember(SHADER_NAME, psName, allocator);
			psObject.AddMember(SHADER_FILE_PATH, psKey, allocator);

			pixelShaderBlock.PushBack(psObject, allocator);
		}

		sceneDocToSave.AddMember(PIXEL_SHADERS, pixelShaderBlock, allocator);

		rapidjson::Value computeShaderBlock(rapidjson::kArrayType);
		for (auto cs : computeShaders) {
			rapidjson::Value csObject(rapidjson::kObjectType);

			rapidjson::Value csName;
			rapidjson::Value csKey;

			csName.SetString(cs->GetName().c_str(), allocator);
			csKey.SetString(cs->GetFileNameKey().c_str(), allocator);

			csObject.AddMember(SHADER_NAME, csName, allocator);
			csObject.AddMember(SHADER_FILE_PATH, csKey, allocator);

			computeShaderBlock.PushBack(csObject, allocator);
		}

		sceneDocToSave.AddMember(COMPUTE_SHADERS, computeShaderBlock, allocator);

		rapidjson::Value skyBlock(rapidjson::kArrayType);
		for (auto sy : skies) {
			rapidjson::Value skyObject(rapidjson::kObjectType);
			rapidjson::Value skyName;
			rapidjson::Value skyFilenameKey;
			rapidjson::Value skyFilenameExtension;
			skyName.SetString(sy->GetName().c_str(), allocator);
			skyFilenameKey.SetString(sy->GetFilenameKey().c_str(), allocator);
			skyFilenameExtension.SetString(sy->GetFileExtension().c_str(), allocator);

			skyObject.AddMember(SKY_NAME, skyName, allocator);
			skyObject.AddMember(SKY_FILENAME_KEY_TYPE, sy->GetFilenameKeyType(), allocator);
			skyObject.AddMember(SKY_FILENAME_KEY, skyFilenameKey, allocator);
			skyObject.AddMember(SKY_FILENAME_EXTENSION, skyFilenameExtension, allocator);

			skyBlock.PushBack(skyObject, allocator);
		}

		sceneDocToSave.AddMember(SKIES, skyBlock, allocator);

		rapidjson::Value soundBlock(rapidjson::kArrayType);
		for (int i = 0; i < globalSounds.size(); i++) {
			rapidjson::Value soundObject(rapidjson::kObjectType);
			FMODUserData* uData;
			FMOD_MODE sMode;

			FMOD_RESULT uDataResult = globalSounds[i]->getUserData((void**)&uData);

#if defined(DEBUG) || defined(_DEBUG)
			if (uDataResult != FMOD_OK) {
				printf("Failed to save sound with user data error!");
			}
#endif

			uDataResult = globalSounds[i]->getMode(&sMode);

#if defined(DEBUG) || defined(_DEBUG)
			if (uDataResult != FMOD_OK) {
				printf("Failed to save sound with mode error!");
			}
#endif

			rapidjson::Value soundFK;
			rapidjson::Value soundN;

			soundFK.SetString(uData->filenameKey->c_str(), allocator);
			soundN.SetString(uData->name->c_str(), allocator);

			soundObject.AddMember(SOUND_FILENAME_KEY, soundFK, allocator);
			soundObject.AddMember(SOUND_NAME, soundN, allocator);
			soundObject.AddMember(SOUND_FMOD_MODE, sMode, allocator);

			soundBlock.PushBack(soundObject, allocator);
		}

		sceneDocToSave.AddMember(SOUNDS, soundBlock, allocator);

		rapidjson::Value terrainMatBlock(rapidjson::kArrayType);
		for (auto tm : globalTerrainMaterials) {
			rapidjson::Value terrainMatObj(rapidjson::kObjectType);
			rapidjson::Value tmName;
			rapidjson::Value tmBlendPath;

			tmName.SetString(tm->GetName().c_str(), allocator);
			tmBlendPath.SetString(tm->GetBlendMapFilenameKey().c_str(), allocator);

			terrainMatObj.AddMember(TERRAIN_MATERIAL_BLEND_MAP_ENABLED, tm->GetUsingBlendMap(), allocator);
			terrainMatObj.AddMember(TERRAIN_MATERIAL_NAME, tmName, allocator);
			terrainMatObj.AddMember(TERRAIN_MATERIAL_BLEND_MAP_PATH, tmBlendPath, allocator);

			// The internal materials are already tracked as regular materials,
			// So we just need an array of pointers to them
			rapidjson::Value terrainInternalMats(rapidjson::kArrayType);
			for (int i = 0; i < tm->GetMaterialCount(); i++) {
				// GUIDs aren't implemented yet, so store array indices for now
				int index;
				for (index = 0; index < globalMaterials.size(); index++) {
					if (globalMaterials[index] == tm->GetMaterialAtID(i)) break;
				}

				terrainInternalMats.PushBack(index, allocator);
			}
			terrainMatObj.AddMember(TERRAIN_MATERIAL_MATERIAL_ARRAY, terrainInternalMats, allocator);

			terrainMatBlock.PushBack(terrainMatObj, allocator);
		}

		sceneDocToSave.AddMember(TERRAIN_MATERIALS, terrainMatBlock, allocator);

		// At the end of gathering data, write it all
		// to the appropriate file
		std::string assetPath;

		std::string namePath = GetFullPathToAssetFile(AssetPathIndex::ASSET_SCENE_PATH, filepath);

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