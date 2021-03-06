
cbuffer ExternalData : register(b0)
{
	uint maxParticles;
}

AppendStructuredBuffer<uint> DeadList : register(u0);

[numthreads(32, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	// For each valid id, fill the array with ids
	if (id.x >= (uint)maxParticles) return;

	DeadList.Append(id.x);
}