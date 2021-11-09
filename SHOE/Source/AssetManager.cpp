#include "../Headers/AssetManager.h"

using namespace DirectX;

#pragma region assetClassHandlers
AssetManager* AssetManager::instance;

AssetManager::~AssetManager() {
	// Everything should be smart-pointer managed
}

void AssetManager::Initialize(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) {
	dxInstance = DXCore::DXCoreInstance;
	this->context = context;
	this->device = device;

	InitializeShaders();
	InitializeCameras();
	InitializeLights();
	InitializeMaterials();
	InitializeMeshes();
	InitializeTerrainMaterials();
	InitializeGameEntities();
	InitializeSkies();
}
#pragma endregion

#pragma region createAssets
std::shared_ptr<Camera> AssetManager::CreateCamera(std::string id, DirectX::XMFLOAT3 pos, float aspectRatio, int type) {
	std::shared_ptr<Camera> newCam;

	newCam = std::make_shared<Camera>(pos, aspectRatio, type);

	this->globalCameras.emplace(id, newCam);

	return newCam;
}

std::shared_ptr<SimpleVertexShader> AssetManager::CreateVertexShader(std::string id, std::wstring nameToLoad) {
	std::shared_ptr<SimpleVertexShader> newVS;

	newVS = std::make_shared<SimpleVertexShader>(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(nameToLoad).c_str());

	vertexShaders.emplace(id, newVS);

	return newVS;
}

std::shared_ptr<SimplePixelShader> AssetManager::CreatePixelShader(std::string id, std::wstring nameToLoad) {
	std::shared_ptr<SimplePixelShader> newPS;

	newPS = std::make_shared<SimplePixelShader>(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(nameToLoad).c_str());

	pixelShaders.emplace(id, newPS);

	return newPS;
}

std::shared_ptr<Mesh> AssetManager::CreateMesh(std::string id, std::string nameToLoad) {
	std::shared_ptr<Mesh> newMesh;
	std::string assetPath;

	assetPath = "../../../Assets/Models/";

	newMesh = std::make_shared<Mesh>(dxInstance->GetFullPathTo(assetPath + nameToLoad).c_str(), device);

	globalMeshes.emplace(id, newMesh);

	return newMesh;
}

std::shared_ptr<Material> AssetManager::CreatePBRMaterial(std::string id,
														  std::wstring albedoNameToLoad,
														  std::wstring normalNameToLoad,
														  std::wstring metalnessNameToLoad,
														  std::wstring roughnessNameToLoad) {
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

	std::shared_ptr<SimpleVertexShader> VSNormal = vertexShaders.at("NormalsVS");
	std::shared_ptr<SimplePixelShader> PSNormal = pixelShaders.at("NormalsPS");

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
										metalness);

	globalMaterials.emplace(id, newMat);

	return newMat;
}

std::shared_ptr<GameEntity> AssetManager::CreateGameEntity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> mat, std::string name) {
	std::shared_ptr<GameEntity> newEnt = std::make_shared<GameEntity>(mesh, XMMatrixIdentity(), mat, name);

	globalEntities.emplace(name, newEnt);

	return newEnt;
}

std::shared_ptr<Sky> AssetManager::CreateSky(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skyTexture) {
	std::shared_ptr<Mesh> Cube = globalMeshes.at("Cube");

	std::shared_ptr<Sky> newSky = std::make_shared<Sky>(textureState, skyTexture, Cube, &pixelShaders, &vertexShaders, device, context);

	skies.push_back(newSky);

	return newSky;
}
#pragma endregion

