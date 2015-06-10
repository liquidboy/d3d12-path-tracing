#define PI                    3.1415926535897932384626422832795028841971
#define TWO_PI				  6.2831853071795864769252867665590057683943
#define SQRT_OF_ONE_THIRD     0.5773502691896257645091487805019574556476
#define E                     2.7182818284590452353602874713526624977572

struct Hit {
	float3 normal;
	float distance;
	uint meshID;
};

struct Ray {
	float3 origin;
	float3 direct;
	float3 color;
	float3 final;
	int active;
	int iorIdx;

	uint index;
	uint prev;
};

struct Vertice {
	float3 v0;
};

struct Indice {
	uint i0;
	uint i1;
	uint i2;
};

struct Typed {
	float3 color;
	uint seed;
};

struct Head {
	uint i;
};

RWStructuredBuffer <Ray> rays : register(u1);
RWStructuredBuffer <Hit> hits : register(u2);
RWStructuredBuffer <Typed> accum : register(u3);
RWStructuredBuffer <Vertice> vertices : register(u4);
RWStructuredBuffer <Indice> indices : register(u5);
RWStructuredBuffer <Head> headers : register(u6);

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 origin: POSITION0;
};

cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
	uint2 resolution;

	uint primitiveID;
	uint primitiveType;
	float3 origin;

	float radius;
	float3 normal;

	uint iterations;

	uint count;

	float3 mcolor;
};

uint hash(uint index) {
	uint a = accum[index].seed;
	a = (a ^ 61) ^ (a >> 16);
	a = a + (a << 3);
	a = a ^ (a >> 4);
	a = a * 0x27d4eb2d;
	a = a ^ (a >> 15);
	accum[index].seed = a;
	return a;
}

float random(uint index) {
	double rand = double(hash(index)) / 2147483647.0;
	double2 seed = double2(rand, rand);
	double2 origin = double2(uint2(index % resolution.x, index / resolution.x)) / double2(resolution);
	double fin = frac(sin(dot(origin + seed, double2(12.9898, 78.233))) * 43758.5453 + rand);
	return float(fin);
}

float3 randomCosineWeightedDirectionInHemisphere(in float3 normal, uint index) {
	float xi1 = random(index);
	float xi2 = random(index);

	float up = sqrt(xi1);
	float over = sqrt(1 - up * up);
	float around = xi2 * TWO_PI;

	float3 directionNotNormal;
	if (abs(normal.x) < SQRT_OF_ONE_THIRD) {
		directionNotNormal = float3(1, 0, 0);
	}
	else if (abs(normal.y) < SQRT_OF_ONE_THIRD) {
		directionNotNormal = float3(0, 1, 0);
	}
	else {
		directionNotNormal = float3(0, 0, 1);
	}
	float3 perpendicular1 = normalize(cross(normal, directionNotNormal));
	float3 perpendicular2 = normalize(cross(normal, perpendicular1)); 

	return normalize((up * normal) + (cos(around) * over * perpendicular1) + (sin(around) * over * perpendicular2));
}

float3 randomDirectionInSphere(uint index) {
	float xi1 = random(index);
	float xi2 = random(index);

	float up = xi1 * 2 - 1; 
	float over = sqrt(1 - up * up);
	float around = xi2 * TWO_PI;

	return normalize(float3(up, cos(around) * over, sin(around) * over));
}