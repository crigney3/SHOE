#include "../ShaderHeaders/ShaderShared.hlsli"

RWStructuredBuffer<Particle> particles				: register(u0);
AppendStructuredBuffer<uint> deadList				: register(u1);
AppendStructuredBuffer<float2> sortList				: register(u2);

cbuffer ExternalData : register(b0) 
{
	float3 startPos;
	float speed;
	float3 endPos;
	float deltaTime;
	float lifeTime;
}

// This shader simulates particle movement.
[numthreads(8, 8, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	// Alright I'm pretty sure this one's just some cool math
	Particle p = particles[threadID.x];

	if (p.age > lifeTime) {
		deadList.Append(threadID.x);
		return;
	}

	float distanceFromEnd = distance(p.position, endPos);

	p.position = p.position * ((distanceFromEnd / (1 / speed)) * deltaTime);

	particles[threadID.x] = p;
}