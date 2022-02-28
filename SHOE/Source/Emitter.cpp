#include "../Headers/Emitter.h"
#include "..\Headers\ComponentManager.h"

Emitter::Emitter(int maxParticles,
				 float particleLifeTime,
				 float particlesPerSecond,
				 DirectX::XMFLOAT3 position,
				 std::shared_ptr<SimplePixelShader> particlePixelShader,
				 std::shared_ptr<SimpleVertexShader> particleVertexShader,
				 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleTextureSRV,
				 Microsoft::WRL::ComPtr<ID3D11Device> device,
				 Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
				 std::string name,
				 bool isMultiParticle,
				 bool additiveBlendState) 
{
	this->maxParticles = maxParticles;
	this->particlesPerSecond = particlesPerSecond;
	this->particleLifetime = particleLifeTime;
	this->secondsPerEmission = 1.0f / particlesPerSecond;

	this->timeSinceEmit = 0.0f;

	this->particlePixelShader = particlePixelShader;
	this->particleVertexShader = particleVertexShader;
	this->particleTextureSRV = particleTextureSRV;

	this->device = device;
	this->context = context;

	this->name = name;
	this->enabled = false;

	this->transform = ComponentManager::Instantiate<Transform>(nullptr, true);
	this->transform->SetPosition(position);

	this->colorTint = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	this->scale = 0.0f;
	this->speed = 1.0f;
	this->destination = DirectX::XMFLOAT3(0.0f, 5.0f, 0.0f);

	this->additiveBlend = additiveBlendState;

	Initialize(this->maxParticles);
}

Emitter::~Emitter() {
	
}

void Emitter::SetColorTint(DirectX::XMFLOAT4 color) {
	this->colorTint = color;
}

DirectX::XMFLOAT4 Emitter::GetColorTint() {
	return this->colorTint;
}

//
// Scale defaults to 0 - anything above 0
// will cause the particle to grow over time
//
void Emitter::SetScale(float scale) {
	this->scale = scale;
}

float Emitter::GetScale() {
	return this->scale;
}

void Emitter::SetBlendState(bool AdditiveOrNot) {
	this->additiveBlend = AdditiveOrNot;
}

bool Emitter::GetBlendState() {
	return this->additiveBlend;
}

void Emitter::SetName(std::string name) {
	this->name = name;
}

std::string Emitter::GetName() {
	return this->name;
}

void Emitter::SetParticlesPerSecond(float particlesPerSecond) {
	this->particlesPerSecond = particlesPerSecond;
	this->secondsPerEmission = 1.0f / particlesPerSecond;
}

float Emitter::GetParticlesPerSecond() {
	return this->particlesPerSecond;
}

void Emitter::SetParticleLifetime(float particleLifetime) {
	this->particleLifetime = particleLifetime;
}

float Emitter::GetParticleLifetime() {
	return this->particleLifetime;
}

void Emitter::SetEnableDisable(bool enabled) {
	this->enabled = enabled;
}

bool Emitter::GetEnableDisable() {
	return this->enabled;
}

