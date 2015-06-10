#include "headers/elements.hlsli"

float4 main(PixelShaderInput input) : SV_TARGET
{
	float2 normalized = input.origin * 0.5 + 0.5;
	uint2 intPos = uint2(round(normalized * float2(resolution)-0.5));
	uint _index = resolution.x * intPos.y + intPos.x;

	uint index = headers[_index].i;
	int counter = 0;
	while (counter < 10 && index != 0xFFFFFFFF) {
		Hit hit;
		hit.distance = 10000.0f; //Avoid errors
		hit.meshID = 0; //Null mesh
		hit.normal = float3(0.0, 0.0, 0.0);
		hits[index] = hit;
		index = rays[index].prev;
		counter++;
	}

	return float4(0.0, 0.0, 0.0, 0.0);
}