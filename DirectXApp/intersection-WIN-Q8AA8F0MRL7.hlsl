#include "elements.hlsli"




float intersectPlane(in Ray ray, in float3 normal, in float3 origin) {
	float d = dot(origin - ray.origin, normal) / dot(ray.direct, normal);
	if(d < 0) return 10000.f;
	return d;
}

float intersectSphere(in Ray ray, in float radius, in float3 origin) {
	float3 v = ray.origin - origin; // Sphere position relative to ray origin.
	float vDotDirection = dot(v, ray.direct);
	float radicand = vDotDirection * vDotDirection - (dot(v, v) - radius * radius);
	if (radicand < 0) return -1;
	float squareRoot = sqrt(radicand);
	float firstTerm = -vDotDirection;
	float t1 = firstTerm + squareRoot;
	float t2 = firstTerm - squareRoot;

	float t;
	if (t1 < 0 && t2 < 0) { 
		return -1;
	}
	else if (t1 > 0 && t2 > 0) { 
		t = min(t1, t2);
	}
	else {
		t = max(t1, t2);
	}
	return t;
}

float3 normalDSphere(in float distance, in Ray ray, in float3 origin) {
	float3 position = ray.origin + ray.direct * distance;
	return normalize(position - origin);
}

float4 main(PixelShaderInput input) : SV_TARGET
{
	float2 normalized = input.origin * 0.5 + 0.5;
	uint2 intPos = uint2(floor(normalized * float2(resolution)));
	uint index = resolution.x * intPos.y + intPos.x;

	float distance = 10000.0f;
	float3 _normal = float3(0.0, 0.0, 0.0);
	if (primitiveType == 0) {
		distance = intersectSphere(rays[index], radius, origin);
		_normal = normalDSphere(distance, rays[index], origin);
	}
	else 
	if (primitiveType == 1){
		distance = intersectPlane(rays[index], normal, origin);
		_normal = normal;
	}

	if (distance > 0.0 && distance < hits[index].distance) {
		hits[index].distance = distance;
		hits[index].normal = _normal;
		hits[index].meshID = primitiveID;
	}

	return float4(0.0, 0.0, 0.0, 0.0);
}
