#include "../ShaderHeaders/ShaderShared.hlsli"

Texture2D screenPixels					: register(t0);
Texture2D refractionSilhouette			: register(t1);

Texture2D textureNormal					: register(t2);

SamplerState sampleState				: register(s0);
SamplerState clampSampler				: register(s1);

cbuffer PerFrame : register(b0)
{
	float3 cameraPos;
	float2 screenSize;
	matrix viewMatrix;
	matrix projMatrix;
}

cbuffer PerMaterial : register(b1)
{
	float uvMult;
	float indexOfRefraction;
	float refractionScale;
}

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixelNormal input) : SV_TARGET
{
	input.uv *= uvMult;

	//sample and unpack normal
	float3 unpackedNormal = textureNormal.Sample(sampleState, input.uv).rgb * 2 - 1;

	float3 N = normalize(input.normal);
	float3 T = normalize(input.tangent);
	T = normalize(T - N * dot(T, N));
	float3 B = cross(T, N);
	float3x3 TBN = float3x3(T, B, N);

	input.normal = mul(unpackedNormal, TBN);

	float3 viewToCam = normalize(cameraPos - input.worldPos);
	float3 viewRefl = normalize(reflect(-viewToCam, input.normal));
	//float NdotV = saturate(dot(input.normal, viewToCam)); // necessary?

	// Credit for most of this to Chris Cascioli
	// The actual screen UV and refraction offset UV
	float2 screenUV = input.position.xy / screenSize;
	float2 offsetUV = float2(0, 0);

	// Calculate the refraction amount in WORLD SPACE
	float3 refrDir = refract(viewToCam, input.normal, indexOfRefraction);

	// Get the refraction XY direction in VIEW SPACE (relative to the camera)
	// We use this as a UV offset when sampling the texture
	offsetUV = mul(viewMatrix, float4(refrDir, 0.0f)).xy;
	offsetUV.x *= -1.0f; // Flip the X to point away from the edge (Y already does this due to view space <-> texture space diff)

	float2 refractedUV = screenUV + offsetUV * refractionScale;

	float silhouette = refractionSilhouette.Sample(clampSampler, refractedUV).r;
	if (silhouette < 1)
	{
		// Invalid spot for the offset so default to THIS pixel's UV for the "refraction"
		refractedUV = screenUV;
	}

	// Get the color at the (now verified) offset UV
	return float4(screenPixels.Sample(clampSampler, refractedUV).rgb, 1) * input.surfaceColor;
}