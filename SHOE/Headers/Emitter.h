#pragma once

#include "DXCore.h"
#include "Vertex.h"
#include "Transform.h"
#include "Camera.h"

#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min
#define RandomIntRange(min, max) (int)rand() / RAND_MAX * (max - min) + min

class Emitter {
public:
	Emitter(int maxParticles,
			float particleLifeTime,
			float particlesPerSecond,
			DirectX::XMFLOAT3 position,
			std::shared_ptr<SimplePixelShader> particlePixelShader,
			std::shared_ptr<SimpleVertexShader> particleVertexShader,
			std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> particleTextureSRV,
			Microsoft::WRL::ComPtr<ID3D11Device> device,
			Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
			std::string name,
			bool isMultiParticle);
	~Emitter();

	void Update(float deltaTime, float totalTime);
	void Draw(std::shared_ptr<Camera> cam, float currentTime);

	void SetColorTint(DirectX::XMFLOAT3 color);
	DirectX::XMFLOAT3 GetColorTint();

	void SetScale(float scale);
	float GetScale();

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleDataSRV;
private:
	void Initialize();
	void EmitParticle(float currentTime);
	void UpdateParticle(float currentTime, int index);

	std::string name;

	Transform transform;

	Particle* particles;
	unsigned int* indices;

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
	bool isMultiParticle;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> particleTextureSRV;
	Microsoft::WRL::ComPtr<ID3D11Buffer> inBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> particleDataBuffer;

	DirectX::XMFLOAT3 colorTint;
	float scale;

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
};