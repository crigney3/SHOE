#include "../Headers/ParticleSystem.h"
#include "../Headers/GameEntity.h"
#include "../Headers/Time.h"

std::shared_ptr<SimplePixelShader> ParticleSystem::defaultParticlePixelShader = nullptr;
std::shared_ptr<SimpleVertexShader> ParticleSystem::defaultParticleVertexShader = nullptr;
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ParticleSystem::defaultParticleTextureSRV = nullptr;
std::shared_ptr<SimpleComputeShader> ParticleSystem::defaultParticleEmitComputeShader = nullptr;
std::shared_ptr<SimpleComputeShader> ParticleSystem::defaultParticleSimComputeShader = nullptr;
std::shared_ptr<SimpleComputeShader> ParticleSystem::defaultParticleCopyComputeShader = nullptr;
std::shared_ptr<SimpleComputeShader> ParticleSystem::defaultParticleDeadListInitComputeShader = nullptr;
std::shared_ptr<Texture> ParticleSystem::defaultParticleTexture = nullptr;
Microsoft::WRL::ComPtr<ID3D11Device> ParticleSystem::defaultDevice = nullptr;
Microsoft::WRL::ComPtr<ID3D11DeviceContext> ParticleSystem::defaultContext = nullptr;

void ParticleSystem::SetColorTint(DirectX::XMFLOAT4 color) {
	this->colorTint = color;
}

DirectX::XMFLOAT4 ParticleSystem::GetColorTint() {
	return this->colorTint;
}

//
// Scale defaults to 0 - anything above 0
// will cause the particle to grow over time
//
void ParticleSystem::SetScale(float scale) {
	this->scale = scale;
}

float ParticleSystem::GetScale() {
	return this->scale;
}

void ParticleSystem::SetBlendState(bool AdditiveOrNot) {
	this->additiveBlend = AdditiveOrNot;
}

bool ParticleSystem::GetBlendState() {
	return this->additiveBlend;
}

void ParticleSystem::SetIsMultiParticle(bool isMultiParticle)
{
	this->isMultiParticle = isMultiParticle;
}

bool ParticleSystem::IsMultiParticle()
{
	return isMultiParticle;
}

void ParticleSystem::SetParticlesPerSecond(float particlesPerSecond) {
	this->particlesPerSecond = particlesPerSecond;
	this->secondsPerEmission = 1.0f / particlesPerSecond;
}

float ParticleSystem::GetParticlesPerSecond() {
	return this->particlesPerSecond;
}

void ParticleSystem::SetParticleLifetime(float particleLifetime) {
	this->particleLifetime = particleLifetime;
}

float ParticleSystem::GetParticleLifetime() {
	return this->particleLifetime;
}

void ParticleSystem::SetMaxParticles(int maxParticles) {
	if (this->maxParticles != maxParticles) {
		// Reset most data
		OnDestroy();

		this->maxParticles = maxParticles;
		Initialize(this->maxParticles);

		// Run the dead list initializer
		// Add +1 thread as a buffer for "hidden counter" bug
		particleDeadListInitComputeShader->SetShader();
		particleDeadListInitComputeShader->SetInt("maxParticles", this->maxParticles);
		particleDeadListInitComputeShader->SetUnorderedAccessView("DeadList", this->deadListUAV);
		particleDeadListInitComputeShader->CopyAllBufferData();
		particleDeadListInitComputeShader->DispatchByThreads(this->maxParticles, 1, 1);
	}
}

int ParticleSystem::GetMaxParticles() {
	return this->maxParticles;
}

void ParticleSystem::SetSpeed(float speed) {
	this->speed = speed;
}

float ParticleSystem::GetSpeed() {
	return this->speed;
}

void ParticleSystem::SetDestination(DirectX::XMFLOAT3 destination) {
	this->destination = destination;
}

DirectX::XMFLOAT3 ParticleSystem::GetDestination() {
	return this->destination;
}

void ParticleSystem::SetFilenameKey(std::string filenameKey) {
	this->filenameKey = filenameKey;
}

std::string ParticleSystem::GetFilenameKey() {
	return this->filenameKey;
}

