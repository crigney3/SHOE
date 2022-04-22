#include "../Headers/AssetManager.h"

using namespace DirectX;

#pragma region assetClassHandlers
AssetManager* AssetManager::instance;

AssetManager::~AssetManager() {
	// Everything should be smart-pointer managed
	// Except sounds
	globalSounds.clear();

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
	InitializeTerrainMaterials();
	InitializeGameEntities();
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
	return "";
}

void AssetManager::LoadScene(std::string filepath) {
	rapidjson::Document sceneDoc;

	std::string fullPath = "../../../Assets/Scenes/" + filepath;
	fullPath = dxInstance->GetFullPathTo(fullPath);
	FILE* file;
	fopen_s(&file, fullPath.c_str(), "rb");

	char readBuffer[FILE_BUFFER_SIZE];
	rapidjson::FileReadStream sceneFileStream(file, readBuffer, sizeof(readBuffer));

	sceneDoc.ParseStream(sceneFileStream);

	fclose(file);
}

void AssetManager::LoadScene(FILE* file) {
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
			for (auto co : ge->GetAllComponents()) {
				rapidjson::Value coValue(rapidjson::kObjectType);

				// Currently this value uses full descriptors.
				// For later optimization, make these strings very short
				// while maintaining unique ids.
				// Additional memory optimization could be achieved by making
				// Member strings unique to only one section, allowing for 
				// super short keys like 'lt' for lightType
				// May be worth using #define so it's still readable
				rapidjson::Value componentType;

				// Is it a Light?
				if (std::dynamic_pointer_cast<Light>(co) != nullptr) {
					std::shared_ptr<Light> light = std::dynamic_pointer_cast<Light>(co);
					componentType.SetString("Light");
					coValue.AddMember(COMPONENT_TYPE, componentType, allocator);

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
				if (std::dynamic_pointer_cast<Collider>(co) != nullptr) {
					componentType.SetString("Collider");
					coValue.AddMember(COMPONENT_TYPE, componentType, allocator);
					std::shared_ptr<Collider> collider = std::dynamic_pointer_cast<Collider>(co);

					coValue.AddMember(COLLIDER_TYPE, collider->GetTriggerStatus(), allocator);
					coValue.AddMember(COLLIDER_ENABLED, collider->GetEnabledStatus(), allocator);
					coValue.AddMember(COLLIDER_IS_VISIBLE, collider->GetVisibilityStatus(), allocator);
					coValue.AddMember(COLLIDER_IS_TRANSFORM_VISIBLE, collider->GetTransformVisibilityStatus(), allocator);
				}

				// Is it Terrain?
				if (std::dynamic_pointer_cast<Terrain>(co) != nullptr) {
					componentType.SetString("Terrain");
					coValue.AddMember(COMPONENT_TYPE, componentType, allocator);

					// Terrain is static rn, can't be saved or loaded for the moment
					// TODO: Stop storing terrain like that, allowing for multiple terrains
				}

				// Is it a Particle System?
				if (std::dynamic_pointer_cast<ParticleSystem>(co) != nullptr) {
					componentType.SetString("ParticleSystem");
					coValue.AddMember(COMPONENT_TYPE, componentType, allocator);
				}

				// Is it a MeshRenderer?
				if (std::dynamic_pointer_cast<MeshRenderer>(co) != nullptr) {
					componentType.SetString("MeshRenderer");
					coValue.AddMember(COMPONENT_TYPE, componentType, allocator);

					std::shared_ptr<MeshRenderer> meshRenderer = std::dynamic_pointer_cast<MeshRenderer>(co);

					// Mesh Renderers are really just storage for a Mesh
					// and a Material, so store those in an object

					{
						// Mesh
						rapidjson::Value meshValue(rapidjson::kObjectType);
						std::shared_ptr<Mesh> mesh = meshRenderer->GetMesh();

						// Simple types first
						meshValue.AddMember(MESH_INDEX_COUNT, mesh->GetIndexCount(), allocator);
						meshValue.AddMember(MESH_MATERIAL_INDEX, mesh->GetMaterialIndex(), allocator);
						meshValue.AddMember(MESH_ENABLED, mesh->GetEnableDisable(), allocator);
						meshValue.AddMember(MESH_NEEDS_DEPTH_PREPASS, mesh->GetDepthPrePass(), allocator);

						// Strings
						rapidjson::Value meshName;
						meshName.SetString(mesh->GetName().c_str(), allocator);
						meshValue.AddMember(MESH_NAME, meshName, allocator);

						// Complex data - filename string key
						rapidjson::Value fileKeyValue;
						fileKeyValue.SetString(mesh->GetFileNameKey().c_str(), allocator);

						meshValue.AddMember(MESH_FILENAME_KEY, fileKeyValue, allocator);

						coValue.AddMember(MESH_OBJECT, meshValue, allocator);
					}

					{
						// Material
						rapidjson::Value matValue(rapidjson::kObjectType);
						std::shared_ptr<Material> mat = meshRenderer->GetMaterial();

						// Simple types first
						matValue.AddMember(MAT_UV_TILING, mat->GetTiling(), allocator);
						matValue.AddMember(MAT_ENABLED, mat->GetEnableDisable(), allocator);
						matValue.AddMember(MAT_IS_TRANSPARENT, mat->GetTransparent(), allocator);
						matValue.AddMember(MAT_IS_REFRACTIVE, mat->GetRefractive(), allocator);
						matValue.AddMember(MAT_INDEX_OF_REFRACTION, mat->GetIndexOfRefraction(), allocator);
						matValue.AddMember(MAT_REFRACTION_SCALE, mat->GetRefractionScale(), allocator);

						// String types
						rapidjson::Value matName;
						rapidjson::Value pixShader;
						rapidjson::Value vertShader;
						rapidjson::Value refractivePixShader;

						matName.SetString(mat->GetName().c_str(), allocator);
						pixShader.SetString(mat->GetPixShader()->GetFileNameKey().c_str(), allocator);
						vertShader.SetString(mat->GetVertShader()->GetFileNameKey().c_str(), allocator);

						matValue.AddMember(MAT_NAME, matName, allocator);
						matValue.AddMember(MAT_PIXEL_SHADER, pixShader, allocator);
						matValue.AddMember(MAT_VERTEX_SHADER, vertShader, allocator);

						if (mat->GetRefractive()) {
							refractivePixShader.SetString(mat->GetRefractivePixelShader()->GetFileNameKey().c_str(), allocator);
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

						// Add everything to the component
						coValue.AddMember(MATERIAL_OBJECT, matValue, allocator);
					}
				}

				// Is it a Transform?
				if (std::dynamic_pointer_cast<Transform>(co) != nullptr) {
					componentType.SetString("Transform");
					coValue.AddMember(COMPONENT_TYPE, componentType, allocator);
					std::shared_ptr<Transform> transform = std::dynamic_pointer_cast<Transform>(co);

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

					coValue.AddMember(TRANSFORM_LOCAL_POSITION, pos, allocator);
					coValue.AddMember(TRANSFORM_LOCAL_SCALE, scale, allocator);
					coValue.AddMember(TRANSFORM_LOCAL_ROTATION, rot, allocator);
				}

				geComponents.PushBack(coValue, allocator);
			}

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
			skyFilenameKey.SetString(sy->GetFileExtension().c_str(), allocator);

			skyObject.AddMember(SKY_NAME, skyName, allocator);
			skyObject.AddMember(SKY_FILENAME_KEY_TYPE, sy->GetFilenameKeyType(), allocator);
			skyObject.AddMember(SKY_FILENAME_KEY, skyFilenameKey, allocator);
			skyObject.AddMember(SKY_FILENAME_EXTENSION, skyFilenameExtension, allocator);

			skyBlock.PushBack(skyObject, allocator);
		}

		sceneDocToSave.AddMember(SKIES, skyBlock, allocator);

		rapidjson::Value soundBlock(rapidjson::kArrayType);
		for (auto so : globalSounds) {
			rapidjson::Value soundObject(rapidjson::kObjectType);
			FMODUserData* uData;
			FMOD_MODE sMode;

			FMOD_RESULT uDataResult = so->getUserData((void**)&uData);

#if defined(DEBUG) || defined(_DEBUG)
			if (uDataResult != FMOD_OK) {
				printf("Failed to save sound with user data error!");
			}	
#endif

			uDataResult = so->getMode(&sMode);

#if defined(DEBUG) || defined(_DEBUG)
			if (uDataResult != FMOD_OK) {
				printf("Failed to save sound with mode error!");
			}
#endif

			rapidjson::Value soundFK;
			rapidjson::Value soundN;

			soundFK.SetString(uData->filenameKey.c_str(), allocator);
			soundN.SetString(uData->name.c_str(), allocator);

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

			terrainMatObj.AddMember(TERRAIN_MATERIAL_ENABLED, tm->GetEnableDisable(), allocator);
			terrainMatObj.AddMember(TERRAIN_MATERIAL_BLEND_MAP_ENABLED, tm->GetUsingBlendMap(), allocator);
			terrainMatObj.AddMember(TERRAIN_MATERIAL_NAME, tmName, allocator);
			terrainMatObj.AddMember(TERRAIN_MATERIAL_BLEND_MAP_PATH, tmBlendPath, allocator);

			// The internal materials are already tracked as regular materials,
			// So we just need an array of pointers to them
			rapidjson::Value terrainInternalMats(rapidjson::kArrayType);
			for (int i = 0; i < tm->GetMaterialCount(); i++) {
				// GUIDs aren't implemented yet, so store names for now
				rapidjson::Value matName;
				matName.SetString(tm->GetMaterialAtID(i)->GetName().c_str(), allocator);

				terrainInternalMats.PushBack(matName, allocator);
			}
			terrainMatObj.AddMember(TERRAIN_MATERIAL_MATERIAL_ARRAY, terrainInternalMats, allocator);

			terrainMatBlock.PushBack(terrainMatObj, allocator);
		}
		
		sceneDocToSave.AddMember(TERRAIN_MATERIALS, terrainMatBlock, allocator);

		// At the end of gathering data, write it all
		// to the appropriate file
		std::string assetPath;

		assetPath = dxInstance->GetAssetPathString(ASSET_SCENE_PATH);
		std::string namePath = assetPath + filepath;
		char pathBuf[1024];

		GetFullPathNameA(namePath.c_str(), sizeof(pathBuf), pathBuf, NULL);

		FILE* file;
		fopen_s(&file, pathBuf, "w");

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
		std::shared_ptr<FMODUserData> uData = std::make_shared<FMODUserData>();
		FMOD::Sound* sound;
		std::string assetPath;

		assetPath = dxInstance->GetAssetPathString(ASSET_SOUND_PATH);
		std::string namePath = assetPath + path;
		char pathBuf[1024];

		GetFullPathNameA(namePath.c_str(), sizeof(pathBuf), pathBuf, NULL);

		sound = audioInstance.LoadSound(pathBuf, mode);

		// Serialize the filename if it's in the right folder
		std::string baseFilename = "";
		std::string assetPathStr = "Assets\\Sounds";
		std::string pathBufString = std::string(pathBuf);
		size_t dirPos = pathBufString.find(assetPathStr);
		if (dirPos != std::string::npos) {
			// File is in the assets folder
			baseFilename = "t";
			baseFilename += pathBufString.substr(dirPos + sizeof(assetPathStr));
		}
		else {
			baseFilename = "f";
			baseFilename += pathBufString;
		}

		uData->filenameKey = baseFilename;
		uData->name = name;

		// On getUserData, we will receive the whole struct
		sound->setUserData(uData.get());

		globalSounds.push_back(sound);

		SetLoadedAndWait("Sounds", path);

		return sound;
	}
	catch (...) {
		SetLoadedAndWait("Sounds", path, std::current_exception());

		return NULL;
	}
}

