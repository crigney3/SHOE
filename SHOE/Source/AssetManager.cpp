#include "../Headers/AssetManager.h"

using namespace DirectX;

#pragma region assetClassHandlers
AssetManager* AssetManager::instance;

AssetManager::~AssetManager() {
	// Everything should be smart-pointer managed
	// Except sounds, which have to have UserData cleared
	// manually (and can't use auto, iterator isn't built)
	for (int i = 0; i < globalSounds.size(); i++) {
		FMODUserData* uData;
		globalSounds[i]->getUserData((void**)&uData);
		uData->filenameKey.reset();
		uData->name.reset();
		delete uData;
	}

	globalSounds.clear();
	globalMeshes.clear();

	// And components
	for (auto ge : globalEntities) {
		ge->Release();
	}
}

void AssetManager::Initialize(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::condition_variable* threadNotifier, std::mutex* threadLock, HWND hwnd) {
	HRESULT hr = CoInitialize(NULL);

	dxInstance = DXCore::DXCoreInstance;
	this->context = context;
	this->device = device;

	this->threadNotifier = threadNotifier;
	this->threadLock = threadLock;

	this->assetManagerLoadState = AMLoadState::INITIALIZING;

	textureSampleStates = std::vector<Microsoft::WRL::ComPtr<ID3D11SamplerState>>();

	// This must occur before the loading screen starts
	InitializeFonts();

	InitializeTextureSampleStates();

	// The rest signal the loading screen each time an object loads
	InitializeShaders();
	InitializeCameras();
	InitializeMaterials();
	InitializeMeshes();
	InitializeGameEntities();
	InitializeTerrainMaterials();
	InitializeTerrainEntities();
	InitializeLights();
	InitializeColliders();
	InitializeEmitters();
	InitializeSkies();
	InitializeAudio();
	InitializeIMGUI(hwnd);

	// Initialize the input manager with the window's handle
	Input::GetInstance().Initialize(hwnd);

	SetLoadedAndWait("Post-Initialization", "Preparing to render");

	this->assetManagerLoadState = AMLoadState::NOT_LOADING;
	this->singleLoadComplete = true;
	threadNotifier->notify_all();
}

bool AssetManager::GetSingleLoadComplete() {
	return this->singleLoadComplete;
}

AMLoadState AssetManager::GetAMLoadState() {
	return this->assetManagerLoadState;
}

std::string AssetManager::GetLastLoadedCategory() {
	return this->loaded.category;
}

std::string AssetManager::GetLastLoadedObject() {
	return this->loaded.object;
}

std::exception_ptr AssetManager::GetLoadingException() {
	return this->loaded.errorCode;
}

void AssetManager::SetAMLoadState(AMLoadState state) {
	this->assetManagerLoadState = state;
	//dxInstance->SetDXOperatingState(state);
}

void AssetManager::SetSingleLoadComplete(bool loadComplete) {
	this->singleLoadComplete = loadComplete;
}

void AssetManager::SetLoadedAndWait(std::string category, std::string object, std::exception_ptr error) {
	if (assetManagerLoadState == AMLoadState::INITIALIZING) {
		loaded.category = category;
		loaded.object = object;
		if (error) {
			loaded.errorCode = error;
		}
		SetSingleLoadComplete(true);
		threadNotifier->notify_all();

		std::unique_lock<std::mutex> lock(*threadLock);
		threadNotifier->wait(lock, [&] {return singleLoadComplete == 0; });
	}
	else if (assetManagerLoadState == AMLoadState::COMPLEX_CREATION) {
		// TODO: handle complex asset importing by file, and do thread logic here
		return;
	}
	else if (assetManagerLoadState == AMLoadState::SCENE_LOAD) {
		// TODO: handle running a loading screen while a new scene loads
		loaded.category = category;
		loaded.object = object;
		if (error) {
			loaded.errorCode = error;
		}
		SetSingleLoadComplete(true);
		threadNotifier->notify_all();

		std::unique_lock<std::mutex> lock(*threadLock);
		threadNotifier->wait(lock, [&] {return singleLoadComplete == 0; });
		return;
	}
	else {
		// This a new asset being loaded without a loading screen.
		// Or this got called while the asset manager wasn't aware
		// of the loading.
		return;
	}
}

void AssetManager::SetLoadingAndWait(std::string category, std::string object) {
	if (assetManagerLoadState == AMLoadState::INITIALIZING) {
		loaded.category = category;
		loaded.object = object;

		SetSingleLoadComplete(true);
		threadNotifier->notify_all();

		std::unique_lock<std::mutex> lock(*threadLock);
		threadNotifier->wait(lock, [&] {return singleLoadComplete == 0; });
	}
	else if (assetManagerLoadState == AMLoadState::COMPLEX_CREATION) {
		// TODO: handle complex asset importing by file, and do thread logic here
		return;
	}
	else if (assetManagerLoadState == AMLoadState::SCENE_LOAD) {
		// TODO: handle running a loading screen while a new scene loads
		loaded.category = category;
		loaded.object = object;

		SetSingleLoadComplete(true);
		threadNotifier->notify_all();

		std::unique_lock<std::mutex> lock(*threadLock);
		threadNotifier->wait(lock, [&] {return singleLoadComplete == 0; });
		return;
	}
	else {
		// This a new asset being loaded without a loading screen.
		// Or this got called while the asset manager wasn't aware
		// of the loading.
		return;
	}
}

std::string AssetManager::GetLoadingSceneName() {
	return loadingSceneName;
}

std::string AssetManager::GetCurrentSceneName() {
	return currentSceneName;
}

