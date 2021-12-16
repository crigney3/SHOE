#include "../ShaderHeaders/ShaderShared.hlsli"

RWStructuredBuffer<Particle> drawList	: register(u0);
RWBuffer<uint> argsList					: register(u1);

[numthreads(1, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	// Copy sort list into buffer that VSParticle can use
	// Copy into ParticleData StructuredBuffer? Would have to guarantee I don't rebind
	argsList[0] = drawList.IncrementCounter() * 4; // Currently there are always 4 verts per particle, would need a cbuffer input otherwise
	argsList[1] = 1;
	argsList[2] = 0;
	argsList[3] = 0;
	argsList[5] = 0;
}