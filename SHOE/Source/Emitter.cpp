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
				 std::string name) 
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

	Initialize();
}

Emitter::~Emitter() {
	delete[] particles;
}

void Emitter::Initialize() {
	particles = new Particle[this->maxParticles];
	ZeroMemory(particles, sizeof(Particle) * maxParticles);

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

	}
}

void Emitter::Update(float deltaTime, float totalTime) {
	if (liveParticleCount > 0) {
		if (firstLiveParticle < firstDeadParticle) {
			for (int i = firstLiveParticle; i < firstDeadParticle; i++) {
				UpdateParticle(totalTime, i);
			}
		}
		else if (firstDeadParticle < firstLiveParticle) {
			for (int i = firstLiveParticle; i < maxParticles; i++) {
				UpdateParticle(totalTime, i);
			}
			for (int i = 0; i < firstDeadParticle; i++) {
				UpdateParticle(totalTime, i);
			}
		}
		else {
			for (int i = 0; i < maxParticles; i++) {
				UpdateParticle(totalTime, i);
			}
		}
	}

	timeSinceEmit += deltaTime;
	while (timeSinceEmit > secondsPerEmission) {
		EmitParticle(totalTime);
		timeSinceEmit -= secondsPerEmission;
	}

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

void Emitter::Draw(std::shared_ptr<Camera> cam, float currentTime) {
	UINT stride = 0;
	UINT offset = 0;
	ID3D11Buffer* emptyBuffer = 0;
	context->IASetVertexBuffers(0, 1, &emptyBuffer, &stride, &offset);
	context->IASetIndexBuffer(inBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	particleVertexShader->SetShader();
	particlePixelShader->SetShader();

	particleVertexShader->SetShaderResourceView("ParticleData", particleDataSRV);
	particlePixelShader->SetShaderResourceView("textureParticle", particleTextureSRV);

	particleVertexShader->SetMatrix4x4("view", cam->GetViewMatrix());
	particleVertexShader->SetMatrix4x4("projection", cam->GetProjectionMatrix());
	particleVertexShader->SetFloat("currentTime", currentTime);
	particleVertexShader->CopyAllBufferData();
	//particlePixelShader->CopyAllBufferData();

	context->DrawIndexed(liveParticleCount * 6, 0, 0);
}

void Emitter::EmitParticle(float currentTime) {
	if (liveParticleCount == maxParticles) return;

	Particle tempParticle = {};
	tempParticle.emitTime = currentTime;
	tempParticle.startPos = transform.GetPosition();

	particles[firstDeadParticle] = tempParticle;
	firstDeadParticle++;
	firstDeadParticle %= maxParticles;

	liveParticleCount++;
}