void Emitter::SetMaxParticles(int maxParticles) {
	if (this->maxParticles != maxParticles) {
		// SRV handle release and wipe
		sortListSRV.Reset();
		drawListSRV.Reset();

		// Unbind all UAVs
		// Don't reset the Computer Shader pointers,
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

int Emitter::GetMaxParticles() {
	return this->maxParticles;
}

void Emitter::SetSpeed(float speed) {
	this->speed = speed;
}

float Emitter::GetSpeed() {
	return this->speed;
}

void Emitter::SetDestination(DirectX::XMFLOAT3 destination) {
	this->destination = destination;
}

DirectX::XMFLOAT3 Emitter::GetDestination() {
	return this->destination;
}

void Emitter::SetParticleComputeShader(std::shared_ptr<SimpleComputeShader> computeShader, ParticleComputeShaderType type) {
	switch(type) {
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
			// Add +1 thread as a buffer for "hidden counter" bug
			particleDeadListInitComputeShader->SetShader();
			particleDeadListInitComputeShader->SetInt("maxParticles", this->maxParticles);
			particleDeadListInitComputeShader->SetUnorderedAccessView("DeadList", this->deadListUAV);
			particleDeadListInitComputeShader->CopyAllBufferData();
			particleDeadListInitComputeShader->DispatchByThreads(this->maxParticles, 1, 1);
			break;
	}
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Emitter::GetSortListSRV() {
	return this->sortListSRV;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Emitter::GetDrawListSRV() {
	return this->drawListSRV;
}

void Emitter::SetMainCamera(std::shared_ptr<Camera> cam) {
	this->cam = cam;
}

void Emitter::Initialize(int maxParticles) {
	// Deadlist for ParticleEmit and ParticleFlow

	ID3D11Buffer* deadListBuffer;

	D3D11_BUFFER_DESC deadDesc = {};
	deadDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	deadDesc.CPUAccessFlags = 0;
	deadDesc.Usage = D3D11_USAGE_DEFAULT;
	deadDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	deadDesc.StructureByteStride = sizeof(unsigned int);
	deadDesc.ByteWidth = (sizeof(unsigned int) * maxParticles) + sizeof(unsigned int);
	device->CreateBuffer(&deadDesc, 0, &deadListBuffer);

	D3D11_UNORDERED_ACCESS_VIEW_DESC deadUavDesc = {};
	deadUavDesc.Format = DXGI_FORMAT_UNKNOWN;
	deadUavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	deadUavDesc.Buffer.FirstElement = 0;
	deadUavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
	deadUavDesc.Buffer.NumElements = maxParticles;
	device->CreateUnorderedAccessView(deadListBuffer, &deadUavDesc, this->deadListUAV.GetAddressOf());

	deadListBuffer->Release();

	// Create and bind CopyDrawCount's sortList
	// sortList defines the memory indices of living
	// particles to be drawn

	ID3D11Buffer* sortListBuffer;

	D3D11_BUFFER_DESC sortDesc = {};
	sortDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	sortDesc.ByteWidth = sizeof(DirectX::XMFLOAT2) * maxParticles;
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
	sortUAVDesc.Buffer.NumElements = maxParticles;
	sortUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	device->CreateUnorderedAccessView(sortListBuffer, &sortUAVDesc, &this->sortListUAV);

	// SRV (for indexing in VS)
	D3D11_SHADER_RESOURCE_VIEW_DESC sortSRVDesc = {};
	sortSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	sortSRVDesc.Buffer.FirstElement = 0;
	sortSRVDesc.Buffer.NumElements = maxParticles;
	sortSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	device->CreateShaderResourceView(sortListBuffer, &sortSRVDesc, sortListSRV.GetAddressOf());

	sortListBuffer->Release();

	// Particle buffer for CS

	ID3D11Buffer* drawListBuffer;

	D3D11_BUFFER_DESC drawDesc = {};
	drawDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	drawDesc.ByteWidth = sizeof(Particle) * maxParticles;
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
	drawUAVDesc.Buffer.NumElements = maxParticles;
	drawUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	device->CreateUnorderedAccessView(drawListBuffer, &drawUAVDesc, &this->drawListUAV);

	// SRV (for indexing in VS)
	D3D11_SHADER_RESOURCE_VIEW_DESC drawSRVDesc = {};
	drawSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	drawSRVDesc.Buffer.FirstElement = 0;
	drawSRVDesc.Buffer.NumElements = maxParticles;
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

	unsigned long* indices = new unsigned long[maxParticles * 6];
	for (unsigned long i = 0; i < maxParticles; i++)
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

	// Old Indices
	/*
	* indices = new unsigned int[maxParticles * 6];
		int indexCount = 0;
		for (int i = 0; i < maxParticles * 4; i += 4) {
			indices[indexCount++] = i;
			indices[indexCount++] = i + 1;
			indices[indexCount++] = i + 2;
			indices[indexCount++] = i;
			indices[indexCount++] = i + 2;
			indices[indexCount++] = i + 3;
		}
	*/

	// Regular (static) index buffer
	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.ByteWidth = sizeof(unsigned long) * maxParticles * 6;
	ibDesc.MiscFlags = 0;
	ibDesc.StructureByteStride = 0;
	device->CreateBuffer(&ibDesc, &indexData, inBuffer.GetAddressOf());
	delete[] indices;
}

void Emitter::Update(float deltaTime, float totalTime) {
	ID3D11UnorderedAccessView* none[8] = {};
	context->CSSetUnorderedAccessViews(0, 8, none, 0);

	// Don't do emission if disabled
	if (this->enabled) {
		timeSinceEmit += deltaTime;
		while (timeSinceEmit > secondsPerEmission) {
			int emitCount = (int)(timeSinceEmit / secondsPerEmission);
			emitCount = min(emitCount, 65535);
			timeSinceEmit = fmod(timeSinceEmit, secondsPerEmission);

			EmitParticle(totalTime, emitCount);
		}

		// Array size is equal to num of uavs
		context->CSSetUnorderedAccessViews(0, 8, none, 0);
	}

	// Don't do update if disabled
	if (this->enabled) {
		particleSimComputeShader->SetShader();
		particleSimComputeShader->SetUnorderedAccessView("particles", this->drawListUAV);
		particleSimComputeShader->SetUnorderedAccessView("deadList", this->deadListUAV);
		particleSimComputeShader->SetUnorderedAccessView("sortList", this->sortListUAV, 0);

		particleSimComputeShader->SetFloat("speed", this->speed);
		particleSimComputeShader->SetFloat3("endPos", this->destination);
		particleSimComputeShader->SetFloat("deltaTime", deltaTime);
		particleSimComputeShader->SetFloat("lifeTime", this->particleLifetime);
		particleSimComputeShader->SetInt("maxParticles", this->maxParticles);

		particleSimComputeShader->CopyAllBufferData();

		particleSimComputeShader->DispatchByThreads(this->maxParticles, 1, 1);

		context->CSSetUnorderedAccessViews(0, 8, none, 0);
	}
}

void Emitter::Draw(std::shared_ptr<Camera> cam, float currentTime, Microsoft::WRL::ComPtr<ID3D11BlendState> particleBlendAdditive) {
	ID3D11UnorderedAccessView* none[8] = {};
	context->CSSetUnorderedAccessViews(0, 8, none, 0);

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
	particlePixelShader->SetShaderResourceView("textureParticle", particleTextureSRV);

	particleVertexShader->SetMatrix4x4("view", cam->GetViewMatrix());
	particleVertexShader->SetMatrix4x4("projection", cam->GetProjectionMatrix());
	particleVertexShader->SetFloat("currentTime", currentTime);
	particleVertexShader->SetFloat("scale", this->scale);
	particleVertexShader->CopyAllBufferData();
	particlePixelShader->SetFloat4("colorTint", this->colorTint);
	particlePixelShader->CopyAllBufferData();

	context->DrawIndexedInstancedIndirect(argsBuffer.Get(), 0);

	ID3D11ShaderResourceView* noneLarge[16] = {};
	context->VSSetShaderResources(0, 16, noneLarge);
}

void Emitter::EmitParticle(float currentTime, int emitCount) {
	particleEmitComputeShader->SetShader();

	particleEmitComputeShader->SetUnorderedAccessView("particles", this->drawListUAV);
	particleEmitComputeShader->SetUnorderedAccessView("deadList", this->deadListUAV);
	//particleEmitComputeShader->SetUnorderedAccessView("sortList", this->sortListUAV);

	particleEmitComputeShader->SetFloat3("startPos", this->transform->GetPosition());
	//particleEmitComputeShader->SetFloat3("cameraPos", cam->GetTransform()->GetPosition());
	particleEmitComputeShader->SetFloat("emitTime", currentTime);
	particleEmitComputeShader->SetInt("maxParticles", this->maxParticles);
	particleEmitComputeShader->SetInt("emitCount", emitCount);

	particleEmitComputeShader->CopyAllBufferData();

	particleEmitComputeShader->DispatchByThreads(emitCount, 1, 1);
}