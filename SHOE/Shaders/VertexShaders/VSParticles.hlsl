#include "../ShaderHeaders/ShaderShared.hlsli"

cbuffer ExternalData : register(b0)
{
	matrix view;
	matrix projection;
	float currentTime;
	float scale;
}

StructuredBuffer<Particle> ParticleData	: register(t0);

VertexToPixelParticle main(uint id : SV_VertexID)
{
	VertexToPixelParticle output;

	uint particleID = id / 4;
	uint cornerID = id % 4;

	Particle p = ParticleData.Load(particleID);

	float age = currentTime - p.emitTime;

	float3 pos = float3(p.startPosition.x, p.startPosition.y + (age * 2), p.startPosition.z);

	float ageScale = 1.0f + age * scale;

	float2 offsets[4];
	offsets[0] = float2(-1.0f, +1.0f) * ageScale;
	offsets[1] = float2(+1.0f, +1.0f) * ageScale;
	offsets[2] = float2(+1.0f, -1.0f) * ageScale;
	offsets[3] = float2(-1.0f, -1.0f) * ageScale;

	pos += float3(view._11, view._12, view._13) * offsets[cornerID].x;
	pos += float3(view._21, view._22, view._23) * offsets[cornerID].y;

	matrix viewProj = mul(projection, view);
	output.position = mul(viewProj, float4(pos, 1.0f));

	float2 uvs[4];
	uvs[0] = float2(0, 0);
	uvs[1] = float2(1, 0);
	uvs[2] = float2(1, 1);
	uvs[3] = float2(0, 1);
	output.uv = uvs[cornerID];

	output.id = particleID;

	return output;
}