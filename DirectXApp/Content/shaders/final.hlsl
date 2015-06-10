#include "headers/elements.hlsli"

float4 main(PixelShaderInput input) : SV_TARGET
{
	float2 normalized = input.origin * 0.5 + 0.5;
	uint2 intPos = uint2(round(normalized * float2(resolution)-0.5));
	uint _index = resolution.x * intPos.y + intPos.x;
	uint index = headers[_index].i;

	float3 color = accum[_index].color.xyz;
	float3 sampled = float3(0.0, 0.0, 0.0);

	int counter = 0;
	while (counter < 10 && index != 0xFFFFFFFF) {
		sampled += rays[index].final;
		rays[index].active = 0;

		index = rays[index].prev;
		counter++;
	}

	accum[_index].color = color + sampled / counter;
	
	return float4(0.0, 0.0, 0.0, 0.0);
}