std::shared_ptr<Camera> AssetManager::CreateCamera(std::string id, DirectX::XMFLOAT3 pos, float aspectRatio, int type) {
	try {
		std::shared_ptr<Camera> newCam;

		newCam = std::make_shared<Camera>(pos, aspectRatio, type, id);

		this->globalCameras.push_back(newCam);

		SetLoadedAndWait("Cameras", id);

		return newCam;
	}
	catch (...) {
		SetLoadedAndWait("Camera", id, std::current_exception());

		return NULL;
	}
	
}

std::shared_ptr<SimpleVertexShader> AssetManager::CreateVertexShader(std::string id, std::string nameToLoad) {
	try {
		std::shared_ptr<SimpleVertexShader> newVS;
		std::string assetPath;

		assetPath = dxInstance->GetAssetPathString(ASSET_SHADER_PATH);
		std::string namePath = assetPath + nameToLoad;
		char pathBuf[1024];

		GetFullPathNameA(namePath.c_str(), sizeof(pathBuf), pathBuf, NULL);

		newVS = std::make_shared<SimpleVertexShader>(device.Get(), context.Get(), pathBuf, id);

		// Serialize the filename if it's in the right folder
		std::string baseFilename = "";
		std::string assetPathStr = "Assets\\Shaders";
		size_t dirPos = nameToLoad.find(assetPathStr);
		if (dirPos != std::string::npos) {
			// File is in the assets folder
			baseFilename = "t";
			baseFilename += nameToLoad.substr(dirPos + sizeof(assetPathStr));
		}
		else {
			baseFilename = "f";
			baseFilename += nameToLoad;
		}

		newVS->SetFileNameKey(baseFilename);

		vertexShaders.push_back(newVS);

		SetLoadedAndWait("Vertex Shaders", id);

		return newVS;
	}
	catch (...) {
		SetLoadedAndWait("Vertex Shaders", id, std::current_exception());

		return NULL;
	}
}

