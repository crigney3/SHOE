#include "../ShaderHeaders/ShaderShared.hlsli"

TextureCube textureSky		: register(t0);
SamplerState sampleState	: register(s0);

struct VertexToPixelSky {
	float4 position		: SV_POSITION;
	float3 sampleDir	: DIRECTION;
};

float4 main(VertexToPixelSky input) : SV_TARGET
{
	return pow(textureSky.Sample(sampleState, input.sampleDir), 2.2f);
}