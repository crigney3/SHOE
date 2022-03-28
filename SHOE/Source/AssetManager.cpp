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

	// This must occur before the loading screen starts
	InitializeFonts();

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

	this->assetManagerLoadState = AMLoadState::SINGLE_CREATION;
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
		// This a new asset being loaded without a loading screen
		return;
	}
}
#pragma endregion

#pragma region createAssets
bool AssetManager::materialSortDirty = false;

FMOD::Sound* AssetManager::CreateSound(std::string path, FMOD_MODE mode) {
	try
	{
		FMOD::Sound* sound;
		std::string assetPath = "../../../Assets/Sounds/";

		sound = audioInstance.LoadSound(dxInstance->GetFullPathTo(assetPath + path), mode);

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

std::shared_ptr<SimpleVertexShader> AssetManager::CreateVertexShader(std::string id, std::wstring nameToLoad) {
	try {
		std::shared_ptr<SimpleVertexShader> newVS;

		newVS = std::make_shared<SimpleVertexShader>(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(nameToLoad).c_str(), id);

		vertexShaders.push_back(newVS);

		SetLoadedAndWait("Vertex Shaders", id);

		return newVS;
	}
	catch (...) {
		SetLoadedAndWait("Vertex Shaders", id, std::current_exception());

		return NULL;
	}
}

std::shared_ptr<SimplePixelShader> AssetManager::CreatePixelShader(std::string id, std::wstring nameToLoad) {
	try {
		std::shared_ptr<SimplePixelShader> newPS;

		newPS = std::make_shared<SimplePixelShader>(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(nameToLoad).c_str(), id);

		pixelShaders.push_back(newPS);

		SetLoadedAndWait("Pixel Shaders", id);

		return newPS;
	}
	catch (...) {
		SetLoadedAndWait("Pixel Shaders", id, std::current_exception());

		return NULL;
	}
}

std::shared_ptr<SimpleComputeShader> AssetManager::CreateComputeShader(std::string id, std::wstring nameToLoad) {
	try {
		std::shared_ptr<SimpleComputeShader> newCS;

		newCS = std::make_shared<SimpleComputeShader>(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(nameToLoad).c_str(), id);

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

		assetPath = "../../../Assets/Models/";

		newMesh = std::make_shared<Mesh>(dxInstance->GetFullPathTo(assetPath + nameToLoad).c_str(), device, id);

		globalMeshes.push_back(newMesh);

		SetLoadedAndWait("Meshes", id);

		return newMesh;
	}
	catch (...) {
		SetLoadedAndWait("Mesh", id, std::current_exception());

		return NULL;
	}
}

std::shared_ptr<Material> AssetManager::CreatePBRMaterial(std::string id,
														  std::wstring albedoNameToLoad,
														  std::wstring normalNameToLoad,
														  std::wstring metalnessNameToLoad,
														  std::wstring roughnessNameToLoad) {
	try {
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> albedo;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normals;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalness;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughness;
		std::shared_ptr<Material> newMat;
		std::wstring assetPathString;

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

		std::shared_ptr<SimpleVertexShader> VSNormal = GetVertexShaderByName("NormalsVS");
		std::shared_ptr<SimplePixelShader> PSNormal = GetPixelShaderByName("NormalsPS");

		assetPathString = L"../../../Assets/PBR/";

		CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(assetPathString + L"Albedo/" + albedoNameToLoad).c_str(), nullptr, &albedo);
		CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(assetPathString + L"Normals/" + normalNameToLoad).c_str(), nullptr, &normals);
		CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(assetPathString + L"Metalness/" + metalnessNameToLoad).c_str(), nullptr, &metalness);
		CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(assetPathString + L"Roughness/" + roughnessNameToLoad).c_str(), nullptr, &roughness);

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

		globalMaterials.push_back(newMat);

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