std::shared_ptr<SimplePixelShader> AssetManager::CreatePixelShader(std::string id, std::string nameToLoad) {
	try {
		std::shared_ptr<SimplePixelShader> newPS;
		std::string assetPath;

		assetPath = dxInstance->GetAssetPathString(ASSET_SHADER_PATH);
		std::string namePath = assetPath + nameToLoad;
		char pathBuf[1024];

		GetFullPathNameA(namePath.c_str(), sizeof(pathBuf), pathBuf, NULL);

		newPS = std::make_shared<SimplePixelShader>(device.Get(), context.Get(), pathBuf, id);

		// Serialize the filename if it's in the right folder
		std::string baseFilename = "";
		std::string assetPathStr = "Assets\\Shaders";
		size_t dirPos = nameToLoad.find(assetPathStr);
		if (dirPos != std::string::npos) {
			// File is in the assets folder
			baseFilename = "t";
			baseFilename += nameToLoad.substr(dirPos + sizeof(assetPathStr));
		}
		else {
			baseFilename = "f";
			baseFilename += nameToLoad;
		}

		newPS->SetFileNameKey(baseFilename);

		pixelShaders.push_back(newPS);

		SetLoadedAndWait("Pixel Shaders", id);

		return newPS;
	}
	catch (...) {
		SetLoadedAndWait("Pixel Shaders", id, std::current_exception());

		return NULL;
	}
}

std::shared_ptr<SimpleComputeShader> AssetManager::CreateComputeShader(std::string id, std::string nameToLoad) {
	try {
		std::shared_ptr<SimpleComputeShader> newCS;
		std::string assetPath;

		assetPath = dxInstance->GetAssetPathString(ASSET_SHADER_PATH);
		std::string namePath = assetPath + nameToLoad;
		char pathBuf[1024];

		GetFullPathNameA(namePath.c_str(), sizeof(pathBuf), pathBuf, NULL);

		newCS = std::make_shared<SimpleComputeShader>(device.Get(), context.Get(), pathBuf, id);

		// Serialize the filename if it's in the right folder
		std::string baseFilename = "";
		std::string assetPathStr = "Assets\\Shaders";
		size_t dirPos = nameToLoad.find(assetPathStr);
		if (dirPos != std::string::npos) {
			// File is in the assets folder
			baseFilename = "t";
			baseFilename += nameToLoad.substr(dirPos + sizeof(assetPathStr));
		}
		else {
			baseFilename = "f";
			baseFilename += nameToLoad;
		}

		newCS->SetFileNameKey(baseFilename);

		computeShaders.push_back(newCS);

		SetLoadedAndWait("Compute Shaders", id);

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
		std::shared_ptr<Mesh> newMesh;
		std::string assetPath;

		assetPath = dxInstance->GetAssetPathString(ASSET_MODEL_PATH);
		std::string namePath = assetPath + nameToLoad;
		char pathBuf[1024];

		GetFullPathNameA(namePath.c_str(), sizeof(pathBuf), pathBuf, NULL);

		newMesh = std::make_shared<Mesh>(pathBuf, device, id);

		globalMeshes.push_back(newMesh);

		SetLoadedAndWait("Meshes", id);

		return newMesh;
	}
	catch (...) {
		SetLoadedAndWait("Mesh", id, std::current_exception());

		return NULL;
	}
}

