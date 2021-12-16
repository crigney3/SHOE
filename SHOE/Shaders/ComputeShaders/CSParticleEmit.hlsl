#include "../ShaderHeaders/ShaderShared.hlsli"

RWStructuredBuffer<Particle> particles		: register(u0);
ConsumeStructuredBuffer<uint> deadList		: register(u1);
AppendStructuredBuffer<float2> sortList		: register(u2);

cbuffer ExternalData : register(b0)
{
	float3 startPos;
	uint maxParticles;
	float3 cameraPos;
	float emitTime;
}

[numthreads(8, 8, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	// Can't get size of deadList - how do I check for maxParticles return?
	// Can't use sortList either

	uint index = deadList.Consume();

	Particle particle;

	particle.age = 0.0f;
	particle.startPosition = startPos;
	particle.emitTime = emitTime;
	particle.position = startPos; // Until it moves, it's at the start

	particles[index] = particle;

	float camDistance = distance(particles[index].startPosition, cameraPos);

	float2 sortObj = float2(index, camDistance);

	sortList.Append(sortObj);
}