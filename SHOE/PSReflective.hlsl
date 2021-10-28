#include "ShaderShared.hlsli"

Texture2D textureDiffuse	: register(t0);
Texture2D textureNormal		: register(t1);
SamplerState sampleState	: register(s0);

cbuffer ExternalData : register(b0)
{
	LightStruct dirLight1;
	LightStruct dirLight2;
	LightStruct dirLight3;
	LightStruct pointLight1;
	float3 ambientColor;
	float specularity;
	float3 cameraPos;
}

float3 calcLightExternal(VertexToPixelNormal input, LightStruct light, float3 textureColor) {
	return calcLight(input, light, specularity, cameraPos, textureColor);
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
	//sample and unpack normal
	float3 unpackedNormal = textureNormal.Sample(sampleState, input.uv).rgb * 2 - 1;

	float3 N = normalize(input.normal);
	float3 T = normalize(input.tangent);
	T = normalize(T - N * dot(T, N));
	float3 B = cross(T, N);
	float3x3 TBN = float3x3(T, B, N);

	input.normal = mul(unpackedNormal, TBN);

	float3 textureColor = textureDiffuse.Sample(sampleState, input.uv).rgb;

	float3 totalLighting = calcLightExternal(input, dirLight1, textureColor) +
							calcLightExternal(input, dirLight2, textureColor) +
							calcLightExternal(input, dirLight3, textureColor) +
							calcLightExternal(input, pointLight1, textureColor) +
							(ambientColor * input.surfaceColor.rgb * textureColor);

	return float4(totalLighting, 1);
}