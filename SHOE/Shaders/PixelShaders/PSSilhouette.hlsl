cbuffer ExternalData : register(b0)
{
	float3 borderColor;
	float pixelWidth;
	float pixelHeight;
	int borderWidth;
	int hasSelected;
}

struct VertexToPixel
{
	float4 position		: SV_POSITION;
	float2 uv           : TEXCOORD0;
};

Texture2D backBuffer	: register(t0);
Texture2D FilledMeshTexture	: register(t1);
SamplerState samplerOptions	: register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
	if (hasSelected == 0) return backBuffer.Sample(samplerOptions, input.uv);

	float thisPixelX = FilledMeshTexture.Sample(samplerOptions, input.uv).x;

	bool same = true;
	for (int x = -borderWidth; x <= borderWidth; x++) {
		for (int y = -borderWidth; y <= borderWidth; y++) {
			if (FilledMeshTexture.Sample(samplerOptions, input.uv + float2(pixelWidth * x, pixelHeight * y)).x != thisPixelX)
				same = false;
		}
	}

	return float4(same ? backBuffer.Sample(samplerOptions, input.uv) : borderColor, 1);
}