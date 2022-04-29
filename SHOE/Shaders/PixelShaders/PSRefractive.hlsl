#include "../ShaderHeaders/ShaderShared.hlsli"

Texture2D screenPixels					: register(t0);
Texture2D refractionSilhouette			: register(t1);

Texture2D textureNormal					: register(t2);
Texture2D textureRoughness				: register(t3);
Texture2D textureMetal					: register(t4);

TextureCube environmentMap				: register(t5);

// IBL Textures
//Texture2D brdfLookUpMap					: register(t5);
//TextureCube irradianceIBLMap			: register(t6);
//TextureCube specularIBLMap				: register(t7);

SamplerState sampleState				: register(s0);
SamplerState clampSampler				: register(s1);

cbuffer PerFrame : register(b0)
{
	LightStruct lights[64];
	float3 cameraPos;
	uint lightCount;
	int specIBLTotalMipLevels;
	float2 screenSize;
	matrix viewMatrix;
	matrix projMatrix;
}

cbuffer PerMaterial : register(b1)
{
	float uvMult;
	float indexOfRefraction;
	float refractionScale;
	// If this is false, the material is just transparent
	bool isRefractive;
}

// Fresnel term - Schlick approx.
float SimpleFresnel(float3 n, float3 v, float f0)
{
	// Pre-calculations
	float NdotV = saturate(dot(n, v));

	// Final value
	return f0 + (1 - f0) * pow(1 - NdotV, 5);
}

float SimplerFresnel(float3 NdotV, float f0)
{
	// Final value
	return f0 + (1 - f0) * pow(1 - NdotV, 5);
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
	float3 totalLighting = float3(0,0,0);

	input.uv *= uvMult;

	//sample and unpack normal
	float3 unpackedNormal = textureNormal.Sample(sampleState, input.uv).rgb * 2.0f - 1.0f;

	float3 N = normalize(input.normal);
	float3 T = normalize(input.tangent);
	T = normalize(T - N * dot(T, N));
	float3 B = cross(T, N);
	float3x3 TBN = float3x3(T, B, N);

	input.normal = normalize(mul(unpackedNormal, TBN));

	//Sample roughness map
	float roughness = textureRoughness.Sample(sampleState, input.uv).r;

	//Sample metal map
	float metal = textureMetal.Sample(sampleState, input.uv).r;

	float3 specularColor = lerp(F0_NON_METAL.rrr, input.surfaceColor.rgb, metal);

	float3 viewToCam = normalize(cameraPos - input.worldPos);
	float3 viewRefl = normalize(reflect(-viewToCam, input.normal));

	// Half of Simple Fresnel
	float NdotV = saturate(dot(input.normal, viewToCam));

	// Credit for most of this to Chris Cascioli
	// The actual screen UV and refraction offset UV
	float2 screenUV = input.position.xy / screenSize;
	float2 refractedUV = float2(0, 0);

	if (isRefractive) {
		// Turns out the refract function is evil. Time to make my own
		// float3 refrDir = refract(viewToCam, input.normal, indexOfRefraction);

		float2 offsetUV = textureNormal.Sample(sampleState, input.uv).xy * 2 - 1;
		offsetUV.y *= -1; // UV's are upside down compared to world space

		refractedUV = screenUV + offsetUV * refractionScale;

		// Maybe stick to just specularity, then use roughness/metalness for "frosted glass" style
		// Ignore SSAO
		// The more rough, the more diffuse instead of refract? Maybe just indirect diffuse

		float silhouette = refractionSilhouette.Sample(clampSampler, refractedUV).r;
		if (silhouette < 0.9f)
		{
			// Invalid spot for the offset so default to THIS pixel's UV for the "refraction"
			refractedUV = screenUV;
		}
	}
	else {
		// Transparency only so default to this pixel's UV
		refractedUV = screenUV;
	}
	

	float3 specularity = float3(0, 0, 0);

	for (uint i = 0; i < lightCount; i++) {
		if (lights[i].enabled) {
			float3 toLight = normalize(-lights[i].direction);

			specularity += MicrofacetBRDF(input.normal, toLight, viewToCam, roughness, metal, specularColor);
		}
	}

	float3 output = pow(screenPixels.Sample(clampSampler, refractedUV).rgb, 2.2f);

	// Skybox reflections
	float3 envSample = environmentMap.Sample(sampleState, viewRefl).rgb;

	float fresnel = SimplerFresnel(NdotV, 0.04f);
	output = lerp(output, envSample, fresnel);

	// Get the color at the (now verified) offset UV
	return float4(pow(output + specularity, 1.0f / 2.2f), 1); // *input.surfaceColor;
}