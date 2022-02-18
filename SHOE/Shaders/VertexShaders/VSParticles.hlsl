#include "../ShaderHeaders/ShaderShared.hlsli"

cbuffer ExternalData : register(b0)
{
	matrix view;
	matrix projection;
	float currentTime;
	float scale;
}

StructuredBuffer<Particle> ParticleData	: register(t0);
StructuredBuffer<float2> SortList		: register(t1);

// Second float2 of sortList is for sorting on distance from camera, currently unused

VertexToPixelParticle main(uint id : SV_VertexID)
{
	VertexToPixelParticle output;

	uint particleID = id / 4;
	uint cornerID = id % 4;

	float2 drawData = SortList.Load(particleID);

	Particle p = ParticleData.Load(drawData.x);

	// Now that calculations are done in the compute shaders,
	// This shader can be much lighter

	float ageScale = 1.0f + p.age * scale;

	float2 offsets[4];
	offsets[0] = float2(-1.0f, +1.0f);
	offsets[1] = float2(+1.0f, +1.0f);
	offsets[2] = float2(+1.0f, -1.0f);
	offsets[3] = float2(-1.0f, -1.0f);

	float3 pos = p.position;

	pos += float3(view._11, view._12, view._13) * offsets[cornerID].x * ageScale;
	pos += float3(view._21, view._22, view._23) * offsets[cornerID].y * ageScale;

	matrix viewProj = mul(projection, view);
	output.position = mul(viewProj, float4(pos, 1.0f));

	float2 uvs[4];
	uvs[0] = float2(0, 0);
	uvs[1] = float2(1, 0);
	uvs[2] = float2(1, 1);
	uvs[3] = float2(0, 1);
	output.uv = saturate(uvs[cornerID]);

	output.id = drawData.x;

	return output;
}