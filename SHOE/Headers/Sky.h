#pragma once

#include "Mesh.h"
#include "SimpleShader.h"
#include "DDSTextureLoader.h"
#include "Camera.h"
#include <map>
#include <memory>

class Sky
{
public:
	Sky(Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skyTexture,
		std::shared_ptr<Mesh> cubeMesh,
		std::vector<std::shared_ptr<SimplePixelShader>> pixShaders,
		std::vector<std::shared_ptr<SimpleVertexShader>> vertShaders,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		std::string name = "sky");
	Sky(Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions, 
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skyTexture, 
		const char* filename,
		std::vector<std::shared_ptr<SimplePixelShader>> pixShaders,
		std::vector<std::shared_ptr<SimpleVertexShader>> vertShaders,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		std::string name = "sky");
	~Sky();

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetIrradianceCubeMap();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetConvolvedSpecularCubeMap();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetBRDFLookupTexture();
	int GetIBLMipLevelCount();
	std::string GetName();

	void SetEnableDisable(bool value);
	bool GetEnableDisable();

	void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> cam);
private:
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>	context;

	std::string name;
	bool enabled;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureSRV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthType;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerOptions;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> irradianceCM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> convolvedSpecularCM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> lookupTexture;
	int mipLevelCount;
	const int mipLevelSkip = 3;
	const int CMFaceSize = 256;
	const int lookupTextureSize = 256;

	std::shared_ptr<Mesh> skyGeometry;
	std::shared_ptr<SimplePixelShader> skyPixelShader;
	std::shared_ptr<SimpleVertexShader> skyVertexShader;
	std::vector<std::shared_ptr<SimplePixelShader>> pixShaders;
	std::vector<std::shared_ptr<SimpleVertexShader>> vertShaders;

	void IBLCreateIrradianceMap();
	void IBLCreateConvolvedSpecularMap();
	void IBLCreateBRDFLookUpTexture();
};

