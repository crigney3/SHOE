#include "ShaderShared.hlsli"

Texture2D texture1Albedo	: register(t0);
Texture2D texture2Albedo	: register(t1);
Texture2D texture3Albedo	: register(t2);
Texture2D texture1Normal	: register(t3);
Texture2D texture2Normal	: register(t4);
Texture2D texture3Normal	: register(t5);
Texture2D texture1Rough		: register(t6);
Texture2D texture2Rough		: register(t7);
Texture2D texture3Rough		: register(t8);
Texture2D texture1Metal		: register(t9);
Texture2D texture2Metal		: register(t10);
Texture2D texture3Metal		: register(t11);

Texture2D blendMap			: register(t12);

Texture2D brdfLookUpMap					: register(t13);
TextureCube irradianceIBLMap			: register(t14);
TextureCube specularIBLMap				: register(t15);

Texture2D shadowMap						: register(t16);
Texture2D envShadowMap					: register(t17);
SamplerState sampleState				: register(s0);
SamplerState clampSampler				: register(s1);
SamplerComparisonState shadowState		: register(s2);

cbuffer ExternalData : register(b0)
{
	LightStruct lights[64];
	float3 cameraPos;
	float uvMultNear;
	float uvMultFar;
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
	//First, get the blend map sample
	float3 blend = blendMap.Sample(sampleState, input.uv).rgb;

	float distanceForUV = distance(cameraPos, input.worldPos.rgb) / 500.0; //Divide by far clip distance

	float2 uvNear = input.uv * uvMultNear;
	float2 uvFar = input.uv * uvMultFar;

	//sample and unpack normals
	float3 unpackedNormal1 = texture1Normal.Sample(sampleState, uvNear).rgb * 2 - 1;
	float3 unpackedNormal2 = texture2Normal.Sample(sampleState, uvNear).rgb * 2 - 1;
	float3 unpackedNormal3 = texture3Normal.Sample(sampleState, uvNear).rgb * 2 - 1;

	float3 unpackedNormal1Far = texture1Normal.Sample(sampleState, uvFar).rgb * 2 - 1;
	float3 unpackedNormal2Far = texture2Normal.Sample(sampleState, uvFar).rgb * 2 - 1;
	float3 unpackedNormal3Far = texture3Normal.Sample(sampleState, uvFar).rgb * 2 - 1;

	float3 lerpedNormal1 = lerp(unpackedNormal1, unpackedNormal1Far, distanceForUV);
	float3 lerpedNormal2 = lerp(unpackedNormal2, unpackedNormal2Far, distanceForUV);
	float3 lerpedNormal3 = lerp(unpackedNormal3, unpackedNormal3Far, distanceForUV);

	float3 N = normalize(input.normal);
	float3 T = normalize(input.tangent);
	T = normalize(T - N * dot(T, N));
	float3 B = cross(T, N);
	float3x3 TBN = float3x3(T, B, N);

	float3 normal1 = mul(lerpedNormal1, TBN);
	float3 normal2 = mul(lerpedNormal2, TBN);
	float3 normal3 = mul(lerpedNormal3, TBN);

	float3 albedoColor1 = pow(texture1Albedo.Sample(sampleState, uvNear).rgb, 2.2f);
	float3 albedoColor2 = pow(texture2Albedo.Sample(sampleState, uvNear).rgb, 2.2f);
	float3 albedoColor3 = pow(texture3Albedo.Sample(sampleState, uvNear).rgb, 2.2f);

	float3 albedoColor1Far = pow(texture1Albedo.Sample(sampleState, uvFar).rgb, 2.2f);
	float3 albedoColor2Far = pow(texture2Albedo.Sample(sampleState, uvFar).rgb, 2.2f);
	float3 albedoColor3Far = pow(texture3Albedo.Sample(sampleState, uvFar).rgb, 2.2f);

	float3 lerpedAlbedo1 = lerp(albedoColor1, albedoColor1Far, distanceForUV);
	float3 lerpedAlbedo2 = lerp(albedoColor2, albedoColor2Far, distanceForUV);
	float3 lerpedAlbedo3 = lerp(albedoColor3, albedoColor3Far, distanceForUV);

	//Sample roughness map
	float roughness1 = texture1Rough.Sample(sampleState, uvNear).r;
	float roughness2 = texture2Rough.Sample(sampleState, uvNear).r;
	float roughness3 = texture3Rough.Sample(sampleState, uvNear).r;

	float roughness1Far = texture1Rough.Sample(sampleState, uvFar).r;
	float roughness2Far = texture2Rough.Sample(sampleState, uvFar).r;
	float roughness3Far = texture3Rough.Sample(sampleState, uvFar).r;

	float3 lerpedRoughness1 = lerp(roughness1, roughness1Far, distanceForUV);
	float3 lerpedRoughness2 = lerp(roughness2, roughness2Far, distanceForUV);
	float3 lerpedRoughness3 = lerp(roughness3, roughness3Far, distanceForUV);

	//Sample metal map - currently, they're all blank for these textures
	float metal1 = texture1Metal.Sample(sampleState, input.uv).r;
	float metal2 = texture2Metal.Sample(sampleState, input.uv).r;
	float metal3 = texture3Metal.Sample(sampleState, input.uv).r;

	float3 specularColor1 = lerp(F0_NON_METAL.rrr, lerpedAlbedo1.rgb, metal1);
	float3 specularColor2 = lerp(F0_NON_METAL.rrr, lerpedAlbedo2.rgb, metal2);
	float3 specularColor3 = lerp(F0_NON_METAL.rrr, lerpedAlbedo3.rgb, metal3);

	float3 albedoColorMain =
		lerpedAlbedo1 * blend.r +
		lerpedAlbedo2 * blend.g +
		lerpedAlbedo3 * blend.b;
	
	float3 specularColorMain =
		specularColor1 * blend.r +
		specularColor2 * blend.g +
		specularColor3 * blend.b;

	float3 roughnessMain =
		roughness1 * blend.r +
		roughness2 * blend.g +
		roughness3 * blend.b;

	float3 metalMain =
		metal1 * blend.r +
		metal2 * blend.g +
		metal3 * blend.b;

	float3 normalMain =
		normal1 * blend.r +
		normal2 * blend.g +
		normal3 * blend.b;

	input.normal = normalMain;

	float3 totalLighting = float3(0, 0, 0);

	//Calculate shadows

	/*float envLightDepth = input.shadowPos2.z / input.shadowPos2.w;

	float2 envShadowUV = input.shadowPos2.xy / input.shadowPos2.w * 0.5f + 0.5f;

	envShadowUV.y = 1.0f - envShadowUV.y;

	float envShadowAmount = envShadowMap.SampleCmpLevelZero(shadowState, envShadowUV, envLightDepth).r;*/

	for (uint i = 0; i < lightCount; i++) {
		if (lights[i].enabled) {
			if (i == 4) {
				float lightDepth = input.shadowPos1.z / input.shadowPos1.w;

				float2 shadowUV = input.shadowPos1.xy / input.shadowPos1.w * 0.5f + 0.5f;

				shadowUV.y = 1.0f - shadowUV.y;

				float flashShadowAmount = shadowMap.SampleCmpLevelZero(shadowState, shadowUV, lightDepth).r;

				totalLighting += calcLightExternal(input, lights[i], specularColorMain, roughnessMain.r, metalMain.r) * flashShadowAmount;
			}
			else {
				totalLighting += calcLightExternal(input, lights[i], specularColorMain, roughnessMain.r, metalMain.r); // *envShadowAmount;
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
											   roughnessMain.r,
											   specularColorMain);

	float3 balancedDiff = DiffuseEnergyConserve(indirectDiffuse.r, indirectSpecular, metalMain.r) * albedoColorMain.rgb;

	totalLighting *= albedoColorMain.rgb;

	PS_Output output;
	output.colorNoAmbient = float4(totalLighting + indirectSpecular, 1);
	output.ambientColor = float4(balancedDiff, 1);
	output.normals = float4(input.normal * 0.5f + 0.5f, 1);
	output.depths = input.position.z;

	return output;
}