#pragma region initAssets
void AssetManager::InitializeGameEntities() {
	globalEntities = std::map<std::string, std::shared_ptr<GameEntity>>();

	//Show example renders
	CreateGameEntity(globalMeshes.at("Cube"), globalMaterials.at("bronzeMat"), "Bronze Cube");
	CreateGameEntity(globalMeshes.at("Cylinder"), globalMaterials.at("floorMat"), "Stone Cylinder");
	CreateGameEntity(globalMeshes.at("Helix"), globalMaterials.at("floorMat"), "Floor Helix");
	CreateGameEntity(globalMeshes.at("Sphere"), globalMaterials.at("paintMat"), "Paint Sphere");
	CreateGameEntity(globalMeshes.at("Torus"), globalMaterials.at("roughMat"), "Rough Torus");
	CreateGameEntity(globalMeshes.at("Sphere"), globalMaterials.at("scratchMat"), "Scratched Sphere");
	CreateGameEntity(globalMeshes.at("Sphere"), globalMaterials.at("woodMat"), "Wood Sphere");
	CreateGameEntity(globalMeshes.at("Cube"), globalMaterials.at("largePaintMat"), "Large Paint Rect");
	CreateGameEntity(globalMeshes.at("Cube"), globalMaterials.at("largeCobbleMat"), "Large Cobble Rect");

	//Reflective objects
	CreateGameEntity(globalMeshes.at("Sphere"), globalMaterials.at("reflectiveMetal"), "Shiny Metal Sphere");
	CreateGameEntity(globalMeshes.at("Sphere"), globalMaterials.at("reflectiveRoughMetal"), "Rough Metal Sphere");
	CreateGameEntity(globalMeshes.at("Sphere"), globalMaterials.at("reflectiveRough"), "Shiny Rough Sphere");
	CreateGameEntity(globalMeshes.at("Sphere"), globalMaterials.at("reflective"), "Shiny Sphere");

	globalEntities.at("Bronze Cube")->GetTransform()->SetPosition(+0.7f, +0.0f, +0.0f);
	globalEntities.at("Stone Cylinder")->GetTransform()->SetPosition(-0.7f, +0.0f, +0.0f);
	globalEntities.at("Floor Helix")->GetTransform()->SetPosition(+0.0f, +0.7f, +0.0f);
	globalEntities.at("Paint Sphere")->GetTransform()->SetPosition(+0.0f, -0.7f, +0.0f);
	globalEntities.at("Scratched Sphere")->GetTransform()->SetPosition(+3.0f, +0.0f, +0.0f);
	globalEntities.at("Wood Sphere")->GetTransform()->SetPosition(-3.0f, +0.0f, +0.0f);
	globalEntities.at("Large Paint Rect")->GetTransform()->SetScale(10.0f, 10.0f, 0.2f);
	globalEntities.at("Large Paint Rect")->GetTransform()->SetPosition(+0.0f, +0.0f, +4.0f);
	globalEntities.at("Large Cobble Rect")->GetTransform()->SetScale(+10.0f, +10.0f, +0.2f);
	globalEntities.at("Large Cobble Rect")->GetTransform()->SetPosition(+0.0f, -5.0f, +0.0f);
	globalEntities.at("Large Cobble Rect")->GetTransform()->Rotate(45.0f, 0.0f, 0.0f);
	globalTerrainEntities.at("Main Terrain")->GetTransform()->SetPosition(-256.0f, -10.0f, -256.0f);

	globalEntities.at("Shiny Metal Sphere")->GetTransform()->SetPosition(+4.0f, +1.0f, 0.0f);
	globalEntities.at("Rough Metal Sphere")->GetTransform()->SetPosition(+5.0f, +1.0f, 0.0f);
	globalEntities.at("Shiny Sphere")->GetTransform()->SetPosition(+4.0f, 0.0f, 0.0f);
	globalEntities.at("Shiny Rough Sphere")->GetTransform()->SetPosition(+5.0f, 0.0f, 0.0f);

	//Set up some parenting examples
	globalEntities.at("Stone Cylinder")->GetTransform()->SetParent(globalEntities.at("Floor Helix")->GetTransform());
	globalEntities.at("Rough Torus")->GetTransform()->SetParent(globalEntities.at("Paint Sphere")->GetTransform());
	globalEntities.at("Floor Helix")->GetTransform()->SetParent(globalEntities.at("Rough Torus")->GetTransform());
	globalEntities.at("Wood Sphere")->GetTransform()->SetParent(globalEntities.at("Floor Helix")->GetTransform());

	CreateComplexGeometry();
}

