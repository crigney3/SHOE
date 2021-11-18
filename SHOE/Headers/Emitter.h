#pragma once

#include "DXCore.h"
#include "Vertex.h"
#include "Transform.h"

class Emitter {
public:
	Emitter(int maxParticles,
			float particleLifeTime,
			float particlesPerSecond,
			DirectX::XMFLOAT3 position,
			std::shared_ptr<SimplePixelShader> particlePixelShader,
			std::shared_ptr<SimpleVertexShader> particleVertexShader,
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleTextureSRV,
			Microsoft::WRL::ComPtr<ID3D11Device> device,
			Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);
	~Emitter();

	void Update(float deltaTime, float totalTime);
	void Draw();
private:
	void Initialize();
	void EmitParticle(float currentTime);

	Transform transform;

	Particle* particles;

	int maxParticles;
	int firstDeadParticle;
	int firstLiveParticle;
	int liveParticleCount;

	float particlesPerSecond;
	float secondsPerEmission;
	float timeSinceEmit;

	float particleLifetime;

	std::shared_ptr<SimplePixelShader> particlePixelShader;
	std::shared_ptr<SimpleVertexShader> particleVertexShader;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleDataSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleTextureSRV;
	Microsoft::WRL::ComPtr<ID3D11Buffer> inBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> particleDataBuffer;

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
};