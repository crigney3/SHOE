#include "../Headers/Mesh.h"

using namespace DirectX;

Mesh::~Mesh() {
	delete[] vertexArray;
	delete[] indices;
}

Mesh::Mesh(Vertex* vertexArray, int vertices, unsigned int* indices, int indexCount, Microsoft::WRL::ComPtr<ID3D11Device> device, std::string name) {
	this->vertexArray = new Vertex[vertices];
	this->indices = new unsigned int[indexCount];
	this->indexCount = indexCount;
	this->materialIndex = -1;
	this->enabled = true;
	this->name = name;
	this->needsDepthPrePass = false;

	std::copy(vertexArray, vertexArray + vertices, this->vertexArray);
	std::copy(indices, indices + vertices, this->indices);

	CalculateTangents(vertexArray, vertices, indices, indexCount);

	MakeBuffers(vertexArray, vertices, indices, indexCount, device);

	CalculateBounds(vertexArray, vertices);
}

Mesh::Mesh(Vertex* vertexArray, int vertices, unsigned int* indices, int indexCount, int associatedMaterialIndex, Microsoft::WRL::ComPtr<ID3D11Device> device, std::string name) {
	this->vertexArray = new Vertex[vertices];
	this->indices = new unsigned int[indexCount];
	this->indexCount = indexCount;
	this->materialIndex = associatedMaterialIndex;
	this->enabled = true;
	this->name = name;
	this->needsDepthPrePass = false;

	std::copy(vertexArray, vertexArray + vertices, this->vertexArray);
	std::copy(indices, indices + vertices, this->indices);

	MakeBuffers(vertexArray, vertices, indices, indexCount, device);

	CalculateBounds(vertexArray, vertices);
}

