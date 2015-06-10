#include "headers/elements.hlsli"

float4 main(PixelShaderInput input) : SV_TARGET
{
	float2 normalized = input.origin * 0.5 + 0.5;
	uint2 intPos = uint2(round(normalized * float2(resolution)-0.5));

	uint _index = resolution.x * intPos.y + intPos.x;
	uint index = headers[_index].i;

	int counter = 0;
	while (counter < 10 && index != 0xFFFFFFFF) {
		if (rays[index].active == 1 && hits[index].meshID == primitiveID) {
			rays[index].origin += rays[index].direct * hits[index].distance;
			rays[index].direct = randomCosineWeightedDirectionInHemisphere(hits[index].normal, index);

			rays[index].final = rays[index].color * pow(mcolor, 2.2);
			rays[index].active = 0;

			rays[index].origin += normalize(dot(rays[index].direct, hits[index].normal) * hits[index].normal) * 0.0001f;
		}
		index = rays[index].prev;
		counter++;
	}

	return float4(0.0, 0.0, 0.0, 0.0);
}