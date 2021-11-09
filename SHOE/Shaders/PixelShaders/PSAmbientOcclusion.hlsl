#include "../ShaderHeaders/ShaderShared.hlsli"

Texture2D normals					: register(t0);
Texture2D depths					: register(t1);
Texture2D random					: register(t2);

SamplerState sampleState			: register(s0);
SamplerState clampSampler			: register(s1);

cbuffer ExternalData : register(b0)
{
	matrix view;
	matrix projection;
	matrix invProjection;
	float4 offsets[64];
	float ssaoRadius;
	int ssaoSamples;
	float2 randomTextureScreenScale;
}

float3 ViewSpaceFromDepth(float depth, float2 uv) {
	uv.y = 1.0f - uv.y; // Y inversion
	uv = uv * 2.0f - 1.0f;
	float4 screenPos = float4(uv, depth, 1.0f);

	// Convert to view space
	float4 viewPos = mul(invProjection, screenPos);
	return viewPos.xyz / viewPos.w;
}

float2 UVFromViewSpacePosition(float3 viewSpacePosition) {
	// Apply projection to the view space, then divide by perspective
	float4 samplePosScreen = mul(projection, float4(viewSpacePosition, 1));
	samplePosScreen.xyz /= samplePosScreen.w;

	// Adjust to UV coords
	samplePosScreen.xy = samplePosScreen.xy * 0.5f + 0.5f;
	samplePosScreen.y = 1.0f - samplePosScreen.y;

	return samplePosScreen.xy;
}

float4 main(VertexToPixelIrradiance input) : SV_TARGET
{
	// Sample depth
	float pixelDepth = depths.Sample(clampSampler, input.uv).r;
	if (pixelDepth == 1.0f) return float4(1, 1, 1, 1);

	// Get position in view space
	float3 pixelPositionViewSpace = ViewSpaceFromDepth(pixelDepth, input.uv);

	// Sample the random texture
	float3 randomDir = random.Sample(sampleState, input.uv * randomTextureScreenScale).xyz;

	// Sample normals
	float3 normal = normals.Sample(sampleState, input.uv).xyz * 2 - 1;
	normal = normalize(mul((float3x3)view, normal));

	// TBN Matrix
	float3 tangent = normalize(randomDir - normal * dot(randomDir, normal));
	float3 bitangent = cross(tangent, normal);
	float3x3 TBN = float3x3(tangent, bitangent, normal);

	// SSAO loop
	float ao = 0.0f;
	for (int i = 0; i < ssaoSamples; i++) {
		// Rotate and scale offset, then apply
		float3 samplePosView = pixelPositionViewSpace + mul(offsets[i].xyz, TBN) * ssaoRadius;

		// Fetch UV coord
		float2 samplePosScreen = UVFromViewSpacePosition(samplePosView);

		// Sample depth and convert to view
		float sampleDepth = depths.SampleLevel(clampSampler, samplePosScreen.xy, 0).r;
		float sampleZ = ViewSpaceFromDepth(sampleDepth, samplePosScreen.xy).z;

		// Compare depths to determine fade (based on range)
		float rangeCheck = smoothstep(0.0f, 1.0f, ssaoRadius / abs(pixelPositionViewSpace.z - sampleZ));
		ao += (sampleZ < samplePosView.z ? rangeCheck : 0.0f);
	}

	ao = 1.0f - ao / ssaoSamples;
	return float4(ao.rrr, 1);
}