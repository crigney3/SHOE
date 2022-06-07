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
		std::vector<std::shared_ptr<SimplePixelShader>> pixShaders,
		std::vector<std::shared_ptr<SimpleVertexShader>> vertShaders,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		std::string name = "sky");
	~Sky();

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetIrradianceCubeMap();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetConvolvedSpecularCubeMap();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetBRDFLookupTexture();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSkyTexture();
	int GetIBLMipLevelCount();

	std::string GetName();
	void SetName(std::string name);

	std::shared_ptr<SimplePixelShader> GetPixShader();
	std::shared_ptr<SimpleVertexShader> GetVertShader();
	Microsoft::WRL::ComPtr<ID3D11SamplerState> GetSampler();

	void SetEnabled(bool value);
	bool IsEnabled();

	bool GetFilenameKeyType();
	void SetFilenameKeyType(bool FKType);

	std::string GetFilenameKey();
	void SetFilenameKey(std::string filenameKey);

	std::string GetFileExtension();
	void SetFileExtension(std::string fileExtension);
private:
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>	context;

	std::string name;
	bool enabled;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureSRV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthType;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerOptions;

	// 0 means this is a .dds filepath, and 1 means
	// this is a directory path containing 6 files
	// correctly labelled as back, down, forward, left, right
	// and up in any filesystem order.
	bool filenameKeyType;
	std::string filenameKey;
	std::string fileExtension;

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

