#include "elements.hlsli"

float4 main(PixelShaderInput input) : SV_TARGET
{
	float2 normalized = input.origin * 0.5 + 0.5;
	uint2 intPos = uint2(floor(normalized * float2(resolution)));
	uint index = resolution.x * intPos.y + intPos.x;


	if (rays[index].active == 1 && hits[index].meshID == primitiveID) {
		rays[index].origin += rays[index].direct * hits[index].distance;
		rays[index].direct = reflect(rays[index].direct, hits[index].normal);

		rays[index].origin += normalize(dot(rays[index].direct, hits[index].normal) * hits[index].normal) * 0.0001f;
	}


	return float4(0.0, 0.0, 0.0, 0.0);
}