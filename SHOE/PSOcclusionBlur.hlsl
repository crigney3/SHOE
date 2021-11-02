#include "ShaderShared.hlsli"

Texture2D SSAO;

SamplerState clampSampler;

cbuffer ExternalData: register(b0) 
{
	float2 pixelSize;
}

float4 main(VertexToPixelIrradiance input) : SV_TARGET
{
	float ao = 0;
	for (float x = -1.5f; x <= 1.5f; x++) { // -1.5, -0.5, 0.5, 1.5
		for (float y = -1.5; y <= 1.5f; y++) {
			ao += SSAO.Sample(clampSampler, float2(x, y) * pixelSize + input.uv).r;
		}
	}

	ao /= 16.0f;
	return float4(ao.rrr, 1);
}