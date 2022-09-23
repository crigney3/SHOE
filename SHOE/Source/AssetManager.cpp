#include "../Headers/AssetManager.h"
#include "..\Headers\FlashlightController.h"
#include "..\Headers\NoclipMovement.h"

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

	// And entities
	CleanAllEntities();
	editingCamera->GetGameEntity()->Release();
	editingCamera = nullptr;
	mainCamera = nullptr;
}

void AssetManager::Initialize(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, HWND hwnd, EngineState* engineState, std::function<void(std::string)> progressListener) {
	*engineState = EngineState::INIT;

	HRESULT hr = CoInitialize(NULL);

	dxInstance = DXCore::DXCoreInstance;
	this->context = context;
	this->device = device;
	this->engineState = engineState;

	CleanAllVectors();

	// This must occur before the loading screen starts
	InitializeFonts();
	InitializeTextureSampleStates();

	// The rest signal the loading screen each time an object loads
	if(progressListener) progressListener("Shaders");
	InitializeShaders();
	if (progressListener) progressListener("Textures");
	InitializeTextures();
	if(progressListener) progressListener("Materials");
	InitializeMaterials();
	if(progressListener) progressListener("Terrain Materials");
	InitializeTerrainMaterials();
	if(progressListener) progressListener("Meshes");
	InitializeMeshes();
	if(progressListener) progressListener("Entities");
	InitializeGameEntities();
	if(progressListener) progressListener("Cameras");
	InitializeCameras();
	if(progressListener) progressListener("Terrain");
	InitializeTerrainEntities();
	if(progressListener) progressListener("Lights");
	InitializeLights();
	if(progressListener) progressListener("Colliders");
	InitializeColliders();
	if(progressListener) progressListener("Emitters");
	InitializeEmitters();
	if(progressListener) progressListener("Skies");
	InitializeSkies();
	if(progressListener) progressListener("Audio");
	InitializeAudio();
	if(progressListener) progressListener("IMGUI");
	InitializeIMGUI(hwnd);

	if(progressListener) progressListener("Editing Camera");
	//Intentionally not tracked by the asset manager
	std::shared_ptr<GameEntity> editingCamObj = std::make_shared<GameEntity>("editingCamera");
	editingCamObj->Initialize();
	editingCamObj->GetTransform()->SetPosition(DirectX::XMFLOAT3(0.0f, 0.0f, -20.0f));
	editingCamera = CreateCameraOnEntity(editingCamObj);
	editingCamObj->AddComponent<NoclipMovement>();

	*engineState = EngineState::EDITING;
}
#pragma endregion

#pragma region createAssets
FMOD::Sound* AssetManager::CreateSound(std::string path, FMOD_MODE mode, std::string name, bool isNameFullPath) {
	FMODUserData* uData = new FMODUserData;
	FMOD::Sound* sound;

	std::string namePath = GetFullPathToAssetFile(AssetPathIndex::ASSET_SOUND_PATH, path);

	if (isNameFullPath) {
		namePath = path;
	}
	else {
		namePath = GetFullPathToAssetFile(AssetPathIndex::ASSET_SOUND_PATH, path);
	}

	sound = audioInstance.LoadSound(namePath, mode);

	// Serialize the filename if it's in the right folder
	std::string assetPathStr = "Assets\\Sounds\\";

	std::string baseFilename = SerializeFileName(assetPathStr, namePath);

	uData->filenameKey = std::make_shared<std::string>(baseFilename);
	uData->name = std::make_shared<std::string>(name);

	// On getUserData, we will receive the whole struct
	sound->setUserData(uData);

	globalSounds.push_back(sound);

	BroadcastGlobalEntityEvent(EntityEventType::OnAudioLoad, std::make_shared<AudioEventPacket>(baseFilename, nullptr));

	return sound;
}

std::shared_ptr<Camera> AssetManager::CreateCamera(std::string name, float aspectRatio) {
	float ar = aspectRatio == 0 ? (float)(dxInstance->width / dxInstance->height) : aspectRatio;
	std::shared_ptr<GameEntity> newEnt = CreateGameEntity(name);
	std::shared_ptr<Camera> cam = CreateCameraOnEntity(newEnt, ar);
	return cam;

}

std::shared_ptr<SimpleVertexShader> AssetManager::CreateVertexShader(std::string id, std::string nameToLoad) {
	std::string namePath = GetFullPathToAssetFile(AssetPathIndex::ASSET_SHADER_PATH, nameToLoad);

	std::shared_ptr<SimpleVertexShader> newVS = std::make_shared<SimpleVertexShader>(device.Get(), context.Get(), namePath, id);

	// Serialize the filename if it's in the right folder
	std::string assetPathStr = "Assets\\Shaders\\";

	std::string baseFilename = SerializeFileName(assetPathStr, namePath);

	newVS->SetFileNameKey(baseFilename);

	vertexShaders.push_back(newVS);

	return newVS;
}

std::shared_ptr<SimplePixelShader> AssetManager::CreatePixelShader(std::string id, std::string nameToLoad) {
	std::string namePath = GetFullPathToAssetFile(AssetPathIndex::ASSET_SHADER_PATH, nameToLoad);

	std::shared_ptr<SimplePixelShader> newPS = std::make_shared<SimplePixelShader>(device.Get(), context.Get(), namePath, id);

	// Serialize the filename if it's in the right folder
	std::string assetPathStr = "Assets\\Shaders\\";

	std::string baseFilename = SerializeFileName(assetPathStr, namePath);

	newPS->SetFileNameKey(baseFilename);

	pixelShaders.push_back(newPS);

	return newPS;
}

