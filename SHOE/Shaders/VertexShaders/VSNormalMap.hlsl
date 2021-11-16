#include "../ShaderHeaders/ShaderShared.hlsli"

// Struct representing a single vertex worth of data
// - This should match the vertex definition in our C++ code
// - By "match", I mean the size, order and number of members
// - The name of the struct itself is unimportant, but should be descriptive
// - Each variable must have a semantic, which defines its usage
struct VertexShaderInput
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float3 position		: POSITION;     // XYZ position
	float3 normal		: NORMAL;
	float3 tangent		: TANGENT;
	float2 uv			: TEXCOORD;
};

//Instead of separately passing in light view and projection, I'm using the existing ones
//as they'll be the same for my flashlight.
cbuffer ExternalData : register(b0)
{
	float4 colorTint;
	matrix world;
	matrix view;
	matrix projection;
	matrix lightView;
	matrix lightProjection;
	matrix envLightView;
	matrix envLightProjection;
}

// --------------------------------------------------------
// The entry point (main method) for our vertex shader
// 
// - Input is exactly one vertex worth of data (defined by a struct)
// - Output is a single struct of data to pass down the pipeline
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
VertexToPixelNormal main(VertexShaderInput input)
{
	// Set up output struct
	VertexToPixelNormal output;

	matrix wvp = mul(projection, mul(view, world));

	// Here we're essentially passing the input position directly through to the next
	// stage (rasterizer), though it needs to be a 4-component vector now.  
	// - To be considered within the bounds of the screen, the X and Y components 
	//   must be between -1 and 1.  
	// - The Z component must be between 0 and 1.  
	// - Each of these components is then automatically divided by the W component, 
	//   which we're leaving at 1.0 for now (this is more useful when dealing with 
	//   a perspective projection matrix, which we'll get to in future assignments).
	output.position = mul(wvp, float4(input.position, 1.0f));

	//Calculate shadowPos
	output.shadowPos1 = mul(mul(lightProjection, mul(lightView, world)), float4(input.position, 1.0f));
	
	output.shadowPos2 = mul(mul(envLightProjection, mul(envLightView, world)), float4(input.position, 1.0f));

	// Pass the color through 
	// - The values will be interpolated per-pixel by the rasterizer
	// - We don't need to alter it here, but we do need to send it to the pixel shader
	output.surfaceColor = colorTint;

	output.normal = normalize(mul((float3x3)world, input.normal));

	output.tangent = normalize(mul((float3x3)world, input.tangent));

	output.worldPos = mul(world, float4(input.position, 1.0f)).xyz;

	output.uv = input.uv;

	// Whatever we return will make its way through the pipeline to the
	// next programmable stage we're using (the pixel shader for now)
	return output;
}