Mesh::Mesh(std::string filename, Microsoft::WRL::ComPtr<ID3D11Device> device, std::string name) {
	this->materialIndex = -1;
	this->name = name;
	this->needsDepthPrePass = false;

	// Serialize the filename if it's in the right folder
	std::string baseFilename = "";
	size_t dirPos = filename.find("Assets\\Models");
	if (dirPos != std::string::npos) {
		// File is in the assets folder
		baseFilename = "t";
		baseFilename += filename.substr(dirPos + sizeof("Assets\\Models"));
	}
	else {
		baseFilename = "f";
		baseFilename += filename;
	}

	this->filenameKey = baseFilename;

	// Author: Chris Cascioli
	// Purpose: Basic .OBJ 3D model loading, supporting positions, uvs and normals
	// 
	// - You are allowed to directly copy/paste this into your code base
	//   for assignments, given that you clearly cite that this is not
	//   code of your own design.
	//
	// - NOTE: You'll need to #include <fstream>

	// File input object
	std::ifstream obj(filename.c_str());

	// Check for successful open
	if (!obj.is_open())
		return;

	// Variables used while reading the file
	std::vector<XMFLOAT3> positions;	// Positions from the file
	std::vector<XMFLOAT3> normals;		// Normals from the file
	std::vector<XMFLOAT2> uvs;		// uvs from the file
	std::vector<Vertex> verts;		// Verts we're assembling
	std::vector<UINT> indices;		// Indices of these verts
	int vertCounter = 0;			// Count of vertices
	int indexCounter = 0;			// Count of indices
	char chars[100];			// String for line reading

	// Still have data left?
	while (obj.good())
	{
		// Get the line (100 characters should be more than enough)
		obj.getline(chars, 100);

		// Check the type of line
		if (chars[0] == 'v' && chars[1] == 'n')
		{
			// Read the 3 numbers directly into an XMFLOAT3
			XMFLOAT3 norm;
			sscanf_s(
				chars,
				"vn %f %f %f",
				&norm.x, &norm.y, &norm.z);

			// Add to the list of normals
			normals.push_back(norm);
		}
		else if (chars[0] == 'v' && chars[1] == 't')
		{
			// Read the 2 numbers directly into an XMFLOAT2
			XMFLOAT2 uv;
			sscanf_s(
				chars,
				"vt %f %f",
				&uv.x, &uv.y);

			// Add to the list of uv's
			uvs.push_back(uv);
		}
		else if (chars[0] == 'v')
		{
			// Read the 3 numbers directly into an XMFLOAT3
			XMFLOAT3 pos;
			sscanf_s(
				chars,
				"v %f %f %f",
				&pos.x, &pos.y, &pos.z);

			// Add to the positions
			positions.push_back(pos);
		}
		else if (chars[0] == 'f')
		{
			// Read the face indices into an array
			// NOTE: This assumes the given obj file contains
			//  vertex positions, uv coordinates AND normals.
			unsigned int i[12];
			int numbersRead = sscanf_s(
				chars,
				"f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d",
				&i[0], &i[1], &i[2],
				&i[3], &i[4], &i[5],
				&i[6], &i[7], &i[8],
				&i[9], &i[10], &i[11]);

			// If we only got the first number, chances are the OBJ
			// file has no uv coordinates.  This isn't great, but we
			// still want to load the model without crashing, so we
			// need to re-read a different pattern (in which we assume
			// there are no uvs denoted for any of the vertices)
			if (numbersRead == 1)
			{
				// Re-read with a different pattern
				numbersRead = sscanf_s(
					chars,
					"f %d//%d %d//%d %d//%d %d//%d",
					&i[0], &i[2],
					&i[3], &i[5],
					&i[6], &i[8],
					&i[9], &i[11]);

				// The following indices are where the uvs should 
				// have been, so give them a valid value
				i[1] = 1;
				i[4] = 1;
				i[7] = 1;
				i[10] = 1;

				// If we have no uvs, create a single uv coordinate
				// that will be used for all vertices
				if (uvs.size() == 0)
					uvs.push_back(XMFLOAT2(0, 0));
			}

			// - Create the verts by looking up
			//    corresponding data from vectors
			// - OBJ File indices are 1-based, so
			//    they need to be adusted
			Vertex v1;
			v1.Position = positions[i[0] - 1];
			v1.uv = uvs[i[1] - 1];
			v1.normal = normals[i[2] - 1];

			Vertex v2;
			v2.Position = positions[i[3] - 1];
			v2.uv = uvs[i[4] - 1];
			v2.normal = normals[i[5] - 1];

			Vertex v3;
			v3.Position = positions[i[6] - 1];
			v3.uv = uvs[i[7] - 1];
			v3.normal = normals[i[8] - 1];

			// The model is most likely in a right-handed space,
			// especially if it came from Maya.  We want to convert
			// to a left-handed space for DirectX.  This means we 
			// need to:
			//  - Invert the Z position
			//  - Invert the normal's Z
			//  - Flip the winding order
			// We also need to flip the uv coordinate since DirectX
			// defines (0,0) as the top left of the texture, and many
			// 3D modeling packages use the bottom left as (0,0)

			// Flip the uv's since they're probably "upside down"
			v1.uv.y = 1.0f - v1.uv.y;
			v2.uv.y = 1.0f - v2.uv.y;
			v3.uv.y = 1.0f - v3.uv.y;

			// Flip Z (LH vs. RH)
			v1.Position.z *= -1.0f;
			v2.Position.z *= -1.0f;
			v3.Position.z *= -1.0f;

			// Flip normal's Z
			v1.normal.z *= -1.0f;
			v2.normal.z *= -1.0f;
			v3.normal.z *= -1.0f;

			// Add the verts to the vector (flipping the winding order)
			verts.push_back(v1);
			verts.push_back(v3);
			verts.push_back(v2);
			vertCounter += 3;

			// Add three more indices
			indices.push_back(indexCounter); indexCounter += 1;
			indices.push_back(indexCounter); indexCounter += 1;
			indices.push_back(indexCounter); indexCounter += 1;

			// Was there a 4th face?
			// - 12 numbers read means 4 faces WITH uv's
			// - 8 numbers read means 4 faces WITHOUT uv's
			if (numbersRead == 12 || numbersRead == 8)
			{
				// Make the last vertex
				Vertex v4;
				v4.Position = positions[i[9] - 1];
				v4.uv = uvs[i[10] - 1];
				v4.normal = normals[i[11] - 1];

				// Flip the uv, Z pos and normal's Z
				v4.uv.y = 1.0f - v4.uv.y;
				v4.Position.z *= -1.0f;
				v4.normal.z *= -1.0f;

				// Add a whole triangle (flipping the winding order)
				verts.push_back(v1);
				verts.push_back(v4);
				verts.push_back(v3);
				vertCounter += 3;

				// Add three more indices
				indices.push_back(indexCounter); indexCounter += 1;
				indices.push_back(indexCounter); indexCounter += 1;
				indices.push_back(indexCounter); indexCounter += 1;
			}
		}
	}

	// Close the file and create the actual buffers
	obj.close();

	// - At this point, "verts" is a vector of Vertex structs, and can be used
	//    directly to create a vertex buffer:  &verts[0] is the address of the first vert
	//
	// - The vector "indices" is similar. It's a vector of unsigned ints and
	//    can be used directly for the index buffer: &indices[0] is the address of the first int
	//
	// - "vertCounter" is the number of vertices
	// - "indexCounter" is the number of indices
	// - Yes, these are effectively the same since OBJs do not index entire vertices!  This means
	//    an index buffer isn't doing much for us.  We could try to optimize the mesh ourselves
	//    and detect duplicate vertices, but at that point it would be better to use a more
	//    sophisticated model loading library like TinyOBJLoader or AssImp (yes, that's its name)

	this->vertexArray = new Vertex[vertCounter];
	this->indices = new unsigned int[indexCounter];
	this->indexCount = indexCounter;
	std::copy(verts.begin(), verts.end(), vertexArray);
	std::copy(indices.begin(), indices.end(), this->indices);

	CalculateTangents(&verts[0], vertCounter, &indices[0], indexCounter);

	MakeBuffers(&verts[0], vertCounter, &indices[0], indexCounter, device);

	CalculateBounds(&verts[0], vertCounter);
}

