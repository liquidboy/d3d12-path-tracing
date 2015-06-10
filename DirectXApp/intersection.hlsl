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


float intersectTriangle(in Ray ray, in float3 vertices[3], out float3 normal){
	const float INFINITY = 1e10;
	float3 u, v, n; // triangle floattors
	float3 w0, w;  // ray floattors
	float r, a, b; 

	u = vertices[1] - vertices[0];
	v = vertices[2] - vertices[0];
	n = cross(u, v);

	normal = n;

	w0 = ray.origin - vertices[0];
	a = -dot(n, w0);
	b = dot(n, ray.direct);
	if (abs(b) < 1e-5)
	{
		return INFINITY;
	}

	r = a / b;
	if (r < 0.0)
		return INFINITY;

	float3 I = ray.origin + r * ray.direct;
	float uu, uv, vv, wu, wv, D;
	uu = dot(u, u);
	uv = dot(u, v);
	vv = dot(v, v);
	w = I - vertices[0];
	wu = dot(w, u);
	wv = dot(w, v);
	D = uv * uv - uu * vv;

	float s, t;
	s = (uv * wv - vv * wu) / D;
	if (s < 0.0 || s > 1.0)
		return INFINITY;
	t = (uv * wu - uu * wv) / D;
	if (t < 0.0 || (s + t) > 1.0)
		return INFINITY;

	return (r > 1e-5) ? r : INFINITY;
}

float intersectMesh(in Ray ray, out float3 normal) {
	float distance = 10000.0f;
	for (int i = 0;i < count;i++) {
		Indice indice = indices[i];

		float3 v[3];
		v[0] = vertices[indice.i0].v0;
		v[1] = vertices[indice.i1].v0;
		v[2] = vertices[indice.i2].v0;

		float3 n;
		float t = intersectTriangle(ray, v, n);
		if (t > 0.0 && t < distance) {
			distance = t;
			normal = n;
		}
	}
	return distance;
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
	else 
	if (primitiveType == 2) {
		distance = intersectMesh(rays[index], _normal);
	}

	if (distance > 0.0 && distance < hits[index].distance) {
		hits[index].distance = distance;
		hits[index].normal = _normal;
		hits[index].meshID = primitiveID;
	}

	return float4(0.0, 0.0, 0.0, 0.0);
}
