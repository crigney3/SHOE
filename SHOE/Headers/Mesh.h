#pragma once

#include "Vertex.h"
#include "DXCore.h"
#include <DirectXMath.h>
#include <wrl/client.h>
#include <fstream>
#include <vector>

class Mesh
{

private:

	Microsoft::WRL::ComPtr<ID3D11Buffer> vBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> inBuffer;
	int indices;
	int materialIndex;
	bool enabled;
	bool needsDepthPrePass;
	std::string name;
public:
	//Load mesh from manual array
	Mesh(Vertex* vertexArray, int vertices, unsigned int* indices, int indexCount, Microsoft::WRL::ComPtr<ID3D11Device> device, std::string name = "mesh");

	//Load mesh from file
	Mesh(const char* filename, Microsoft::WRL::ComPtr<ID3D11Device> device, std::string name = "mesh");

	//Load mesh from assimp (don't reset tangents)
	Mesh(Vertex* vertexArray, int vertices, unsigned int* indices, int indexCount, int associatedMaterialIndex, Microsoft::WRL::ComPtr<ID3D11Device> device, std::string name = "mesh");

	~Mesh();

	void MakeBuffers(Vertex* vertexArray, int vertices, unsigned int* indices, int indexCount, Microsoft::WRL::ComPtr<ID3D11Device> device);
	void CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices);

	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer();
	Microsoft::WRL::ComPtr<ID3D11Buffer> GetIndexBuffer();
	int GetIndexCount();

	void SetDepthPrePass(bool prePass);
	bool GetDepthPrePass();

	std::string GetName();

	void SetEnableDisable(bool value);
	bool GetEnableDisable();
};