std::shared_ptr<SimpleComputeShader> AssetManager::CreateComputeShader(std::string id, std::string nameToLoad) {
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

std::shared_ptr<Mesh> AssetManager::CreateMesh(std::string id, std::string nameToLoad, bool isNameFullPath) {
	std::string namePath;

	if (isNameFullPath) {
		namePath = nameToLoad;
	}
	else {
		namePath = GetFullPathToAssetFile(AssetPathIndex::ASSET_MODEL_PATH, nameToLoad);
	}

	std::shared_ptr<Mesh> newMesh = std::make_shared<Mesh>(namePath.c_str(), device, id);

	globalMeshes.push_back(newMesh);

	return newMesh;
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

//// deprecated
//HRESULT AssetManager::LoadPBRTexture(std::string nameToLoad, OUT Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* texture, PBRTextureTypes textureType) {
//	AssetPathIndex assetPath;
//
//	switch (textureType) {
//	case PBRTextureTypes::ALBEDO:
//		assetPath = ASSET_TEXTURE_PATH_PBR_ALBEDO;
//		break;
//	case PBRTextureTypes::NORMAL:
//		assetPath = ASSET_TEXTURE_PATH_PBR_NORMALS;
//		break;
//	case PBRTextureTypes::METAL:
//		assetPath = ASSET_TEXTURE_PATH_PBR_METALNESS;
//		break;
//	case PBRTextureTypes::ROUGH:
//		assetPath = ASSET_TEXTURE_PATH_PBR_ROUGHNESS;
//		break;
//	};
//
//	std::string namePath = GetFullPathToAssetFile(assetPath, nameToLoad);
//	std::wstring widePath;
//
//	HRESULT hr = ISimpleShader::ConvertToWide(namePath, widePath);
//
//	CreateWICTextureFromFile(device.Get(), context.Get(), widePath.c_str(), nullptr, texture->GetAddressOf());
//
//	std::shared_ptr<Texture> newTexture = std::make_shared<Texture>(texture, GetTextureFileKey(nameToLoad));
//
//	globalTextures.push_back(newTexture);
//
//	return hr;
//}
//
//// deprecated, for now
//HRESULT AssetManager::LoadPBRTexture(std::string nameToLoad, OUT Texture* texture, PBRTextureTypes textureType) {
//	AssetPathIndex assetPath;
//
//	switch (textureType) {
//	case PBRTextureTypes::ALBEDO:
//		assetPath = ASSET_TEXTURE_PATH_PBR_ALBEDO;
//		break;
//	case PBRTextureTypes::NORMAL:
//		assetPath = ASSET_TEXTURE_PATH_PBR_NORMALS;
//		break;
//	case PBRTextureTypes::METAL:
//		assetPath = ASSET_TEXTURE_PATH_PBR_METALNESS;
//		break;
//	case PBRTextureTypes::ROUGH:
//		assetPath = ASSET_TEXTURE_PATH_PBR_ROUGHNESS;
//		break;
//	};
//
//	std::string namePath = GetFullPathToAssetFile(assetPath, nameToLoad);
//	std::wstring widePath;
//
//	HRESULT hr = ISimpleShader::ConvertToWide(namePath, widePath);
//
//	CreateWICTextureFromFile(device.Get(), context.Get(), widePath.c_str(), nullptr, texture->GetTexture().GetAddressOf());
//
//	std::shared_ptr<Texture> newTexture = std::make_shared<Texture>(texture, GetTextureFileKey(nameToLoad));
//
//	globalTextures.push_back(newTexture);
//
//	return hr;
//}

std::string AssetManager::GetTextureFileKey(std::string textureFilename) {
	AssetPathIndex assetPath;
	std::string directAssetPath;

	assetPath = ASSET_TEXTURE_PATH_BASIC;
	directAssetPath = "Assets\\Textures\\";

	std::string namePath = GetFullPathToAssetFile(assetPath, textureFilename);
	std::string fileKey = SerializeFileName(directAssetPath, namePath);

	return fileKey;
}

std::shared_ptr<Texture> AssetManager::CreateTexture(std::string nameToLoad, std::string textureName, AssetPathIndex assetPath, bool isNameFullPath)
{
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> coreTexture;

	std::string namePath;
	if (isNameFullPath) {
		namePath = nameToLoad;
	}
	else {
		namePath = GetFullPathToAssetFile(assetPath, nameToLoad);
	}
	std::wstring widePath;

	HRESULT hr = ISimpleShader::ConvertToWide(namePath, widePath);

	CreateWICTextureFromFile(device.Get(), context.Get(), widePath.c_str(), nullptr, coreTexture.GetAddressOf());

	std::shared_ptr<Texture> newTexture = std::make_shared<Texture>(coreTexture, GetTextureFileKey(nameToLoad), textureName);

	if (isNameFullPath) {
		newTexture->SetTextureFilenameKey(nameToLoad);
	}
	else {
		newTexture->SetAssetPathIndex(assetPath);
	}

	globalTextures.push_back(newTexture);

	return newTexture;
}

std::shared_ptr<Material> AssetManager::CreatePBRMaterial(std::string id,
	std::string albedoNameToLoad,
	std::string normalNameToLoad,
	std::string metalnessNameToLoad,
	std::string roughnessNameToLoad,
	bool addToGlobalList) 
{
	std::shared_ptr<Texture> albedo;
	std::shared_ptr<Texture> normals;
	std::shared_ptr<Texture> metalness;
	std::shared_ptr<Texture> roughness;

	std::shared_ptr<SimpleVertexShader> VSNormal = GetVertexShaderByName("NormalsVS");
	std::shared_ptr<SimplePixelShader> PSNormal = GetPixelShaderByName("NormalsPS");

	albedo = CreateTexture(albedoNameToLoad);
	normals = CreateTexture(normalNameToLoad);
	metalness = CreateTexture(metalnessNameToLoad);
	roughness = CreateTexture(roughnessNameToLoad);

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

	if (addToGlobalList) globalMaterials.push_back(newMat);

	return newMat;
}

std::shared_ptr<Material> AssetManager::CreatePBRMaterial(std::string id,
	std::shared_ptr<Texture> albedoTexture,
	std::shared_ptr<Texture> normalTexture,
	std::shared_ptr<Texture> metalnessTexture,
	std::shared_ptr<Texture> roughnessTexture,
	bool addToGlobalList)
{
	std::shared_ptr<SimpleVertexShader> VSNormal = GetVertexShaderByName("NormalsVS");
	std::shared_ptr<SimplePixelShader> PSNormal = GetPixelShaderByName("NormalsPS");

	std::shared_ptr<Material> newMat = std::make_shared<Material>(whiteTint,
		PSNormal,
		VSNormal,
		albedoTexture,
		textureState,
		clampState,
		normalTexture,
		roughnessTexture,
		metalnessTexture,
		id);

	if (addToGlobalList) globalMaterials.push_back(newMat);

	return newMat;

}

/// <summary>
/// Creates a GameEntity
/// </summary>
/// <param name="name">Name of the GameEntity</param>
/// <returns>Pointer to the new GameEntity</returns>
std::shared_ptr<GameEntity> AssetManager::CreateGameEntity(std::string name)
{
	std::shared_ptr<GameEntity> newEnt = std::make_shared<GameEntity>(name);
	newEnt->Initialize();

	globalEntities.push_back(newEnt);

	return newEnt;
}

/// <summary>
/// Creates a GameEntity and gives it a MeshRenderer component
/// </summary>
/// <param name="mesh">Mesh to render</param>
/// <param name="mat">Material to render</param>
/// <param name="name">Name of the GameEntity</param>
/// <returns>Pointer to the new GameEntity</returns>
std::shared_ptr<GameEntity> AssetManager::CreateGameEntity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> mat, std::string name) {
	std::shared_ptr<GameEntity> newEnt = CreateGameEntity(name);

	std::shared_ptr<MeshRenderer> renderer = newEnt->AddComponent<MeshRenderer>();
	renderer->SetMesh(mesh);
	renderer->SetMaterial(mat);

	return newEnt;
}

/// <summary>
/// Creates a GameEntity, gives it a light component, and populates it with Directional Light values
/// </summary>
/// <param name="name">Name of the GameEntity</param>
/// <param name="direction">Direction of the light</param>
/// <param name="color">Color of the light</param>
/// <param name="intensity">Intensity of the light</param>
/// <returns>Pointer to the new Light component</returns>
std::shared_ptr<Light> AssetManager::CreateDirectionalLight(std::string name, DirectX::XMFLOAT3 color, float intensity)
{
	std::shared_ptr<GameEntity> newEnt = CreateGameEntity(name);
	std::shared_ptr<Light> light = CreateDirectionalLightOnEntity(newEnt, color, intensity);
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
	std::shared_ptr<GameEntity> newEnt = CreateGameEntity(name);
	std::shared_ptr<Light> light = CreatePointLightOnEntity(newEnt, range, color, intensity);
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
std::shared_ptr<Light> AssetManager::CreateSpotLight(std::string name, float range, DirectX::XMFLOAT3 color, float intensity)
{
	std::shared_ptr<GameEntity> newEnt = CreateGameEntity(name);
	std::shared_ptr<Light> light = CreateSpotLightOnEntity(newEnt, range, color, intensity);
	return light;
}

/// <summary>
/// Creates a GameEntity and gives it a Terrain component
/// </summary>
/// <param name="name">Name of the GameEntity</param>
/// <returns>Pointer to the new Terrain</returns>
std::shared_ptr<Terrain> AssetManager::CreateTerrainEntity(std::string name) {
	return CreateGameEntity(name)->AddComponent<Terrain>();
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
	return CreateTerrainOnEntity(CreateGameEntity(name), heightmap, material, mapWidth, mapHeight, heightScale);
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
	return CreateTerrainOnEntity(CreateGameEntity(name), terrainMesh, material);
}

std::shared_ptr<TerrainMaterial> AssetManager::CreateTerrainMaterial(std::string name, std::vector<std::shared_ptr<Material>> materials, std::string blendMapPath) {
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

	std::shared_ptr<Sky> newSky = std::make_shared<Sky>(textureState, newSkyTexture, importantSkyPixelShaders, importantSkyVertexShaders, device, context, name);

	newSky->SetFilenameKeyType(fileType);
	newSky->SetFilenameKey(filenameKey);
	newSky->SetFileExtension(fileExtension);

	skies.push_back(newSky);

	return newSky;
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
	return CreateParticleEmitterOnEntity(CreateGameEntity(name), textureNameToLoad, isMultiParticle);
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
	return CreateParticleEmitterOnEntity(CreateGameEntity(name), textureNameToLoad, maxParticles, particleLifeTime, particlesPerSecond, isMultiParticle, additiveBlendState);
}

std::shared_ptr<SHOEFont> AssetManager::CreateSHOEFont(std::string name, std::string filePath, bool preInitializing, bool isNameFullPath) {
	std::string namePath;
	if (isNameFullPath) {
		namePath = filePath;
	}
	else {
		namePath = GetFullPathToAssetFile(AssetPathIndex::ASSET_FONT_PATH, filePath);
	}
	std::wstring wPathBuf;

	ISimpleShader::ConvertToWide(namePath, wPathBuf);

	std::shared_ptr<SHOEFont> newFont = std::make_shared<SHOEFont>();
	newFont->fileNameKey = SerializeFileName("Assets\\Fonts\\", namePath);
	newFont->name = name;
	newFont->spritefont = std::make_shared<DirectX::SpriteFont>(device.Get(), wPathBuf.c_str());

	globalFonts.push_back(newFont);

	return newFont;
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
	DirectX::XMFLOAT3 color,
	float intensity) {

	std::shared_ptr<Light> light = entityToEdit->AddComponent<Light>();
	if (light != nullptr) {
		light->SetType(0.0f);
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
	float range,
	DirectX::XMFLOAT3 color,
	float intensity) {

	std::shared_ptr<Light> light = entityToEdit->AddComponent<Light>();
	if (light != nullptr) {
		light->SetType(2.0f);
		light->SetRange(range);
		light->SetColor(color);
		light->SetIntensity(intensity);
	}

	return light;
}

std::shared_ptr<Camera> AssetManager::CreateCameraOnEntity(std::shared_ptr<GameEntity> entityToEdit, float aspectRatio)
{
	float ar = aspectRatio == 0 ? (float)(dxInstance->width / dxInstance->height) : aspectRatio;
	std::shared_ptr<Camera> cam = entityToEdit->AddComponent<Camera>();
	cam->SetAspectRatio(ar);
	return cam;
}

std::shared_ptr<Collider> AssetManager::CreateColliderOnEntity(std::shared_ptr<GameEntity> entityToEdit) {
	return entityToEdit->AddComponent<Collider>();
}

std::shared_ptr<Collider> AssetManager::CreateTriggerBoxOnEntity(std::shared_ptr<GameEntity> entityToEdit) {
	std::shared_ptr<Collider> c = entityToEdit->AddComponent<Collider>();
	c->SetIsTrigger(true);
	return c;
}

#pragma endregion

#pragma region initAssets
void AssetManager::InitializeTextureSampleStates() {
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

	device->CreateSamplerState(&textureDesc, &textureState);

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

	device->CreateSamplerState(&clampDesc, &clampState);

	textureSampleStates.push_back(textureState);
	textureSampleStates.push_back(clampState);
}

void AssetManager::InitializeGameEntities() {
	//Initializes default values for components
	MeshRenderer::SetDefaults(GetMeshByName("Cube"), GetMaterialByName("largeCobbleMat"));

	// Show example render
	CreateGameEntity(GetMeshByName("Cube"), GetMaterialByName("paintMat"), "Paint Cube");

	// OUTDATED: Initializes lots of objects for demo.
	// These can still be viewed by loading the demo scene.
	//CreateGameEntity(GetMeshByName("Cube"), GetMaterialByName("floorMat"), "Floor Cube");
	//CreateGameEntity(GetMeshByName("Cube"), GetMaterialByName("scratchMat"), "Scratched Cube");

	//CreateGameEntity(GetMeshByName("Cylinder"), GetMaterialByName("floorMat"), "Stone Cylinder");
	//CreateGameEntity(GetMeshByName("Helix"), GetMaterialByName("floorMat"), "Floor Helix");
	//CreateGameEntity(GetMeshByName("Sphere"), GetMaterialByName("paintMat"), "Paint Sphere");
	//CreateGameEntity(GetMeshByName("Torus"), GetMaterialByName("roughMat"), "Rough Torus");
	//CreateGameEntity(GetMeshByName("Sphere"), GetMaterialByName("scratchMat"), "Scratched Sphere");
	//CreateGameEntity(GetMeshByName("Sphere"), GetMaterialByName("woodMat"), "Wood Sphere");
	//CreateGameEntity(GetMeshByName("Cube"), GetMaterialByName("largePaintMat"), "Large Paint Rect");
	//CreateGameEntity(GetMeshByName("Cube"), GetMaterialByName("largeCobbleMat"), "Large Cobble Rect");

	//// Reflective objects
	//CreateGameEntity(GetMeshByName("Sphere"), GetMaterialByName("reflectiveMetal"), "Shiny Metal Sphere");
	//CreateGameEntity(GetMeshByName("Sphere"), GetMaterialByName("reflectiveRoughMetal"), "Rough Metal Sphere");
	//CreateGameEntity(GetMeshByName("Sphere"), GetMaterialByName("reflectiveRough"), "Shiny Rough Sphere");
	//CreateGameEntity(GetMeshByName("Sphere"), GetMaterialByName("reflective"), "Shiny Sphere");

	//// Refractive Objects
	//CreateGameEntity(GetMeshByName("Sphere"), GetMaterialByName("refractivePaintMat"), "Refractive Sphere");
	//CreateGameEntity(GetMeshByName("Sphere"), GetMaterialByName("refractiveWoodMat"), "Refractive Sphere 2");
	//CreateGameEntity(GetMeshByName("Cube"), GetMaterialByName("refractiveRoughMat"), "Refractive Cube");
	//CreateGameEntity(GetMeshByName("Torus"), GetMaterialByName("refractiveBronzeMat"), "Refractive Torus");

	//GetGameEntityByName("Bronze Cube")->GetTransform()->SetPosition(+0.0f, +0.0f, +0.0f);
	//GetGameEntityByName("Floor Cube")->GetTransform()->SetPosition(+2.0f, +0.0f, +0.0f);
	//GetGameEntityByName("Scratched Cube")->GetTransform()->SetPosition(+0.5f, +2.0f, +0.0f);
	//GetGameEntityByName("Stone Cylinder")->GetTransform()->SetPosition(-0.7f, +0.0f, +0.0f);
	//GetGameEntityByName("Floor Helix")->GetTransform()->SetPosition(+0.0f, +0.7f, +0.0f);
	//GetGameEntityByName("Paint Sphere")->GetTransform()->SetPosition(+0.0f, -0.7f, +0.0f);
	//GetGameEntityByName("Scratched Sphere")->GetTransform()->SetPosition(+3.0f, +0.0f, +0.0f);
	//GetGameEntityByName("Wood Sphere")->GetTransform()->SetPosition(-3.0f, +0.0f, +0.0f);
	//GetGameEntityByName("Large Paint Rect")->GetTransform()->SetScale(10.0f, 10.0f, 0.2f);
	//GetGameEntityByName("Large Paint Rect")->GetTransform()->SetPosition(+0.0f, +0.0f, +4.0f);
	//GetGameEntityByName("Large Cobble Rect")->GetTransform()->SetScale(+10.0f, +10.0f, +0.2f);
	//GetGameEntityByName("Large Cobble Rect")->GetTransform()->SetPosition(+0.0f, -5.0f, +0.0f);
	//GetGameEntityByName("Large Cobble Rect")->GetTransform()->Rotate(45.0f, 0.0f, 0.0f);

	//GetGameEntityByName("Shiny Metal Sphere")->GetTransform()->SetPosition(+4.0f, +1.0f, 0.0f);
	//GetGameEntityByName("Rough Metal Sphere")->GetTransform()->SetPosition(+5.0f, +1.0f, 0.0f);
	//GetGameEntityByName("Shiny Sphere")->GetTransform()->SetPosition(+4.0f, 0.0f, 0.0f);
	//GetGameEntityByName("Shiny Rough Sphere")->GetTransform()->SetPosition(+5.0f, 0.0f, 0.0f);

	//GetGameEntityByName("Refractive Sphere")->GetTransform()->SetPosition(+4.0f, +1.0f, -1.0f);
	//GetGameEntityByName("Refractive Sphere 2")->GetTransform()->SetPosition(+5.0f, +1.0f, -1.0f);
	//GetGameEntityByName("Refractive Cube")->GetTransform()->SetPosition(+4.0f, +0.0f, -1.0f);
	//GetGameEntityByName("Refractive Torus")->GetTransform()->SetPosition(+5.0f, +0.0f, -1.0f);

	////Set up some parenting examples
	// This data is NOT stored in scene at the moment
	//GetGameEntityByName("Floor Cube")->GetTransform()->SetParent(GetGameEntityByName("Bronze Cube")->GetTransform());
	//GetGameEntityByName("Scratched Cube")->GetTransform()->SetParent(GetGameEntityByName("Floor Cube")->GetTransform());
	//GetGameEntityByName("Rough Torus")->GetTransform()->SetParent(GetGameEntityByName("Paint Sphere")->GetTransform());
	//GetGameEntityByName("Floor Helix")->GetTransform()->SetParent(GetGameEntityByName("Rough Torus")->GetTransform());
	//GetGameEntityByName("Stone Cylinder")->GetTransform()->SetParent(GetGameEntityByName("Floor Helix")->GetTransform());
	//GetGameEntityByName("Wood Sphere")->GetTransform()->SetParent(GetGameEntityByName("Floor Helix")->GetTransform());

	//CreateComplexGeometry();
}

void AssetManager::InitializeTextures() {
	CreateTexture("BlankAlbedo.png", "BlankTexture", ASSET_TEXTURE_PATH_PBR_ALBEDO);
	CreateTexture("GenericRoughness100.png", "HighRoughness", ASSET_TEXTURE_PATH_PBR_ROUGHNESS);

	CreateTexture("bronze_albedo.png", "BronzeAlbedo", ASSET_TEXTURE_PATH_PBR_ALBEDO);
	CreateTexture("bronze_normals.png", "BronzeNormals", ASSET_TEXTURE_PATH_PBR_NORMALS);
	CreateTexture("bronze_metal.png", "BronzeMetal", ASSET_TEXTURE_PATH_PBR_METALNESS);
	CreateTexture("bronze_roughness.png", "BronzeRough", ASSET_TEXTURE_PATH_PBR_ROUGHNESS);

	CreateTexture("cobblestone_albedo.png", "CobbleAlbedo", ASSET_TEXTURE_PATH_PBR_ALBEDO);
	CreateTexture("cobblestone_normals.png", "CobbleNormals", ASSET_TEXTURE_PATH_PBR_NORMALS);
	CreateTexture("cobblestone_metal.png", "CobbleMetal", ASSET_TEXTURE_PATH_PBR_METALNESS);
	CreateTexture("cobblestone_roughness.png", "CobbleRough", ASSET_TEXTURE_PATH_PBR_ROUGHNESS);

	CreateTexture("floor_albedo.png", "FloorAlbedo", ASSET_TEXTURE_PATH_PBR_ALBEDO);
	CreateTexture("floor_normals.png", "FloorNormals", ASSET_TEXTURE_PATH_PBR_NORMALS);
	CreateTexture("floor_metal.png", "FloorMetal", ASSET_TEXTURE_PATH_PBR_METALNESS);
	CreateTexture("floor_roughness.png", "FloorRough", ASSET_TEXTURE_PATH_PBR_ROUGHNESS);

	CreateTexture("paint_albedo.png", "PaintAlbedo", ASSET_TEXTURE_PATH_PBR_ALBEDO);
	CreateTexture("paint_normals.png", "PaintNormals", ASSET_TEXTURE_PATH_PBR_NORMALS);
	CreateTexture("paint_metal.png", "PaintMetal", ASSET_TEXTURE_PATH_PBR_METALNESS);
	CreateTexture("paint_roughness.png", "PaintRough", ASSET_TEXTURE_PATH_PBR_ROUGHNESS);

	CreateTexture("wood_albedo.png", "WoodAlbedo", ASSET_TEXTURE_PATH_PBR_ALBEDO);
	CreateTexture("wood_normals.png", "WoodNormals", ASSET_TEXTURE_PATH_PBR_NORMALS);
	CreateTexture("wood_metal.png", "WoodMetal", ASSET_TEXTURE_PATH_PBR_METALNESS);
	CreateTexture("wood_roughness.png", "WoodRough", ASSET_TEXTURE_PATH_PBR_ROUGHNESS);

	CreateTexture("scratched_albedo.png", "ScratchAlbedo", ASSET_TEXTURE_PATH_PBR_ALBEDO);
	CreateTexture("scratched_normals.png", "ScratchNormals", ASSET_TEXTURE_PATH_PBR_NORMALS);
	CreateTexture("scratched_metal.png", "ScratchMetal", ASSET_TEXTURE_PATH_PBR_METALNESS);
	CreateTexture("scratched_roughness.png", "ScratchRough", ASSET_TEXTURE_PATH_PBR_ROUGHNESS);

	CreateTexture("rough_albedo.png", "RoughAlbedo", ASSET_TEXTURE_PATH_PBR_ALBEDO);
	CreateTexture("rough_normals.png", "RoughNormals", ASSET_TEXTURE_PATH_PBR_NORMALS);
	CreateTexture("rough_metal.png", "RoughMetal", ASSET_TEXTURE_PATH_PBR_METALNESS);
	CreateTexture("rough_roughness.png", "RoughRough", ASSET_TEXTURE_PATH_PBR_ROUGHNESS);
}

void AssetManager::InitializeMaterials() {
	// Make reflective PBR materials
	CreatePBRMaterial(std::string("reflectiveMetal"),
		GetTextureByName("BlankTexture"),
		GetTextureByName("BlankTexture"),
		GetTextureByName("BronzeMetal"),
		GetTextureByName("BlankTexture"));

	CreatePBRMaterial(std::string("reflective"),
		GetTextureByName("BlankTexture"),
		GetTextureByName("BlankTexture"),
		GetTextureByName("WoodMetal"),
		GetTextureByName("BlankTexture"));

	CreatePBRMaterial(std::string("reflectiveRough"),
		GetTextureByName("BlankTexture"),
		GetTextureByName("BlankTexture"),
		GetTextureByName("WoodMetal"),
		GetTextureByName("HighRoughness"));

	CreatePBRMaterial(std::string("reflectiveRoughMetal"),
		GetTextureByName("BlankTexture"),
		GetTextureByName("BlankTexture"),
		GetTextureByName("BronzeMetal"),
		GetTextureByName("HighRoughness"));

	//Make PBR materials
	CreatePBRMaterial(std::string("bronzeMat"),
		GetTextureByName("BronzeAlbedo"),
		GetTextureByName("BronzeNormals"),
		GetTextureByName("BronzeMetal"),
		GetTextureByName("BronzeRough"))->SetTiling(0.3f);

	CreatePBRMaterial(std::string("cobbleMat"),
		GetTextureByName("CobbleAlbedo"),
		GetTextureByName("CobbleNormals"),
		GetTextureByName("CobbleMetal"),
		GetTextureByName("CobbleRough"));

	CreatePBRMaterial(std::string("largeCobbleMat"),
		GetTextureByName("CobbleAlbedo"),
		GetTextureByName("CobbleNormals"),
		GetTextureByName("CobbleMetal"),
		GetTextureByName("CobbleRough"))->SetTiling(5.0f);

	CreatePBRMaterial(std::string("floorMat"),
		GetTextureByName("FloorAlbedo"),
		GetTextureByName("FloorNormals"),
		GetTextureByName("FloorMetal"),
		GetTextureByName("FloorRough"));

	CreatePBRMaterial(std::string("terrainFloorMat"),
		GetTextureByName("FloorAlbedo"),
		GetTextureByName("FloorNormals"),
		GetTextureByName("FloorMetal"),
		GetTextureByName("FloorRough"))->SetTiling(256.0f);

	CreatePBRMaterial(std::string("paintMat"),
		GetTextureByName("PaintAlbedo"),
		GetTextureByName("PaintNormals"),
		GetTextureByName("PaintMetal"),
		GetTextureByName("PaintRough"));

	CreatePBRMaterial(std::string("largePaintMat"),
		GetTextureByName("PaintAlbedo"),
		GetTextureByName("PaintNormals"),
		GetTextureByName("PaintMetal"),
		GetTextureByName("PaintRough"))->SetTiling(5.0f);

	CreatePBRMaterial(std::string("roughMat"),
		GetTextureByName("RoughAlbedo"),
		GetTextureByName("RoughNormals"),
		GetTextureByName("RoughMetal"),
		GetTextureByName("RoughRough"));

	CreatePBRMaterial(std::string("scratchMat"),
		GetTextureByName("ScratchAlbedo"),
		GetTextureByName("ScratchNormals"),
		GetTextureByName("ScratchMetal"),
		GetTextureByName("ScratchRough"));

	CreatePBRMaterial(std::string("woodMat"),
		GetTextureByName("WoodAlbedo"),
		GetTextureByName("WoodNormals"),
		GetTextureByName("WoodMetal"),
		GetTextureByName("WoodRough"));

	std::shared_ptr<Material> refractive = CreatePBRMaterial(std::string("refractivePaintMat"),
		GetTextureByName("PaintAlbedo"),
		GetTextureByName("PaintNormals"),
		GetTextureByName("PaintMetal"),
		GetTextureByName("PaintRough"));
	refractive->SetRefractive(true);
	refractive->SetRefractivePixelShader(GetPixelShaderByName("RefractivePS"));

	refractive = CreatePBRMaterial(std::string("refractiveWoodMat"),
		GetTextureByName("WoodAlbedo"),
		GetTextureByName("WoodNormals"),
		GetTextureByName("WoodMetal"),
		GetTextureByName("WoodRough"));
	refractive->SetTransparent(true);
	refractive->SetRefractivePixelShader(GetPixelShaderByName("RefractivePS"));

	refractive = CreatePBRMaterial(std::string("refractiveRoughMat"),
		GetTextureByName("RoughAlbedo"),
		GetTextureByName("RoughNormals"),
		GetTextureByName("RoughMetal"),
		GetTextureByName("RoughRough"));
	refractive->SetRefractive(true);
	refractive->SetRefractivePixelShader(GetPixelShaderByName("RefractivePS"));

	refractive = CreatePBRMaterial(std::string("refractiveBronzeMat"),
		GetTextureByName("BronzeAlbedo"),
		GetTextureByName("BronzeNormals"),
		GetTextureByName("BronzeMetal"),
		GetTextureByName("BronzeRough"));
	refractive->SetRefractive(true);
	refractive->SetRefractivePixelShader(GetPixelShaderByName("RefractivePS"));
}

void AssetManager::InitializeMeshes() {
	// Test loading failure
	//CreateMesh("ExceptionTest", "InvalidPath");

	CreateMesh("Cube", "cube.obj");
	CreateMesh("Sphere", "sphere.obj");
	CreateMesh("Cylinder", "cylinder.obj");
	//CreateMesh("Helix", "helix.obj");
	//CreateMesh("Torus", "torus.obj");
}


void AssetManager::InitializeSkies() {
	// Temporarily, we only load 3 skies, as they take a while to load

	//CreateSky(spaceTexture, "space");
	CreateSky("SunnyCubeMap.dds", 0, "sunny");
	//CreateSky(mountainTexture, "mountain");
	//CreateSky("Niagara/", 1, "niagara", ".jpg");
	// Default is .png, which this is
	//CreateSky("Stars/", 1, "stars");

	currentSky = skies[0];
}

void AssetManager::InitializeLights() {
	//white light from the top left
	std::shared_ptr<Light> mainLight = CreateDirectionalLight("MainLight", DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f), 0.7f);
	mainLight->GetTransform()->SetPosition(DirectX::XMFLOAT3(0.0f, 20.0f, -200.0f));
	mainLight->GetTransform()->Rotate(XM_PIDIV2, XM_PI, 0);
	mainLight->SetCastsShadows(true);

	//white light from the back
	CreateDirectionalLight("BackLight", DirectX::XMFLOAT3(0, 0, -1))->SetEnabled(false);

	// OUTDATED: Initializes lots of objects for demo.
	// These can still be viewed by loading the demo scene.
	////red light on the bottom
	//std::shared_ptr<Light> bottomLight = CreateDirectionalLight("BottomLight", DirectX::XMFLOAT3(1.0f, 0.2f, 0.2f));
	//bottomLight->GetTransform()->Rotate(-XM_PIDIV2, 0, 0);

	////red pointlight in the center
	//std::shared_ptr<Light> centerLight = CreatePointLight("CenterLight", 2.0f, DirectX::XMFLOAT3(0.1f, 1.0f, 0.2f));
	//centerLight->GetTransform()->SetPosition(DirectX::XMFLOAT3(0, 1.5f, 0));

	//flashlight attached to camera +.5z and x
	std::shared_ptr<Light> flashlight = CreateSpotLight("Flashlight", 10.0f);
	flashlight->GetGameEntity()->AddComponent<FlashlightController>();
	flashlight->GetTransform()->SetParent(mainCamera->GetTransform());
	flashlight->GetTransform()->SetPosition(DirectX::XMFLOAT3(0.5f, 0.0f, 0.5f));
	flashlight->SetCastsShadows(true);
	flashlight->GetGameEntity()->SetEnabled(false);
}

void AssetManager::InitializeTerrainEntities() {
	Terrain::SetDefaults(GetMeshByName("Cube"), GetTerrainMaterialByName("Forest Terrain Material"));

	// OUTDATED: Initializes lots of objects for demo.
	// These can still be viewed by loading the demo scene.
	std::shared_ptr<Terrain> mainTerrain = CreateTerrainEntity("valley.raw16", GetTerrainMaterialByName("Forest Terrain Material"), "Basic Terrain");
	mainTerrain->GetTransform()->SetPosition(-256.0f, -14.0f, -256.0f);
	mainTerrain->SetEnabled(false);
}

void AssetManager::InitializeTerrainMaterials() {
	std::vector<std::shared_ptr<Material>> tMats = std::vector<std::shared_ptr<Material>>();

	// OUTDATED: Initializes lots of objects for demo.
	// These can still be viewed by loading the demo scene.

	std::shared_ptr<Texture> albedoTexture;
	std::shared_ptr<Texture> normalTexture;
	std::shared_ptr<Texture> roughTexture;

	albedoTexture = CreateTexture("forest_floor_albedo.png", "ForestAlbedo", ASSET_TEXTURE_PATH_PBR_ALBEDO);
	normalTexture = CreateTexture("forest_floor_Normal-ogl.png", "ForestNormals", ASSET_TEXTURE_PATH_PBR_NORMALS);
	roughTexture = CreateTexture("forest_floor_Roughness.png", "ForestRough", ASSET_TEXTURE_PATH_PBR_ROUGHNESS);

	std::shared_ptr<Material> forestMat = CreatePBRMaterial("Forest TMaterial", albedoTexture, normalTexture, GetTextureByName("WoodMetal"), roughTexture);

	albedoTexture = CreateTexture("bog_albedo.png", "BogAlbedo", ASSET_TEXTURE_PATH_PBR_ALBEDO);
	normalTexture = CreateTexture("bog_normal-ogl.png", "BogNormals", ASSET_TEXTURE_PATH_PBR_NORMALS);
	roughTexture = CreateTexture("bog_roughness.png", "BogRough", ASSET_TEXTURE_PATH_PBR_ROUGHNESS);

	std::shared_ptr<Material> bogMat = CreatePBRMaterial("Bog TMaterial", albedoTexture, normalTexture, GetTextureByName("WoodMetal"), roughTexture);

	albedoTexture = CreateTexture("rocky_dirt1-albedo.png", "RockyAlbedo", ASSET_TEXTURE_PATH_PBR_ALBEDO);
	normalTexture = CreateTexture("rocky_dirt1-normal-ogl.png", "RockyNormals", ASSET_TEXTURE_PATH_PBR_NORMALS);
	roughTexture = CreateTexture("rocky_dirt1_Roughness.png", "RockyRough", ASSET_TEXTURE_PATH_PBR_ROUGHNESS);

	std::shared_ptr<Material> rockyMat = CreatePBRMaterial("Rocky TMaterial", albedoTexture, normalTexture, GetTextureByName("WoodMetal"), roughTexture);

	tMats.push_back(forestMat);
	tMats.push_back(bogMat);
	tMats.push_back(rockyMat);

	//Set appropriate tiling
	forestMat->SetTiling(10.0f);
	bogMat->SetTiling(10.0f);

	std::shared_ptr<TerrainMaterial> forestTerrainMaterial = CreateTerrainMaterial("Forest Terrain Material", tMats, "blendMap.png");

	tMats.clear();

	// OUTDATED: Initializes lots of objects for demo.
	// These can still be viewed by loading the demo scene.
	//// Must be kept in PBR order!
	//// This sections is only for testing and should be commented on push.
	//// Since these materials already exist, it's quicker and more efficient
	//// to just grab them.
	//std::shared_ptr<TerrainMaterial> industrialTerrainMaterial;

	////std::vector<std::string> metalPaths;
	////std::vector<std::string> metalNames;

	///*metalPaths.push_back("floor_albedo.png");
	//metalPaths.push_back("floor_normals.png");
	//metalPaths.push_back("floor_metal.png");
	//metalPaths.push_back("floor_roughness.png");

	//metalPaths.push_back("cobblestone_albedo.png");
	//metalPaths.push_back("cobblestone_normals.png");
	//metalPaths.push_back("cobblestone_metal.png");
	//metalPaths.push_back("cobblestone_roughness.png");

	//metalPaths.push_back("rough_albedo.png");
	//metalPaths.push_back("rough_normals.png");
	//metalPaths.push_back("rough_metal.png");
	//metalPaths.push_back("rough_roughness.png");

	//metalNames.push_back("Floor");
	//metalNames.push_back("Stone");
	//metalNames.push_back("Rough");

	//industrialTerrainMaterial = CreateTerrainMaterial("Industrial Terrain Material", metalPaths, metalNames, true, "blendMap.png");*/

	//// This is the correct way to create a tMat when the materials are already loaded:
	//tMats.push_back(GetMaterialByName("floorMat"));
	//tMats.push_back(GetMaterialByName("cobbleMat"));
	//tMats.push_back(GetMaterialByName("roughMat"));

	//industrialTerrainMaterial = CreateTerrainMaterial("Industrial Terrain Material", tMats, "blendMap.png");

	//// This is the correct way to load a tMat that doesn't use blend mapping
	//// Note that even with one material, it must be pushed to the vector
	//std::shared_ptr<TerrainMaterial> floorTerrainMaterial;

	//tMats.clear();

	//tMats.push_back(GetMaterialByName("terrainFloorMat"));

	//floorTerrainMaterial = CreateTerrainMaterial("Floor Terrain Material", tMats);
}

void AssetManager::InitializeCameras() {
	mainCamera = CreateCamera("mainCamera");
	mainCamera->GetTransform()->SetPosition(DirectX::XMFLOAT3(0.0f, 0.0f, -20.0f));
	mainCamera->GetGameEntity()->AddComponent<NoclipMovement>();
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
	CreatePixelShader("CompressRGBPS", "PSRGBCompress.cso");

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

	// OUTDATED: Initializes lots of objects for demo.
	// These can still be viewed by loading the demo scene.
	/*std::shared_ptr<ParticleSystem> basicMultiEmitter = CreateParticleEmitter("basicParticles", "Smoke/", true);
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
	starEmitter->SetEnabled(false);*/

	// This was terrifying. Take graphics ideas from Jimmy Digrazia at your own peril.
	/*CreateParticleEmitter(10, 4.0f, 2.0f, DirectX::XMFLOAT3(-4.0f, 0.0f, 0.0f), L"Emoji/", "emojiParticles", true, false);
	GetEmitterByName("emojiParticles")->SetColorTint(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f));*/
}

void AssetManager::InitializeAudio() {
	audioInstance.Initialize();

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
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(device.Get(), context.Get());
}

void AssetManager::InitializeColliders() {
	// OUTDATED: Initializes lots of objects for demo.
	// These can still be viewed by loading the demo scene.
	//CreateColliderOnEntity(GetGameEntityByName("Bronze Cube"));
	//CreateColliderOnEntity(GetGameEntityByName("Scratched Sphere"));
	//CreateColliderOnEntity(GetGameEntityByName("Shiny Rough Sphere"));
	//CreateColliderOnEntity(GetGameEntityByName("Floor Cube"));
	//CreateTriggerBoxOnEntity(GetGameEntityByName("Scratched Cube"));
}
#pragma endregion

#pragma region importMethods

void AssetManager::ImportTexture() {
	char filename[MAX_PATH];
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(ofn));
	ZeroMemory(&filename, sizeof(filename));
	ofn.lpstrFilter = _T("Image Files\0*.png;*.jpg;*.jpeg;\0Any File\0*.*\0");
	ofn.lpstrTitle = _T("Select a texture file:");
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = dxInstance->hWnd;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn)) {
		CreateTexture(ofn.lpstrFile, "newTexture", ASSET_TEXTURE_PATH_BASIC, true);
	}
}

