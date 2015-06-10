#include "elements.hlsli"

float4 main(PixelShaderInput input) : SV_TARGET
{
	float2 normalized = input.origin * 0.5 + 0.5;
	uint2 intPos = uint2(floor(normalized * float2(resolution)));
	uint index = resolution.x * intPos.y + intPos.x;

	float4 eye = mul(float4(0.0, 0.0, 0.0, 1.0), view);
	float4 clip = mul(mul(float4(input.origin + (float2(random(index), random(index)) / resolution / 2), 0.0, 1.0), projection), view);
	float3 pixel = clip.xyz / clip.w;
	float3 direct = normalize(pixel - eye.xyz);

	Ray ray;
	ray.origin = eye.xyz;
	ray.direct = direct;
	ray.color = float3(1.0, 1.0, 1.0);
	ray.final = float3(0.0, 0.0, 0.0);
	ray.active = 1;
	rays[index] = ray;

	return float4(rays[index].direct * 0.5 + 0.5, 1.0f);
}