void Mesh::MakeBuffers(Vertex* vertexArray, int vertices, unsigned int* indices, int indexCount, Microsoft::WRL::ComPtr<ID3D11Device> device) {
	this->indexCount = indexCount;

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * vertices;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER; // Tells DirectX this is a vertex buffer
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	// Create the proper struct to hold the initial vertex data
	// - This is how we put the initial data into the buffer
	D3D11_SUBRESOURCE_DATA initialVertexData;
	initialVertexData.pSysMem = vertexArray;

	// Actually create the buffer with the initial data
	// - Once we do this, we'll NEVER CHANGE THE BUFFER AGAIN
	device->CreateBuffer(&vbd, &initialVertexData, vBuffer.GetAddressOf());

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(unsigned int) * indexCount;	// 3 = number of indices in the buffer
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;	// Tells DirectX this is an index buffer
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;

	// Create the proper struct to hold the initial index data
	// - This is how we put the initial data into the buffer
	D3D11_SUBRESOURCE_DATA initialIndexData;
	initialIndexData.pSysMem = indices;

	// Actually create the buffer with the initial data
	// - Once we do this, we'll NEVER CHANGE THE BUFFER AGAIN
	device->CreateBuffer(&ibd, &initialIndexData, inBuffer.GetAddressOf());
}

