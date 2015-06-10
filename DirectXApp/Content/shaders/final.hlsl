#include "headers/elements.hlsli"

float4 main(PixelShaderInput input) : SV_TARGET
{
	float2 normalized = input.origin * 0.5 + 0.5;
	uint2 intPos = uint2(floor(normalized * float2(resolution)));
	uint index = resolution.x * intPos.y + intPos.x;
	
	float3 color = accum[index].color.xyz;
	accum[index].color = color + rays[index].final;
	rays[index].active = 0;
	
	return float4(0.0, 0.0, 0.0, 0.0);
}