#pragma once

namespace DirectXApp
{
	// Constant buffer used to send MVP matrices to the vertex shader.
	struct ModelViewProjectionConstantBuffer
	{
		DirectX::XMFLOAT4X4 model;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
		DirectX::XMUINT2 resolution;

		//Primitive value
		unsigned primitiveID;
		unsigned primitiveType; //0 - sphere, 1 - plane
		DirectX::XMFLOAT3 origin;

		//Sphere values
		float radius;

		//Plane values
		DirectX::XMFLOAT3 normal;

		//Iterations
		unsigned iterations;
	};

	// Used to send per-vertex data to the vertex shader.
	struct VertexPositionColor
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 color;
	};

	struct PixelPosition {
		DirectX::XMFLOAT2 pos;
	};

	struct Typed {
		DirectX::XMFLOAT3 color;
		unsigned seed;
	};

	struct Ray {
		DirectX::XMFLOAT3 origin;
		DirectX::XMFLOAT3 direct;
		DirectX::XMFLOAT3 color;
		DirectX::XMFLOAT3 final;
		unsigned active;
	};

	struct Hit {
		DirectX::XMFLOAT3 normal;
		float distance;
		unsigned meshID;
	};
}