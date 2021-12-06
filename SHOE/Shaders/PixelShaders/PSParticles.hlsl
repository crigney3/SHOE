#include "../ShaderHeaders/ShaderShared.hlsli"

Texture2DArray textureParticle		: register(t0);
SamplerState sampleState			: register(s0);

cbuffer ExternalData : register(b0) 
{
	float4 colorTint;
}

float4 main(VertexToPixelParticle input) : SV_TARGET
{
	float elements;
	float width;
	float height;
	textureParticle.GetDimensions(width, height, elements);

	float arrayElement = input.id % elements;

	float3 uvs = float3(input.uv.x, input.uv.y, arrayElement);

	float4 output = textureParticle.Sample(sampleState, uvs);

	output *= colorTint.rgba;

	return pow(output, 2.2f);
}