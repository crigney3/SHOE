#pragma once

#include "Camera.h"
#include "IComponent.h"
#include "DXCore.h"
#include "Vertex.h"

#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min
#define RandomIntRange(min, max) (int)rand() / RAND_MAX * (max - min) + min

class ParticleSystem : public IComponent
{
public:
	static void SetDefaults(
		std::shared_ptr<SimplePixelShader> particlePixelShader,
		std::shared_ptr<SimpleVertexShader> particleVertexShader,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleTextureSRV,
		std::shared_ptr<SimpleComputeShader> particleEmitComputeShader,
		std::shared_ptr<SimpleComputeShader> particleSimComputeShader,
		std::shared_ptr<SimpleComputeShader> particleCopyComputeShader,
		std::shared_ptr<SimpleComputeShader> particleDeadListInitComputeShader,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context
	);

	void OnDestroy() override;

	void Draw(std::shared_ptr<Camera> cam, Microsoft::WRL::ComPtr<ID3D11BlendState> particleBlendAdditive);

	void SetParticleTextureSRV(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleTextureSRV);

	void SetColorTint(DirectX::XMFLOAT4 color);
	DirectX::XMFLOAT4 GetColorTint();

	void SetScale(float scale);
	float GetScale();

	void SetBlendState(bool AdditiveOrNot);
	bool GetBlendState();

	void SetIsMultiParticle(bool isMultiParticle);
	bool IsMultiParticle();

	void SetParticlesPerSecond(float particlesPerSecond);
	float GetParticlesPerSecond();

	void SetParticleLifetime(float particleLifetime);
	float GetParticleLifetime();

	void SetMaxParticles(int maxParticles);
	int GetMaxParticles();

	void SetSpeed(float speed);
	float GetSpeed();

	void SetDestination(DirectX::XMFLOAT3 destination);
	DirectX::XMFLOAT3 GetDestination();

	void SetFilenameKey(std::string filenameKey);
	std::string GetFilenameKey();

	void SetParticleComputeShader(std::shared_ptr<SimpleComputeShader> shader, ParticleComputeShaderType type);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetDrawListSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSortListSRV();

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleDataSRV;
private:
	static std::shared_ptr<SimplePixelShader> defaultParticlePixelShader;
	static std::shared_ptr<SimpleVertexShader> defaultParticleVertexShader;
	static std::shared_ptr<SimpleComputeShader> defaultParticleEmitComputeShader;
	static std::shared_ptr<SimpleComputeShader> defaultParticleSimComputeShader;
	static std::shared_ptr<SimpleComputeShader> defaultParticleCopyComputeShader;
	static std::shared_ptr<SimpleComputeShader> defaultParticleDeadListInitComputeShader;
	static Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> defaultParticleTextureSRV;
	static Microsoft::WRL::ComPtr<ID3D11Device> defaultDevice;
	static Microsoft::WRL::ComPtr<ID3D11DeviceContext> defaultContext;

	void Start() override;
	void Update() override;

	void Initialize(int maxParticles);
	void EmitParticle(int emitCount);

	unsigned int* indices;

	int maxParticles;

	float particlesPerSecond;
	float secondsPerEmission;
	float timeSinceEmit;

	float particleLifetime;

	std::string filenameKey;

	std::shared_ptr<SimplePixelShader> particlePixelShader;
	std::shared_ptr<SimpleVertexShader> particleVertexShader;
	std::shared_ptr<SimpleComputeShader> particleEmitComputeShader;
	std::shared_ptr<SimpleComputeShader> particleSimComputeShader;
	std::shared_ptr<SimpleComputeShader> particleCopyComputeShader;
	std::shared_ptr<SimpleComputeShader> particleDeadListInitComputeShader;

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

	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
};