void AssetManager::InitializeMaterials() {
	globalMaterials = std::map<std::string, std::shared_ptr<Material>>();

	// Make reflective PBR materials
	CreatePBRMaterial(std::string("reflectiveMetal"),
					  L"BlankAlbedo.png",
					  L"blank_normals.png",
					  L"bronze_metal.png",
					  L"GenericRoughness100.png");

	CreatePBRMaterial(std::string("reflective"),
					  L"BlankAlbedo.png",
					  L"blank_normals.png",
					  L"wood_metal.png",
					  L"GenericRoughness100.png");

	CreatePBRMaterial(std::string("reflectiveRough"),
					  L"BlankAlbedo.png",
					  L"blank_normals.png",
					  L"wood_metal.png",
					  L"GenericRoughness50.png");

	CreatePBRMaterial(std::string("reflectiveRoughMetal"),
					  L"BlankAlbedo.png",
					  L"blank_normals.png",
					  L"bronze_metal.png",
					  L"GenericRoughness50.png");

	//Make PBR materials
	CreatePBRMaterial(std::string("bronzeMat"),
					  L"bronze_albedo.png",
					  L"bronze_normals.png",
					  L"bronze_metal.png",
					  L"bronze_roughness.png");

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
}

void AssetManager::InitializeMeshes() {
	globalMeshes = std::map<std::string, std::shared_ptr<Mesh>>();

	CreateMesh("Cube", "cube.obj");
	CreateMesh("Cylinder", "cylinder.obj");
	CreateMesh("Helix", "helix.obj");
	CreateMesh("Sphere", "sphere.obj");
	CreateMesh("Torus", "torus.obj");
}

void AssetManager::InitializeSkies() {
	skies = std::vector<std::shared_ptr<Sky>>();

	std::shared_ptr<SimpleVertexShader> VSSky = vertexShaders.at("SkyVS");
	std::shared_ptr<SimplePixelShader> PSSky = pixelShaders.at("SkyPS");
	std::shared_ptr<Mesh> Cube = globalMeshes.at("Cube");

	//Skybox map pointers
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> spaceTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sunnyTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mountainTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> niagaraTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> starTexture;

	CreateDDSTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/SpaceCubeMap.dds").c_str(), nullptr, &spaceTexture);
	CreateDDSTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/SunnyCubeMap.dds").c_str(), nullptr, &sunnyTexture);

	mountainTexture = CreateCubemap(dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Mountain/right.jpg").c_str(),
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
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Niagara/back.jpg").c_str());
	starTexture = CreateCubemap(dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Stars/right.png").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Stars/left.png").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Stars/up.png").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Stars/down.png").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Stars/forward.png").c_str(),
		dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/Skies/Stars/back.png").c_str());

	CreateSky(spaceTexture);
	CreateSky(sunnyTexture);
	CreateSky(mountainTexture);
	CreateSky(niagaraTexture);
	CreateSky(starTexture);

	currentSky = skies[0];
}

void AssetManager::InitializeLights() {
	//Init lights list
	globalLights = std::vector<Light>();

	//white light from the top left
	Light mainLight = Light();
	mainLight.type = 0.0f;
	mainLight.enabled = true;
	mainLight.color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	mainLight.intensity = 0.7f;
	mainLight.direction = DirectX::XMFLOAT3(1, -1, 0);
	mainLight.range = 100.0f;

	//white light from the back
	Light backLight = Light();
	backLight.type = 0.0f;
	backLight.enabled = false;
	backLight.color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	backLight.intensity = 1.0f;
	backLight.direction = DirectX::XMFLOAT3(0, 0, -1);
	backLight.range = 100.0f;

	//red light on the bottom
	Light bottomLight = Light();
	bottomLight.type = 0.0f;
	bottomLight.enabled = false;
	bottomLight.color = DirectX::XMFLOAT3(1.0f, 0.2f, 0.2f);
	bottomLight.intensity = 1.0f;
	bottomLight.direction = DirectX::XMFLOAT3(0, 1, 0);
	bottomLight.range = 100.0f;

	//red pointlight in the center
	Light centerLight = Light();
	centerLight.type = 1.0f;
	centerLight.enabled = true;
	centerLight.color = DirectX::XMFLOAT3(0.1f, 1.0f, 0.2f);
	centerLight.intensity = 1.0f;
	centerLight.position = DirectX::XMFLOAT3(0, 1.5f, 0);
	centerLight.range = 2.0f;

	//flashlight attached to camera +.5z and x
	DirectionalLight flashLight = DirectionalLight();
	flashLight.type = 2.0f;
	flashLight.enabled = false;
	flashLight.color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	flashLight.intensity = 20.0f;
	flashLight.direction = DirectX::XMFLOAT3(0, 0, -1);
	flashLight.position = DirectX::XMFLOAT3(0.5f, 0.0f, 0.5f);
	flashLight.range = 10.0f;

	globalLights.push_back(mainLight);
	globalLights.push_back(backLight);
	globalLights.push_back(bottomLight);
	globalLights.push_back(centerLight);
	globalLights.push_back(flashLight);
}

