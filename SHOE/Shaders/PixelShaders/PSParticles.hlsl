#include "../ShaderHeaders/ShaderShared.hlsli"

Texture2D textureParticle		: register(t0);
SamplerState sampleState		: register(s0);

cbuffer ExternalData : register(b0) 
{
	float3 colorTint;
}

float4 main(VertexToPixelParticle input) : SV_TARGET
{
	float4 output = textureParticle.Sample(sampleState, input.uv);

	output *= float4(colorTint.rgb, 1.0f);

	return pow(output, 2.2f);
}