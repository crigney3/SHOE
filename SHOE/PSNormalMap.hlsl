#include "ShaderShared.hlsli"

Texture2D textureAlbedo					: register(t0);
Texture2D textureNormal					: register(t1);
Texture2D textureRough					: register(t2);
Texture2D textureMetal					: register(t3);
Texture2D shadowMap						: register(t4);
Texture2D envShadowMap					: register(t5);

Texture2D brdfLookUpMap					: register(t6);
TextureCube irradianceIBLMap			: register(t7);
TextureCube specularIBLMap				: register(t8);

SamplerState sampleState				: register(s0);
SamplerState clampSampler				: register(s1);

SamplerComparisonState shadowState		: register(s2);

cbuffer ExternalData : register(b0)
{
	LightStruct lights[64];
	float3 ambientColor;
	float specularity;
	float3 cameraPos;
	float uvMult;
	uint lightCount;
	int specIBLTotalMipLevels;
}

float3 calcLightExternal(VertexToPixelNormal input, LightStruct light, float3 specColor, float rough, float metal) {
	return calcLight(input, light, cameraPos, specColor, rough, metal);
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
PS_Output main(VertexToPixelNormal input)
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

	float3 albedoColor = pow(textureAlbedo.Sample(sampleState, input.uv).rgb, 2.2f);

	//Sample roughness map
	float roughness = textureRough.Sample(sampleState, input.uv).r;

	//Sample metal map
	float metal = textureMetal.Sample(sampleState, input.uv).r;

	float3 specularColor = lerp(F0_NON_METAL.rrr, albedoColor.rgb, metal);

	float3 totalLighting = float3(0,0,0);

	//Calculate shadows

	float envLightDepth = input.shadowPos2.z / input.shadowPos2.w;

	float2 envShadowUV = input.shadowPos2.xy / input.shadowPos2.w * 0.5f + 0.5f;

	envShadowUV.y = 1.0f - envShadowUV.y;

	float envShadowAmount = envShadowMap.SampleCmpLevelZero(shadowState, envShadowUV, envLightDepth).r;

	for (uint i = 0; i < lightCount; i++) {
		if (lights[i].enabled) {
			if (i == 4) {
				float lightDepth = input.shadowPos1.z / input.shadowPos1.w;

				float2 shadowUV = input.shadowPos1.xy / input.shadowPos1.w * 0.5f + 0.5f;

				shadowUV.y = 1.0f - shadowUV.y;

				float flashShadowAmount = shadowMap.SampleCmpLevelZero(shadowState, shadowUV, lightDepth).r;

				totalLighting += calcLightExternal(input, lights[i], specularColor, roughness, metal) * flashShadowAmount;
			}
			else {
				totalLighting += calcLightExternal(input, lights[i], specularColor, roughness, metal) * envShadowAmount;
			}			
		}
	}

	float3 viewToCam = normalize(cameraPos - input.worldPos);
	float3 viewRefl = normalize(reflect(-viewToCam, input.normal));
	float NdotV = saturate(dot(input.normal, viewToCam));

	float3 indirectDiffuse = IndirectDiffuse(irradianceIBLMap, sampleState, input.normal);
	float3 indirectSpecular = IndirectSpecular(specularIBLMap,
											   specIBLTotalMipLevels,
											   brdfLookUpMap,
											   clampSampler,
											   viewRefl,
											   NdotV,
											   roughness,
											   specularColor);

	float3 balancedDiff = DiffuseEnergyConserve(indirectDiffuse, indirectSpecular, metal) * albedoColor.rgb;

	totalLighting *= albedoColor.rgb;

	//totalLighting += fullIndirect;

	PS_Output output;
	output.colorNoAmbient = float4(pow(totalLighting + indirectSpecular, 1.0f / 2.2f), 1);
	output.ambientColor   = float4(pow(balancedDiff, 1.0f / 2.2f), 1);
	output.normals		  = float4(input.normal * 0.5f + 0.5f, 1);
	output.depths		  = input.position.z;

	return output;
}