HRESULT AssetManager::LoadPBRTexture(std::string nameToLoad, OUT Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* texture, PBRTextureTypes textureType) {
	HRESULT hr;
	std::string assetPath;

	switch (textureType) {
		case PBRTextureTypes::ALBEDO:
			assetPath = dxInstance->GetAssetPathString(ASSET_TEXTURE_PATH_PBR_ALBEDO);
			break;
		case PBRTextureTypes::NORMAL:
			assetPath = dxInstance->GetAssetPathString(ASSET_TEXTURE_PATH_PBR_NORMALS);
			break;
		case PBRTextureTypes::METAL:
			assetPath = dxInstance->GetAssetPathString(ASSET_TEXTURE_PATH_PBR_METALNESS);
			break;
		case PBRTextureTypes::ROUGH:
			assetPath = dxInstance->GetAssetPathString(ASSET_TEXTURE_PATH_PBR_ROUGHNESS);
			break;
	};

	std::string namePath = assetPath + nameToLoad;
	std::wstring widePath;
	char pathBuf[1024];

	GetFullPathNameA(namePath.c_str(), sizeof(pathBuf), pathBuf, NULL);
	hr = ISimpleShader::ConvertToWide(pathBuf, widePath);

	CreateWICTextureFromFile(device.Get(), context.Get(), widePath.c_str(), nullptr, texture->GetAddressOf());

	return hr;
}

