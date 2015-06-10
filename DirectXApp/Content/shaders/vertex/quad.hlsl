struct VertexShaderInput
{
	float2 pos : POSITION;
};

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 origin: POSITION0;
};

PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput output;
	float4 pos = float4(input.pos, 0.0, 1.0f);

	output.pos = pos;
	output.origin = pos;

	return output;
}