void AssetManager::LoadScene(std::string filepath, std::condition_variable* threadNotifier, std::mutex* threadLock) {
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

		// Cameras
		const rapidjson::Value& cameraBlock = sceneDoc[CAMERAS];
		assert(cameraBlock.IsArray());
		for (rapidjson::SizeType i = 0; i < cameraBlock.Size(); i++) {
			DirectX::XMFLOAT3 cameraPos;
			DirectX::XMFLOAT3 cameraRot;
			DirectX::XMFLOAT3 cameraScale;

			const rapidjson::Value& cameraTransformBlock = cameraBlock[i].FindMember(CAMERA_TRANSFORM)->value;
			const rapidjson::Value& transformPosBlock = cameraTransformBlock.FindMember(TRANSFORM_LOCAL_POSITION)->value;
			const rapidjson::Value& transformRotBlock = cameraTransformBlock.FindMember(TRANSFORM_LOCAL_ROTATION)->value;
			const rapidjson::Value& transformScaleBlock = cameraTransformBlock.FindMember(TRANSFORM_LOCAL_SCALE)->value;

			cameraPos.x = transformPosBlock[0].GetDouble();
			cameraPos.y = transformPosBlock[1].GetDouble();
			cameraPos.z = transformPosBlock[2].GetDouble();

			cameraRot.x = transformRotBlock[0].GetDouble();
			cameraRot.y = transformRotBlock[1].GetDouble();
			cameraRot.z = transformRotBlock[2].GetDouble();

			cameraScale.x = transformScaleBlock[0].GetDouble();
			cameraScale.y = transformScaleBlock[1].GetDouble();
			cameraScale.z = transformScaleBlock[2].GetDouble();

			std::shared_ptr<Camera> loadedCam = CreateCamera(cameraBlock[i].FindMember(CAMERA_NAME)->value.GetString(),
															 cameraPos,
															 cameraBlock[i].FindMember(CAMERA_ASPECT_RATIO)->value.GetDouble(),
															 (int)cameraBlock[i].FindMember(CAMERA_PROJECTION_MATRIX_TYPE)->value.GetBool());

			loadedCam->GetTransform()->SetRotation(cameraRot);

			loadedCam->GetTransform()->SetScale(cameraScale);

			loadedCam->SetTag((CameraType)cameraBlock[i].FindMember(CAMERA_TAG)->value.GetInt());

			loadedCam->SetLookSpeed(cameraBlock[i].FindMember(CAMERA_LOOK_SPEED)->value.GetDouble());

			loadedCam->SetMoveSpeed(cameraBlock[i].FindMember(CAMERA_MOVE_SPEED)->value.GetDouble());

			loadedCam->SetNearDist(cameraBlock[i].FindMember(CAMERA_NEAR_DISTANCE)->value.GetDouble());

			loadedCam->SetFarDist(cameraBlock[i].FindMember(CAMERA_FAR_DISTANCE)->value.GetDouble());

			loadedCam->SetFOV(cameraBlock[i].FindMember(CAMERA_FIELD_OF_VIEW)->value.GetDouble());

			loadedCam->SetEnableDisable(cameraBlock[i].FindMember(CAMERA_ENABLED)->value.GetBool());
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
			newEnt->SetEnableDisable(entityBlock[i].FindMember(ENTITY_ENABLED)->value.GetBool());
			newEnt->UpdateHierarchyIsEnabled(entityBlock[i].FindMember(ENTITY_HIERARCHY_ENABLED)->value.GetBool());

			const rapidjson::Value& componentBlock = entityBlock[i].FindMember(COMPONENTS)->value;
			assert(componentBlock.IsArray());
			for (rapidjson::SizeType i = 0; i < componentBlock.Size(); i++) {
				int componentType = componentBlock[i].FindMember(COMPONENT_TYPE)->value.GetInt();
				if (componentType == ComponentTypes::TRANSFORM) {
					std::shared_ptr<Transform> trans = newEnt->GetTransform();
					DirectX::XMFLOAT3 pos;
					DirectX::XMFLOAT3 rot;
					DirectX::XMFLOAT3 scale;

					const rapidjson::Value& posBlock = componentBlock[i].FindMember(TRANSFORM_LOCAL_POSITION)->value;
					const rapidjson::Value& rotBlock = componentBlock[i].FindMember(TRANSFORM_LOCAL_ROTATION)->value;
					const rapidjson::Value& scaleBlock = componentBlock[i].FindMember(TRANSFORM_LOCAL_SCALE)->value;

					pos.x = posBlock[0].GetDouble();
					pos.y = posBlock[1].GetDouble();
					pos.z = posBlock[2].GetDouble();

					rot.x = rotBlock[0].GetDouble();
					rot.y = rotBlock[1].GetDouble();
					rot.z = rotBlock[2].GetDouble();

					scale.x = scaleBlock[0].GetDouble();
					scale.y = scaleBlock[1].GetDouble();
					scale.z = scaleBlock[2].GetDouble();

					trans->SetPosition(pos);
					trans->SetRotation(rot);
					trans->SetScale(scale);
				}
				else if (componentType == ComponentTypes::COLLIDER) {
					std::shared_ptr<Collider> collider = newEnt->AddComponent<Collider>();

					collider->SetEnabled(componentBlock[i].FindMember(COLLIDER_ENABLED)->value.GetBool());
					collider->SetVisible(componentBlock[i].FindMember(COLLIDER_IS_VISIBLE)->value.GetBool());
					collider->SetIsTrigger(componentBlock[i].FindMember(COLLIDER_TYPE)->value.GetBool());

					DirectX::XMFLOAT3 pos;
					DirectX::XMFLOAT3 rot;
					DirectX::XMFLOAT3 scale;

					const rapidjson::Value& posBlock = componentBlock[i].FindMember(COLLIDER_POSITION_OFFSET)->value;
					const rapidjson::Value& rotBlock = componentBlock[i].FindMember(COLLIDER_ROTATION_OFFSET)->value;
					const rapidjson::Value& scaleBlock = componentBlock[i].FindMember(COLLIDER_SCALE_OFFSET)->value;

					pos.x = posBlock[0].GetDouble();
					pos.y = posBlock[1].GetDouble();
					pos.z = posBlock[2].GetDouble();

					rot.x = rotBlock[0].GetDouble();
					rot.y = rotBlock[1].GetDouble();
					rot.z = rotBlock[2].GetDouble();

					scale.x = scaleBlock[0].GetDouble();
					scale.y = scaleBlock[1].GetDouble();
					scale.z = scaleBlock[2].GetDouble();

					collider->SetPositionOffset(pos);
					collider->SetRotationOffset(rot);
					collider->SetScale(pos);
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

					DirectX::XMFLOAT3 destination;
					DirectX::XMFLOAT4 color;
					bool enabled = componentBlock[i].FindMember(PARTICLE_SYSTEM_ENABLED)->value.GetBool();

					const rapidjson::Value& destBlock = componentBlock[i].FindMember(PARTICLE_SYSTEM_DESTINATION)->value;
					const rapidjson::Value& colorBlock = componentBlock[i].FindMember(PARTICLE_SYSTEM_COLOR_TINT)->value;

					destination.x = destBlock[0].GetDouble();
					destination.y = destBlock[1].GetDouble();
					destination.z = destBlock[2].GetDouble();

					color.x = colorBlock[0].GetDouble();
					color.y = colorBlock[1].GetDouble();
					color.z = colorBlock[2].GetDouble();
					color.w = colorBlock[3].GetDouble();

					newParticles->SetColorTint(color);
					newParticles->SetDestination(destination);

					// Particles are a bit of a mess, and need to be initially disabled for at least the first frame.
					newParticles->SetEnabled(false);
				}
				else if (componentType == ComponentTypes::LIGHT) {
					std::shared_ptr<Light> light;

					DirectX::XMFLOAT3 direction;
					DirectX::XMFLOAT3 color;
					float type = componentBlock[i].FindMember(LIGHT_TYPE)->value.GetDouble();
					float intensity = componentBlock[i].FindMember(LIGHT_INTENSITY)->value.GetDouble();
					float range = componentBlock[i].FindMember(LIGHT_RANGE)->value.GetDouble();
					bool enabled = componentBlock[i].FindMember(LIGHT_ENABLED)->value.GetBool();

					const rapidjson::Value& dirBlock = componentBlock[i].FindMember(LIGHT_DIRECTION)->value;
					const rapidjson::Value& colorBlock = componentBlock[i].FindMember(LIGHT_COLOR)->value;

					direction.x = dirBlock[0].GetDouble();
					direction.y = dirBlock[1].GetDouble();
					direction.z = dirBlock[2].GetDouble();

					color.x = colorBlock[0].GetDouble();
					color.y = colorBlock[1].GetDouble();
					color.z = colorBlock[2].GetDouble();

					if (type == 0.0f) {
						light = CreateDirectionalLightOnEntity(newEnt, direction, color, intensity);
					}
					else if (type == 1.0f) {
						light = CreatePointLightOnEntity(newEnt, range, color, intensity);
					}
					else if (type == 2.0f) {
						light = CreateSpotLightOnEntity(newEnt, direction, range, color, intensity);
					}
					else {
						// Unrecognized light type, do nothing
					}
				}
				else if (componentType == ComponentTypes::MESH_RENDERER) {
					std::shared_ptr<MeshRenderer> mRenderer = newEnt->AddComponent<MeshRenderer>();
					mRenderer->SetMaterial(GetMaterialAtID(componentBlock[i].FindMember(MATERIAL_COMPONENT_INDEX)->value.GetInt()));
					mRenderer->SetMesh(GetMeshAtID(componentBlock[i].FindMember(MESH_COMPONENT_INDEX)->value.GetInt()));
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

void AssetManager::LoadScene(FILE* file, std::condition_variable* threadNotifier, std::mutex* threadLock) {
	rapidjson::Document sceneDoc;

	char readBuffer[FILE_BUFFER_SIZE];
	rapidjson::FileReadStream sceneFileStream(file, readBuffer, sizeof(readBuffer));

	sceneDoc.ParseStream(sceneFileStream);

	fclose(file);
}

void AssetManager::SaveScene(std::string filepath, std::string sceneName) {
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
		bool shouldBreak = false;
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

			meshValue.AddMember(MESH_FILENAME_KEY, fileKeyValue, allocator);

			meshBlock.PushBack(meshValue, allocator);
		}

		sceneDocToSave.AddMember(MESHES, meshBlock, allocator);

		rapidjson::Value materialBlock(rapidjson::kArrayType);
		for(auto mat : this->globalMaterials) {
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
			geValue.AddMember(ENTITY_ENABLED, ge->GetEnableDisable(), allocator);
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

					// Treat FLOATX as float array[x]
					rapidjson::Value color(rapidjson::kArrayType);
					DirectX::XMFLOAT3 lightColor = light->GetColor();
					color.PushBack(lightColor.x, allocator);
					color.PushBack(lightColor.y, allocator);
					color.PushBack(lightColor.z, allocator);

					coValue.AddMember(LIGHT_COLOR, color, allocator);

					rapidjson::Value direction(rapidjson::kArrayType);
					DirectX::XMFLOAT3 lightDir = light->GetDirection();
					direction.PushBack(lightDir.x, allocator);
					direction.PushBack(lightDir.y, allocator);
					direction.PushBack(lightDir.z, allocator);

					coValue.AddMember(LIGHT_DIRECTION, direction, allocator);
					
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

		rapidjson::Value cameraBlock(rapidjson::kArrayType);
		for (auto ca : globalCameras) {
			rapidjson::Value caObject(rapidjson::kObjectType);
			rapidjson::Value caName;

			caObject.AddMember(CAMERA_NAME, caName.SetString(ca->GetName().c_str(), allocator), allocator);
			caObject.AddMember(CAMERA_ASPECT_RATIO, ca->GetAspectRatio(), allocator);
			caObject.AddMember(CAMERA_PROJECTION_MATRIX_TYPE, ca->GetProjectionMatrixType(), allocator);
			caObject.AddMember(CAMERA_TAG, ca->GetTag(), allocator);
			caObject.AddMember(CAMERA_LOOK_SPEED, ca->GetLookSpeed(), allocator);
			caObject.AddMember(CAMERA_MOVE_SPEED, ca->GetMoveSpeed(), allocator);
			caObject.AddMember(CAMERA_ENABLED, ca->GetEnableDisable(), allocator);
			caObject.AddMember(CAMERA_NEAR_DISTANCE, ca->GetNearDist(), allocator);
			caObject.AddMember(CAMERA_FAR_DISTANCE, ca->GetFarDist(), allocator);
			caObject.AddMember(CAMERA_FIELD_OF_VIEW, ca->GetFOV(), allocator);

			rapidjson::Value caTransform(rapidjson::kObjectType);
			std::shared_ptr<Transform> transform = ca->GetTransform();
			
			{
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

				scale.PushBack(transform->GetLocalPosition().x, allocator);
				scale.PushBack(transform->GetLocalPosition().y, allocator);
				scale.PushBack(transform->GetLocalPosition().z, allocator);

				rot.PushBack(transform->GetLocalPitchYawRoll().x, allocator);
				rot.PushBack(transform->GetLocalPitchYawRoll().y, allocator);
				rot.PushBack(transform->GetLocalPitchYawRoll().z, allocator);

				caTransform.AddMember(TRANSFORM_LOCAL_POSITION, pos, allocator);
				caTransform.AddMember(TRANSFORM_LOCAL_SCALE, scale, allocator);
				caTransform.AddMember(TRANSFORM_LOCAL_ROTATION, rot, allocator);
			}

			caObject.AddMember(CAMERA_TRANSFORM, caTransform, allocator);

			cameraBlock.PushBack(caObject, allocator);
		}

		sceneDocToSave.AddMember(CAMERAS, cameraBlock, allocator);

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

void AssetManager::SaveScene(FILE* file, std::string sceneName) {
	rapidjson::Document sceneDocToSave;

	// At the end of gathering data, write it all
	// to the appropriate file
	char writeBuffer[FILE_BUFFER_SIZE];
	rapidjson::FileWriteStream sceneFileStream(file, writeBuffer, sizeof(writeBuffer));

	rapidjson::Writer<rapidjson::FileWriteStream> writer(sceneFileStream);
	sceneDocToSave.Accept(writer);

	fclose(file);
}
#pragma endregion

#pragma region createAssets
bool AssetManager::materialSortDirty = false;

FMOD::Sound* AssetManager::CreateSound(std::string path, FMOD_MODE mode, std::string name) {
	try
	{
		SetLoadingAndWait("Sounds", path);

		FMODUserData* uData = new FMODUserData;
		FMOD::Sound* sound;

		std::string namePath = GetFullPathToAssetFile(AssetPathIndex::ASSET_SOUND_PATH, path);

		sound = audioInstance.LoadSound(namePath, mode);

		// Serialize the filename if it's in the right folder
		std::string assetPathStr = "Assets\\Sounds\\";

		std::string baseFilename = SerializeFileName(assetPathStr, namePath);

		uData->filenameKey = std::make_shared<std::string>(baseFilename);
		uData->name = std::make_shared<std::string>(name);

		// On getUserData, we will receive the whole struct
		sound->setUserData(uData);

		globalSounds.push_back(sound);

		return sound;
	}
	catch (...) {
		SetLoadedAndWait("Sounds", path, std::current_exception());

		return NULL;
	}
}

std::shared_ptr<Camera> AssetManager::CreateCamera(std::string id, DirectX::XMFLOAT3 pos, float aspectRatio, int type) {
	try {
		SetLoadingAndWait("Cameras", id);

		std::shared_ptr<Camera> newCam = std::make_shared<Camera>(pos, aspectRatio, type, id);

		this->globalCameras.push_back(newCam);

		return newCam;
	}
	catch (...) {
		SetLoadedAndWait("Camera", id, std::current_exception());

		return NULL;
	}
	
}

std::shared_ptr<SimpleVertexShader> AssetManager::CreateVertexShader(std::string id, std::string nameToLoad) {
	try {
		SetLoadingAndWait("Vertex Shaders", id);

		std::string namePath = GetFullPathToAssetFile(AssetPathIndex::ASSET_SHADER_PATH, nameToLoad);

		std::shared_ptr<SimpleVertexShader> newVS = std::make_shared<SimpleVertexShader>(device.Get(), context.Get(), namePath, id);

		// Serialize the filename if it's in the right folder
		std::string assetPathStr = "Assets\\Shaders\\";

		std::string baseFilename = SerializeFileName(assetPathStr, namePath);

		newVS->SetFileNameKey(baseFilename);

		vertexShaders.push_back(newVS);

		return newVS;
	}
	catch (...) {
		SetLoadedAndWait("Vertex Shaders", id, std::current_exception());

		return NULL;
	}
}

std::shared_ptr<SimplePixelShader> AssetManager::CreatePixelShader(std::string id, std::string nameToLoad) {
	try {
		SetLoadingAndWait("Pixel Shaders", id);

		std::string namePath = GetFullPathToAssetFile(AssetPathIndex::ASSET_SHADER_PATH, nameToLoad);

		std::shared_ptr<SimplePixelShader> newPS = std::make_shared<SimplePixelShader>(device.Get(), context.Get(), namePath, id);

		// Serialize the filename if it's in the right folder
		std::string assetPathStr = "Assets\\Shaders\\";

		std::string baseFilename = SerializeFileName(assetPathStr, namePath);

		newPS->SetFileNameKey(baseFilename);

		pixelShaders.push_back(newPS);

		return newPS;
	}
	catch (...) {
		SetLoadedAndWait("Pixel Shaders", id, std::current_exception());

		return NULL;
	}
}

std::shared_ptr<SimpleComputeShader> AssetManager::CreateComputeShader(std::string id, std::string nameToLoad) {
	try {
		SetLoadingAndWait("Compute Shaders", id);

		std::string namePath = GetFullPathToAssetFile(AssetPathIndex::ASSET_SHADER_PATH, nameToLoad);

		std::shared_ptr<SimpleComputeShader> newCS = std::make_shared<SimpleComputeShader>(device.Get(), context.Get(), namePath.c_str(), id);

		// Serialize the filename if it's in the right folder
		std::string assetPathStr = "Assets\\Shaders\\";
		
		std::string baseFilename = SerializeFileName(assetPathStr, namePath);

		newCS->SetFileNameKey(baseFilename);

		computeShaders.push_back(newCS);

		// Prints the shader to a readable blob
		/*Microsoft::WRL::ComPtr<ID3DBlob> assembly;
		Microsoft::WRL::ComPtr<ID3DBlob> binary;

		binary = newCS->GetShaderBlob();

		HRESULT hr = D3DDisassemble(
			binary->GetBufferPointer(),
			binary->GetBufferSize(),
			D3D_DISASM_ENABLE_INSTRUCTION_NUMBERING, nullptr,
			&assembly);

		D3DWriteBlobToFile(assembly.Get(), nameToLoad.c_str(), 1);*/

		return newCS;
	}
	catch (...) {
		SetLoadedAndWait("Compute Shaders", id, std::current_exception());

		return NULL;
	}
}

std::shared_ptr<Mesh> AssetManager::CreateMesh(std::string id, std::string nameToLoad) {
	try {
		SetLoadingAndWait("Meshes", id);

		std::string namePath = GetFullPathToAssetFile(AssetPathIndex::ASSET_MODEL_PATH, nameToLoad);
		std::shared_ptr<Mesh> newMesh = std::make_shared<Mesh>(namePath.c_str(), device, id);

		globalMeshes.push_back(newMesh);

		return newMesh;
	}
	catch (...) {
		SetLoadedAndWait("Mesh", id, std::current_exception());

		return NULL;
	}
}

/// <summary>
/// Given a path within the Assets/ dir, checks if the fullPathToAsset contains
/// that subpath. If so, it returns a serialized filepath string to be used as
/// a filename key. If not, it returns the full path to the asset.
/// </summary>
/// <param name="assetFolderPath">Include slashes or important info may be lost in serialization.</param>
/// <param name="fullPathToAsset">Full path from GetFullPathNameA preferred, but all full paths
/// should work.</param>
/// <returns>Either a serialized file key string or the full path.</returns>
std::string AssetManager::SerializeFileName(std::string assetFolderPath, std::string fullPathToAsset) {
	std::string filenameKey;

	// Serialize the filename if it's in the right folder
	size_t dirPos = fullPathToAsset.find(assetFolderPath);
	if (dirPos != std::string::npos) {
		// File is in the assets folder
		filenameKey = "t" + fullPathToAsset.substr(dirPos + assetFolderPath.size());;
	}
	else {
		filenameKey = "f" + fullPathToAsset;
	}

	return filenameKey;
}

std::string AssetManager::DeSerializeFileName(std::string assetPath) {
	if (assetPath[0] == 't') {
		// Return the assetPath and remove the t
		return assetPath.substr(1);
	}
	else {
		return assetPath;
	}
}

HRESULT AssetManager::LoadPBRTexture(std::string nameToLoad, OUT Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* texture, PBRTextureTypes textureType) {
	AssetPathIndex assetPath;

	switch (textureType) {
		case PBRTextureTypes::ALBEDO:
			assetPath = ASSET_TEXTURE_PATH_PBR_ALBEDO;
			break;
		case PBRTextureTypes::NORMAL:
			assetPath = ASSET_TEXTURE_PATH_PBR_NORMALS;
			break;
		case PBRTextureTypes::METAL:
			assetPath = ASSET_TEXTURE_PATH_PBR_METALNESS;
			break;
		case PBRTextureTypes::ROUGH:
			assetPath = ASSET_TEXTURE_PATH_PBR_ROUGHNESS;
			break;
	};

	std::string namePath = GetFullPathToAssetFile(assetPath, nameToLoad);
	std::wstring widePath;

	HRESULT hr = ISimpleShader::ConvertToWide(namePath, widePath);

	CreateWICTextureFromFile(device.Get(), context.Get(), widePath.c_str(), nullptr, texture->GetAddressOf());

	return hr;
}

void AssetManager::SetMaterialTextureFileKey(std::string textureFilename, std::shared_ptr<Material> mat, PBRTextureTypes textureType) {
	AssetPathIndex assetPath;
	std::string directAssetPath;

	switch (textureType) {
		case PBRTextureTypes::ALBEDO:
			assetPath = ASSET_TEXTURE_PATH_PBR_ALBEDO;
			directAssetPath = "Assets\\PBR\\Albedo\\";
			break;
		case PBRTextureTypes::NORMAL:
			assetPath = ASSET_TEXTURE_PATH_PBR_NORMALS;
			directAssetPath = "Assets\\PBR\\Normals\\";
			break;
		case PBRTextureTypes::METAL:
			assetPath = ASSET_TEXTURE_PATH_PBR_METALNESS;
			directAssetPath = "Assets\\PBR\\Metalness\\";
			break;
		case PBRTextureTypes::ROUGH:
			assetPath = ASSET_TEXTURE_PATH_PBR_ROUGHNESS;
			directAssetPath = "Assets\\PBR\\Roughness\\";
			break;
	};

	std::string namePath = GetFullPathToAssetFile(assetPath, textureFilename);

	mat->SetTextureFilenameKey(textureType, SerializeFileName(directAssetPath, namePath));
}

std::shared_ptr<Material> AssetManager::CreatePBRMaterial(std::string id,
														  std::string albedoNameToLoad,
														  std::string normalNameToLoad,
														  std::string metalnessNameToLoad,
														  std::string roughnessNameToLoad,
														  bool addToGlobalList) {
	try {
		SetLoadingAndWait("PBR Materials", id);

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedo;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normals;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalness;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughness;

		std::shared_ptr<SimpleVertexShader> VSNormal = GetVertexShaderByName("NormalsVS");
		std::shared_ptr<SimplePixelShader> PSNormal = GetPixelShaderByName("NormalsPS");

		LoadPBRTexture(albedoNameToLoad, &albedo, PBRTextureTypes::ALBEDO);
		LoadPBRTexture(normalNameToLoad, &normals, PBRTextureTypes::NORMAL);
		LoadPBRTexture(metalnessNameToLoad, &metalness, PBRTextureTypes::METAL);
		LoadPBRTexture(roughnessNameToLoad, &roughness, PBRTextureTypes::ROUGH);

		std::shared_ptr<Material> newMat = std::make_shared<Material>(whiteTint,
			PSNormal,
			VSNormal,
			albedo,
			textureState,
			clampState,
			normals,
			roughness,
			metalness,
			id);

		SetMaterialTextureFileKey(albedoNameToLoad, newMat, PBRTextureTypes::ALBEDO);
		SetMaterialTextureFileKey(normalNameToLoad, newMat, PBRTextureTypes::NORMAL);
		SetMaterialTextureFileKey(metalnessNameToLoad, newMat, PBRTextureTypes::METAL);
		SetMaterialTextureFileKey(roughnessNameToLoad, newMat, PBRTextureTypes::ROUGH);

		if (addToGlobalList) globalMaterials.push_back(newMat);

		return newMat;
	}
	catch (...) {
		SetLoadedAndWait("PBR Material", id, std::current_exception());

		return NULL;
	}
}

/// <summary>
/// Creates a GameEntity
/// </summary>
/// <param name="name">Name of the GameEntity</param>
/// <returns>Pointer to the new GameEntity</returns>
std::shared_ptr<GameEntity> AssetManager::CreateGameEntity(std::string name)
{
	try {
		// Don't show loading if it's an empty entity
		//SetLoadingAndWait("Game Entities", name);

		std::shared_ptr<GameEntity> newEnt = std::make_shared<GameEntity>(XMMatrixIdentity(), name);
		newEnt->Initialize();

		globalEntities.push_back(newEnt);

		return newEnt;
	}
	catch (...) {
		SetLoadedAndWait("Game Entities", name, std::current_exception());

		return NULL;
	}
}

/// <summary>
/// Creates a GameEntity and gives it a MeshRenderer component
/// </summary>
/// <param name="mesh">Mesh to render</param>
/// <param name="mat">Material to render</param>
/// <param name="name">Name of the GameEntity</param>
/// <returns>Pointer to the new GameEntity</returns>
std::shared_ptr<GameEntity> AssetManager::CreateGameEntity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> mat, std::string name) {
	try {
		SetLoadingAndWait("Game Entities", name + " Components");

		std::shared_ptr<GameEntity> newEnt = CreateGameEntity(name);

		std::shared_ptr<MeshRenderer> renderer = newEnt->AddComponent<MeshRenderer>();
		renderer->SetMesh(mesh);
		renderer->SetMaterial(mat);

		return newEnt;
	}
	catch (...) {
		SetLoadedAndWait("Game Entities", name + " Components", std::current_exception());

		return NULL;
	}
}

/// <summary>
/// Creates a GameEntity, gives it a light component, and populates it with Directional Light values
/// </summary>
/// <param name="name">Name of the GameEntity</param>
/// <param name="direction">Direction of the light</param>
/// <param name="color">Color of the light</param>
/// <param name="intensity">Intensity of the light</param>
/// <returns>Pointer to the new Light component</returns>
std::shared_ptr<Light> AssetManager::CreateDirectionalLight(std::string name, DirectX::XMFLOAT3 direction, DirectX::XMFLOAT3 color, float intensity)
{
	try {
		SetLoadingAndWait("Lights", name);

		std::shared_ptr<GameEntity> newEnt = CreateGameEntity(name);
		std::shared_ptr<Light> light = CreateDirectionalLightOnEntity(newEnt, direction, color, intensity);
		return light;
	}
	catch (...) {
		SetLoadedAndWait("Lights", name, std::current_exception());

		return NULL;
	}
}

/// <summary>
/// Creates a GameEntity, gives it a light component, and populates it with Point Light values
/// </summary>
/// <param name="name">Name of the GameEntity</param>
/// <param name="range">Range of the light</param>
/// <param name="color">Color of the light</param>
/// <param name="intensity">Intensity of the light</param>
/// <returns>Pointer to the new Light component</returns>
std::shared_ptr<Light> AssetManager::CreatePointLight(std::string name, float range, DirectX::XMFLOAT3 color, float intensity)
{
	try {
		SetLoadingAndWait("Lights", name);

		std::shared_ptr<GameEntity> newEnt = CreateGameEntity(name);
		std::shared_ptr<Light> light = CreatePointLightOnEntity(newEnt, range, color, intensity);
		return light;
	}
	catch (...) {
		SetLoadedAndWait("Lights", name, std::current_exception());
	
		return NULL;
	}
}

/// <summary>
/// Creates a GameEntity, gives it a light component, and populates it with Spot Light values
/// </summary>
/// <param name="name">Name of the GameEntity</param>
/// <param name="direction">Direction of the light</param>
/// <param name="range">Range of the light</param>
/// <param name="color">Color of the light</param>
/// <param name="intensity">Intensity of the light</param>
/// <returns>Pointer to the new Light component</returns>
std::shared_ptr<Light> AssetManager::CreateSpotLight(std::string name, DirectX::XMFLOAT3 direction, float range, DirectX::XMFLOAT3 color, float intensity)
{
	try {
		SetLoadingAndWait("Lights", name);

		std::shared_ptr<GameEntity> newEnt = CreateGameEntity(name);
		std::shared_ptr<Light> light = CreateSpotLightOnEntity(newEnt, direction, range, color, intensity);
		return light;
	}
	catch (...) {
		SetLoadedAndWait("Lights", name, std::current_exception());

		return NULL;
	}
}

/// <summary>
/// Creates a GameEntity and gives it a Terrain component
/// </summary>
/// <param name="name">Name of the GameEntity</param>
/// <returns>Pointer to the new Terrain</returns>
std::shared_ptr<Terrain> AssetManager::CreateTerrainEntity(std::string name) {
	try {
		SetLoadingAndWait("Terrain Entities", name);

		std::shared_ptr<GameEntity> newEnt = CreateGameEntity(name);

		return newEnt->AddComponent<Terrain>();
	}
	catch (...) {
		SetLoadedAndWait("Terrain Entities", name, std::current_exception());

		return NULL;
	}
}

/// <summary>
/// Creates a GameEntity and gives it a Terrain component.
/// Loads the mesh from a heightmap, then applies a material.
/// </summary>
/// <param name="heightmap"></param>
/// <param name="material"></param>
/// <param name="name"></param>
/// <param name="mapWidth"></param>
/// <param name="mapHeight"></param>
/// <param name="heightScale"></param>
/// <returns></returns>
std::shared_ptr<Terrain> AssetManager::CreateTerrainEntity(const char* heightmap,
														   std::shared_ptr<TerrainMaterial> material,
														   std::string name,
														   unsigned int mapWidth,
														   unsigned int mapHeight,
														   float heightScale) 
{
	try {
		SetLoadingAndWait("Terrain Entities", name);

		std::shared_ptr<GameEntity> newEnt = CreateGameEntity(name);
		std::shared_ptr<Terrain> newTerrain = CreateTerrainOnEntity(newEnt, heightmap, material, mapWidth, mapHeight, heightScale);

		return newTerrain;
	}
	catch (...) {
		SetLoadedAndWait("Terrain Entities", name, std::current_exception());

		return NULL;
	}
}

/// <summary>
/// Creates a GameEntity and gives it a Terrain component.
/// Sets the mesh and material.
/// </summary>
/// <param name="terrainMesh"></param>
/// <param name="material"></param>
/// <param name="name"></param>
/// <returns></returns>
std::shared_ptr<Terrain> AssetManager::CreateTerrainEntity(std::shared_ptr<Mesh> terrainMesh, 
														   std::shared_ptr<TerrainMaterial> material, 
														   std::string name) 
{
	try {
		SetLoadingAndWait("Terrain Entities", name);

		std::shared_ptr<GameEntity> newEnt = CreateGameEntity(name);
		std::shared_ptr<Terrain> newTerrain = CreateTerrainOnEntity(newEnt, terrainMesh, material);

		return newTerrain;
	}
	catch (...) {
		SetLoadedAndWait("Terrain Entities", name, std::current_exception());

		return NULL;
	}
}

std::shared_ptr<TerrainMaterial> AssetManager::CreateTerrainMaterial(std::string name, std::vector<std::shared_ptr<Material>> materials, std::string blendMapPath) {
	SetLoadingAndWait("Terrain Materials", name);

	std::shared_ptr<TerrainMaterial> newTMat = std::make_shared<TerrainMaterial>(name);

	for (auto m : materials) {
		newTMat->AddMaterial(m);
	}

	if (blendMapPath != "") {
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> blendMap;
		std::string namePath = GetFullPathToAssetFile(AssetPathIndex::ASSET_TEXTURE_PATH_BASIC, blendMapPath);

		std::wstring wPath;
		ISimpleShader::ConvertToWide(namePath, wPath);

		CreateWICTextureFromFile(device.Get(), context.Get(), wPath.c_str(), nullptr, blendMap.GetAddressOf());

		newTMat->SetBlendMap(blendMap);

		newTMat->SetBlendMapFilenameKey(SerializeFileName("Assets\\Textures\\", namePath));
	}

	newTMat->SetPixelShader(GetPixelShaderByName("TerrainPS"));
	newTMat->SetVertexShader(GetVertexShaderByName("TerrainVS"));

	globalTerrainMaterials.push_back(newTMat);

	return newTMat;
}

std::shared_ptr<TerrainMaterial> AssetManager::CreateTerrainMaterial(std::string name,
																	 std::vector<std::string> texturePaths,
																	 std::vector<std::string> matNames,
																	 bool isPBRMat,
																	 std::string blendMapPath)
{
	SetLoadingAndWait("Terrain Materials", name);

	std::shared_ptr<TerrainMaterial> newTMat = std::make_shared<TerrainMaterial>(name);

	for (int i = 0; i < matNames.size(); i++) {
		std::shared_ptr<Material> newMat;

		if (isPBRMat) {
			int textureIndex = i * 4;
			newMat = CreatePBRMaterial(matNames[i], texturePaths[textureIndex], texturePaths[textureIndex + 1], texturePaths[textureIndex + 2], texturePaths[textureIndex + 3]);
		}
		else {
			// Currently unimplemented
		}

		newTMat->AddMaterial(newMat);
	}

	if (blendMapPath != "") {
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> blendMap;
		std::string namePath = GetFullPathToAssetFile(AssetPathIndex::ASSET_TEXTURE_PATH_BASIC, blendMapPath);

		std::wstring wPath;
		ISimpleShader::ConvertToWide(namePath, wPath);

		CreateWICTextureFromFile(device.Get(), context.Get(), wPath.c_str(), nullptr, blendMap.GetAddressOf());

		newTMat->SetBlendMap(blendMap);
		newTMat->SetBlendMapFilenameKey(SerializeFileName("Assets\\Textures\\", namePath));
	}

	newTMat->SetPixelShader(GetPixelShaderByName("TerrainPS"));
	newTMat->SetVertexShader(GetVertexShaderByName("TerrainVS"));

	globalTerrainMaterials.push_back(newTMat);

	return newTMat;
}

std::shared_ptr<Sky> AssetManager::CreateSky(std::string filepath, bool fileType, std::string name, std::string fileExtension) {
	try {
		SetLoadingAndWait("Skies", name);

		std::shared_ptr<Mesh> Cube = GetMeshByName("Cube");

		std::vector<std::shared_ptr<SimplePixelShader>> importantSkyPixelShaders;
		std::vector<std::shared_ptr<SimpleVertexShader>> importantSkyVertexShaders;

		importantSkyPixelShaders.push_back(GetPixelShaderByName("SkyPS"));
		importantSkyPixelShaders.push_back(GetPixelShaderByName("IrradiancePS"));
		importantSkyPixelShaders.push_back(GetPixelShaderByName("SpecularConvolutionPS"));
		importantSkyPixelShaders.push_back(GetPixelShaderByName("BRDFLookupTablePS"));

		importantSkyVertexShaders.push_back(GetVertexShaderByName("SkyVS"));
		importantSkyVertexShaders.push_back(GetVertexShaderByName("FullscreenVS"));

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> newSkyTexture;

		std::string assetPath = GetFullPathToAssetFile(AssetPathIndex::ASSET_TEXTURE_PATH_SKIES, filepath);

		if (fileType) {
			// Process as 6 textures in a directory

			std::wstring skyDirWide;
			std::wstring fileExtensionW;
			ISimpleShader::ConvertToWide(assetPath, skyDirWide);
			ISimpleShader::ConvertToWide(fileExtension, fileExtensionW);

			newSkyTexture = CreateCubemap((skyDirWide + L"right" + fileExtensionW).c_str(),
				(skyDirWide + L"left" + fileExtensionW).c_str(),
				(skyDirWide + L"up" + fileExtensionW).c_str(),
				(skyDirWide + L"down" + fileExtensionW).c_str(),
				(skyDirWide + L"forward" + fileExtensionW).c_str(),
				(skyDirWide + L"back" + fileExtensionW).c_str());
		}
		else {
			// Process as a .dds

			std::wstring skyDDSWide;
			ISimpleShader::ConvertToWide(assetPath, skyDDSWide);

			CreateDDSTextureFromFile(device.Get(), context.Get(), skyDDSWide.c_str(), nullptr, &newSkyTexture);
		}

		std::string filenameKey = SerializeFileName("Assets\\Textures\\Skies\\", assetPath);

		std::shared_ptr<Sky> newSky = std::make_shared<Sky>(textureState, newSkyTexture, Cube, importantSkyPixelShaders, importantSkyVertexShaders, device, context, name);

		newSky->SetFilenameKeyType(fileType);
		newSky->SetFilenameKey(filenameKey);
		newSky->SetFileExtension(fileExtension);

		skies.push_back(newSky);

		return newSky;
	}
	catch (...) {
		SetLoadedAndWait("Skies", name, std::current_exception());

		return NULL;
	}
}

/// <summary>
/// Creates a GameEntity and gives it a default ParticleSystem component
/// </summary>
/// <param name="name">Name of the GameEntity</param>
/// <param name="textureNameToLoad">Name of the file or file path to load the texture(s) from</param>
/// <param name="isMultiParticle">True to recursively load textures from the file path</param>
/// <returns>Pointer to the new GameEntity</returns>
std::shared_ptr<ParticleSystem> AssetManager::CreateParticleEmitter(std::string name,
	std::string textureNameToLoad,
	bool isMultiParticle)
{
	try {
		SetLoadingAndWait("Particle Emitter", name);

		std::shared_ptr<GameEntity> emitterEntity = CreateGameEntity(name);
		std::shared_ptr<ParticleSystem> newEmitter = CreateParticleEmitterOnEntity(emitterEntity, textureNameToLoad, isMultiParticle);

		return newEmitter;
	}
	catch (...) {
		SetLoadedAndWait("Particle Emitter", name, std::current_exception());

		return NULL;
	}
}

/// <summary>
/// Creates a GameEntity and gives it a ParticleSystem component
/// </summary>
/// <param name="name">Name of the GameEntity</param>
/// <param name="textureNameToLoad">Name of the file or file path to load the texture(s) from</param>
/// <param name="maxParticles">Max number of active particles at one time</param>
/// <param name="particleLifeTime">How long a particle exists for</param>
/// <param name="particlesPerSecond">Rate of particle spawns</param>
/// <param name="isMultiParticle">True to recursively load textures from the file path</param>
/// <param name="additiveBlendState">Whether to use an additive blend state when rendering</param>
/// <returns></returns>
std::shared_ptr<ParticleSystem> AssetManager::CreateParticleEmitter(std::string name,
															 std::string textureNameToLoad,
															 int maxParticles,
															 float particleLifeTime,
															 float particlesPerSecond,
															 bool isMultiParticle,
															 bool additiveBlendState) {
	try {
		SetLoadingAndWait("Particle Emitter", name);

		std::shared_ptr<GameEntity> emitterEntity = CreateGameEntity(name);
		std::shared_ptr<ParticleSystem> newEmitter = CreateParticleEmitterOnEntity(emitterEntity, textureNameToLoad, maxParticles, particleLifeTime, particlesPerSecond, isMultiParticle, additiveBlendState);

		return newEmitter;
	}
	catch (...) {
		SetLoadedAndWait("Particle Emitter", name, std::current_exception());

		return NULL;
	}
}

std::shared_ptr<SHOEFont> AssetManager::CreateSHOEFont(std::string name, std::string filePath, bool preInitializing) {
	try {
		// If the loading screen fonts aren't loaded, don't trigger
		// the loading screen.
		if (!preInitializing) SetLoadingAndWait("Font", name);

		std::string namePath = GetFullPathToAssetFile(AssetPathIndex::ASSET_FONT_PATH, filePath);
		std::wstring wPathBuf;

		std::string baseFilename = SerializeFileName("Assets\\Fonts\\", namePath);

		ISimpleShader::ConvertToWide(namePath, wPathBuf);

		std::shared_ptr<SHOEFont> newFont = std::make_shared<SHOEFont>();
		newFont->fileNameKey = baseFilename;
		newFont->name = name;
		newFont->spritefont = std::make_shared<DirectX::SpriteFont>(device.Get(), wPathBuf.c_str());

		globalFonts.push_back(newFont);

		return newFont;
	}
	catch (...) {
		if (!preInitializing) SetLoadedAndWait("Font", name, std::current_exception());

		return NULL;
	}
}
#pragma endregion

#pragma region createOnEntity

std::shared_ptr<Terrain> AssetManager::CreateTerrainOnEntity(std::shared_ptr<GameEntity> entityToEdit,
	const char* heightmap,
	std::shared_ptr<TerrainMaterial> material,
	unsigned int mapWidth,
	unsigned int mapHeight,
	float heightScale) {

	std::shared_ptr<Terrain> newTerrain = entityToEdit->AddComponent<Terrain>();

	newTerrain->SetMesh(LoadTerrain(heightmap, mapWidth, mapHeight, heightScale));
	newTerrain->SetMaterial(material);

	return newTerrain;
}

std::shared_ptr<Terrain> AssetManager::CreateTerrainOnEntity(std::shared_ptr<GameEntity> entityToEdit,
	std::shared_ptr<Mesh> terrainMesh,
	std::shared_ptr<TerrainMaterial> material) {

	std::shared_ptr<Terrain> newTerrain = entityToEdit->AddComponent<Terrain>();

	newTerrain->SetMesh(terrainMesh);
	newTerrain->SetMaterial(material);

	return newTerrain;
}

std::shared_ptr<ParticleSystem> AssetManager::CreateParticleEmitterOnEntity(std::shared_ptr<GameEntity> entityToEdit,
	std::string textureNameToLoad,
	bool isMultiParticle) {

	std::shared_ptr<ParticleSystem> newEmitter = entityToEdit->AddComponent<ParticleSystem>();

	newEmitter->SetIsMultiParticle(isMultiParticle);
	newEmitter->SetParticleTextureSRV(LoadParticleTexture(textureNameToLoad, isMultiParticle));

	std::string asset = GetFullPathToAssetFile(AssetPathIndex::ASSET_PARTICLE_PATH, textureNameToLoad);

	newEmitter->SetFilenameKey(SerializeFileName("Assets\\Particles\\", asset));

	// Set all the compute shaders here
	newEmitter->SetParticleComputeShader(GetComputeShaderByName("ParticleEmitCS"), Emit);
	newEmitter->SetParticleComputeShader(GetComputeShaderByName("ParticleMoveCS"), Simulate);
	newEmitter->SetParticleComputeShader(GetComputeShaderByName("ParticleCopyCS"), Copy);
	newEmitter->SetParticleComputeShader(GetComputeShaderByName("ParticleInitDeadCS"), DeadListInit);

	return newEmitter;

}

std::shared_ptr<ParticleSystem> AssetManager::CreateParticleEmitterOnEntity(std::shared_ptr<GameEntity> entityToEdit,
	std::string textureNameToLoad,
	int maxParticles,
	float particleLifeTime,
	float particlesPerSecond,
	bool isMultiParticle,
	bool additiveBlendState) {

	std::shared_ptr<ParticleSystem> newEmitter = CreateParticleEmitterOnEntity(entityToEdit, textureNameToLoad, isMultiParticle);
	newEmitter->SetMaxParticles(maxParticles);
	newEmitter->SetParticleLifetime(particleLifeTime);
	newEmitter->SetParticlesPerSecond(particlesPerSecond);
	newEmitter->SetBlendState(additiveBlendState);

	return newEmitter;
}

std::shared_ptr<Light> AssetManager::CreateDirectionalLightOnEntity(std::shared_ptr<GameEntity> entityToEdit,
	DirectX::XMFLOAT3 direction,
	DirectX::XMFLOAT3 color,
	float intensity) {

	std::shared_ptr<Light> light = entityToEdit->AddComponent<Light>();
	if (light != nullptr) {
		light->SetType(0.0f);
		light->SetDirection(direction);
		light->SetColor(color);
		light->SetIntensity(intensity);
	}

	return light;
}

std::shared_ptr<Light> AssetManager::CreatePointLightOnEntity(std::shared_ptr<GameEntity> entityToEdit,
	float range,
	DirectX::XMFLOAT3 color,
	float intensity) {

	std::shared_ptr<Light> light = entityToEdit->AddComponent<Light>();
	if (light != nullptr) {
		light->SetType(1.0f);
		light->SetRange(range);
		light->SetColor(color);
		light->SetIntensity(intensity);
	}

	return light;
}

std::shared_ptr<Light> AssetManager::CreateSpotLightOnEntity(std::shared_ptr<GameEntity> entityToEdit,
	DirectX::XMFLOAT3 direction,
	float range,
	DirectX::XMFLOAT3 color,
	float intensity) {

	std::shared_ptr<Light> light = entityToEdit->AddComponent<Light>();
	if (light != nullptr) {
		light->SetType(2.0f);
		light->SetDirection(direction);
		light->SetRange(range);
		light->SetColor(color);
		light->SetIntensity(intensity);
	}

	return light;
}

#pragma endregion

#pragma region initAssets
void AssetManager::InitializeTextureSampleStates() {
	Microsoft::WRL::ComPtr<ID3D11SamplerState> basicSampler;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> clampSampler;

	//Create sampler state
	D3D11_SAMPLER_DESC textureDesc;
	textureDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	textureDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	textureDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	textureDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	textureDesc.MaxAnisotropy = 10;
	textureDesc.MaxLOD = D3D11_FLOAT32_MAX;
	textureDesc.MipLODBias = 0;
	textureDesc.MinLOD = 0;

	device->CreateSamplerState(&textureDesc, &basicSampler);

	//Create clamp sampler state
	D3D11_SAMPLER_DESC clampDesc;
	clampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	clampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	clampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	clampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	clampDesc.MaxAnisotropy = 10;
	clampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	clampDesc.MipLODBias = 0;
	clampDesc.MinLOD = 0;

	device->CreateSamplerState(&clampDesc, &clampSampler);

	textureSampleStates.push_back(basicSampler);
	textureSampleStates.push_back(clampSampler);

	textureState = textureSampleStates[0];
	clampState = textureSampleStates[1];
}

void AssetManager::InitializeGameEntities() {
	//Initializes default values for components
	MeshRenderer::SetDefaults(GetMeshByName("Cube"), GetMaterialByName("largeCobbleMat"));

	globalEntities = std::vector<std::shared_ptr<GameEntity>>();

	// Show example renders
	CreateGameEntity(GetMeshByName("Cube"), GetMaterialByName("bronzeMat"), "Bronze Cube");
	CreateGameEntity(GetMeshByName("Cube"), GetMaterialByName("floorMat"), "Floor Cube");
	CreateGameEntity(GetMeshByName("Cube"), GetMaterialByName("scratchMat"), "Scratched Cube");

	CreateGameEntity(GetMeshByName("Cylinder"), GetMaterialByName("floorMat"), "Stone Cylinder");
	CreateGameEntity(GetMeshByName("Helix"), GetMaterialByName("floorMat"), "Floor Helix");
	CreateGameEntity(GetMeshByName("Sphere"), GetMaterialByName("paintMat"), "Paint Sphere");
	CreateGameEntity(GetMeshByName("Torus"), GetMaterialByName("roughMat"), "Rough Torus");
	CreateGameEntity(GetMeshByName("Sphere"), GetMaterialByName("scratchMat"), "Scratched Sphere");
	CreateGameEntity(GetMeshByName("Sphere"), GetMaterialByName("woodMat"), "Wood Sphere");
	CreateGameEntity(GetMeshByName("Cube"), GetMaterialByName("largePaintMat"), "Large Paint Rect");
	CreateGameEntity(GetMeshByName("Cube"), GetMaterialByName("largeCobbleMat"), "Large Cobble Rect");

	// Reflective objects
	CreateGameEntity(GetMeshByName("Sphere"), GetMaterialByName("reflectiveMetal"), "Shiny Metal Sphere");
	CreateGameEntity(GetMeshByName("Sphere"), GetMaterialByName("reflectiveRoughMetal"), "Rough Metal Sphere");
	CreateGameEntity(GetMeshByName("Sphere"), GetMaterialByName("reflectiveRough"), "Shiny Rough Sphere");
	CreateGameEntity(GetMeshByName("Sphere"), GetMaterialByName("reflective"), "Shiny Sphere");

	// Refractive Objects
	CreateGameEntity(GetMeshByName("Sphere"), GetMaterialByName("refractivePaintMat"), "Refractive Sphere");
	CreateGameEntity(GetMeshByName("Sphere"), GetMaterialByName("refractiveWoodMat"), "Refractive Sphere 2");
	CreateGameEntity(GetMeshByName("Cube"), GetMaterialByName("refractiveRoughMat"), "Refractive Cube");
	CreateGameEntity(GetMeshByName("Torus"), GetMaterialByName("refractiveBronzeMat"), "Refractive Torus");

	GetGameEntityByName("Bronze Cube")->GetTransform()->SetPosition(+0.0f, +0.0f, +0.0f);
	GetGameEntityByName("Floor Cube")->GetTransform()->SetPosition(+2.0f, +0.0f, +0.0f);
	GetGameEntityByName("Scratched Cube")->GetTransform()->SetPosition(+0.5f, +2.0f, +0.0f);
	GetGameEntityByName("Stone Cylinder")->GetTransform()->SetPosition(-0.7f, +0.0f, +0.0f);
	GetGameEntityByName("Floor Helix")->GetTransform()->SetPosition(+0.0f, +0.7f, +0.0f);
	GetGameEntityByName("Paint Sphere")->GetTransform()->SetPosition(+0.0f, -0.7f, +0.0f);
	GetGameEntityByName("Scratched Sphere")->GetTransform()->SetPosition(+3.0f, +0.0f, +0.0f);
	GetGameEntityByName("Wood Sphere")->GetTransform()->SetPosition(-3.0f, +0.0f, +0.0f);
	GetGameEntityByName("Large Paint Rect")->GetTransform()->SetScale(10.0f, 10.0f, 0.2f);
	GetGameEntityByName("Large Paint Rect")->GetTransform()->SetPosition(+0.0f, +0.0f, +4.0f);
	GetGameEntityByName("Large Cobble Rect")->GetTransform()->SetScale(+10.0f, +10.0f, +0.2f);
	GetGameEntityByName("Large Cobble Rect")->GetTransform()->SetPosition(+0.0f, -5.0f, +0.0f);
	GetGameEntityByName("Large Cobble Rect")->GetTransform()->Rotate(45.0f, 0.0f, 0.0f);

	GetGameEntityByName("Shiny Metal Sphere")->GetTransform()->SetPosition(+4.0f, +1.0f, 0.0f);
	GetGameEntityByName("Rough Metal Sphere")->GetTransform()->SetPosition(+5.0f, +1.0f, 0.0f);
	GetGameEntityByName("Shiny Sphere")->GetTransform()->SetPosition(+4.0f, 0.0f, 0.0f);
	GetGameEntityByName("Shiny Rough Sphere")->GetTransform()->SetPosition(+5.0f, 0.0f, 0.0f);

	GetGameEntityByName("Refractive Sphere")->GetTransform()->SetPosition(+4.0f, +1.0f, -1.0f);
	GetGameEntityByName("Refractive Sphere 2")->GetTransform()->SetPosition(+5.0f, +1.0f, -1.0f);
	GetGameEntityByName("Refractive Cube")->GetTransform()->SetPosition(+4.0f, +0.0f, -1.0f);
	GetGameEntityByName("Refractive Torus")->GetTransform()->SetPosition(+5.0f, +0.0f, -1.0f);

	//Set up some parenting examples
	GetGameEntityByName("Floor Cube")->GetTransform()->SetParent(GetGameEntityByName("Bronze Cube")->GetTransform());
	GetGameEntityByName("Scratched Cube")->GetTransform()->SetParent(GetGameEntityByName("Floor Cube")->GetTransform());
	GetGameEntityByName("Rough Torus")->GetTransform()->SetParent(GetGameEntityByName("Paint Sphere")->GetTransform());
	GetGameEntityByName("Floor Helix")->GetTransform()->SetParent(GetGameEntityByName("Rough Torus")->GetTransform());
	GetGameEntityByName("Stone Cylinder")->GetTransform()->SetParent(GetGameEntityByName("Floor Helix")->GetTransform());
	GetGameEntityByName("Wood Sphere")->GetTransform()->SetParent(GetGameEntityByName("Floor Helix")->GetTransform());

	CreateComplexGeometry();
}

void AssetManager::InitializeMaterials() {
	globalMaterials = std::vector<std::shared_ptr<Material>>();

	// Make reflective PBR materials
	CreatePBRMaterial(std::string("reflectiveMetal"),
					  "BlankAlbedo.png",
					  "blank_normals.png",
					  "bronze_metal.png",
					  "GenericRoughness0.png");

	CreatePBRMaterial(std::string("reflective"),
					  "BlankAlbedo.png",
					  "blank_normals.png",
					  "wood_metal.png",
					  "GenericRoughness0.png");

	CreatePBRMaterial(std::string("reflectiveRough"),
					  "BlankAlbedo.png",
					  "blank_normals.png",
					  "wood_metal.png",
					  "GenericRoughness100.png");

	CreatePBRMaterial(std::string("reflectiveRoughMetal"),
					  "BlankAlbedo.png",
					  "blank_normals.png",
					  "bronze_metal.png",
					  "GenericRoughness100.png");

	//Make PBR materials
	CreatePBRMaterial(std::string("bronzeMat"),
					  "bronze_albedo.png",
					  "bronze_normals.png",
					  "bronze_metal.png",
					  "bronze_roughness.png")->SetTiling(0.3f);

	CreatePBRMaterial(std::string("cobbleMat"),
					  "cobblestone_albedo.png",
					  "cobblestone_normals.png",
					  "cobblestone_metal.png",
					  "cobblestone_roughness.png");

	CreatePBRMaterial(std::string("largeCobbleMat"),
					  "cobblestone_albedo.png",
					  "cobblestone_normals.png",
					  "cobblestone_metal.png",
					  "cobblestone_roughness.png")->SetTiling(5.0f);

	CreatePBRMaterial(std::string("floorMat"),
					  "floor_albedo.png",
					  "floor_normals.png",
					  "floor_metal.png",
					  "floor_roughness.png");

	CreatePBRMaterial(std::string("terrainFloorMat"),
					  "floor_albedo.png",
					  "floor_normals.png",
					  "floor_metal.png",
					  "floor_roughness.png")->SetTiling(256.0f);

	CreatePBRMaterial(std::string("paintMat"),
					  "paint_albedo.png",
					  "paint_normals.png",
					  "paint_metal.png",
					  "paint_roughness.png");

	CreatePBRMaterial(std::string("largePaintMat"),
					  "paint_albedo.png",
					  "paint_normals.png",
					  "paint_metal.png",
					  "paint_roughness.png")->SetTiling(5.0f);

	CreatePBRMaterial(std::string("roughMat"),
					  "rough_albedo.png",
					  "rough_normals.png",
					  "rough_metal.png",
					  "rough_roughness.png");

	CreatePBRMaterial(std::string("scratchMat"),
					  "scratched_albedo.png",
					  "scratched_normals.png",
					  "scratched_metal.png",
					  "scratched_roughness.png");

	CreatePBRMaterial(std::string("woodMat"),
					  "wood_albedo.png",
					  "wood_normals.png",
					  "wood_metal.png",
					  "wood_roughness.png");

	CreatePBRMaterial(std::string("refractivePaintMat"),
					  "paint_albedo.png",
					  "paint_normals.png",
					  "paint_metal.png",
					  "paint_roughness.png")->SetRefractive(true);
	GetMaterialByName("refractivePaintMat")->SetRefractivePixelShader(GetPixelShaderByName("RefractivePS"));

	CreatePBRMaterial(std::string("refractiveWoodMat"),
					  "wood_albedo.png",
					  "wood_normals.png",
					  "wood_metal.png",
					  "wood_roughness.png")->SetTransparent(true);
	GetMaterialByName("refractiveWoodMat")->SetRefractivePixelShader(GetPixelShaderByName("RefractivePS"));

	CreatePBRMaterial(std::string("refractiveRoughMat"),
					  "rough_albedo.png",
					  "rough_normals.png",
					  "rough_metal.png",
					  "rough_roughness.png")->SetRefractive(true);
	GetMaterialByName("refractiveRoughMat")->SetRefractivePixelShader(GetPixelShaderByName("RefractivePS"));

	CreatePBRMaterial(std::string("refractiveBronzeMat"),
					  "bronze_albedo.png",
					  "bronze_normals.png",
					  "bronze_metal.png",
					  "bronze_roughness.png")->SetRefractive(true);
	GetMaterialByName("refractiveBronzeMat")->SetRefractivePixelShader(GetPixelShaderByName("RefractivePS"));
}

void AssetManager::InitializeMeshes() {
	globalMeshes = std::vector<std::shared_ptr<Mesh>>();

	// Test loading failure
	//CreateMesh("ExceptionTest", "InvalidPath");

	CreateMesh("Cube", "cube.obj");
	CreateMesh("Cylinder", "cylinder.obj");
	CreateMesh("Helix", "helix.obj");
	CreateMesh("Sphere", "sphere.obj");
	CreateMesh("Torus", "torus.obj");
}


void AssetManager::InitializeSkies() {
	skies = std::vector<std::shared_ptr<Sky>>();

	// Temporarily, we only load 3 skies, as they take a while to load

	//CreateSky(spaceTexture, "space");
	CreateSky("SunnyCubeMap.dds", 0, "sunny");
	//CreateSky(mountainTexture, "mountain");
	CreateSky("Niagara/", 1, "niagara", ".jpg");
	// Default is .png, which this is
	CreateSky("Stars/", 1, "stars");

	currentSky = skies[0];
}

void AssetManager::InitializeLights() {
	try {
		//white light from the top left
		CreateDirectionalLight("MainLight", DirectX::XMFLOAT3(1, -1, 0), DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), 0.7f);

		//white light from the back
		CreateDirectionalLight("BackLight", DirectX::XMFLOAT3(0, 0, -1));

		SetLoadingAndWait("Lights", "backLight");

		//red light on the bottom
		CreateDirectionalLight("BottomLight", DirectX::XMFLOAT3(0, 1, 0), DirectX::XMFLOAT3(1.0f, 0.2f, 0.2f));

		SetLoadingAndWait("Lights", "bottomLight");

		//red pointlight in the center
		std::shared_ptr<Light> bottomLight = CreatePointLight("CenterLight", 2.0f, DirectX::XMFLOAT3(0.1f, 1.0f, 0.2f));
		bottomLight->GetTransform()->SetPosition(DirectX::XMFLOAT3(0, 1.5f, 0));

		SetLoadingAndWait("Lights", "centerLight");

		//flashlight attached to camera +.5z and x
		std::shared_ptr<Light> flashlight = CreateSpotLight("Flashlight", DirectX::XMFLOAT3(0, 0, -1), 10.0f);
		flashlight->GetTransform()->SetPosition(DirectX::XMFLOAT3(0.5f, 0.0f, 0.5f));
		flashlight->SetEnabled(false);

		SetLoadingAndWait("Lights", "flashLight");
	}
	catch (...) {
		SetLoadedAndWait("Lights", "Unknown Light", std::current_exception());

		return;
	}
}

void AssetManager::InitializeTerrainEntities() {
	Terrain::SetDefaults(GetMeshByName("Cube"), GetTerrainMaterialByName("Forest Terrain Material"));

	std::shared_ptr<Terrain> mainTerrain = CreateTerrainEntity("valley.raw16", GetTerrainMaterialByName("Forest Terrain Material"), "Basic Terrain");
	mainTerrain->GetTransform()->SetPosition(-256.0f, -14.0f, -256.0f);
}

void AssetManager::InitializeTerrainMaterials() {
	try {
		globalTerrainMaterials = std::vector<std::shared_ptr<TerrainMaterial>>();

		std::vector<std::shared_ptr<Material>> tMats = std::vector<std::shared_ptr<Material>>();

		std::shared_ptr<Material> forestMat = CreatePBRMaterial("Forest TMaterial", "forest_floor_albedo.png", "forest_floor_Normal-ogl.png", "wood_metal.png", "forest_floor_Roughness.png");
		std::shared_ptr<Material> bogMat = CreatePBRMaterial("Bog TMaterial", "bog_albedo.png", "bog_normal-ogl.png", "wood_metal.png", "bog_roughness.png");
		std::shared_ptr<Material> rockyMat = CreatePBRMaterial("Rocky TMaterial", "rocky_dirt1-albedo.png", "rocky_dirt1-normal-ogl.png", "wood_metal.png", "rocky_dirt1_Roughness.png");

		tMats.push_back(forestMat);
		tMats.push_back(bogMat);
		tMats.push_back(rockyMat);

		//Set appropriate tiling
		forestMat->SetTiling(10.0f);
		bogMat->SetTiling(10.0f);

		std::shared_ptr<TerrainMaterial> forestTerrainMaterial = CreateTerrainMaterial("Forest Terrain Material", tMats, "blendMap.png");

		tMats.clear();

		std::vector<std::string> metalPaths;
		std::vector<std::string> metalNames;

		// Must be kept in PBR order!
		// This sections is only for testing and should be commented on push.
		// Since these materials already exist, it's quicker and more efficient
		// to just grab them.
		std::shared_ptr<TerrainMaterial> industrialTerrainMaterial;

		/*metalPaths.push_back("floor_albedo.png");
		metalPaths.push_back("floor_normals.png");
		metalPaths.push_back("floor_metal.png");
		metalPaths.push_back("floor_roughness.png");

		metalPaths.push_back("cobblestone_albedo.png");
		metalPaths.push_back("cobblestone_normals.png");
		metalPaths.push_back("cobblestone_metal.png");
		metalPaths.push_back("cobblestone_roughness.png");

		metalPaths.push_back("rough_albedo.png");
		metalPaths.push_back("rough_normals.png");
		metalPaths.push_back("rough_metal.png");
		metalPaths.push_back("rough_roughness.png");

		metalNames.push_back("Floor");
		metalNames.push_back("Stone");
		metalNames.push_back("Rough");

		industrialTerrainMaterial = CreateTerrainMaterial("Industrial Terrain Material", metalPaths, metalNames, true, "blendMap.png");*/

		// This is the correct way to create a tMat when the materials are already loaded:
		tMats.push_back(GetMaterialByName("floorMat"));
		tMats.push_back(GetMaterialByName("cobbleMat"));
		tMats.push_back(GetMaterialByName("roughMat"));

		industrialTerrainMaterial = CreateTerrainMaterial("Industrial Terrain Material", tMats, "blendMap.png");

		// This is the correct way to load a tMat that doesn't use blend mapping
		// Note that even with one material, it must be pushed to the vector
		std::shared_ptr<TerrainMaterial> floorTerrainMaterial;

		tMats.clear();

		tMats.push_back(GetMaterialByName("terrainFloorMat"));

		floorTerrainMaterial = CreateTerrainMaterial("Floor Terrain Material", tMats);
	}
	catch (...) {
		SetLoadedAndWait("Terrain Materials", "Unknown Terrain Material", std::current_exception());

		return;
	}
}

void AssetManager::InitializeCameras() {
	globalCameras = std::vector<std::shared_ptr<Camera>>();

	float aspectRatio = (float)(dxInstance->width / dxInstance->height);
	CreateCamera("mainCamera", DirectX::XMFLOAT3(0.0f, 0.0f, -20.0f), aspectRatio, 1)->SetTag(CameraType::MAIN);
	std::shared_ptr<Camera> scTemp = CreateCamera("mainShadowCamera", DirectX::XMFLOAT3(0.0f, 20.0f, -200.0f), 1.0f, 0);
	std::shared_ptr<Camera> fscTemp = CreateCamera("flashShadowCamera", DirectX::XMFLOAT3(0.0f, 0.0f, -5.5f), 1.0f, 1);

	scTemp->SetTag(CameraType::MISC_SHADOW);
	scTemp->GetTransform()->SetRotation(-10.0f, 0.0f, 0.0f);

	fscTemp->SetTag(CameraType::MISC_SHADOW);
	fscTemp->GetTransform()->SetRotation(0, 0, 0);
}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void AssetManager::InitializeShaders() {
	vertexShaders = std::vector<std::shared_ptr<SimpleVertexShader>>();
	pixelShaders = std::vector<std::shared_ptr<SimplePixelShader>>();
	computeShaders = std::vector<std::shared_ptr<SimpleComputeShader>>();

	// Make vertex shaders
	CreateVertexShader("BasicVS", "VertexShader.cso");
	CreateVertexShader("NormalsVS", "VSNormalMap.cso");
	CreateVertexShader("SkyVS", "VSSkybox.cso");
	CreateVertexShader("TerrainVS", "VSTerrainBlend.cso");
	CreateVertexShader("ShadowVS", "VSShadowMap.cso");
	CreateVertexShader("ParticlesVS", "VSParticles.cso");
	CreateVertexShader("FullscreenVS", "FullscreenVS.cso");

	// Make pixel shaders
	CreatePixelShader("BasicPS", "PixelShader.cso");
	CreatePixelShader("SolidColorPS", "PSSolidColor.cso");
	CreatePixelShader("NormalsPS", "PSNormalMap.cso");
	CreatePixelShader("SkyPS", "PSSkybox.cso");
	CreatePixelShader("TerrainPS", "PSTerrainBlend.cso");
	CreatePixelShader("IrradiancePS", "IBLIrradianceMapPS.cso");
	CreatePixelShader("SpecularConvolutionPS", "IBLSpecularConvolutionPS.cso");
	CreatePixelShader("BRDFLookupTablePS", "IBLBrdfLookUpTablePS.cso");
	CreatePixelShader("SSAOPS", "PSAmbientOcclusion.cso");
	CreatePixelShader("SSAOBlurPS", "PSOcclusionBlur.cso");
	CreatePixelShader("SSAOCombinePS", "PSOcclusionCombine.cso");
	CreatePixelShader("ParticlesPS", "PSParticles.cso");
	CreatePixelShader("TextureSamplePS", "PSTextureSample.cso");
	CreatePixelShader("RefractivePS", "PSRefractive.cso");
	CreatePixelShader("OutlinePS", "PSSilhouette.cso");

	// Make compute shaders
	CreateComputeShader("ParticleMoveCS", "CSParticleFlow.cso");
	CreateComputeShader("ParticleEmitCS", "CSParticleEmit.cso");
	CreateComputeShader("ParticleCopyCS", "CSCopyDrawCount.cso");
	CreateComputeShader("ParticleInitDeadCS", "CSInitDeadList.cso");
}

void AssetManager::InitializeEmitters() {
	ParticleSystem::SetDefaults(
		GetPixelShaderByName("ParticlesPS"),
		GetVertexShaderByName("ParticlesVS"),
		LoadParticleTexture("Smoke/", true),
		GetComputeShaderByName("ParticleEmitCS"),
		GetComputeShaderByName("ParticleMoveCS"),
		GetComputeShaderByName("ParticleCopyCS"),
		GetComputeShaderByName("ParticleInitDeadCS"),
		device,
		context);

	std::shared_ptr<ParticleSystem> basicEmitter = CreateParticleEmitter("basicParticle", "Smoke/smoke_01.png", 20, 1.0f, 1.0f);
	basicEmitter->GetTransform()->SetPosition(XMFLOAT3(1.0f, 0.0f, 0.0f));
	basicEmitter->SetEnabled(false);

	std::shared_ptr<ParticleSystem> basicMultiEmitter = CreateParticleEmitter("basicParticles", "Smoke/", true);
	basicMultiEmitter->SetScale(1.0f);
	basicMultiEmitter->SetEnabled(false);

	std::shared_ptr<ParticleSystem> flameEmitter = CreateParticleEmitter("flameParticles", "Flame/", 10, 2.0f, 5.0f, true);
	flameEmitter->GetTransform()->SetPosition(XMFLOAT3(-1.0f, 0.0f, 0.0f));
	flameEmitter->SetColorTint(XMFLOAT4(0.8f, 0.3f, 0.2f, 1.0f));
	flameEmitter->SetEnabled(false);

	std::shared_ptr<ParticleSystem> starMultiEmitter = CreateParticleEmitter("starParticles", "Star/", 300, 2.0f, 80.0f, true);
	starMultiEmitter->GetTransform()->SetPosition(XMFLOAT3(-2.0f, 0.0f, 0.0f));
	starMultiEmitter->SetColorTint(XMFLOAT4(0.96f, 0.89f, 0.1f, 1.0f));
	starMultiEmitter->SetEnabled(false);

	std::shared_ptr<ParticleSystem> starEmitter = CreateParticleEmitter("starParticle", "Star/star_08.png", 300, 2.0f, 80.0f);
	starEmitter->GetTransform()->SetPosition(XMFLOAT3(-3.0f, 0.0f, 0.0f));
	starEmitter->SetColorTint(XMFLOAT4(0.96f, 0.89f, 0.1f, 1.0f));
	starEmitter->SetScale(0.75f);
	starEmitter->SetEnabled(false);

	// This was terrifying. Take graphics ideas from Jimmy Digrazia at your own peril.
	/*CreateParticleEmitter(10, 4.0f, 2.0f, DirectX::XMFLOAT3(-4.0f, 0.0f, 0.0f), L"Emoji/", "emojiParticles", true, false);
	GetEmitterByName("emojiParticles")->SetColorTint(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f));*/
}

void AssetManager::InitializeAudio() {
	audioInstance.Initialize();

	globalSounds = std::vector<FMOD::Sound*>();
	
	CreateSound("PianoNotes/pinkyfinger__piano-a.wav", FMOD_DEFAULT, "piano-a");
	CreateSound("PianoNotes/pinkyfinger__piano-b.wav", FMOD_DEFAULT, "piano-b");
	CreateSound("PianoNotes/pinkyfinger__piano-bb.wav", FMOD_DEFAULT, "piano-bb");
	CreateSound("PianoNotes/pinkyfinger__piano-c.wav", FMOD_DEFAULT, "piano-c");
	CreateSound("PianoNotes/pinkyfinger__piano-e.wav", FMOD_DEFAULT, "piano-e");
	CreateSound("PianoNotes/pinkyfinger__piano-eb.wav", FMOD_DEFAULT, "piano-eb");
	CreateSound("PianoNotes/pinkyfinger__piano-d.wav", FMOD_DEFAULT, "piano-d");
	CreateSound("PianoNotes/pinkyfinger__piano-f.wav", FMOD_DEFAULT, "piano-f");
	CreateSound("PianoNotes/pinkyfinger__piano-g.wav", FMOD_DEFAULT, "piano-g");
}

void AssetManager::InitializeFonts() {
	globalFonts = std::vector<std::shared_ptr<SHOEFont>>();

	CreateSHOEFont("Roboto-Bold-72pt", "RobotoCondensed-Bold-72pt.spritefont", true);
	CreateSHOEFont("SmoochSans-Bold", "SmoochSans-Bold.spritefont", true);
	CreateSHOEFont("SmoochSans-Italic", "SmoochSans-Italic.spritefont", true);
	CreateSHOEFont("Arial", "Arial.spritefont");
	CreateSHOEFont("Roboto-Bold", "RobotoCondensed-Bold.spritefont");
	CreateSHOEFont("Roboto-BoldItalic", "RobotoCondensed-BoldItalic.spritefont");
	CreateSHOEFont("Roboto-Italic", "RobotoCondensed-Italic.spritefont");
	CreateSHOEFont("Roboto-Regular", "RobotoCondensed-Regular.spritefont");
	CreateSHOEFont("SmoochSans-BoldItalic", "SmoochSans-BoldItalic.spritefont");
	CreateSHOEFont("SmoochSans-Regular", "SmoochSans-Regular.spritefont");
}

void AssetManager::InitializeIMGUI(HWND hwnd) {
	SetLoadingAndWait("UI", "Window Initialization");

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(device.Get(), context.Get());
}

void AssetManager::InitializeColliders() {
	std::shared_ptr<GameEntity> e = GetGameEntityByName("Bronze Cube");
	std::shared_ptr<GameEntity> e2 = GetGameEntityByName("Scratched Sphere");
	std::shared_ptr<GameEntity> e5 = GetGameEntityByName("Shiny Rough Sphere");
	std::shared_ptr<GameEntity> e6 = GetGameEntityByName("Floor Cube");
	std::shared_ptr<GameEntity> e7 = GetGameEntityByName("Scratched Cube");

	std::shared_ptr<Collider> c1 = AddColliderToGameEntity(e);
	std::shared_ptr<Collider> c2 = AddColliderToGameEntity(e2);
	std::shared_ptr<Collider> c5 = AddColliderToGameEntity(e5);
	std::shared_ptr<Collider> c6 = AddColliderToGameEntity(e6);
	std::shared_ptr<Collider> c7 = AddTriggerBoxToGameEntity(e7);
}
#pragma endregion

#pragma region addComponent

std::shared_ptr<Collider> AssetManager::AddColliderToGameEntity(OUT std::shared_ptr<GameEntity> entity) {
	try {
		SetLoadingAndWait("Colliders", "Generic Collider");

		std::shared_ptr<Collider> c = entity->AddComponent<Collider>();

		return c;
	}
	catch (...) {
		SetLoadedAndWait("Colliders", "Generic Collider", std::current_exception());

		return nullptr;
	}
}

std::shared_ptr<Collider> AssetManager::AddTriggerBoxToGameEntity(OUT std::shared_ptr<GameEntity> entity) {
	try {
		SetLoadingAndWait("Colliders", "Generic Trigger Box");

		std::shared_ptr<Collider> c = entity->AddComponent<Collider>();

		c->SetIsTrigger(true);

		return c;
	}
	catch (...) {
		SetLoadedAndWait("Colliders", "Generic Trigger Box", std::current_exception());

		return nullptr;
	}
}

#pragma endregion

#pragma region getMethods
Microsoft::WRL::ComPtr<ID3D11InputLayout> AssetManager::GetInputLayout() {
	return this->inputLayout;
}

Microsoft::WRL::ComPtr<ID3D11Device> AssetManager::GetDevice() {
	return this->device;
}

Microsoft::WRL::ComPtr<ID3D11DeviceContext> AssetManager::GetContext() {
	return this->context;
}

size_t AssetManager::GetPixelShaderArraySize() {
	return this->pixelShaders.size();
}

size_t AssetManager::GetVertexShaderArraySize() {
	return this->vertexShaders.size();
}

size_t AssetManager::GetComputeShaderArraySize() {
	return this->computeShaders.size();
}

size_t AssetManager::GetSkyArraySize() {
	return this->skies.size();
}

size_t AssetManager::GetCameraArraySize() {
	return this->globalCameras.size();
}

size_t AssetManager::GetMeshArraySize() {
	return this->globalMeshes.size();
}

size_t AssetManager::GetMaterialArraySize() {
	return this->globalMaterials.size();
}

size_t AssetManager::GetGameEntityArraySize() {
	return this->globalEntities.size();
}

size_t AssetManager::GetTerrainMaterialArraySize() {
	return this->globalTerrainMaterials.size();
}

size_t AssetManager::GetSoundArraySize() {
	return this->globalSounds.size();
}

FMOD::Sound* AssetManager::GetSoundAtID(int id) {
	return this->globalSounds[id];
}

std::shared_ptr<Camera> AssetManager::GetCameraAtID(int id) {
	return this->globalCameras[id];
}

std::shared_ptr<Sky> AssetManager::GetSkyAtID(int id) {
	return this->skies[id];
}

/// <summary>
/// Broadcasts a message to all entities
/// </summary>
/// <param name="event">Message type to send</param>
/// <param name="message">Body of the message, should one be needed</param>
void AssetManager::BroadcastGlobalEntityEvent(EntityEventType event, std::shared_ptr<void> message)
{
	for (std::shared_ptr<GameEntity> entity : globalEntities) {
		entity->PropagateEvent(event);
	}
}

std::shared_ptr<Material> AssetManager::GetMaterialAtID(int id) {
	return this->globalMaterials[id];
}

std::shared_ptr<Mesh> AssetManager::GetMeshAtID(int id) {
	return this->globalMeshes[id];
}

std::shared_ptr<SimpleVertexShader> AssetManager::GetVertexShaderAtID(int id) {
	return this->vertexShaders[id];
}

std::shared_ptr<SimplePixelShader> AssetManager::GetPixelShaderAtID(int id) {
	return this->pixelShaders[id];
}

std::shared_ptr<SimpleComputeShader> AssetManager::GetComputeShaderAtID(int id) {
	return this->computeShaders[id];
}

std::shared_ptr<GameEntity> AssetManager::GetGameEntityAtID(int id) {
	return this->globalEntities[id];
}

std::shared_ptr<TerrainMaterial> AssetManager::GetTerrainMaterialAtID(int id) {
	return this->globalTerrainMaterials[id];
}

std::shared_ptr<Camera> AssetManager::GetMainCamera() {
	for (auto ca : globalCameras) {
		if (ca->GetTag() == CameraType::MAIN) {
			return ca;
		}
	}
	return nullptr;
}

std::shared_ptr<Camera> AssetManager::GetPlayCamera() {
	for (auto ca : globalCameras) {
		if (ca->GetTag() == CameraType::PLAY) {
			return ca;
		}
	}
	return nullptr;
}

std::vector<std::shared_ptr<Camera>> AssetManager::GetCamerasByTag(CameraType type) {
	std::vector<std::shared_ptr<Camera>> cams;

	for (auto ca : globalCameras) {
		if (ca->GetTag() == type) {
			cams.push_back(ca);
		}
	}

	return cams;
}

/// <summary>
/// Safely sets volatile camera tags such as MAIN and PLAY.
/// If one of these is passed, whichever camera previously had the tag
/// will be swapped to Misc.
/// </summary>
/// <param name="cam"></param>
/// <param name="tag"></param>
void AssetManager::SetCameraTag(std::shared_ptr<Camera> cam, CameraType tag) {
	if (tag == CameraType::MAIN) {
		GetMainCamera()->SetTag(CameraType::MISC);
	}
	else if (tag == CameraType::PLAY) {
		GetPlayCamera()->SetTag(CameraType::MISC);
	}

	cam->SetTag(tag);
}
#pragma endregion

#pragma region buildAssetData
std::shared_ptr<Mesh> AssetManager::LoadTerrain(const char* filename, unsigned int mapWidth, unsigned int mapHeight, float heightScale) {

	std::string fullPath = GetFullPathToAssetFile(AssetPathIndex::ASSET_HEIGHTMAP_PATH, filename);

	unsigned int numVertices = mapWidth * mapHeight;
	unsigned int numIndices = (mapWidth - 1) * (mapHeight - 1) * 6;

	std::vector<unsigned short> heights(numVertices);
	std::vector<float> finalHeights(numVertices);

	std::vector<Vertex> vertices(numVertices);
	std::vector<unsigned int> indices(numIndices);
	std::vector<XMFLOAT3> triangleNormals;

	//Read the file
	std::ifstream file;
	file.open(fullPath.c_str(), std::ios_base::binary);

	if (file) {
		file.read((char*)&heights[0], (std::streamsize)numVertices * 2);
		file.close();
	}
	else {
		return nullptr;
	}

	int index = 0;
	int indexCounter = 0;
	for (int z = 0; z < (int)mapHeight; z++) {
		for (int x = 0; x < (int)mapWidth; x++) {
			//Get height map and positional data
			index = mapWidth * z + x;
			finalHeights[index] = (heights[index] / 65535.0f) * heightScale;

			XMFLOAT3 position = XMFLOAT3((float)x, finalHeights[index], (float)z);

			if (z != mapHeight - 1 && x != mapWidth - 1) {
				//Calculate indices
				int i0 = index;
				int i1 = index + mapWidth;
				int i2 = index + 1 + mapWidth;

				int i3 = index;
				int i4 = index + 1 + mapWidth;
				int i5 = index + 1;

				indices[indexCounter++] = i0;
				indices[indexCounter++] = i1;
				indices[indexCounter++] = i2;

				indices[indexCounter++] = i3;
				indices[indexCounter++] = i4;
				indices[indexCounter++] = i5;

				XMVECTOR pos0 = XMLoadFloat3(&vertices[i0].Position);
				XMVECTOR pos1 = XMLoadFloat3(&vertices[i1].Position);
				XMVECTOR pos2 = XMLoadFloat3(&vertices[i2].Position);

				XMVECTOR pos3 = XMLoadFloat3(&vertices[i3].Position);
				XMVECTOR pos4 = XMLoadFloat3(&vertices[i4].Position);
				XMVECTOR pos5 = XMLoadFloat3(&vertices[i5].Position);

				XMFLOAT3 normal0;
				XMFLOAT3 normal1;

				XMStoreFloat3(&normal0, XMVector3Normalize(XMVector3Cross(pos1 - pos0, pos2 - pos0)));
				XMStoreFloat3(&normal1, XMVector3Normalize(XMVector3Cross(pos4 - pos3, pos5 - pos3)));

				triangleNormals.push_back(normal0);
				triangleNormals.push_back(normal1);
			}

			//Calculate normals
			int triIndex = index * 2 - (2 * z);
			int triIndexPrevRow = triIndex - (mapWidth * 2 - 1);

			int normalCount = 0;
			XMVECTOR normalTotal = XMVectorSet(0, 0, 0, 0);

			//Diagram stolen from Chris's code because it's too useful not to have
			// x-----x-----x
			// |\    |\    |  
			// | \ u | \   |  
			// |  \  |  \  |  ul = up left
			// |   \ |   \ |  u  = up
			// | ul \| ur \|  ur = up right
			// x-----O-----x
			// |\ dl |\ dr |  dl = down left
			// | \   | \   |  d  = down
			// |  \  |  \  |  dr = down right
			// |   \ | d \ |
			// |    \|    \|
			// x-----x-----x

			if (z > 0 && x > 0)
			{
				// "Up left" and "up"
				normalTotal += XMLoadFloat3(&triangleNormals[triIndexPrevRow - 1]);
				normalTotal += XMLoadFloat3(&triangleNormals[triIndexPrevRow]);

				normalCount += 2;
			}

			if (z > 0 && x < (int)mapWidth - 1)
			{
				// "Up right"
				normalTotal += XMLoadFloat3(&triangleNormals[triIndexPrevRow + 1]);

				normalCount++;
			}

			if (z < (int)mapHeight - 1 && x > 0)
			{
				// "Down left"
				normalTotal += XMLoadFloat3(&triangleNormals[triIndex - 1]);

				normalCount++;
			}

			if (z < (int)mapHeight - 1 && x < (int)mapWidth - 1)
			{
				// "Down right" and "down"
				normalTotal += XMLoadFloat3(&triangleNormals[triIndex]);
				normalTotal += XMLoadFloat3(&triangleNormals[triIndex + 1]);

				normalCount += 2;
			}

			normalTotal /= normalCount;
			XMStoreFloat3(&vertices[index].normal, normalTotal);

			//Store data in vertex
			XMFLOAT3 normal = XMFLOAT3(+0.0f, +1.0f, -0.0f);
			XMFLOAT3 tangents = XMFLOAT3(+0.0f, +0.0f, +0.0f);
			XMFLOAT2 UV = XMFLOAT2(x / (float)mapWidth, z / (float)mapWidth);
			vertices[index] = { position, normal, tangents, UV };
		}
	}

	//Mesh handles tangents
	std::shared_ptr<Mesh> finalTerrain = std::make_shared<Mesh>(vertices.data(), numVertices, indices.data(), numIndices, device, "TerrainMesh");

	finalTerrain->SetFileNameKey(SerializeFileName("Assets\\HeightMaps\\", fullPath));
	globalMeshes.push_back(finalTerrain);
	Terrain::SetDefaults(finalTerrain, globalTerrainMaterials[0]);

	return finalTerrain;
}

// --------------------------------------------------------
// Loads six individual textures (the six faces of a cube map), then
// creates a blank cube map and copies each of the six textures to
// another face. Afterwards, creates a shader resource view for
// the cube map and cleans up all of the temporary resources.
// --------------------------------------------------------
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> AssetManager::CreateCubemap(
	const wchar_t* right,
	const wchar_t* left,
	const wchar_t* up,
	const wchar_t* down,
	const wchar_t* front,
	const wchar_t* back)
{
	// Load the 6 textures into an array.
	// - We need references to the TEXTURES, not the SHADER RESOURCE VIEWS!
	// - Specifically NOT generating mipmaps, as we usually don't need them for the sky!
	// - Order matters here! +X, -X, +Y, -Y, +Z, -Z
	ID3D11Texture2D* textures[6] = {};
	CreateWICTextureFromFile(device.Get(), right, (ID3D11Resource**)&textures[0], 0);
	CreateWICTextureFromFile(device.Get(), left, (ID3D11Resource**)&textures[1], 0);
	CreateWICTextureFromFile(device.Get(), up, (ID3D11Resource**)&textures[2], 0);
	CreateWICTextureFromFile(device.Get(), down, (ID3D11Resource**)&textures[3], 0);
	CreateWICTextureFromFile(device.Get(), front, (ID3D11Resource**)&textures[4], 0);
	CreateWICTextureFromFile(device.Get(), back, (ID3D11Resource**)&textures[5], 0);
	// We'll assume all of the textures are the same color format and resolution,
	// so get the description of the first shader resource view
	D3D11_TEXTURE2D_DESC faceDesc = {};
	textures[0]->GetDesc(&faceDesc);
	// Describe the resource for the cube map, which is simply
	// a "texture 2d array". This is a special GPU resource format,
	// NOT just a C++ array of textures!!!
	D3D11_TEXTURE2D_DESC cubeDesc = {};
	cubeDesc.ArraySize = 6; // Cube map!
	cubeDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // We'll be using as a texture in a shader
	cubeDesc.CPUAccessFlags = 0; // No read back
	cubeDesc.Format = faceDesc.Format; // Match the loaded texture's color format
	cubeDesc.Width = faceDesc.Width; // Match the size
	cubeDesc.Height = faceDesc.Height; // Match the size
	cubeDesc.MipLevels = 1; // Only need 1
	cubeDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE; // A CUBE MAP, not 6 separate textures
	cubeDesc.Usage = D3D11_USAGE_DEFAULT; // Standard usage
	cubeDesc.SampleDesc.Count = 1;
	cubeDesc.SampleDesc.Quality = 0;
	// Create the actual texture resource
	ID3D11Texture2D* cubeMapTexture = 0;
	device->CreateTexture2D(&cubeDesc, 0, &cubeMapTexture);
	// Loop through the individual face textures and copy them,
	// one at a time, to the cube map texure
	for (int i = 0; i < 6; i++)
	{
		// Calculate the subresource position to copy into
		unsigned int subresource = D3D11CalcSubresource(
			0, // Which mip (zero, since there's only one)
			i, // Which array element?
			1); // How many mip levels are in the texture?
			// Copy from one resource (texture) to another
		context->CopySubresourceRegion(
			cubeMapTexture, // Destination resource
			subresource, // Dest subresource index (one of the array elements)
			0, 0, 0, // XYZ location of copy
			textures[i], // Source resource
			0, // Source subresource index (we're assuming there's only one)
			0); // Source subresource "box" of data to copy (zero means the whole thing)
	}
	// At this point, all of the faces have been copied into the
	// cube map texture, so we can describe a shader resource view for it
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = cubeDesc.Format; // Same format as texture
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE; // Treat this as a cube!
	srvDesc.TextureCube.MipLevels = 1; // Only need access to 1 mip
	srvDesc.TextureCube.MostDetailedMip = 0; // Index of the first mip we want to see
	// Make the SRV
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cubeSRV;
	device->CreateShaderResourceView(cubeMapTexture, &srvDesc, cubeSRV.GetAddressOf());
	// Now that we're done, clean up the stuff we don't need anymore
	cubeMapTexture->Release(); // Done with this particular reference (the SRV has another)
	for (int i = 0; i < 6; i++)
		textures[i]->Release();
	// Send back the SRV, which is what we need for our shaders
	return cubeSRV;
}

/// <summary>
/// Loads a particle texture or set of particle textures
/// </summary>
/// <param name="textureNameToLoad">Name of the file or file path to load the texture(s) from</param>
/// <param name="isMultiParticle">True to recursively load from the file path</param>
/// <returns>An SRV for the loaded textures</returns>
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> AssetManager::LoadParticleTexture(std::string textureNameToLoad, bool isMultiParticle)
{
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleTextureSRV;

	if (isMultiParticle) {
		// Load all particle textures in a specific subfolder
		std::string assets = GetFullPathToAssetFile(AssetPathIndex::ASSET_PARTICLE_PATH, textureNameToLoad);

		std::vector<Microsoft::WRL::ComPtr<ID3D11Texture2D>> textures;
		int i = 0;
		for (auto& p : std::experimental::filesystem::recursive_directory_iterator(assets)) {
			textures.push_back(nullptr);
			std::wstring path = L"";
			ISimpleShader::ConvertToWide(p.path().string().c_str(), path);

			CreateWICTextureFromFile(device.Get(), context.Get(), (path).c_str(), (ID3D11Resource**)textures[i].GetAddressOf(), nullptr);

			i++;
		}

		D3D11_TEXTURE2D_DESC faceDesc = {};
		textures[0]->GetDesc(&faceDesc);

		D3D11_TEXTURE2D_DESC multiTextureDesc = {};
		multiTextureDesc.ArraySize = (int)textures.size();
		multiTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		multiTextureDesc.CPUAccessFlags = 0;
		multiTextureDesc.Format = faceDesc.Format;
		multiTextureDesc.Width = faceDesc.Width;
		multiTextureDesc.Height = faceDesc.Height;
		multiTextureDesc.MipLevels = 1;
		multiTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		multiTextureDesc.SampleDesc.Count = 1;
		multiTextureDesc.SampleDesc.Quality = 0;

		ID3D11Texture2D* outputTexture;
		device->CreateTexture2D(&multiTextureDesc, 0, &outputTexture);

		for (int i = 0; i < (int)textures.size(); i++) {
			unsigned int subresource = D3D11CalcSubresource(0, i, 1);

			if (textures[i] != nullptr) {
				context->CopySubresourceRegion(outputTexture, subresource, 0, 0, 0, (ID3D11Resource*)textures[i].Get(), 0, 0);
			}
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = multiTextureDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MipLevels = 1;
		srvDesc.Texture2DArray.ArraySize = (int)textures.size();

		device->CreateShaderResourceView(outputTexture, &srvDesc, particleTextureSRV.GetAddressOf());

		outputTexture->Release();
	}
	else {
		std::wstring wAssetString;
		ISimpleShader::ConvertToWide(dxInstance->GetAssetPathString(ASSET_PARTICLE_PATH) + textureNameToLoad, wAssetString);

		CreateWICTextureFromFile(device.Get(), context.Get(), wAssetString.c_str(), nullptr, &particleTextureSRV);
	}

	return particleTextureSRV;
}
#pragma endregion

#pragma region complexModels
void AssetManager::CreateComplexGeometry() {
	Assimp::Importer importer;
	std::vector<std::shared_ptr<Material>> specialMaterials = std::vector<std::shared_ptr<Material>>();
	std::string namePath = GetFullPathToAssetFile(AssetPathIndex::ASSET_MODEL_PATH, "human.obj");
	std::string serializedKey = SerializeFileName("Assets\\Models\\", namePath);

	const aiScene* flashLightModel = importer.ReadFile(dxInstance->GetFullPathTo("..\\..\\..\\Assets\\Models\\human.obj").c_str(),
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_GenNormals |
		aiProcess_ConvertToLeftHanded |
		aiProcess_CalcTangentSpace);

	if (flashLightModel != NULL) {
		ProcessComplexModel(flashLightModel->mRootNode, flashLightModel, serializedKey, "Human");
	}

	namePath = GetFullPathToAssetFile(AssetPathIndex::ASSET_MODEL_PATH, "hat.obj");
	serializedKey = SerializeFileName("Assets\\Models\\", namePath);

	const aiScene* hatModel = importer.ReadFile(dxInstance->GetFullPathTo("..\\..\\..\\Assets\\Models\\hat.obj").c_str(),
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_GenNormals |
		aiProcess_ConvertToLeftHanded |
		aiProcess_CalcTangentSpace);

	if (hatModel != NULL) {
		ProcessComplexModel(hatModel->mRootNode, hatModel, serializedKey, "Hat");
	}
	
}

void AssetManager::ProcessComplexModel(aiNode* node, const aiScene* scene, std::string serializedFilenameKey, std::string name) {
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		mesh->mName = name + "CM" + std::to_string(i);

		std::shared_ptr<Mesh> newMesh = ProcessComplexMesh(mesh, scene);
		std::shared_ptr<GameEntity> newEntity = CreateGameEntity(newMesh, GetMaterialByName("cobbleMat"), mesh->mName.C_Str());;

		globalMeshes.push_back(newMesh);
		
		newEntity->GetTransform()->SetPosition(0.0f, 3.0f * i, 1.0f);
		newEntity->GetTransform()->SetScale(0.25f, 0.25f, 0.25f);

		newMesh->SetFileNameKey(serializedFilenameKey);
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		ProcessComplexModel(node->mChildren[i], scene, serializedFilenameKey, name + "Child" + std::to_string(i));
	}
}

std::shared_ptr<Mesh> AssetManager::ProcessComplexMesh(aiMesh* mesh, const aiScene* scene) {
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	bool hasTangents = true;

	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		Vertex tempV;

		//Process position
		tempV.Position.x = mesh->mVertices[i].x;
		tempV.Position.y = mesh->mVertices[i].y;
		tempV.Position.z = mesh->mVertices[i].z;

		//Process normals
		tempV.normal.x = mesh->mNormals[i].x;
		tempV.normal.y = mesh->mNormals[i].y;
		tempV.normal.z = mesh->mNormals[i].z;

		//Process tangents
		if (mesh->mTangents != NULL) {
			tempV.Tangent.x = mesh->mTangents[i].x;
			tempV.Tangent.y = mesh->mTangents[i].y;
			tempV.Tangent.z = mesh->mTangents[i].z;
		}
		else {
			hasTangents = false;
		}

		//Process UV coords
		for (int j = 0; j < 8; j++) {
			if (mesh->mNumUVComponents[j] > 0 && mesh->mTextureCoords[j] != NULL) {
				tempV.uv.x = mesh->mTextureCoords[0]->x;
				tempV.uv.y = mesh->mTextureCoords[0]->y;
			}
			else {
				tempV.uv.x = 0;
				tempV.uv.y = 0;
				break;
			}
		}

		vertices.push_back(tempV);
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}

	//Old version of assimp doesn't match with material tutorial; implement manually?
	//if (mesh->mMaterialIndex >= 0) {
		//aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		//loadMaterial
	//}

	if (hasTangents) return std::make_shared<Mesh>(vertices.data(), vertices.size(), indices.data(), indices.size(), 0, device, mesh->mName.C_Str() + std::string("Mesh"));
	else return std::make_shared<Mesh>(vertices.data(), vertices.size(), indices.data(), indices.size(), device);
}
#pragma endregion

#pragma region removeAssets

//
// Asset Removal methods
//

void AssetManager::CleanAllVectors() {
	for (auto ge : globalEntities) {
		ge->Release();
	}

	for (int i = 0; i < globalSounds.size(); i++) {
		FMODUserData* uData;
		globalSounds[i]->getUserData((void**)&uData);
		uData->filenameKey.reset();
		uData->name.reset();
		delete uData;
	}

	pixelShaders.clear();
	vertexShaders.clear();
	computeShaders.clear();
	skies.clear();
	globalCameras.clear();
	globalMeshes.clear();
	globalMaterials.clear();
	globalEntities.clear();
	globalTerrainMaterials.clear();
	globalSounds.clear();
	globalFonts.clear();
	textureSampleStates.clear();
	textureState = nullptr;
	clampState = nullptr;

	context->Flush();
}

void AssetManager::RemoveGameEntity(std::string name) {
	RemoveGameEntity(GetGameEntityIDByName(name));
}

void AssetManager::RemoveGameEntity(int id) {
	globalEntities[id]->Release();
	globalEntities.erase(globalEntities.begin() + id);
}

void AssetManager::RemoveSky(std::string name) {
	skies.erase(skies.begin() + GetSkyIDByName(name));
}

void AssetManager::RemoveSky(int id) {
	skies.erase(skies.begin() + id);
}

void AssetManager::RemoveVertexShader(std::string name) {
	vertexShaders.erase(vertexShaders.begin() + GetVertexShaderIDByName(name));
}

void AssetManager::RemoveVertexShader(int id) {
	vertexShaders.erase(vertexShaders.begin() + id);
}

void AssetManager::RemovePixelShader(std::string name) {
	pixelShaders.erase(pixelShaders.begin() + GetPixelShaderIDByName(name));
}

void AssetManager::RemovePixelShader(int id) {
	pixelShaders.erase(pixelShaders.begin() + id);
}

void AssetManager::RemoveMesh(std::string name) {
	globalMeshes.erase(globalMeshes.begin() + GetMeshIDByName(name));
}

void AssetManager::RemoveMesh(int id) {
	globalMeshes.erase(globalMeshes.begin() + id);
}

void AssetManager::RemoveCamera(std::string name) {
	globalCameras.erase(globalCameras.begin() + GetCameraIDByName(name));
}

void AssetManager::RemoveCamera(int id) {
	globalCameras.erase(globalCameras.begin() + id);
}

void AssetManager::RemoveMaterial(std::string name) {
	globalMaterials.erase(globalMaterials.begin() + GetMaterialIDByName(name));
}

void AssetManager::RemoveMaterial(int id) {
	globalMaterials.erase(globalMaterials.begin() + id);
}
/*
void AssetManager::RemoveTerrainMaterial(std::string name) {
	globalEntities.erase(globalEntities.begin() + GetGameEntityIDByName(name));
}

void AssetManager::RemoveTerrainMaterial(int id) {
	globalEntities.erase(globalEntities.begin() + id);
}
*/
#pragma endregion

#pragma region enableDisableAssets

//
// Enables or disables assets into or
// out of the rendering pipeline
//

void AssetManager::EnableDisableCamera(std::string name, bool value) {
	GetCameraByName(name)->SetEnableDisable(value);
}

void AssetManager::EnableDisableCamera(int id, bool value) {
	globalCameras[id]->SetEnableDisable(value);
}

#pragma endregion

#pragma region nameSearch

//
// Asset Search-By-Name methods
//

std::shared_ptr<GameEntity> AssetManager::GetGameEntityByName(std::string name) {
	for (int i = 0; i < globalEntities.size(); i++) {
		if (globalEntities[i]->GetName() == name) {
			return globalEntities[i];
		}
	}
	return nullptr;
}

std::shared_ptr<Sky> AssetManager::GetSkyByName(std::string name) {
	for (int i = 0; i < skies.size(); i++) {
		if (skies[i]->GetName() == name) {
			return skies[i];
		}
	}
	return nullptr;
}

std::shared_ptr<SimpleVertexShader> AssetManager::GetVertexShaderByName(std::string name) {
	for (int i = 0; i < vertexShaders.size(); i++) {
		if (vertexShaders[i]->GetName() == name) {
			return vertexShaders[i];
		}
	}
	return nullptr;
}

std::shared_ptr<SimplePixelShader> AssetManager::GetPixelShaderByName(std::string name) {
	for (int i = 0; i < pixelShaders.size(); i++) {
		if (pixelShaders[i]->GetName() == name) {
			return pixelShaders[i];
		}
	}
	return nullptr;
}

std::shared_ptr<SimpleComputeShader> AssetManager::GetComputeShaderByName(std::string name) {
	for (int i = 0; i < computeShaders.size(); i++) {
		if (computeShaders[i]->GetName() == name) {
			return computeShaders[i];
		}
	}
	return nullptr;
}

std::shared_ptr<Mesh> AssetManager::GetMeshByName(std::string name) {
	for (int i = 0; i < globalMeshes.size(); i++) {
		if (globalMeshes[i]->GetName() == name) {
			return globalMeshes[i];
		}
	}
	return nullptr;
}

std::shared_ptr<Camera> AssetManager::GetCameraByName(std::string name) {
	for (int i = 0; i < globalCameras.size(); i++) {
		if (globalCameras[i]->GetName() == name) {
			return globalCameras[i];
		}
	}
	return nullptr;
}

std::shared_ptr<Material> AssetManager::GetMaterialByName(std::string name) {
	for (int i = 0; i < globalMaterials.size(); i++) {
		if (globalMaterials[i]->GetName() == name) {
			return globalMaterials[i];
		}
	}
	return nullptr;
}

std::shared_ptr<TerrainMaterial> AssetManager::GetTerrainMaterialByName(std::string name) {
	for (int i = 0; i < globalTerrainMaterials.size(); i++) {
		if (globalTerrainMaterials[i]->GetName() == name) {
			return globalTerrainMaterials[i];
		}
	}
	return nullptr;
}

//
// Dict return by key
//

std::shared_ptr<SHOEFont> AssetManager::GetFontByName(std::string name) {
	for (int i = 0; i < globalFonts.size(); i++) {
		if (globalFonts[i]->name == name) {
			return globalFonts[i];
		}
	}
	return nullptr;
}
#pragma endregion

#pragma region getAssetID

//
// These methods return the location of the entity in the vector
//

int AssetManager::GetGameEntityIDByName(std::string name) {
	for (int i = 0; i < globalEntities.size(); i++) {
		if (globalEntities[i]->GetName() == name) {
			return i;
		}
	}
	return -1;
}

int AssetManager::GetSkyIDByName(std::string name) {
	for (int i = 0; i < skies.size(); i++) {
		if (skies[i]->GetName() == name) {
			return i;
		}
	}
	return -1;
}

int AssetManager::GetVertexShaderIDByName(std::string name) {
	for (int i = 0; i < vertexShaders.size(); i++) {
		if (vertexShaders[i]->GetName() == name) {
			return i;
		}
	}
	return -1;
}

int AssetManager::GetPixelShaderIDByName(std::string name) {
	for (int i = 0; i < pixelShaders.size(); i++) {
		if (pixelShaders[i]->GetName() == name) {
			return i;
		}
	}
	return -1;
}

int AssetManager::GetMeshIDByName(std::string name) {
	for (int i = 0; i < globalMeshes.size(); i++) {
		if (globalMeshes[i]->GetName() == name) {
			return i;
		}
	}
	return -1;
}

int AssetManager::GetCameraIDByName(std::string name) {
	for (int i = 0; i < globalCameras.size(); i++) {
		if (globalCameras[i]->GetName() == name) {
			return i;
		}
	}
	return -1;
}

int AssetManager::GetMaterialIDByName(std::string name) {
	for (int i = 0; i < globalMaterials.size(); i++) {
		if (globalMaterials[i]->GetName() == name) {
			return i;
		}
	}
	return -1;
}

int AssetManager::GetPixelShaderIDByPointer(std::shared_ptr<SimplePixelShader> pixelPointer) {
	for (int i = 0; i < pixelShaders.size(); i++) {
		if (pixelPointer == pixelShaders[i]) {
			return i;
		}
	}
	return -1;
}

int AssetManager::GetVertexShaderIDByPointer(std::shared_ptr<SimpleVertexShader> vertexPointer) {
	for (int i = 0; i < vertexShaders.size(); i++) {
		if (vertexPointer == vertexShaders[i]) {
			return i;
		}
	}
	return -1;
}

// Same as above
//int AssetManager::GetTerrainMaterialIDByName(std::string name) {
//	for (int i = 0; i < globalTerrainMaterials.size(); i++) {
//		if (globalTerrainMaterials[i]->GetName() == name) {
//			return i;
//		}
//	}
//	return -1;
//}

std::string AssetManager::GetFullPathToAssetFile(AssetPathIndex index, std::string filename) {
	std::string asset = dxInstance->GetAssetPathString(index) + filename;
	char pathBuf[1024];
	GetFullPathNameA(asset.c_str(), sizeof(pathBuf), pathBuf, NULL);
	return pathBuf;
}

std::string AssetManager::GetFullPathToExternalAssetFile(std::string filename) {
	char pathBuf[1024];
	GetFullPathNameA(filename.c_str(), sizeof(pathBuf), pathBuf, NULL);
	return pathBuf;
}

#pragma endregion
