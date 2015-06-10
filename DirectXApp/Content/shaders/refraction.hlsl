#include "headers/elements.hlsli"



float computeFresnel(in float3 normal, in float3 incident, in float3 transmissionDirection, float refractiveIndexIncident, float refractiveIndexTransmitted) {
	float cosThetaIncident = dot(normal, incident);
	float cosThetaTransmitted = dot(normal, transmissionDirection);
	float reflectionCoefficientSPolarized = pow((refractiveIndexIncident * cosThetaIncident - refractiveIndexTransmitted * cosThetaTransmitted) / (refractiveIndexIncident * cosThetaIncident + refractiveIndexTransmitted * cosThetaTransmitted), 2);
	float reflectionCoefficientPPolarized = pow((refractiveIndexIncident * cosThetaTransmitted - refractiveIndexTransmitted * cosThetaIncident) / (refractiveIndexIncident * cosThetaTransmitted + refractiveIndexTransmitted * cosThetaIncident), 2);
	float reflectionCoefficientUnpolarized = (reflectionCoefficientSPolarized + reflectionCoefficientPPolarized) / 2.0;
	return reflectionCoefficientUnpolarized;
}

float4 main(PixelShaderInput input) : SV_TARGET
{
	float2 normalized = input.origin * 0.5 + 0.5;
	uint2 intPos = uint2(round(normalized * float2(resolution)-0.5));
	uint _index = resolution.x * intPos.y + intPos.x;
	uint index = headers[_index].i;

	int counter = 0;
	while (counter < 10 && index != 0xFFFFFFFF) {
		if (rays[index].applyed == 0 && rays[index].active == 1 && hits[index].meshID == primitiveID) {
			rays[index].origin += rays[index].direct * hits[index].distance;

			float inIor = 1.0;
			float outIor = 1.3333;

			float factor = dot(rays[index].direct, hits[index].normal);
			if (factor > 0) {
				float _temp = inIor;
				inIor = outIor;
				outIor = _temp;
			}

			float3 normalized = -normalize(dot(rays[index].direct, hits[index].normal) * hits[index].normal);

			float3 refraction = refract(rays[index].direct, normalized, inIor / outIor);
			float3 reflection = reflect(rays[index].direct, normalized);

			float fresnel = computeFresnel(hits[index].normal, rays[index].direct, refraction, inIor, outIor);
			if (random(_index) <= fresnel) {
				rays[index].direct = reflection;
			}
			else {
				rays[index].direct = refraction;
			}

			rays[index].origin += normalize(dot(rays[index].direct, hits[index].normal) * hits[index].normal) * 0.0001f;
			rays[index].applyed = 1;
		}
		index = rays[index].prev;
		counter++;
	}


	return float4(0.0, 0.0, 0.0, 0.0);
}