std::shared_ptr<Sky> AssetManager::CreateSky(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skyTexture, std::string name) {
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

		std::shared_ptr<Sky> newSky = std::make_shared<Sky>(textureState, skyTexture, Cube, importantSkyPixelShaders, importantSkyVertexShaders, device, context, name);

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

std::shared_ptr<SpriteFont> AssetManager::CreateSHOEFont(std::string name, std::wstring filePath, bool preInitializing) {
	try {
		std::wstring assetPath = L"../../../Assets/Fonts/";

		std::shared_ptr<SpriteFont> newFont = std::make_shared<SpriteFont>(device.Get(), dxInstance->GetFullPathTo_Wide(assetPath + filePath).c_str());

		globalFonts.emplace(name, newFont);

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
					  L"BlankAlbedo.png",
					  L"blank_normals.png",
					  L"bronze_metal.png",
					  L"GenericRoughness0.png");

	CreatePBRMaterial(std::string("reflective"),
					  L"BlankAlbedo.png",
					  L"blank_normals.png",
					  L"wood_metal.png",
					  L"GenericRoughness0.png");

	CreatePBRMaterial(std::string("reflectiveRough"),
					  L"BlankAlbedo.png",
					  L"blank_normals.png",
					  L"wood_metal.png",
					  L"GenericRoughness100.png");

	CreatePBRMaterial(std::string("reflectiveRoughMetal"),
					  L"BlankAlbedo.png",
					  L"blank_normals.png",
					  L"bronze_metal.png",
					  L"GenericRoughness100.png");

	//Make PBR materials
	CreatePBRMaterial(std::string("bronzeMat"),
					  L"bronze_albedo.png",
					  L"bronze_normals.png",
					  L"bronze_metal.png",
					  L"bronze_roughness.png")->SetTiling(0.3f);

	CreatePBRMaterial(std::string("cobbleMat"),
					  L"cobblestone_albedo.png",
					  L"cobblestone_normals.png",
					  L"cobblestone_metal.png",
					  L"cobblestone_roughness.png");

	CreatePBRMaterial(std::string("largeCobbleMat"),
					  L"cobblestone_albedo.png",
					  L"cobblestone_normals.png",
					  L"cobblestone_metal.png",
					  L"cobblestone_roughness.png")->SetTiling(5.0f);

	CreatePBRMaterial(std::string("floorMat"),
					  L"floor_albedo.png",
					  L"floor_normals.png",
					  L"floor_metal.png",
					  L"floor_roughness.png");

	CreatePBRMaterial(std::string("terrainFloorMat"),
					  L"floor_albedo.png",
					  L"floor_normals.png",
					  L"floor_metal.png",
					  L"floor_roughness.png")->SetTiling(256.0f);

	CreatePBRMaterial(std::string("paintMat"),
					  L"paint_albedo.png",
					  L"paint_normals.png",
					  L"paint_metal.png",
					  L"paint_roughness.png");

	CreatePBRMaterial(std::string("largePaintMat"),
					  L"paint_albedo.png",
					  L"paint_normals.png",
					  L"paint_metal.png",
					  L"paint_roughness.png")->SetTiling(5.0f);

	CreatePBRMaterial(std::string("roughMat"),
					  L"rough_albedo.png",
					  L"rough_normals.png",
					  L"rough_metal.png",
					  L"rough_roughness.png");

	CreatePBRMaterial(std::string("scratchMat"),
					  L"scratched_albedo.png",
					  L"scratched_normals.png",
					  L"scratched_metal.png",
					  L"scratched_roughness.png");

	CreatePBRMaterial(std::string("woodMat"),
					  L"wood_albedo.png",
					  L"wood_normals.png",
					  L"wood_metal.png",
					  L"wood_roughness.png");

	/*CreatePBRMaterial(std::string("transparentScratchMat"),
					  L"scratched_albedo.png",
					  L"scratched_normals.png",
					  L"scratched_metal.png",
					  L"scratched_roughness.png")->SetTransparent(true);*/

	CreatePBRMaterial(std::string("refractivePaintMat"),
					  L"paint_albedo.png",
					  L"paint_normals.png",
					  L"paint_metal.png",
					  L"paint_roughness.png")->SetRefractive(true);
	GetMaterialByName("refractivePaintMat")->SetRefractivePixelShader(GetPixelShaderByName("RefractivePS"));

	CreatePBRMaterial(std::string("refractiveWoodMat"),
					  L"wood_albedo.png",
					  L"wood_normals.png",
					  L"wood_metal.png",
					  L"wood_roughness.png")->SetTransparent(true);
	GetMaterialByName("refractiveWoodMat")->SetRefractivePixelShader(GetPixelShaderByName("RefractivePS"));

	CreatePBRMaterial(std::string("refractiveRoughMat"),
					  L"rough_albedo.png",
					  L"rough_normals.png",
					  L"rough_metal.png",
					  L"rough_roughness.png")->SetRefractive(true);
	GetMaterialByName("refractiveRoughMat")->SetRefractivePixelShader(GetPixelShaderByName("RefractivePS"));

	CreatePBRMaterial(std::string("refractiveBronzeMat"),
					  L"bronze_albedo.png",
					  L"bronze_normals.png",
					  L"bronze_metal.png",
					  L"bronze_roughness.png")->SetRefractive(true);
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

	std::shared_ptr<SimpleVertexShader> VSSky = GetVertexShaderByName("SkyVS");
	std::shared_ptr<SimplePixelShader> PSSky = GetPixelShaderByName("SkyPS");
	std::shared_ptr<Mesh> Cube = GetMeshByName("Cube");

	// Skybox map pointers
	// Temporarily, we only load 2 skies, as they take a while to load
	//Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> spaceTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sunnyTexture;
	//Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mountainTexture;
	//Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> niagaraTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> starTexture;

	//CreateDDSTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/SpaceCubeMap.dds").c_str(), nullptr, &spaceTexture);
	CreateDDSTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/SunnyCubeMap.dds").c_str(), nullptr, &sunnyTexture);

	/*mountainTexture = CreateCubemap(dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Mountain/right.jpg").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Mountain/left.jpg").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Mountain/up.jpg").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Mountain/down.jpg").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Mountain/forward.jpg").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Mountain/back.jpg").c_str());
	niagaraTexture = CreateCubemap(dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Niagara/right.jpg").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Niagara/left.jpg").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Niagara/up.jpg").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Niagara/down.jpg").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Niagara/forward.jpg").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Niagara/back.jpg").c_str());*/
	starTexture = CreateCubemap(dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Stars/right.png").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Stars/left.png").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Stars/up.png").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Stars/down.png").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Stars/forward.png").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Stars/back.png").c_str());

	//CreateSky(spaceTexture, "space");
	CreateSky(sunnyTexture, "sunny");
	//CreateSky(mountainTexture, "mountain");
	//CreateSky(niagaraTexture, "niagara");
	CreateSky(starTexture, "stars");

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
		globalTerrainMaterials = std::vector<std::shared_ptr<TerrainMats>>();

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bogAlbedo;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> forestAlbedo;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rockyAlbedo;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bogMetal;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> forestMetal;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rockyMetal;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bogNormal;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> forestNormal;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rockyNormal;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bogRough;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> forestRough;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> rockyRough;

		CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Albedo/bog_albedo.png").c_str(), nullptr, &bogAlbedo);
		CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Albedo/forest_floor_albedo.png").c_str(), nullptr, &forestAlbedo);
		CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Albedo/rocky_dirt1-albedo.png").c_str(), nullptr, &rockyAlbedo);

		SetLoadedAndWait("Terrain", "TerrainMatAlbedo");

		CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Metalness/wood_metal.png").c_str(), nullptr, &bogMetal); //These metal maps are blank, so just load the wood
		CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Metalness/wood_metal.png").c_str(), nullptr, &forestMetal); //Could maybe optimize loading times with memcpy
		CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Metalness/wood_metal.png").c_str(), nullptr, &rockyMetal);

		SetLoadedAndWait("Terrain", "TerrainMatMetal");

		CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Normals/bog_normal-ogl.png").c_str(), nullptr, &bogNormal);
		CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Normals/forest_floor_Normal-ogl.png").c_str(), nullptr, &forestNormal);
		CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Normals/rocky_dirt1-normal-ogl.png").c_str(), nullptr, &rockyNormal);

		SetLoadedAndWait("Terrain", "TerrainMatNormal");

		CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Roughness/bog_roughness.png").c_str(), nullptr, &bogRough);
		CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Roughness/forest_floor_Roughness.png").c_str(), nullptr, &forestRough);
		CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Roughness/rocky_dirt1_Roughness.png").c_str(), nullptr, &rockyRough);

		//Load terrain and add a blend map
		std::shared_ptr<Mesh> mainTerrain = LoadTerrain(dxInstance->GetFullPathTo("../../../Assets/HeightMaps/valley.raw16").c_str(), 512, 512, 25.0f);
		globalMeshes.push_back(mainTerrain);
		std::shared_ptr<TerrainMats> mainTerrainMaterials = std::make_shared<TerrainMats>();
		mainTerrainMaterials->blendMaterials = std::vector<Material>();
		CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/blendMap.png").c_str(), nullptr, &mainTerrainMaterials->blendMap);
		mainTerrainMaterials->blendMaterials.push_back(Material(whiteTint, nullptr, nullptr, forestAlbedo, textureState, clampState, forestNormal, forestRough, forestMetal));
		mainTerrainMaterials->blendMaterials.push_back(Material(whiteTint, nullptr, nullptr, bogAlbedo, textureState, clampState, bogNormal, bogRough, bogMetal)); //Set these mats' shaders to null as they use the blend map shader
		mainTerrainMaterials->blendMaterials.push_back(Material(whiteTint, nullptr, nullptr, rockyAlbedo, textureState, clampState, rockyNormal, rockyRough, rockyMetal));

		//Set appropriate tiling
		mainTerrainMaterials->blendMaterials[0].SetTiling(10.0f);
		mainTerrainMaterials->blendMaterials[1].SetTiling(10.0f);
		mainTerrainMaterials->name = std::string("Forest TMaterial");

		globalTerrainMaterials.push_back(mainTerrainMaterials);

		SetLoadedAndWait("Terrain", "MainTerrainMaterial");
	}
	catch (...) {
		SetLoadedAndWait("Terrain Materials", "Unknown Terrain Material", std::current_exception());

		return;
	}
}