// Calculates the tangents of the vertices in a mesh
// - Code originally adapted from: http://www.terathon.com/code/tangent.html
//   - Updated version now found here: http://foundationsofgameenginedev.com/FGED2-sample.pdf
//   - See listing 7.4 in section 7.5 (page 9 of the PDF)
//
// - Note: For this code to work, your Vertex format must
//         contain an XMFLOAT3 called Tangent
//
// - Be sure to call this BEFORE creating your D3D vertex/index buffers
//
void Mesh::CalculateTangents(Vertex* verts, int numVerts, unsigned int* indices, int numIndices)
{
	// Reset tangents
	for (int i = 0; i < numVerts; i++)
	{
		verts[i].Tangent = XMFLOAT3(0, 0, 0);
	}

	// Calculate tangents one whole triangle at a time
	for (int i = 0; i < numIndices;)
	{
		// Grab indices and vertices of first triangle
		unsigned int i1 = indices[i++];
		unsigned int i2 = indices[i++];
		unsigned int i3 = indices[i++];
		Vertex* v1 = &verts[i1];
		Vertex* v2 = &verts[i2];
		Vertex* v3 = &verts[i3];

		// Calculate vectors relative to triangle positions
		float x1 = v2->Position.x - v1->Position.x;
		float y1 = v2->Position.y - v1->Position.y;
		float z1 = v2->Position.z - v1->Position.z;

		float x2 = v3->Position.x - v1->Position.x;
		float y2 = v3->Position.y - v1->Position.y;
		float z2 = v3->Position.z - v1->Position.z;

		// Do the same for vectors relative to triangle uv's
		float s1 = v2->uv.x - v1->uv.x;
		float t1 = v2->uv.y - v1->uv.y;

		float s2 = v3->uv.x - v1->uv.x;
		float t2 = v3->uv.y - v1->uv.y;

		// Create vectors for tangent calculation
		float r = 1.0f / (s1 * t2 - s2 * t1);

		float tx = (t2 * x1 - t1 * x2) * r;
		float ty = (t2 * y1 - t1 * y2) * r;
		float tz = (t2 * z1 - t1 * z2) * r;

		// Adjust tangents of each vert of the triangle
		v1->Tangent.x += tx;
		v1->Tangent.y += ty;
		v1->Tangent.z += tz;

		v2->Tangent.x += tx;
		v2->Tangent.y += ty;
		v2->Tangent.z += tz;

		v3->Tangent.x += tx;
		v3->Tangent.y += ty;
		v3->Tangent.z += tz;
	}

	// Ensure all of the tangents are orthogonal to the normals
	for (int i = 0; i < numVerts; i++)
	{
		// Grab the two vectors
		XMVECTOR normal = XMLoadFloat3(&verts[i].normal);
		XMVECTOR tangent = XMLoadFloat3(&verts[i].Tangent);

		// Use Gram-Schmidt orthogonalize
		tangent = XMVector3Normalize(
			tangent - normal * XMVector3Dot(normal, tangent));

		// Store the tangent
		XMStoreFloat3(&verts[i].Tangent, tangent);
	}
}

void Mesh::CalculateBounds(Vertex* verts, int numVerts)
{
	DirectX::XMFLOAT3* positions = new DirectX::XMFLOAT3[numVerts];
	for (int i = 0; i < numVerts; i++) positions[i] = verts[i].Position;
	DirectX::BoundingOrientedBox::CreateFromPoints(bounds, numVerts, positions, sizeof(DirectX::XMFLOAT3));
	delete[] positions;
}

Microsoft::WRL::ComPtr<ID3D11Buffer> Mesh::GetVertexBuffer() {
	return vBuffer;
}

Microsoft::WRL::ComPtr<ID3D11Buffer> Mesh::GetIndexBuffer() {
	return inBuffer;
}

Vertex* Mesh::GetVertexArray()
{
	return vertexArray;
}

unsigned int* Mesh::GetIndexArray()
{
	return indices;
}

int Mesh::GetIndexCount() {
	return this->indexCount;
}

void Mesh::SetMaterialIndex(int matIndex) {
	this->materialIndex = matIndex;
}

int Mesh::GetMaterialIndex() {
	return this->materialIndex;
}

std::string Mesh::GetName() {
	return this->name;
}

std::string Mesh::GetFileNameKey() {
	return this->filenameKey;
}

DirectX::BoundingOrientedBox Mesh::GetBounds()
{
	return bounds;
}

void Mesh::SetDepthPrePass(bool prePass) {
	this->needsDepthPrePass = prePass;
}

bool Mesh::GetDepthPrePass() {
	return this->needsDepthPrePass;
}