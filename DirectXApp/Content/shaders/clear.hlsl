#include "headers/elements.hlsli"

float4 main(PixelShaderInput input) : SV_TARGET
{
	float2 normalized = input.origin * 0.5 + 0.5;
	uint2 intPos = uint2(round(normalized * float2(resolution)-0.5));
	uint index = resolution.x * intPos.y + intPos.x;

	accum[index].color = float3(0.0, 0.0, 0.0);

	return float4(0.0, 0.0, 0.0, 0.0);
}