void AssetManager::ImportSkyTexture() {
	// unimplemented, requires some regex and stuff to deal with dds vs path
	/*OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lpstrFilter = _T("Image Files\0*.png;*.jpg;*.jpeg;\0Any File\0*.*\0");
	ofn.lpstrTitle = _T("Select a texture file:");

	CreateSky(GetImportedFileString(&ofn), "newTexture", ASSET_TEXTURE_PATH_BASIC, true);*/
}

void AssetManager::ImportHeightMap() {
	// unimplemented, heightmap is tied to terrain for now
	/*OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lpstrFilter = _T("Heightmap Files\0*.raw;*.raw16;\0Any File\0*.*\0");
	ofn.lpstrTitle = _T("Select a heightmap file:");

	CreateTexture(GetImportedFileString(&ofn), "newTexture", ASSET_TEXTURE_PATH_BASIC, true);*/
}

void AssetManager::ImportSound() {
	char filename[MAX_PATH];
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(ofn));
	ZeroMemory(&filename, sizeof(filename));
	ofn.lpstrFilter = _T("Sound Files\0*.wav;*.mp3;*.ogg;\0Any File\0*.*\0");
	ofn.lpstrTitle = _T("Select a Sound file:");
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = dxInstance->hWnd;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn)) {
		CreateSound(ofn.lpstrFile, FMOD_DEFAULT, "newSound", true);
	}
}

