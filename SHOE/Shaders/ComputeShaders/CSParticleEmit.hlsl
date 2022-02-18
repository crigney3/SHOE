#include "../ShaderHeaders/ShaderShared.hlsli"

RWStructuredBuffer<Particle> particles		: register(u0);
ConsumeStructuredBuffer<uint> deadList		: register(u1);
//RWStructuredBuffer<float2> sortList			: register(u2);

cbuffer ExternalData : register(b0)
{
	float3 startPos;
	int maxParticles;
	//float3 cameraPos;
	float emitTime;
	int emitCount;
}

[numthreads(32, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	if (threadID.x >= emitCount) return;

	uint index = deadList.Consume();

	uint indexWithOffset = index - 1;

	Particle particle = particles.Load(indexWithOffset);

	float3 startPosition = float3(startPos.x, startPos.y, startPos.z);

	particle.age = 0.0f;
	particle.startPosition = startPosition;
	particle.emitTime = emitTime;
	particle.position = startPosition; // Until it moves, it's at the start
	particle.alive = 1.0f;
	particle.debugTrackingAlive = 1.0f;

	particles[indexWithOffset] = particle;
}