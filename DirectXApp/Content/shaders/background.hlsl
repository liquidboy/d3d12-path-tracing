#include "headers/elements.hlsli"

float4 main(PixelShaderInput input) : SV_TARGET
{
	float2 normalized = input.origin * 0.5 + 0.5;
	uint2 intPos = uint2(round(normalized * float2(resolution)-0.5));
	uint _index = resolution.x * intPos.y + intPos.x;
	uint index = headers[_index].i;

	int counter = 0;
	while(counter < 10 && index != 0xFFFFFFFF) {
		if (hits[index].meshID == 0 && rays[index].active == 1) {
			rays[index].origin += hits[index].distance * rays[index].direct;
			rays[index].active = 0;
			rays[index].color = float3(0.0, 0.0, 0.0);
			rays[index].final = float3(0.0, 0.0, 0.0);
		}
		index = rays[index].prev;
		counter++;
	}

	return float4(0.0, 0.0, 0.0, 0.0);
}