void AssetManager::InitializeTerrainMaterials() {
	globalTerrainEntities = std::map<std::string, std::shared_ptr<GameEntity>>();
	globalTerrainMaterials = std::map<std::string, std::shared_ptr<TerrainMats>>();

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

	CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Metalness/wood_metal.png").c_str(), nullptr, &bogMetal); //These metal maps are blank, so just load the wood
	CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Metalness/wood_metal.png").c_str(), nullptr, &forestMetal); //Could maybe optimize loading times with memcpy
	CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Metalness/wood_metal.png").c_str(), nullptr, &rockyMetal);

	CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Normals/bog_normal-ogl.png").c_str(), nullptr, &bogNormal);
	CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Normals/forest_floor_Normal-ogl.png").c_str(), nullptr, &forestNormal);
	CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Normals/rocky_dirt1-normal-ogl.png").c_str(), nullptr, &rockyNormal);

	CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Roughness/bog_roughness.png").c_str(), nullptr, &bogRough);
	CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Roughness/forest_floor_Roughness.png").c_str(), nullptr, &forestRough);
	CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/PBR/Roughness/rocky_dirt1_Roughness.png").c_str(), nullptr, &rockyRough);

	//Load terrain and add a blend map
	std::shared_ptr<Mesh> mainTerrain = LoadTerrain(dxInstance->GetFullPathTo("../../../Assets/HeightMaps/valley.raw16").c_str(), 512, 512, 25.0f);
	std::shared_ptr<TerrainMats> mainTerrainMaterials = std::make_shared<TerrainMats>();
	mainTerrainMaterials->blendMaterials = std::vector<Material>();
	CreateWICTextureFromFile(device.Get(), context.Get(), dxInstance->GetFullPathTo_Wide(L"../../../Assets/Textures/blendMap.png").c_str(), nullptr, &mainTerrainMaterials->blendMap);
	mainTerrainMaterials->blendMaterials.push_back(Material(whiteTint, nullptr, nullptr, forestAlbedo, textureState, clampState, forestNormal, forestRough, forestMetal));
	mainTerrainMaterials->blendMaterials.push_back(Material(whiteTint, nullptr, nullptr, bogAlbedo, textureState, clampState, bogNormal, bogRough, bogMetal)); //Set these mats' shaders to null as they use the blend map shader
	mainTerrainMaterials->blendMaterials.push_back(Material(whiteTint, nullptr, nullptr, rockyAlbedo, textureState, clampState, rockyNormal, rockyRough, rockyMetal));

	//Set appropriate tiling
	mainTerrainMaterials->blendMaterials[0].SetTiling(10.0f);
	mainTerrainMaterials->blendMaterials[1].SetTiling(10.0f);

	std::shared_ptr<GameEntity> terrainEntity = std::make_shared<GameEntity>(mainTerrain, XMMatrixIdentity(), nullptr);
	globalTerrainEntities.emplace(std::string("Main Terrain"), terrainEntity);
	globalTerrainMaterials.emplace(std::string("Forest TMaterial"), mainTerrainMaterials);
}

