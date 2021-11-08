#include "ShaderShared.hlsli"

Texture2D sceneColorsNoAmbient	: register(t0);
Texture2D ambient				: register(t1);
Texture2D SSAOBlur				: register(t2);
SamplerState basicSampler		: register(s0);

float4 main(VertexToPixelIrradiance input) : SV_TARGET
{
	float3 sceneColors = sceneColorsNoAmbient.Sample(basicSampler, input.uv).rgb;
	float3 Ambient = ambient.Sample(basicSampler, input.uv).rgb;
	float ao = SSAOBlur.Sample(basicSampler, input.uv).r;

	float3 final = ((Ambient * ao) + sceneColors);

	return float4(pow(final, 1.0f / 2.2f), 1);
}