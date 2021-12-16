#include "../Headers/Emitter.h"

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
	this->firstDeadParticle = 0;
	this->firstLiveParticle = 0;
	this->liveParticleCount = 0;

	this->particlePixelShader = particlePixelShader;
	this->particleVertexShader = particleVertexShader;
	this->particleTextureSRV = particleTextureSRV;

	this->device = device;
	this->context = context;

	this->name = name;

	this->transform = Transform(DirectX::XMMatrixIdentity(), position);

	this->colorTint = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	this->scale = 0.0f;
	this->speed = 1.0f;
	this->destination = DirectX::XMFLOAT3(0.0f, 5.0f, 0.0f);

	this->additiveBlend = additiveBlendState;

	Initialize(this->maxParticles);
}

Emitter::~Emitter() {
	delete[this->maxParticles] particles;
	argsBuffer->Release();
	//particleCopyComputeShader->SetUnorderedAccessView("drawList", NULL);
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

void Emitter::SetMaxParticles(int maxParticles) {
	if (this->maxParticles != maxParticles) {
		this->firstDeadParticle = 0;
		this->firstLiveParticle = 0;
		this->liveParticleCount = 0;

		delete[] particles;
		particleDataSRV.Reset();
		sortListSRV.Reset();
		drawListSRV.Reset();

		particleUAV.Reset();
		deadListUAV.Reset();
		sortListUAV.Reset();
		argsListUAV.Reset();
		drawListUAV.Reset();

		// Need to reset compute shaders fully
		particleCopyComputeShader->SetUnorderedAccessView("drawList", NULL);

		inBuffer.Reset();
		this->maxParticles = maxParticles;
		Initialize(this->maxParticles);
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
			particleSimComputeShader->SetShader();
			particleSimComputeShader->SetUnorderedAccessView("particles", drawListUAV);
			break;
		case 2:
			this->particleCopyComputeShader = computeShader;
			break;
	}
}

void Emitter::SetMainCamera(std::shared_ptr<Camera> cam) {
	this->cam = cam;
}

void Emitter::Initialize(int maxParticles) {
	ID3D11Buffer* particleBuffer;

	// Create and bind the particle buffer for Emit and Flow

	D3D11_BUFFER_DESC desc = {};
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.CPUAccessFlags = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = sizeof(Particle);
	desc.ByteWidth = sizeof(Particle) * maxParticles;
	device->CreateBuffer(&desc, 0, &particleBuffer);

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.Flags = 0;
	uavDesc.Buffer.NumElements = maxParticles;
	device->CreateUnorderedAccessView(particleBuffer, &uavDesc, particleUAV.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = maxParticles;
	device->CreateShaderResourceView(particleBuffer, &srvDesc, particleDataSRV.GetAddressOf());

	particleBuffer->Release();

	// Deadlist for ParticleEmit and ParticleFlow
	// I'm only binding this with append, which only fits ParticleFlow -
	// That could be a problem

	ID3D11Buffer* deadListBuffer;

	D3D11_BUFFER_DESC deadDesc = {};
	deadDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	deadDesc.CPUAccessFlags = 0;
	deadDesc.Usage = D3D11_USAGE_DEFAULT;
	deadDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	deadDesc.StructureByteStride = sizeof(unsigned int);
	deadDesc.ByteWidth = sizeof(unsigned int) * maxParticles;
	device->CreateBuffer(&deadDesc, 0, &deadListBuffer);

	D3D11_UNORDERED_ACCESS_VIEW_DESC deadUavDesc = {};
	deadUavDesc.Format = DXGI_FORMAT_UNKNOWN;
	deadUavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	deadUavDesc.Buffer.FirstElement = 0;
	deadUavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
	deadUavDesc.Buffer.NumElements = maxParticles;
	device->CreateUnorderedAccessView(deadListBuffer, &deadUavDesc, deadListUAV.GetAddressOf());

	deadListBuffer->Release();

	// Create and bind CopyDrawCount's drawList
	// Wait, I'm binding it to sortlist? What?
	// Maybe they're the same structure but I'm passing particleUAV to Copy's drawList

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
	sortUAVDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND; // IncrementCounter() in HLSL
	sortUAVDesc.Buffer.NumElements = maxParticles;
	sortUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	device->CreateUnorderedAccessView(sortListBuffer, &sortUAVDesc, &sortListUAV);

	// SRV (for indexing in VS)
	D3D11_SHADER_RESOURCE_VIEW_DESC sortSRVDesc = {};
	sortSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	sortSRVDesc.Buffer.FirstElement = 0;
	sortSRVDesc.Buffer.NumElements = maxParticles;
	sortSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	device->CreateShaderResourceView(sortListBuffer, &sortSRVDesc, sortListSRV.GetAddressOf());

	sortListBuffer->Release();

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
	device->CreateUnorderedAccessView(drawListBuffer, &drawUAVDesc, &drawListUAV);

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
	device->CreateUnorderedAccessView(argsBuffer.Get(), &argsUAVDesc, argsListUAV.GetAddressOf());

	// Index data, currently always the same

	indices = new unsigned int[maxParticles * 6];
	int indexCount = 0;
	for (int i = 0; i < maxParticles * 4; i += 4) {
		indices[indexCount++] = i;
		indices[indexCount++] = i + 1;
		indices[indexCount++] = i + 2;
		indices[indexCount++] = i;
		indices[indexCount++] = i + 2;
		indices[indexCount++] = i + 3;
	}
	D3D11_SUBRESOURCE_DATA indexData = {};
	indexData.pSysMem = indices;

	// Regular (static) index buffer
	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.ByteWidth = sizeof(unsigned int) * maxParticles * 6;
	device->CreateBuffer(&ibDesc, &indexData, inBuffer.GetAddressOf());
	delete[] indices;
}

void Emitter::UpdateParticle(float currentTime, int index) {
	float age = currentTime - particles[index].emitTime;
	if (age >= particleLifetime) {
		firstLiveParticle++;
		firstLiveParticle %= maxParticles;
		liveParticleCount--;
	}
}

void Emitter::Update(float deltaTime, float totalTime) {
	particleSimComputeShader->SetShader();
	//particleSimComputeShader->SetUnorderedAccessView("particles", particleUAV);
	particleSimComputeShader->SetUnorderedAccessView("deadList", deadListUAV);
	particleSimComputeShader->SetUnorderedAccessView("sortList", sortListUAV);

	particleSimComputeShader->SetFloat3("startPos", this->transform.GetPosition());
	particleSimComputeShader->SetFloat("speed", this->speed);
	particleSimComputeShader->SetFloat3("endPos", this->destination);
	particleSimComputeShader->SetFloat("deltaTime", deltaTime);
	particleSimComputeShader->SetFloat("lifeTime", particleLifetime);

	particleSimComputeShader->DispatchByThreads(maxParticles, 0, 0);

	timeSinceEmit += deltaTime;
	while (timeSinceEmit > secondsPerEmission) {
		EmitParticle(totalTime);
		timeSinceEmit -= secondsPerEmission;
	}
}

void Emitter::Draw(std::shared_ptr<Camera> cam, float currentTime, Microsoft::WRL::ComPtr<ID3D11BlendState> particleBlendAdditive) {
	particleCopyComputeShader->SetShader();

	//particleCopyComputeShader->SetUnorderedAccessView("drawList", drawListUAV);
	particleCopyComputeShader->SetUnorderedAccessView("argsList", argsListUAV);

	particleCopyComputeShader->DispatchByThreads(1, 1, 1);

	if (additiveBlend) {
		context->OMSetBlendState(particleBlendAdditive.Get(), 0, 0xFFFFFFFF);
	}
	else {
		context->OMSetBlendState(0, 0, 0xFFFFFFFF);
	}

	UINT stride = 0;
	UINT offset = 0;
	ID3D11Buffer* emptyBuffer = 0;
	context->IASetVertexBuffers(0, 1, &emptyBuffer, &stride, &offset);
	context->IASetIndexBuffer(inBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	particleVertexShader->SetShader();
	particlePixelShader->SetShader();

	particleVertexShader->SetShaderResourceView("ParticleData", particleDataSRV); // Does this need to be different now?
	particlePixelShader->SetShaderResourceView("textureParticle", particleTextureSRV);

	particleVertexShader->SetMatrix4x4("view", cam->GetViewMatrix());
	particleVertexShader->SetMatrix4x4("projection", cam->GetProjectionMatrix());
	particleVertexShader->SetFloat("currentTime", currentTime);
	particleVertexShader->SetFloat("scale", this->scale);
	particleVertexShader->CopyAllBufferData();
	particlePixelShader->SetFloat4("colorTint", this->colorTint);
	particlePixelShader->CopyAllBufferData();

	context->DrawInstancedIndirect(argsBuffer.Get(), 0);
}

void Emitter::EmitParticle(float currentTime) {
	if (liveParticleCount == maxParticles) return;

	//liveParticleCount++;

	particleEmitComputeShader->SetShader();

	particleEmitComputeShader->SetUnorderedAccessView("particles", drawListUAV);
	particleEmitComputeShader->SetUnorderedAccessView("deadList", deadListUAV);
	particleEmitComputeShader->SetUnorderedAccessView("sortList", sortListUAV);

	particleEmitComputeShader->SetFloat3("startPos", this->transform.GetPosition());
	particleEmitComputeShader->SetFloat3("cameraPos", cam->GetTransform()->GetPosition());
	particleEmitComputeShader->SetFloat("emitTime", currentTime);

	particleEmitComputeShader->DispatchByThreads(maxParticles, 0, 0);
}