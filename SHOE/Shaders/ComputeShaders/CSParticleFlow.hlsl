#include "../ShaderHeaders/ShaderShared.hlsli"

RWStructuredBuffer<Particle> particles				: register(u0);
AppendStructuredBuffer<uint> deadList				: register(u1);
RWStructuredBuffer<float2> sortList					: register(u2);

cbuffer ExternalData : register(b0) 
{
	//float3 startPos;
	float speed;
	float lifeTime;
	uint maxParticles;
	float deltaTime;
	float3 endPos;
}

// This shader simulates particle movement.
[numthreads(32, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	// Need threads between 0 and maxParticles + 1, exclusive
	if (threadID.x > (uint)maxParticles | threadID.x == 0) return;

	uint threadIndexWithOffset = threadID.x - 1;

	Particle p = particles.Load(threadIndexWithOffset);

	if (p.age > lifeTime) {
		// These are already dead, don't append
		return;
	}

	p.age += deltaTime;


	uint numStructs;
	uint strides;
	deadList.GetDimensions(numStructs, strides);
	//float distanceFromEnd = distance(p.position, endPos);

	p.position.y += deltaTime * speed;
	p.position.z += (numStructs % maxParticles) * 2;

	particles[threadIndexWithOffset] = p;

	// float camDistance = distance(particles[threadID.x].startPosition, cameraPos);

	if (p.age >= lifeTime) {
		// If it died this frame, skip drawing and append to dead
		deadList.Append(threadID.x);
	}
	else {
		// Add to sortlist for drawing
		uint sortIndex = sortList.IncrementCounter();

		float2 sortObj = float2(threadIndexWithOffset, threadID.x);

		sortList[sortIndex] = sortObj;
	}
}