void AssetManager::ImportFont() {
	char filename[MAX_PATH];
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(ofn));
	ZeroMemory(&filename, sizeof(filename));
	ofn.lpstrFilter = _T("Spritefont Files\0*.spritefont;\0Any File\0*.*\0");
	ofn.lpstrTitle = _T("Select a font file:");
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = dxInstance->hWnd;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn)) {
		CreateSHOEFont("newFont", ofn.lpstrFile, false, true);
	}
}

void AssetManager::ImportMesh() {
	char filename[MAX_PATH];
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(ofn));
	ZeroMemory(&filename, sizeof(filename));
	ofn.lpstrFilter = _T("Mesh or Model Files\0*.obj;\0Any File\0*.*\0");
	ofn.lpstrTitle = _T("Select a mesh file:");
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = dxInstance->hWnd;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn)) {
		CreateMesh("NewMesh", ofn.lpstrFile, true);
	}
}

std::string AssetManager::GetImportedFileString(OPENFILENAME* file) {
	

	return "0";
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

size_t AssetManager::GetMeshArraySize() {
	return this->globalMeshes.size();
}

size_t AssetManager::GetTextureArraySize() {
	return this->globalTextures.size();
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

std::shared_ptr<Texture> AssetManager::GetTextureAtID(int id) {
	return this->globalTextures[id];
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
	if (mainCamera != nullptr)
		return mainCamera;
	return editingCamera;
}

void AssetManager::SetMainCamera(std::shared_ptr<Camera> newMain)
{
	mainCamera = newMain;
	//mainCamera->SetAspectRatio((float)windowWidth / (float)this->height);
}

std::shared_ptr<Camera> AssetManager::GetEditingCamera() {
	return editingCamera;
}

void AssetManager::UpdateEditingCamera()
{
	editingCamera->GetGameEntity()->PropagateEvent(EntityEventType::Update);
}

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

void AssetManager::CleanAllEntities()
{
	for (auto ge : globalEntities) {
		ge->Release();
	}
	globalEntities.clear();
}

void AssetManager::CleanAllVectors() {
	CleanAllEntities();

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
	globalMeshes.clear();
	globalTextures.clear();
	globalMaterials.clear();
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

std::shared_ptr<Texture> AssetManager::GetTextureByName(std::string name) {
	for (int i = 0; i < globalTextures.size(); i++) {
		if (globalTextures[i]->GetName() == name) {
			return globalTextures[i];
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
