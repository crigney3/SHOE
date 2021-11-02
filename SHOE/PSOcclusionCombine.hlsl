#include "ShaderShared.hlsli"

Texture2D sceneColorsNoAmbient	: register(t0);
Texture2D ambient				: register(t1);
Texture2D SSAOBlur				: register(t2);
SamplerState basicSampler		: register(s0);

float4 main(VertexToPixelIrradiance input) : SV_TARGET
{
	float3 sceneColors = sceneColorsNoAmbient.Sample(basicSampler, input.uv).rgb;
	float3 ambient = Ambient.Sample(basicSampler, input.uv).rgb;
	float ao = SSAOBlur.Sample(basicSampler, input.uv).r;

	return float4(ambient * ao + sceneColors, 1);
}