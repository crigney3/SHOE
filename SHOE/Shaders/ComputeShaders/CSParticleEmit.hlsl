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

cbuffer DeadListCounterBuffer : register(b1)
{
    uint DeadListCounter;
}

// Try locking this and flow to make sure they don't run at the same time
[numthreads(32, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	if (threadID.x >= emitCount || DeadListCounter == 0 || threadID.x >= DeadListCounter) return;

	uint index = deadList.Consume();

	Particle particle = particles.Load(index);

	float3 startPosition = float3(startPos.x, startPos.y, startPos.z);

	particle.age = 0.0f;
	particle.startPosition = startPosition;
	particle.emitTime = emitTime;
	particle.position = startPosition; // Until it moves, it's at the start
	particle.alive = 1.0f;

	particles[index] = particle;
}