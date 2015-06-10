#include "elements.hlsli"

float4 main(PixelShaderInput input) : SV_TARGET
{
	float2 normalized = input.origin * 0.5 + 0.5;
	uint2 intPos = uint2(floor(normalized * float2(resolution)));
	uint index = resolution.x * intPos.y + intPos.x;

	Hit hit;
	hit.distance = 10000.0f; //Avoid errors
	hit.meshID = 0; //Null mesh
	hit.normal = float3(0.0, 0.0, 0.0);
	hits[index] = hit;

	return float4(0.0, 0.0, 0.0, 0.0);
}