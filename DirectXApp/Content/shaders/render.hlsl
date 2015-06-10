#include "headers/elements.hlsli"

float4 main(PixelShaderInput input) : SV_TARGET
{
	float2 normalized = input.origin * 0.5 + 0.5;
	uint2 intPos = uint2(round(normalized * float2(resolution)-0.5));
	uint index = resolution.x * intPos.y + intPos.x;

	return float4(pow(accum[index].color / iterations, 1.0/2.2), 1.0);
}