void AssetManager::InitializeCameras() {
	globalCameras = std::vector<std::shared_ptr<Camera>>();

	float aspectRatio = (float)(dxInstance->width / dxInstance->height);
	CreateCamera("mainCamera", DirectX::XMFLOAT3(0.0f, 0.0f, -20.0f), aspectRatio, 1);
	CreateCamera("mainShadowCamera", DirectX::XMFLOAT3(0.0f, 10.0f, -20.0f), 1.0f, 0);
	std::shared_ptr<Camera> fscTemp = CreateCamera("flashShadowCamera", DirectX::XMFLOAT3(0.0f, 0.0f, -5.5f), 1.0f, 1);
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
	CreateVertexShader("BasicVS", L"VertexShader.cso");
	CreateVertexShader("NormalsVS", L"VSNormalMap.cso");
	CreateVertexShader("SkyVS", L"VSSkybox.cso");
	CreateVertexShader("TerrainVS", L"VSTerrainBlend.cso");
	CreateVertexShader("ShadowVS", L"VSShadowMap.cso");
	CreateVertexShader("ParticlesVS", L"VSParticles.cso");
	CreateVertexShader("FullscreenVS", L"FullscreenVS.cso");

	// Make pixel shaders
	CreatePixelShader("BasicPS", L"PixelShader.cso");
	CreatePixelShader("SolidColorPS", L"PSSolidColor.cso");
	CreatePixelShader("NormalsPS", L"PSNormalMap.cso");
	CreatePixelShader("SkyPS", L"PSSkybox.cso");
	CreatePixelShader("TerrainPS", L"PSTerrainBlend.cso");
	CreatePixelShader("IrradiancePS", L"IBLIrradianceMapPS.cso");
	CreatePixelShader("SpecularConvolutionPS", L"IBLSpecularConvolutionPS.cso");
	CreatePixelShader("BRDFLookupTablePS", L"IBLBrdfLookUpTablePS.cso");
	CreatePixelShader("SSAOPS", L"PSAmbientOcclusion.cso");
	CreatePixelShader("SSAOBlurPS", L"PSOcclusionBlur.cso");
	CreatePixelShader("SSAOCombinePS", L"PSOcclusionCombine.cso");
	CreatePixelShader("ParticlesPS", L"PSParticles.cso");
	CreatePixelShader("TextureSamplePS", L"PSTextureSample.cso");
	CreatePixelShader("RefractivePS", L"PSRefractive.cso");

	// Make compute shaders
	CreateComputeShader("ParticleMoveCS", L"CSParticleFlow.cso");
	CreateComputeShader("ParticleEmitCS", L"CSParticleEmit.cso");
	CreateComputeShader("ParticleCopyCS", L"CSCopyDrawCount.cso");
	CreateComputeShader("ParticleInitDeadCS", L"CSInitDeadList.cso");
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
	
	CreateSound("PianoNotes/pinkyfinger__piano-a.wav", FMOD_DEFAULT);
	CreateSound("PianoNotes/pinkyfinger__piano-b.wav", FMOD_DEFAULT);
	CreateSound("PianoNotes/pinkyfinger__piano-bb.wav", FMOD_DEFAULT);
	CreateSound("PianoNotes/pinkyfinger__piano-c.wav", FMOD_DEFAULT);
	CreateSound("PianoNotes/pinkyfinger__piano-e.wav", FMOD_DEFAULT);
	CreateSound("PianoNotes/pinkyfinger__piano-eb.wav", FMOD_DEFAULT);
	CreateSound("PianoNotes/pinkyfinger__piano-d.wav", FMOD_DEFAULT);
	CreateSound("PianoNotes/pinkyfinger__piano-f.wav", FMOD_DEFAULT);
	CreateSound("PianoNotes/pinkyfinger__piano-g.wav", FMOD_DEFAULT);
}

