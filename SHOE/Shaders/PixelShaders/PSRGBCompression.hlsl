#include "../ShaderHeaders/ShaderShared.hlsli"

struct VertexToPixelTextureSample
{
	float4 position	: SV_POSITION;
	float2 uv		: TEXCOORD0;
};

Texture2D RGBFrame			: register(t0);
SamplerState BasicSampler	: register(s0);

float4 main(VertexToPixelTextureSample input) : SV_TARGET
{
	float4 sampleOut = RGBFrame.Sample(BasicSampler, input.uv);
	return float4(sampleOut.b, sampleOut.g, sampleOut.r, sampleOut.a);
}