void ParticleSystem::SetParticleComputeShader(std::shared_ptr<SimpleComputeShader> computeShader, ParticleComputeShaderType type) {
	switch (type) {
	case 0:
		this->particleEmitComputeShader = computeShader;
		break;
	case 1:
		this->particleSimComputeShader = computeShader;
		break;
	case 2:
		this->particleCopyComputeShader = computeShader;
		break;
	case 3:
		this->particleDeadListInitComputeShader = computeShader;
		// Run the dead list initializer
		particleDeadListInitComputeShader->SetShader();
		particleDeadListInitComputeShader->SetInt("maxParticles", this->maxParticles);
		particleDeadListInitComputeShader->SetUnorderedAccessView("DeadList", this->deadListUAV);
		particleDeadListInitComputeShader->CopyAllBufferData();
		particleDeadListInitComputeShader->DispatchByThreads(this->maxParticles, 1, 1);
		break;
	}
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ParticleSystem::GetSortListSRV() {
	return this->sortListSRV;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ParticleSystem::GetDrawListSRV() {
	return this->drawListSRV;
}

std::shared_ptr<Texture> ParticleSystem::GetParticleTexture() {
	return this->particleTexture;
}

void ParticleSystem::SetParticleTexture(std::shared_ptr<Texture> particleTexture) {
	this->particleTexture = particleTexture;
	SetParticleTextureSRV(particleTexture->GetDX11Texture());
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ParticleSystem::GetParticleTextureSRV() {
	return this->particleTextureSRV;
}

void ParticleSystem::SetDefaults(std::shared_ptr<SimplePixelShader> particlePixelShader,
								 std::shared_ptr<SimpleVertexShader> particleVertexShader,
								 std::shared_ptr<SimpleComputeShader> particleEmitComputeShader,
								 std::shared_ptr<SimpleComputeShader> particleSimComputeShader,
								 std::shared_ptr<SimpleComputeShader> particleCopyComputeShader,
								 std::shared_ptr<SimpleComputeShader> particleDeadListInitComputeShader,
								 std::shared_ptr<Texture> particleBaseTexture,
								 Microsoft::WRL::ComPtr<ID3D11Device> device,
								 Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
{
	defaultParticlePixelShader = particlePixelShader;
	defaultParticleVertexShader = particleVertexShader;
	defaultParticleCopyComputeShader = particleCopyComputeShader;
	defaultParticleEmitComputeShader = particleEmitComputeShader;
	defaultParticleSimComputeShader = particleSimComputeShader;
	defaultParticleDeadListInitComputeShader = particleDeadListInitComputeShader;
	defaultParticleTexture = particleBaseTexture;
	defaultParticleTextureSRV = particleBaseTexture->GetDX11Texture();
	defaultDevice = device;
	defaultContext = context;
}

void ParticleSystem::Start()
{
	this->maxParticles = 10;

	this->particlesPerSecond = 1.0f;
	this->particleLifetime = 3.0f;
	this->secondsPerEmission = 1.0f / particlesPerSecond;

	this->timeSinceEmit = 0.0f;

	this->particlePixelShader = defaultParticlePixelShader;
	this->particleVertexShader = defaultParticleVertexShader;
	this->particleTexture = defaultParticleTexture;
	this->particleTextureSRV = defaultParticleTextureSRV;

	this->device = defaultDevice;
	this->context = defaultContext;

	this->colorTint = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	this->scale = 0.0f;
	this->speed = 1.0f;
	this->destination = DirectX::XMFLOAT3(0.0f, 5.0f, 0.0f);

	this->additiveBlend = true;
	this->isMultiParticle = true;

	// TODO: Set up a function to calculate if this emitter will overflow its buffer
	// Then recalculate maxParticles if it will

	Initialize(this->maxParticles);

	SetParticleComputeShader(defaultParticleCopyComputeShader, ParticleComputeShaderType::Copy);
	SetParticleComputeShader(defaultParticleSimComputeShader, ParticleComputeShaderType::Simulate);
	SetParticleComputeShader(defaultParticleEmitComputeShader, ParticleComputeShaderType::Emit);
	SetParticleComputeShader(defaultParticleDeadListInitComputeShader, ParticleComputeShaderType::DeadListInit);
}

void ParticleSystem::Update() {
	ID3D11UnorderedAccessView* none[8] = {};
	context->CSSetUnorderedAccessViews(0, 8, none, 0);

	// New Emission
	timeSinceEmit += Time::deltaTime;
	while (timeSinceEmit > secondsPerEmission) {
		int emitCount = (int)(timeSinceEmit / secondsPerEmission);
		emitCount = min(emitCount, 65535);
		timeSinceEmit = fmod(timeSinceEmit, secondsPerEmission);

		EmitParticle(emitCount);
	}

	// Array size is equal to num of uavs
	context->CSSetUnorderedAccessViews(0, 8, none, 0);

	// Updates particles
	particleSimComputeShader->SetShader();
	particleSimComputeShader->SetUnorderedAccessView("particles", this->drawListUAV);
	particleSimComputeShader->SetUnorderedAccessView("deadList", this->deadListUAV);
	particleSimComputeShader->SetUnorderedAccessView("sortList", this->sortListUAV, 0);

	particleSimComputeShader->SetFloat("speed", this->speed);
	particleSimComputeShader->SetFloat3("endPos", this->destination);
	particleSimComputeShader->SetFloat("deltaTime", Time::deltaTime);
	particleSimComputeShader->SetFloat("lifeTime", this->particleLifetime);
	particleSimComputeShader->SetInt("maxParticles", this->maxParticles);

	particleSimComputeShader->CopyAllBufferData();

	particleSimComputeShader->DispatchByThreads(this->maxParticles, 1, 1);

	context->CSSetUnorderedAccessViews(0, 8, none, 0);

	// Copy the dead list's count to the counter buffer
	context->CopyStructureCount(deadListCounterBuffer.Get(), 0, deadListUAV.Get());
}

void ParticleSystem::OnDestroy()
{
	// SRV handle release and wipe
	sortListSRV.Reset();
	drawListSRV.Reset();

	// Unbind all UAVs
	// Don't reset the Compute Shader pointers,
	// We still need those
	ID3D11UnorderedAccessView* none[8] = {};
	context->CSSetUnorderedAccessViews(0, 8, none, 0);

	// UAV handle release and wipe
	this->deadListUAV.Reset();
	this->sortListUAV.Reset();
	this->argsListUAV.Reset();
	this->drawListUAV.Reset();

	// Buffer reset and wipe
	inBuffer.Reset();
	argsBuffer.Reset();
}

void ParticleSystem::Draw(std::shared_ptr<Camera> cam, Microsoft::WRL::ComPtr<ID3D11BlendState> particleBlendAdditive)
{
	ID3D11UnorderedAccessView* none[8] = {};
	context->CSSetUnorderedAccessViews(0, 8, none, 0);

	this->Update();

	particleCopyComputeShader->SetShader();

	particleCopyComputeShader->SetUnorderedAccessView("sortList", this->sortListUAV);
	particleCopyComputeShader->SetUnorderedAccessView("argsList", this->argsListUAV);

	particleCopyComputeShader->DispatchByThreads(1, 1, 1);

	if (additiveBlend) {
		context->OMSetBlendState(particleBlendAdditive.Get(), 0, 0xFFFFFFFF);
	}
	else {
		context->OMSetBlendState(0, 0, 0xFFFFFFFF);
	}

	context->CSSetUnorderedAccessViews(0, 8, none, 0);

	UINT stride = 0;
	UINT offset = 0;
	ID3D11Buffer* emptyBuffer = 0;
	context->IASetVertexBuffers(0, 1, &emptyBuffer, &stride, &offset);
	context->IASetIndexBuffer(inBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	particleVertexShader->SetShader();
	particlePixelShader->SetShader();

	context->VSSetShaderResources(0, 1, this->drawListSRV.GetAddressOf());
	context->VSSetShaderResources(1, 1, this->sortListSRV.GetAddressOf());
	if (this->particleTextureSRV != nullptr) {
		particlePixelShader->SetShaderResourceView("textureParticle", particleTextureSRV);
	}

	particleVertexShader->SetMatrix4x4("view", cam->GetViewMatrix());
	particleVertexShader->SetMatrix4x4("projection", cam->GetProjectionMatrix());
	particleVertexShader->SetFloat("currentTime", Time::currentTime);
	particleVertexShader->SetFloat("scale", this->scale);
	particleVertexShader->CopyAllBufferData();
	particlePixelShader->SetFloat4("colorTint", this->colorTint);
	particlePixelShader->CopyAllBufferData();

	context->DrawIndexedInstancedIndirect(argsBuffer.Get(), 0);

	ID3D11ShaderResourceView* noneLarge[16] = {};
	context->VSSetShaderResources(0, 16, noneLarge);
}

void ParticleSystem::SetParticleTextureSRV(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleTextureSRV)
{
	this->particleTextureSRV = particleTextureSRV;
}

void ParticleSystem::Initialize(int maxParticles) 
{
	// Deadlist for ParticleEmit and ParticleFlow

	Microsoft::WRL::ComPtr<ID3D11Buffer> deadListBuffer;

	this->maxParticles = maxParticles;

	D3D11_BUFFER_DESC deadDesc = {};
	deadDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	deadDesc.CPUAccessFlags = 0;
	deadDesc.Usage = D3D11_USAGE_DEFAULT;
	deadDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	deadDesc.StructureByteStride = sizeof(unsigned int);
	deadDesc.ByteWidth = (sizeof(unsigned int) * this->maxParticles);// +sizeof(unsigned int);
	device->CreateBuffer(&deadDesc, 0, deadListBuffer.GetAddressOf());

	D3D11_UNORDERED_ACCESS_VIEW_DESC deadUavDesc = {};
	deadUavDesc.Format = DXGI_FORMAT_UNKNOWN;
	deadUavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	deadUavDesc.Buffer.FirstElement = 0;
	deadUavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
	deadUavDesc.Buffer.NumElements = this->maxParticles;
	device->CreateUnorderedAccessView(deadListBuffer.Get(), &deadUavDesc, this->deadListUAV.GetAddressOf());

	// Create a buffer that holds the dead list counter
	D3D11_BUFFER_DESC deadCountDesc = {};
	deadCountDesc.Usage = D3D11_USAGE_DEFAULT;
	deadCountDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	deadCountDesc.ByteWidth = 16; // technically sizeof(unsigned int), but cbuffers must be multiples of 16
	deadCountDesc.CPUAccessFlags = 0;
	deadCountDesc.MiscFlags = 0;
	deadCountDesc.StructureByteStride = 0;
	device->CreateBuffer(&deadCountDesc, 0, deadListCounterBuffer.GetAddressOf());

	// Create and bind CopyDrawCount's sortList
	// sortList defines the memory indices of living
	// particles to be drawn

	ID3D11Buffer* sortListBuffer;

	D3D11_BUFFER_DESC sortDesc = {};
	sortDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	sortDesc.ByteWidth = sizeof(DirectX::XMFLOAT2) * this->maxParticles;
	sortDesc.CPUAccessFlags = 0;
	sortDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	sortDesc.StructureByteStride = sizeof(DirectX::XMFLOAT2);
	sortDesc.Usage = D3D11_USAGE_DEFAULT;
	device->CreateBuffer(&sortDesc, 0, &sortListBuffer);

	// UAV
	D3D11_UNORDERED_ACCESS_VIEW_DESC sortUAVDesc = {};
	sortUAVDesc.Format = DXGI_FORMAT_UNKNOWN; // Needed for RW structured buffers
	sortUAVDesc.Buffer.FirstElement = 0;
	sortUAVDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER; // IncrementCounter() in HLSL
	sortUAVDesc.Buffer.NumElements = this->maxParticles;
	sortUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	device->CreateUnorderedAccessView(sortListBuffer, &sortUAVDesc, &this->sortListUAV);

	// SRV (for indexing in VS)
	D3D11_SHADER_RESOURCE_VIEW_DESC sortSRVDesc = {};
	sortSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	sortSRVDesc.Buffer.FirstElement = 0;
	sortSRVDesc.Buffer.NumElements = this->maxParticles;
	sortSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	device->CreateShaderResourceView(sortListBuffer, &sortSRVDesc, sortListSRV.GetAddressOf());

	sortListBuffer->Release();

	// Particle buffer for CS

	ID3D11Buffer* drawListBuffer;

	D3D11_BUFFER_DESC drawDesc = {};
	drawDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	drawDesc.ByteWidth = sizeof(Particle) * this->maxParticles;
	drawDesc.CPUAccessFlags = 0;
	drawDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	drawDesc.StructureByteStride = sizeof(Particle);
	drawDesc.Usage = D3D11_USAGE_DEFAULT;
	device->CreateBuffer(&drawDesc, 0, &drawListBuffer);

	// UAV
	D3D11_UNORDERED_ACCESS_VIEW_DESC drawUAVDesc = {};
	drawUAVDesc.Format = DXGI_FORMAT_UNKNOWN; // Needed for RW structured buffers
	drawUAVDesc.Buffer.FirstElement = 0;
	drawUAVDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER; // IncrementCounter() in HLSL
	drawUAVDesc.Buffer.NumElements = this->maxParticles;
	drawUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	device->CreateUnorderedAccessView(drawListBuffer, &drawUAVDesc, &this->drawListUAV);

	// SRV (for indexing in VS)
	D3D11_SHADER_RESOURCE_VIEW_DESC drawSRVDesc = {};
	drawSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	drawSRVDesc.Buffer.FirstElement = 0;
	drawSRVDesc.Buffer.NumElements = this->maxParticles;
	drawSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	device->CreateShaderResourceView(drawListBuffer, &drawSRVDesc, drawListSRV.GetAddressOf());

	drawListBuffer->Release();

	// Create and bind CopyDrawCount's argsList

	D3D11_BUFFER_DESC argsDesc = {};
	argsDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	argsDesc.ByteWidth = sizeof(unsigned int) * 5;
	argsDesc.CPUAccessFlags = 0;
	argsDesc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
	argsDesc.StructureByteStride = 0;
	argsDesc.Usage = D3D11_USAGE_DEFAULT;
	device->CreateBuffer(&argsDesc, 0, argsBuffer.GetAddressOf());

	D3D11_UNORDERED_ACCESS_VIEW_DESC argsUAVDesc = {};
	argsUAVDesc.Format = DXGI_FORMAT_R32_UINT;
	argsUAVDesc.Buffer.FirstElement = 0;
	argsUAVDesc.Buffer.Flags = 0;
	argsUAVDesc.Buffer.NumElements = 5;
	argsUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	device->CreateUnorderedAccessView(argsBuffer.Get(), &argsUAVDesc, this->argsListUAV.GetAddressOf());

	// Index data, currently always the same

	unsigned long* indices = new unsigned long[this->maxParticles * 6];
	for (unsigned long i = 0; i < this->maxParticles; i++)
	{
		unsigned long indexCounter = i * 6;
		indices[indexCounter + 0] = 0 + i * 4;
		indices[indexCounter + 1] = 1 + i * 4;
		indices[indexCounter + 2] = 2 + i * 4;
		indices[indexCounter + 3] = 0 + i * 4;
		indices[indexCounter + 4] = 2 + i * 4;
		indices[indexCounter + 5] = 3 + i * 4;
	}
	D3D11_SUBRESOURCE_DATA indexData = {};
	indexData.pSysMem = indices;

	// Regular (static) index buffer
	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.ByteWidth = sizeof(unsigned long) * this->maxParticles * 6;
	ibDesc.MiscFlags = 0;
	ibDesc.StructureByteStride = 0;
	device->CreateBuffer(&ibDesc, &indexData, inBuffer.GetAddressOf());
	delete[] indices;

	// Copy the dead list's count to the counter buffer
	context->CopyStructureCount(deadListCounterBuffer.Get(), 0, deadListUAV.Get());
}

void ParticleSystem::EmitParticle(int emitCount) {
	particleEmitComputeShader->SetShader();

	particleEmitComputeShader->SetUnorderedAccessView("particles", this->drawListUAV);
	particleEmitComputeShader->SetUnorderedAccessView("deadList", this->deadListUAV);
	context->CSSetConstantBuffers(1, 1, deadListCounterBuffer.GetAddressOf());
	//particleEmitComputeShader->SetUnorderedAccessView("sortList", this->sortListUAV);

	particleEmitComputeShader->SetFloat3("startPos", this->GetTransform()->GetGlobalPosition());
	//particleEmitComputeShader->SetFloat3("cameraPos", cam->GetTransform()->GetPosition());
	particleEmitComputeShader->SetFloat("emitTime", Time::currentTime);
	particleEmitComputeShader->SetInt("maxParticles", this->maxParticles);
	particleEmitComputeShader->SetInt("emitCount", emitCount);

	particleEmitComputeShader->CopyAllBufferData();

	particleEmitComputeShader->DispatchByThreads(emitCount, 1, 1);
}