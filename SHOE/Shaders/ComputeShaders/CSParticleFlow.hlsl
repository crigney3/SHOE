#include "../ShaderHeaders/ShaderShared.hlsli"

RWStructuredBuffer<Particle> particles	: register(u0);
RWStructuredBuffer<uint> deadList		: register(u1);
RWStructuredBuffer<float2> sortList		: register(u2);

cbuffer ExternalData : register(b0) 
{
	float3 startPos;
	float speed;
	float3 endPos;
	float deltaTime;
	float3 cameraPos;
}

// This shader simulates particle movement.
[numthreads(8, 8, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	
}