void AssetManager::InitializeFonts() {
	globalFonts = std::map<std::string, std::shared_ptr<DirectX::SpriteFont>>();

	CreateSHOEFont("Roboto-Bold-72pt", L"RobotoCondensed-Bold-72pt.spritefont", true);
	CreateSHOEFont("SmoochSans-Bold", L"SmoochSans-Bold.spritefont", true);
	CreateSHOEFont("SmoochSans-Italic", L"SmoochSans-Italic.spritefont", true);
	CreateSHOEFont("Arial", L"Arial.spritefont");
	CreateSHOEFont("Roboto-Bold", L"RobotoCondensed-Bold.spritefont");
	CreateSHOEFont("Roboto-BoldItalic", L"RobotoCondensed-BoldItalic.spritefont");
	CreateSHOEFont("Roboto-Italic", L"RobotoCondensed-Italic.spritefont");
	CreateSHOEFont("Roboto-Regular", L"RobotoCondensed-Regular.spritefont");
	CreateSHOEFont("SmoochSans-BoldItalic", L"SmoochSans-BoldItalic.spritefont");
	CreateSHOEFont("SmoochSans-Regular", L"SmoochSans-Regular.spritefont");
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
			std::wstring path = ConvertToWide(p.path().string().c_str());

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

std::shared_ptr<TerrainMats> AssetManager::GetTerrainMaterialByName(std::string name) {
	for (int i = 0; i < globalTerrainMaterials.size(); i++) {
		if (globalTerrainMaterials[i]->name == name) {
			return globalTerrainMaterials[i];
		}
	}
	return nullptr;
}

//
// Dict return by key
//

std::shared_ptr<SpriteFont> AssetManager::GetFontByName(std::string name) {
	return globalFonts[name];
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

inline std::wstring AssetManager::ConvertToWide(const std::string& as)
{
	wchar_t* buf = new wchar_t[as.size() * 2 + 2];
	swprintf_s(buf, as.size() * 2 + 2, L"%S", as.c_str());
	std::wstring rval = buf;
	delete[] buf;
	return rval;
}

#pragma endregion