#include "../ShaderHeaders/ShaderShared.hlsli"

struct VertexToPixelTextureSample
{
	float4 position	: SV_POSITION;
	float2 uv		: TEXCOORD0;
};

Texture2D Pixels			: register(t0);
SamplerState BasicSampler	: register(s0);

float4 main(VertexToPixelTextureSample input) : SV_TARGET
{
	return float4(Pixels.Sample(BasicSampler, input.uv).rgb, 1);
}