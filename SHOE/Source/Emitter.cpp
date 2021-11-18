#include "../Headers/Emitter.h"

Emitter::Emitter(int maxParticles,
				 float particleLifeTime,
				 float particlesPerSecond,
				 DirectX::XMFLOAT3 position,
				 std::shared_ptr<SimplePixelShader> particlePixelShader,
				 std::shared_ptr<SimpleVertexShader> particleVertexShader,
				 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleTextureSRV,
				 Microsoft::WRL::ComPtr<ID3D11Device> device,
				 Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) 
{
	this->maxParticles = maxParticles;
	this->particlesPerSecond = particlesPerSecond;
	this->particleLifetime = particleLifeTime;

	this->firstDeadParticle = 0;
	this->firstLiveParticle = 0;
	this->liveParticleCount = 0;

	this->particlePixelShader = particlePixelShader;
	this->particleVertexShader = particleVertexShader;
	this->particleTextureSRV = particleTextureSRV;

	this->device = device;
	this->context = context;

	this->transform = Transform(DirectX::XMMatrixIdentity(), position);

	Initialize();
}

Emitter::~Emitter() {
	delete[] particles;
}

void Emitter::Initialize() {
	particles = new Particle[this->maxParticles];

	D3D11_BUFFER_DESC desc = {};
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = sizeof(Particle);
	desc.ByteWidth = sizeof(Particle) * maxParticles;
	device->CreateBuffer(&desc, 0, particleDataBuffer.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = maxParticles;
	device->CreateShaderResourceView(particleDataBuffer.Get(), &srvDesc, particleDataSRV.GetAddressOf());
	
	for (int i = 0; i < maxParticles; i++) {
		particles[i].emitTime = 0;
		particles[i].startPos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	}
}

void Emitter::Update(float deltaTime, float totalTime) {
	timeSinceEmit += deltaTime;
	while (timeSinceEmit > secondsPerEmission) {
		EmitParticle(totalTime);
		timeSinceEmit -= secondsPerEmission;
	}

	if (totalTime - particles[firstLiveParticle].emitTime > particleLifetime) {
		firstLiveParticle++;
		if (firstLiveParticle > maxParticles) firstLiveParticle = 0;
		liveParticleCount--;
	}
}

void Emitter::Draw() {
	D3D11_MAPPED_SUBRESOURCE mapped = {};
	context->Map(particleDataBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

	if (firstLiveParticle < firstDeadParticle) {
		memcpy(mapped.pData,
			   particles + firstLiveParticle,
			   sizeof(Particle) * liveParticleCount);
	}
	else {
		memcpy(mapped.pData,
			   particles,
			   sizeof(Particle) * firstDeadParticle);

		memcpy((void*)((Particle*)mapped.pData + firstDeadParticle),
			   particles + firstLiveParticle,
			   sizeof(Particle) * (maxParticles - firstLiveParticle));
	}

	context->Unmap(particleDataBuffer.Get(), 0);
}

void Emitter::EmitParticle(float currentTime) {
	Particle tempParticle = {};
	tempParticle.emitTime = currentTime;
	tempParticle.startPos = transform.GetPosition();

	particles[firstDeadParticle] = tempParticle;
	firstDeadParticle++;
	liveParticleCount++;

	if (firstDeadParticle > maxParticles) {
		firstDeadParticle = 0;
	}
}