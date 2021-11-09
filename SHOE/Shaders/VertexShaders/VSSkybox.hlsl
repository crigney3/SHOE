#include "ShaderShared.hlsli"

struct VertexShaderInput
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float3 position		: POSITION;     // XYZ position
	float3 normal		: NORMAL;
	float3 tangent		: TANGENT;
	float2 uv			: TEXCOORD;
};

struct VertexToPixelSky {
	float4 position		: SV_POSITION;
	float3 sampleDir	: DIRECTION;
};

cbuffer ExternalData : register(b0) {
	matrix viewMat;
	matrix projMat;
}

VertexToPixelSky main(VertexShaderInput input)
{
	VertexToPixelSky final;

	matrix viewMatFixed = viewMat;
	viewMatFixed._14 = 0;
	viewMatFixed._24 = 0;
	viewMatFixed._34 = 0;

	final.position = mul(mul(projMat, viewMatFixed), float4(input.position, 1));

	final.position.z = final.position.w;

	final.sampleDir = input.position;

	return final;
}