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
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleTextureSRV,
			Microsoft::WRL::ComPtr<ID3D11Device> device,
			Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
			std::string name,
			bool isMultiParticle = false,
			bool additiveBlend = true);
	~Emitter();

	void Update(float deltaTime, float totalTime);
	void Draw(std::shared_ptr<Camera> cam, float currentTime, Microsoft::WRL::ComPtr<ID3D11BlendState> particleBlendAdditive);

	void SetColorTint(DirectX::XMFLOAT4 color);
	DirectX::XMFLOAT4 GetColorTint();

	void SetScale(float scale);
	float GetScale();

	void SetBlendState(bool AdditiveOrNot);
	bool GetBlendState();

	void SetParticlesPerSecond(float particlesPerSecond);
	float GetParticlesPerSecond();

	void SetParticleLifetime(float particleLifetime);
	float GetParticleLifetime();

	void SetMaxParticles(int maxParticles);
	int GetMaxParticles();

	void SetName(std::string name);
	std::string GetName();

	void SetSpeed(float speed);
	float GetSpeed();

	void SetDestination(DirectX::XMFLOAT3 destination);
	DirectX::XMFLOAT3 GetDestination();

	void SetParticleComputeShader(std::shared_ptr<SimpleComputeShader> shader, ParticleComputeShaderType type);

	void SetMainCamera(std::shared_ptr<Camera> cam);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleDataSRV;
private:
	void Initialize(int maxParticles);
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
	std::shared_ptr<SimpleComputeShader> particleEmitComputeShader;
	std::shared_ptr<SimpleComputeShader> particleSimComputeShader;
	std::shared_ptr<SimpleComputeShader> particleCopyComputeShader;
	bool isMultiParticle;
	bool additiveBlend;
	bool usesComputeShader;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleTextureSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sortListSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> drawListSRV;
	Microsoft::WRL::ComPtr<ID3D11Buffer> inBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> argsBuffer;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> particleUAV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> deadListUAV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> sortListUAV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> drawListUAV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> argsListUAV;

	DirectX::XMFLOAT4 colorTint;
	float scale;
	float speed;
	DirectX::XMFLOAT3 destination;

	std::shared_ptr<Camera> cam;
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
};