#include "../ShaderHeaders/ShaderShared.hlsli"

RWStructuredBuffer<float2> sortList		: register(u0);
RWBuffer<uint> argsList					: register(u1);

[numthreads(1, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	// Copy sort list into buffer that VSParticle can use
	argsList[0] = sortList.IncrementCounter() * 6; // Currently there are always 6 indices per particle, would need a cbuffer input otherwise
	argsList[1] = 1; // Instance Count
	argsList[2] = 0; // Offsets
	argsList[3] = 0; // Offsets
	argsList[5] = 0; // Offsets
}