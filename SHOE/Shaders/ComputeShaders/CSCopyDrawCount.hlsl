#include "../ShaderHeaders/ShaderShared.hlsli"

RWStructuredBuffer<Particle> particles	: register(u0);
RWStructuredBuffer<uint> deadList		: register(u1);
RWStructuredBuffer<float2> sortList		: register(u2);

[numthreads(1, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	// Copy sort list into buffer that VSParticle can use
}