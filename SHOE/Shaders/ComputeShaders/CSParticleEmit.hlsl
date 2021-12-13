#include "../ShaderHeaders/ShaderShared.hlsli"

RWStructuredBuffer<Particle> particles	: register(u0);
RWStructuredBuffer<uint> deadList		: register(u1);
RWStructuredBuffer<float2> sortList		: register(u2);

cbuffer ExternalData : register(b0)
{
	float3 startPos;
	float3 cameraPos;
}

[numthreads(8, 8, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	// Do all particle emission calculations in here
	// Would need more data
	// And if statements, but I think those are fine for compute shaders
	// The internet is unhelpful in verifying that
	// Can essentially copy over current particle emission and reformat for HLSL
	// I think? Maybe I'm overthinking this. Oh well, ask Chris
	uint index = deadList.Consume();

	Particle particle;

	particle.age = 0.0f;
	particle.startPosition = startPos;

	particles[index] = particle;

	float camDistance = distance(particles[index].startPosition, cameraPos);

	sortList.Append(index, camDistance);
}