#include "headers/elements.hlsli"

float4 main(PixelShaderInput input) : SV_TARGET
{
	float2 normalized = input.origin * 0.5 + 0.5;
	uint2 intPos = uint2(round(normalized * float2(resolution) - 0.5));
	uint _index = resolution.x * intPos.y + intPos.x;


		Ray ray;
		ray.prev = headers[_index].i;

		uint index = headers.IncrementCounter();
		headers[_index].i = index;

		float2 _b = float2(random(_index), random(_index));
		float2 _a = float2(intPos)+_b;
		_a /= float2(resolution);
		_a = _a * 2.0 - 1.0;

		float4 eye = mul(float4(0.0, 0.0, 0.0, 1.0), view);
		float4 clip = mul(mul(float4(_a, 0.0, 1.0), projection), view);
		float3 pixel = clip.xyz / clip.w;
		float3 direct = normalize(pixel - eye.xyz);

		ray.index = index;
		ray.origin = eye.xyz;
		ray.direct = direct;
		ray.color = float3(1.0, 1.0, 1.0);
		ray.final = float3(0.0, 0.0, 0.0);
		ray.active = 1;
		ray.iorIdx = -1;
		rays[index] = ray;

	return float4(0.0, 0.0, 0.0, 0.0);
}
