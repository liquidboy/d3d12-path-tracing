#include "elements.hlsli"

float4 main(PixelShaderInput input) : SV_TARGET
{
	float2 normalized = input.origin * 0.5 + 0.5;
	uint2 intPos = uint2(floor(normalized * float2(resolution)));
	uint index = resolution.x * intPos.y + intPos.x;

	if (hits[index].meshID == 0 && rays[index].active == 1) {
		rays[index].origin += hits[index].distance * rays[index].direct;
		rays[index].active = 0;
		rays[index].color = float3(0.0, 0.0, 0.0);
		rays[index].final = float3(0.0, 0.0, 0.0);
	}

	return float4(0.0, 0.0, 0.0, 0.0);
}