void AssetManager::InitializeCameras() {
	globalCameras = std::map<std::string, std::shared_ptr<Camera>>();

	float aspectRatio = (float)(dxInstance->width / dxInstance->height);
	CreateCamera("mainCamera", DirectX::XMFLOAT3(0.0f, 0.0f, -5.5f), aspectRatio, 1);
	CreateCamera("mainShadowCamera", DirectX::XMFLOAT3(0.0f, 10.0f, -5.5f), 1.0f, 0);
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
	vertexShaders = std::map<std::string, std::shared_ptr<SimpleVertexShader>>();
	pixelShaders = std::map<std::string, std::shared_ptr<SimplePixelShader>>();

	// Make vertex shaders
	CreateVertexShader("BasicVS", L"VertexShader.cso");
	CreateVertexShader("NormalsVS", L"VSNormalMap.cso");
	CreateVertexShader("SkyVS", L"VSSkybox.cso");
	CreateVertexShader("TerrainVS", L"VSTerrainBlend.cso");
	CreateVertexShader("ShadowVS", L"VSShadowMap.cso");
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

size_t AssetManager::GetLightArraySize() {
	return this->globalLights.size();
}

size_t AssetManager::GetTerrainMaterialArraySize() {
	return this->globalTerrainMaterials.size();
}

size_t AssetManager::GetTerrainEntityArraySize() {
	return this->globalTerrainEntities.size();
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
	finalTerrain = std::make_shared<Mesh>(vertices.data(), numVertices, indices.data(), numIndices, device);

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
		globalMeshes.emplace(mesh->mName.C_Str(), ProcessComplexMesh(mesh, scene));
		CreateGameEntity(globalMeshes.at(mesh->mName.C_Str()), globalMaterials.at("cobbleMat"), mesh->mName.C_Str());
		globalEntities.at(mesh->mName.C_Str())->GetTransform()->SetPosition(0.0f, 3.0f * i, 1.0f);
		globalEntities.at(mesh->mName.C_Str())->GetTransform()->SetScale(0.25f, 0.25f, 0.25f);
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

	if (hasTangents) return std::make_shared<Mesh>(vertices.data(), vertices.size(), indices.data(), indices.size(), 0, device);
	else return std::make_shared<Mesh>(vertices.data(), vertices.size(), indices.data(), indices.size(), device);
}
#pragma endregion

#pragma region removeAssets

//
// Asset Removal methods
//

void AssetManager::RemoveGameEntity(std::string name) {
	globalEntities.erase(globalEntities.begin() + GetGameEntityIDByName(name));
}

void AssetManager::RemoveGameEntity(int id) {
	globalEntities.erase(globalEntities.begin() + id);
}

void AssetManager::RemoveSky(std::string name) {
	globalEntities.erase(globalEntities.begin() + GetGameEntityIDByName(name));
}

void AssetManager::RemoveSky(int id) {
	globalEntities.erase(globalEntities.begin() + id);
}

void AssetManager::RemoveVertexShader(std::string name) {
	globalEntities.erase(globalEntities.begin() + GetGameEntityIDByName(name));
}

void AssetManager::RemoveVertexShader(int id) {
	globalEntities.erase(globalEntities.begin() + id);
}

void AssetManager::RemovePixelShader(std::string name) {
	globalEntities.erase(globalEntities.begin() + GetGameEntityIDByName(name));
}

void AssetManager::RemovePixelShader(int id) {
	globalEntities.erase(globalEntities.begin() + id);
}

void AssetManager::RemoveMesh(std::string name) {
	globalEntities.erase(globalEntities.begin() + GetGameEntityIDByName(name));
}

void AssetManager::RemoveMesh(int id) {
	globalEntities.erase(globalEntities.begin() + id);
}

void AssetManager::RemoveCamera(std::string name) {
	globalEntities.erase(globalEntities.begin() + GetGameEntityIDByName(name));
}

void AssetManager::RemoveCamera(int id) {
	globalEntities.erase(globalEntities.begin() + id);
}

void AssetManager::RemoveTerrain(std::string name) {
	globalEntities.erase(globalEntities.begin() + GetGameEntityIDByName(name));
}

void AssetManager::RemoveTerrain(int id) {
	globalEntities.erase(globalEntities.begin() + id);
}

void AssetManager::RemoveMaterial(std::string name) {
	globalEntities.erase(globalEntities.begin() + GetGameEntityIDByName(name));
}

void AssetManager::RemoveMaterial(int id) {
	globalEntities.erase(globalEntities.begin() + id);
}

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
#pragma endregion

#pragma region enableDisableAssets

//
// Enables or disables assets into or
// out of the rendering pipeline
//

void AssetManager::EnableDisableGameEntity(std::string name, bool value) {
	GetGameEntityByName(name)->SetEnableDisable(value);
}

void AssetManager::EnableDisableGameEntity(int id, bool value) {
	globalEntities[id]->SetEnableDisable(value);
}

void AssetManager::EnableDisableSky(std::string name, bool value) {
	GetSkyByName(name)->SetEnableDisable(value);
}

void AssetManager::EnableDisableSky(int id, bool value) {
	skies[id]->SetEnableDisable(value);
}

void AssetManager::EnableDisableVertexShader(std::string name, bool value) {
	GetVertexShaderByName(name)->SetEnableDisable(value);
}

void AssetManager::EnableDisableVertexShader(int id, bool value) {
	vertexShaders[id]->SetEnableDisable(value);
}

void AssetManager::EnableDisablePixelShader(std::string name, bool value) {
	GetPixelShaderByName(name)->SetEnableDisable(value);
}

void AssetManager::EnableDisablePixelShader(int id, bool value) {
	pixelShaders[id]->SetEnableDisable(value);
}

void AssetManager::EnableDisableMesh(std::string name, bool value) {
	GetMeshByName(name)->SetEnableDisable(value);
}

void AssetManager::EnableDisableMesh(int id, bool value) {
	globalMeshes[id]->SetEnableDisable(value);
}

void AssetManager::EnableDisableCamera(std::string name, bool value) {
	GetCameraByName(name)->SetEnableDisable(value);
}

void AssetManager::EnableDisableCamera(int id, bool value) {
	globalCameras[id]->SetEnableDisable(value);
}

void AssetManager::EnableDisableTerrain(std::string name, bool value) {
	GetTerrainByName(name)->SetEnableDisable(value);
}

void AssetManager::EnableDisableTerrain(int id, bool value) {
	globalTerrainEntities[id]->SetEnableDisable(value);
}

void AssetManager::EnableDisableMaterial(std::string name, bool value) {
	GetMaterialByName(name)->SetEnableDisable(value);
}

void AssetManager::EnableDisableMaterial(int id, bool value) {
	globalMaterials[id]->SetEnableDisable(value);
}

void AssetManager::EnableDisableTerrainMaterial(std::string name, bool value) {
	GetTerrainMaterialByName(name)->SetEnableDisable(value);
}

void AssetManager::EnableDisableTerrainMaterial(int id, bool value) {
	globalTerrainMaterials[id]->SetEnableDisable(value);
}

// See GetLightIDByName
//void AssetManager::EnableDisableLight(std::string name, bool value) {
//	GetLightByName(name)->SetEnableDisable(value);
//}

void AssetManager::EnableDisableLight(int id, bool value) {
	globalLights[id]->enabled = value;
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

std::shared_ptr<GameEntity> AssetManager::GetTerrainByName(std::string name) {
	for (int i = 0; i < globalEntities.size(); i++) {
		if (globalEntities[i]->GetName() == name) {
			return globalEntities[i];
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
		if (globalTerrainMaterials[i]->GetName() == name) {
			return globalTerrainMaterials[i];
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

int AssetManager::GetTerrainIDByName(std::string name) {
	for (int i = 0; i < globalTerrainEntities.size(); i++) {
		if (globalTerrainEntities[i]->GetName() == name) {
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

int AssetManager::GetTerrainMaterialIDByName(std::string name) {
	for (int i = 0; i < globalTerrainMaterials.size(); i++) {
		if (globalTerrainMaterials[i]->GetName() == name) {
			return i;
		}
	}
	return -1;
}

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