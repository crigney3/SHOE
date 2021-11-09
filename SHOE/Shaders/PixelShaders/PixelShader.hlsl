#include "ShaderShared.hlsli"

Texture2D textureAlbedo		: register(t0);
Texture2D textureRough		: register(t1);
Texture2D textureMetal		: register(t2);
SamplerState sampleState	: register(s0);

cbuffer ExternalData : register(b0)
{
	LightStruct lights[64];
	float3 ambientColor;
	float specularity;
	float3 cameraPos;
	float uvMult;
	uint lightCount;
}

float3 calcLightExternal(VertexToPixel input, LightStruct light, float3 specColor, float rough, float metal) {
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
float4 main(VertexToPixel input) : SV_TARGET
{
	input.uv *= uvMult;

	float3 albedoColor = pow(textureAlbedo.Sample(sampleState, input.uv).rgb, 2.2f);

	//Sample roughness map
	float roughness = textureRough.Sample(sampleState, input.uv).r;

	//Sample metal map
	float metal = textureMetal.Sample(sampleState, input.uv).r;

	float3 specularColor = lerp(F0_NON_METAL.rrr, albedoColor.rgb, metal);

	float3 totalLighting;

	for (uint i = 0; i < lightCount; i++) {
		totalLighting += calcLightExternal(input, lights[i], specularColor, roughness, metal);
	}

	totalLighting *= albedoColor;

	return float4(pow(totalLighting, 1.0f / 2.2f), 1);
}

