#include "../ShaderHeaders/ShaderShared.hlsli"

Texture2D textureParticle		: register(t0);
SamplerState sampleState		: register(s0);

float4 main(VertexToPixelParticle input) : SV_TARGET
{
	return pow(textureParticle.Sample(sampleState, input.uv), 2.2f);
}