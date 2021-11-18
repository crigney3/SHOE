struct Particle
{
	float EmitTime;
	float3 StartPosition;
};

StructuredBuffer<Particle> ParticleData	: register(t0);