std::shared_ptr<Material> AssetManager::CreatePBRMaterial(std::string id,
														  std::string albedoNameToLoad,
														  std::string normalNameToLoad,
														  std::string metalnessNameToLoad,
														  std::string roughnessNameToLoad,
														  bool addToGlobalList) {
	try {
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedo;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normals;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalness;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughness;
		std::shared_ptr<Material> newMat;

		std::shared_ptr<SimpleVertexShader> VSNormal = GetVertexShaderByName("NormalsVS");
		std::shared_ptr<SimplePixelShader> PSNormal = GetPixelShaderByName("NormalsPS");

		LoadPBRTexture(albedoNameToLoad, &albedo, PBRTextureTypes::ALBEDO);
		LoadPBRTexture(normalNameToLoad, &normals, PBRTextureTypes::NORMAL);
		LoadPBRTexture(metalnessNameToLoad, &metalness, PBRTextureTypes::METAL);
		LoadPBRTexture(roughnessNameToLoad, &roughness, PBRTextureTypes::ROUGH);

		newMat = std::make_shared<Material>(whiteTint,
			PSNormal,
			VSNormal,
			albedo,
			textureState,
			clampState,
			normals,
			roughness,
			metalness,
			id);

		if (addToGlobalList) globalMaterials.push_back(newMat);

		SetLoadedAndWait("PBR Materials", id);

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
		std::shared_ptr<GameEntity> newEnt = std::make_shared<GameEntity>(XMMatrixIdentity(), name);
		newEnt->Initialize();

		globalEntities.push_back(newEnt);

		SetLoadedAndWait("Game Entities", name);

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
		std::shared_ptr<GameEntity> newEnt = CreateGameEntity(name);

		std::shared_ptr<MeshRenderer> renderer = newEnt->AddComponent<MeshRenderer>();
		renderer->SetMesh(mesh);
		renderer->SetMaterial(mat);

		SetLoadedAndWait("Game Entities", name + " Components");

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
	std::shared_ptr<Light> light = CreateGameEntity(name)->AddComponent<Light>();
	if (light != nullptr) {
		light->SetType(0.0f);
		light->SetDirection(direction);
		light->SetColor(color);
		light->SetIntensity(intensity);
	}
	return light;
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
	std::shared_ptr<Light> light = CreateGameEntity(name)->AddComponent<Light>();
	if (light != nullptr) {
		light->SetType(1.0f);
		light->SetRange(range);
		light->SetColor(color);
		light->SetIntensity(intensity);
	}
	return light;
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
	std::shared_ptr<Light> light = CreateGameEntity(name)->AddComponent<Light>();
	if (light != nullptr) {
		light->SetType(2.0f);
		light->SetDirection(direction);
		light->SetRange(range);
		light->SetColor(color);
		light->SetIntensity(intensity);
	}
	return light;
}

/// <summary>
/// Creates a GameEntity and gives it a Terrain component
/// </summary>
/// <param name="name">Name of the GameEntity</param>
/// <returns>Pointer to the new GameEntity</returns>
std::shared_ptr<Terrain> AssetManager::CreateTerrainEntity(std::string name) {
	try {
		std::shared_ptr<GameEntity> newEnt = CreateGameEntity(name);

		SetLoadedAndWait("Terrain Entities", name);

		return newEnt->AddComponent<Terrain>();
	}
	catch (...) {
		SetLoadedAndWait("Terrain Entities", name, std::current_exception());

		return NULL;
	}
}

std::shared_ptr<Sky> AssetManager::CreateSky(std::string filepath, bool fileType, std::string name, std::string fileExtension) {
	try {
		std::shared_ptr<Mesh> Cube = GetMeshByName("Cube");

		std::vector<std::shared_ptr<SimplePixelShader>> importantSkyPixelShaders;
		std::vector<std::shared_ptr<SimpleVertexShader>> importantSkyVertexShaders;

		importantSkyPixelShaders.push_back(GetPixelShaderByName("SkyPS"));
		importantSkyPixelShaders.push_back(GetPixelShaderByName("IrradiancePS"));
		importantSkyPixelShaders.push_back(GetPixelShaderByName("SpecularConvolutionPS"));
		importantSkyPixelShaders.push_back(GetPixelShaderByName("BRDFLookupTablePS"));

		importantSkyVertexShaders.push_back(GetVertexShaderByName("SkyVS"));
		importantSkyVertexShaders.push_back(GetVertexShaderByName("FullscreenVS"));

		std::string assetPath;
		std::string filenameKey;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> newSkyTexture;

		assetPath = dxInstance->GetAssetPathString(ASSET_TEXTURE_PATH_SKIES);
		std::string namePath = assetPath + filepath;
		char pathBuf[1024];
		std::string stringifiedPathBuf;

		if (fileType) {
			// Process as 6 textures in a directory
			GetFullPathNameA(namePath.c_str(), sizeof(pathBuf), pathBuf, NULL);

			std::wstring skyDDSWide;
			std::wstring fileExtensionW;
			ISimpleShader::ConvertToWide(pathBuf, skyDDSWide);
			ISimpleShader::ConvertToWide(fileExtension, fileExtensionW);

			newSkyTexture = CreateCubemap((skyDDSWide + L"right" + fileExtensionW).c_str(),
				(skyDDSWide + L"left" + fileExtensionW).c_str(),
				(skyDDSWide + L"up" + fileExtensionW).c_str(),
				(skyDDSWide + L"down" + fileExtensionW).c_str(),
				(skyDDSWide + L"forward" + fileExtensionW).c_str(),
				(skyDDSWide + L"back" + fileExtensionW).c_str());

			stringifiedPathBuf = pathBuf;

			// Serialize the filename if it's in the right folder
			size_t dirPos = stringifiedPathBuf.find("Assets\\Textures\\Skies");
			if (dirPos != std::string::npos) {
				// File is in the assets folder
				filenameKey = "t";
				filenameKey += stringifiedPathBuf.substr(dirPos + sizeof("Assets\\Textures\\Skies"));
			}
			else {
				filenameKey = "f";
				filenameKey += stringifiedPathBuf;
			}
		}
		else {
			// Process as a .dds
			GetFullPathNameA(namePath.c_str(), sizeof(pathBuf), pathBuf, NULL);

			std::wstring skyDDSWide;
			ISimpleShader::ConvertToWide(pathBuf, skyDDSWide);

			CreateDDSTextureFromFile(device.Get(), context.Get(), skyDDSWide.c_str(), nullptr, &newSkyTexture);

			stringifiedPathBuf = pathBuf;

			// Serialize the filename if it's in the right folder
			size_t dirPos = stringifiedPathBuf.find("Assets\\Textures\\Skies");
			if (dirPos != std::string::npos) {
				// File is in the assets folder
				filenameKey = "t";
				filenameKey += stringifiedPathBuf.substr(dirPos + sizeof("Assets\\Textures\\Skies"));
			}
			else {
				filenameKey = "f";
				filenameKey += stringifiedPathBuf;
			}
		}

		std::shared_ptr<Sky> newSky = std::make_shared<Sky>(textureState, newSkyTexture, Cube, importantSkyPixelShaders, importantSkyVertexShaders, device, context, name);

		newSky->SetFilenameKeyType(fileType);
		newSky->SetFilenameKey(filenameKey);
		newSky->SetFileExtension(fileExtension);

		skies.push_back(newSky);

		SetLoadedAndWait("Skies", name);

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
	std::wstring textureNameToLoad,
	bool isMultiParticle)
{
	try {
		std::shared_ptr<GameEntity> emitterEntity = CreateGameEntity(name);
		std::shared_ptr<ParticleSystem> newEmitter = emitterEntity->AddComponent<ParticleSystem>();

		newEmitter->SetIsMultiParticle(isMultiParticle);
		newEmitter->SetParticleTextureSRV(LoadParticleTexture(textureNameToLoad, isMultiParticle));

		// Set all the compute shaders here
		newEmitter->SetParticleComputeShader(GetComputeShaderByName("ParticleEmitCS"), Emit);
		newEmitter->SetParticleComputeShader(GetComputeShaderByName("ParticleMoveCS"), Simulate);
		newEmitter->SetParticleComputeShader(GetComputeShaderByName("ParticleCopyCS"), Copy);
		newEmitter->SetParticleComputeShader(GetComputeShaderByName("ParticleInitDeadCS"), DeadListInit);

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
															 std::wstring textureNameToLoad,
															 int maxParticles,
															 float particleLifeTime,
															 float particlesPerSecond,
															 bool isMultiParticle,
															 bool additiveBlendState) {
	std::shared_ptr<ParticleSystem> newEmitter = CreateParticleEmitter(name, textureNameToLoad, isMultiParticle);
	newEmitter->SetMaxParticles(maxParticles);
	newEmitter->SetParticleLifetime(particleLifeTime);
	newEmitter->SetParticlesPerSecond(particlesPerSecond);
	newEmitter->SetBlendState(additiveBlendState);

	return newEmitter;
}

std::shared_ptr<SHOEFont> AssetManager::CreateSHOEFont(std::string name, std::string filePath, bool preInitializing) {
	try {
		std::string assetPath;

		assetPath = dxInstance->GetAssetPathString(ASSET_FONT_PATH);
		std::string namePath = assetPath + filePath;
		std::wstring wPathBuf;
		char pathBuf[1024];

		GetFullPathNameA(namePath.c_str(), sizeof(pathBuf), pathBuf, NULL);

		// Serialize the filename if it's in the right folder
		std::string baseFilename = "";
		size_t dirPos = filePath.find("Assets\\Fonts");
		if (dirPos != std::string::npos) {
			// File is in the assets folder
			baseFilename = "t";
			baseFilename += filePath.substr(dirPos + sizeof("Assets\\Fonts"));
		}
		else {
			baseFilename = "f";
			baseFilename += filePath;
		}

		ISimpleShader::ConvertToWide(pathBuf, wPathBuf);

		std::shared_ptr<DirectX::SpriteFont> sFont = std::make_shared<DirectX::SpriteFont>(device.Get(), wPathBuf.c_str());

		std::shared_ptr<SHOEFont> newFont = std::make_shared<SHOEFont>();
		newFont->fileNameKey = baseFilename;
		newFont->name = name;
		newFont->spritefont = sFont;

		globalFonts.push_back(newFont);

		// If the loading screen fonts aren't loaded, don't trigger
		// the loading screen.
		if (!preInitializing) SetLoadedAndWait("Font", name);

		return newFont;
	}
	catch (...) {
		if (!preInitializing) SetLoadedAndWait("Font", name, std::current_exception());

		return NULL;
	}
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

	std::shared_ptr<Terrain> terrainEntity = CreateTerrainEntity("Main Terrain");
	terrainEntity->GetTransform()->SetPosition(-256.0f, -10.0f, -256.0f);

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

	/*CreatePBRMaterial(std::string("transparentScratchMat"),
					  "scratched_albedo.png",
					  "scratched_normals.png",
					  "scratched_metal.png",
					  "scratched_roughness.png")->SetTransparent(true);*/

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
	//GetMaterialByName("refractiveScratchMat")->SetTint(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
}

void AssetManager::InitializeMeshes() {
	globalMeshes = std::vector<std::shared_ptr<Mesh>>();

	// Test loading failure
	CreateMesh("ExceptionTest", "InvalidPath");

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

		SetLoadedAndWait("Lights", "mainLight");

		//white light from the back
		CreateDirectionalLight("BackLight", DirectX::XMFLOAT3(0, 0, -1));

		SetLoadedAndWait("Lights", "backLight");

		//red light on the bottom
		CreateDirectionalLight("BottomLight", DirectX::XMFLOAT3(0, 1, 0), DirectX::XMFLOAT3(1.0f, 0.2f, 0.2f));

		SetLoadedAndWait("Lights", "bottomLight");

		//red pointlight in the center
		std::shared_ptr<Light> bottomLight = CreatePointLight("CenterLight", 2.0f, DirectX::XMFLOAT3(0.1f, 1.0f, 0.2f));
		bottomLight->GetTransform()->SetPosition(DirectX::XMFLOAT3(0, 1.5f, 0));

		SetLoadedAndWait("Lights", "centerLight");

		//flashlight attached to camera +.5z and x
		std::shared_ptr<Light> flashlight = CreateSpotLight("Flashlight", DirectX::XMFLOAT3(0, 0, -1), 10.0f);
		flashlight->GetTransform()->SetPosition(DirectX::XMFLOAT3(0.5f, 0.0f, 0.5f));
		flashlight->SetEnabled(false);

		SetLoadedAndWait("Lights", "flashLight");
	}
	catch (...) {
		SetLoadedAndWait("Lights", "Unknown Light", std::current_exception());

		return;
	}
}

void AssetManager::InitializeTerrainMaterials() {
	try {
		globalTerrainMaterials = std::vector<std::shared_ptr<TerrainMaterial>>();
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> defaultBlendMap;
		std::shared_ptr<TerrainMaterial> forestTerrainMaterial;

		//Load terrain and add a blend map

		std::string assetPath;

		assetPath = dxInstance->GetAssetPathString(ASSET_HEIGHTMAP_PATH);
		std::string namePath = assetPath + "valley.raw16";
		char pathBuf[1024];

		GetFullPathNameA(namePath.c_str(), sizeof(pathBuf), pathBuf, NULL);

		std::shared_ptr<Mesh> mainTerrain = LoadTerrain(pathBuf, 512, 512, 25.0f);
		globalMeshes.push_back(mainTerrain);

		assetPath = dxInstance->GetAssetPathString(ASSET_TEXTURE_PATH_BASIC);
		namePath = assetPath + "blendMap.png";

		GetFullPathNameA(namePath.c_str(), sizeof(pathBuf), pathBuf, NULL);

		std::wstring wPath;
		ISimpleShader::ConvertToWide(pathBuf, wPath);

		CreateWICTextureFromFile(device.Get(), context.Get(), wPath.c_str(), nullptr, defaultBlendMap.GetAddressOf());

		// Serialize the filename if it's in the right folder
		std::string baseFilename = "";
		std::string assetPathStr = "Assets\\Textures";
		std::string pathBufString = std::string(pathBuf);
		size_t dirPos = pathBufString.find(assetPathStr);
		if (dirPos != std::string::npos) {
			// File is in the assets folder
			baseFilename = "t";
			baseFilename += pathBufString.substr(dirPos + assetPathStr.size() + 1);
		}
		else {
			baseFilename = "f";
			baseFilename += pathBufString;
		}

		SetLoadedAndWait("Terrain", "Height and Blend Maps");

		std::shared_ptr<Material> forestMat = CreatePBRMaterial("Forest TMaterial", "forest_floor_albedo.png", "forest_floor_Normal-ogl.png", "wood_metal.png", "forest_floor_Roughness.png");
		std::shared_ptr<Material> bogMat = CreatePBRMaterial("Bog TMaterial", "bog_albedo.png.png", "bog_normal-ogl.png", "wood_metal.png", "bog_roughness.png");
		std::shared_ptr<Material> rockyMat = CreatePBRMaterial("Rocky TMaterial", "rocky_dirt1-albedo.png", "rocky_dirt1-normal-ogl.png", "wood_metal.png", "rocky_dirt1_Roughness.png");

		//Set appropriate tiling
		forestMat->SetTiling(10.0f);
		bogMat->SetTiling(10.0f);

		forestTerrainMaterial = std::make_shared<TerrainMaterial>("Forest Terrain Material", defaultBlendMap);

		forestTerrainMaterial->AddMaterial(forestMat);
		forestTerrainMaterial->AddMaterial(bogMat);
		forestTerrainMaterial->AddMaterial(rockyMat);

		forestTerrainMaterial->SetBlendMapFilenameKey(baseFilename);

		globalTerrainMaterials.push_back(forestTerrainMaterial);

		SetLoadedAndWait("Terrain", "Forest Terrain Material");
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
	CreateCamera("mainShadowCamera", DirectX::XMFLOAT3(0.0f, 10.0f, -20.0f), 1.0f, 0)->SetTag(CameraType::MISC_SHADOW);
	std::shared_ptr<Camera> fscTemp = CreateCamera("flashShadowCamera", DirectX::XMFLOAT3(0.0f, 0.0f, -5.5f), 1.0f, 1);
	fscTemp->SetTag(CameraType::MISC_SHADOW);
	fscTemp->GetTransform()->SetRotation(0, 0, 0);
	fscTemp->UpdateViewMatrix();
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
		LoadParticleTexture(L"Smoke/", true),
		GetComputeShaderByName("ParticleEmitCS"),
		GetComputeShaderByName("ParticleMoveCS"),
		GetComputeShaderByName("ParticleCopyCS"),
		GetComputeShaderByName("ParticleInitDeadCS"),
		device,
		context);

	std::shared_ptr<ParticleSystem> basicEmitter = CreateParticleEmitter("basicParticle", L"Smoke/smoke_01.png", 20, 1.0f, 1.0f);
	basicEmitter->GetTransform()->SetPosition(XMFLOAT3(1.0f, 0.0f, 0.0f));
	basicEmitter->SetEnabled(false);

	std::shared_ptr<ParticleSystem> basicMultiEmitter = CreateParticleEmitter("basicParticles", L"Smoke/", true);
	basicMultiEmitter->SetScale(1.0f);
	basicMultiEmitter->SetEnabled(false);

	std::shared_ptr<ParticleSystem> flameEmitter = CreateParticleEmitter("flameParticles", L"Flame/", 10, 2.0f, 5.0f, true);
	flameEmitter->GetTransform()->SetPosition(XMFLOAT3(-1.0f, 0.0f, 0.0f));
	flameEmitter->SetColorTint(XMFLOAT4(0.8f, 0.3f, 0.2f, 1.0f));
	flameEmitter->SetEnabled(false);

	std::shared_ptr<ParticleSystem> starMultiEmitter = CreateParticleEmitter("starParticles", L"Star/", 300, 2.0f, 80.0f, true);
	starMultiEmitter->GetTransform()->SetPosition(XMFLOAT3(-2.0f, 0.0f, 0.0f));
	starMultiEmitter->SetColorTint(XMFLOAT4(0.96f, 0.89f, 0.1f, 1.0f));
	starMultiEmitter->SetEnabled(false);

	std::shared_ptr<ParticleSystem> starEmitter = CreateParticleEmitter("starParticle", L"Star/star_08.png", 300, 2.0f, 80.0f);
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
	SetLoadedAndWait("UI", "Window Initialization");

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(device.Get(), context.Get());
}

void AssetManager::InitializeColliders() {
	std::shared_ptr<GameEntity> e = GetGameEntityByName("Bronze Cube");
	std::shared_ptr<GameEntity> e2 = GetGameEntityByName("Scratched Sphere");
	std::shared_ptr<GameEntity> e3 = GetGameEntityByName("Rough Torus");
	std::shared_ptr<GameEntity> e4 = GetGameEntityByName("Floor Helix");
	std::shared_ptr<GameEntity> e5 = GetGameEntityByName("Shiny Rough Sphere");

	std::shared_ptr<GameEntity> e6 = GetGameEntityByName("Floor Cube");
	std::shared_ptr<GameEntity> e7 = GetGameEntityByName("Scratched Cube");
	//TODO: the wood sphere will become a child of the spinning stuff if you disable and enable it via IMGUI....

	std::shared_ptr<Collider> c1 = AddColliderToGameEntity(e);
	std::shared_ptr<Collider> c2 = AddColliderToGameEntity(e2);
	std::shared_ptr<Collider> c3 = AddColliderToGameEntity(e3);
	std::shared_ptr<Collider> c4 = AddColliderToGameEntity(e4);
	std::shared_ptr<Collider> c5 = AddColliderToGameEntity(e5);
	std::shared_ptr<Collider> c6 = AddColliderToGameEntity(e6);
	std::shared_ptr<Collider> c7 = AddTriggerBoxToGameEntity(e7);
	c6->SetExtents(XMFLOAT3(1.002f, 1.002f, 1.002f)); //TODO: check and see if setting this alters where children end up (coordinate space scaling)
}
#pragma endregion

#pragma region addComponent

std::shared_ptr<Collider> AssetManager::AddColliderToGameEntity(OUT std::shared_ptr<GameEntity> entity) {
	try {
		std::shared_ptr<Collider> c = entity->AddComponent<Collider>();

		SetLoadedAndWait("Colliders", "Generic Collider");

		return c;
	}
	catch (...) {
		SetLoadedAndWait("Colliders", "Generic Collider", std::current_exception());

		return nullptr;
	}
}

std::shared_ptr<Collider> AssetManager::AddTriggerBoxToGameEntity(OUT std::shared_ptr<GameEntity> entity) {
	try {
		std::shared_ptr<Collider> c = entity->AddComponent<Collider>();

		c->SetTriggerStatus(true);

		SetLoadedAndWait("Colliders", "Generic Trigger Box");

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

std::vector<std::shared_ptr<GameEntity>>* AssetManager::GetActiveGameEntities() {
	return &this->globalEntities;
}

std::vector<std::shared_ptr<Sky>>* AssetManager::GetSkyArray() {
	return &this->skies;
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

std::shared_ptr<GameEntity> AssetManager::GetGameEntityByID(int id) {
	return this->globalEntities[id];
}

ComponentTypes AssetManager::GetAllCurrentComponentTypes() {
	return this->allCurrentComponentTypes;
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
	std::shared_ptr<Mesh> finalTerrain;

	unsigned int numVertices = mapWidth * mapHeight;
	unsigned int numIndices = (mapWidth - 1) * (mapHeight - 1) * 6;

	std::vector<unsigned short> heights(numVertices);
	std::vector<float> finalHeights(numVertices);

	std::vector<Vertex> vertices(numVertices);
	std::vector<unsigned int> indices(numIndices);
	std::vector<XMFLOAT3> triangleNormals;

	//Read the file
	std::ifstream file;
	file.open(filename, std::ios_base::binary);

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
			XMFLOAT3 normal = XMFLOAT3(+0.0f, +1.0f, +0.0f);
			XMFLOAT3 tangents = XMFLOAT3(+0.0f, +0.0f, +0.0f);
			XMFLOAT2 UV = XMFLOAT2(x / (float)mapWidth, z / (float)mapWidth);
			vertices[index] = { position, normal, tangents, UV };
		}


	}

	//Mesh handles tangents
	finalTerrain = std::make_shared<Mesh>(vertices.data(), numVertices, indices.data(), numIndices, device, "TerrainMesh");

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
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> AssetManager::LoadParticleTexture(std::wstring textureNameToLoad, bool isMultiParticle)
{
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleTextureSRV;

	if (isMultiParticle) {
		// Load all particle textures in a specific subfolder
		std::string assets = "../../../Assets/Particles/";
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
		std::string subfolderPath = converter.to_bytes(textureNameToLoad);

		std::vector<Microsoft::WRL::ComPtr<ID3D11Texture2D>> textures;
		int i = 0;
		for (auto& p : std::experimental::filesystem::recursive_directory_iterator(dxInstance->GetFullPathTo(assets + subfolderPath))) {
			textures.push_back(nullptr);
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedo;
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
		std::wstring assetPathString = L"../../../Assets/Particles/";
		CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(assetPathString + textureNameToLoad).c_str(), nullptr, &particleTextureSRV);
	}

	return particleTextureSRV;
}
#pragma endregion

#pragma region complexModels
void AssetManager::CreateComplexGeometry() {
	Assimp::Importer importer;
	std::vector<std::shared_ptr<Material>> specialMaterials = std::vector<std::shared_ptr<Material>>();

	/*const aiScene* tree1Model = importer.ReadFile(GetFullPathTo("..\\..\\..\\Assets\\Models\\trees9.obj").c_str(),
		aiProcess_Triangulate			|
		aiProcess_JoinIdenticalVertices	|
		aiProcess_GenNormals			|
		aiProcess_ConvertToLeftHanded	|
		aiProcess_CalcTangentSpace		);*/

	const aiScene* flashLightModel = importer.ReadFile(dxInstance->GetFullPathTo("..\\..\\..\\Assets\\Models\\human.obj").c_str(),
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_GenNormals |
		aiProcess_ConvertToLeftHanded |
		aiProcess_CalcTangentSpace);

	//ProcessComplexModel(tree1Model->mRootNode, tree1Model);

	if (flashLightModel != NULL) {
		ProcessComplexModel(flashLightModel->mRootNode, flashLightModel);
	}

	const aiScene* hatModel = importer.ReadFile(dxInstance->GetFullPathTo("..\\..\\..\\Assets\\Models\\hat.obj").c_str(),
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_GenNormals |
		aiProcess_ConvertToLeftHanded |
		aiProcess_CalcTangentSpace);

	if (hatModel != NULL) {
		ProcessComplexModel(hatModel->mRootNode, hatModel);
	}
	
}

void AssetManager::ProcessComplexModel(aiNode* node, const aiScene* scene) {
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		std::string newName = "ComplexMesh" + std::to_string(i);
		mesh->mName = newName;
		globalMeshes.push_back(ProcessComplexMesh(mesh, scene));
		CreateGameEntity(GetMeshByName(mesh->mName.C_Str() + std::string("Mesh")), GetMaterialByName("cobbleMat"), mesh->mName.C_Str());
		GetGameEntityByName(mesh->mName.C_Str())->GetTransform()->SetPosition(0.0f, 3.0f * i, 1.0f);
		GetGameEntityByName(mesh->mName.C_Str())->GetTransform()->SetScale(0.25f, 0.25f, 0.25f);
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		ProcessComplexModel(node->mChildren[i], scene);
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

void AssetManager::RemoveLight(std::string name) {
	globalEntities.erase(globalEntities.begin() + GetGameEntityIDByName(name));
}

void AssetManager::RemoveLight(int id) {
	globalEntities.erase(globalEntities.begin() + id);
}
*/
#pragma endregion

#pragma region enableDisableAssets

//
// Enables or disables assets into or
// out of the rendering pipeline
//

void AssetManager::EnableDisableSky(std::string name, bool value) {
	GetSkyByName(name)->SetEnableDisable(value);
}

void AssetManager::EnableDisableSky(int id, bool value) {
	skies[id]->SetEnableDisable(value);
}

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

// Unlikely to be implemented, see GetLightIdByName
//std::shared_ptr<Light> AssetManager::GetLightByName(std::string name) {
//	for (int i = 0; i < globalLights.size(); i++) {
//		if (globalLights[i]->GetName() == name) {
//			return globalLights[i];
//		}
//	}
//	return nullptr;
//}
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

// Same as above
//int AssetManager::GetTerrainMaterialIDByName(std::string name) {
//	for (int i = 0; i < globalTerrainMaterials.size(); i++) {
//		if (globalTerrainMaterials[i]->GetName() == name) {
//			return i;
//		}
//	}
//	return -1;
//}

// Unsure if current design pattern allows this
// Lights must stay at a specific size to be passed to GPU
// Could theoretically create map
// Could also create a wrapper struct with:
//		Name
//		Light
// and do pointer math to return Lights as an array that skips name
//int AssetManager::GetLightIDByName(std::string name) {
//	for (int i = 0; i < globalLights.size(); i++) {
//		if (globalLights[i]-> == name) {
//			return i;
//		}
//	}
//	return -1;
//}


#pragma endregion

#pragma region inlines



#pragma endregion