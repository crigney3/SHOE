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

	// Consume() has undefined (and entirely undocumented) behavior
	// when consuming from an empty list - it will return 0.
	// This is considered a valid array index, but it corrupts the
	// buffer with all 0s eventually. To get around this,
	// early out the shader on 0, and index all particle buffers
	// and related data from 1 to maxParticles + 1.
	// 
	// consume will pull 30 (29), sort is 0 thru 28 (and then 0 again at index 29) (indices match otherwise), particles 0 and 1 have never been accessed while particle 29 has been emitted but never updated

	uint index = deadList.Consume();

	if (index == 0) return;

	uint indexWithOffset = index - 1;

	Particle particle = particles.Load(indexWithOffset);

	// float offset = threadID.x / 5;

	uint numStructs;
	uint stride;
	deadList.GetDimensions(numStructs, stride);

	float3 startPosition = float3(startPos.x, startPos.y, startPos.z); // +(numStructs / 10));

	particle.age = 0.0f;
	particle.startPosition = startPosition;
	particle.emitTime = emitTime;
	particle.position = startPosition; // Until it moves, it's at the start

	particles[indexWithOffset] = particle;
}