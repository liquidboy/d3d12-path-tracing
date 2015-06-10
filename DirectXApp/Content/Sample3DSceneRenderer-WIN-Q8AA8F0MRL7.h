#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Common\StepTimer.h"

using namespace DX;

namespace DirectXApp
{
	// This sample renderer instantiates a basic rendering pipeline.
	class Sample3DSceneRenderer
	{
	public:
		Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		~Sample3DSceneRenderer();
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void Update(DX::StepTimer const& timer);
		void Render();
		void SaveState();
		bool IsTracking() { return m_tracking; }

		void useProgram(unsigned i);

	private:
		void LoadState();

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// Direct3D resources for cube geometry.
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		m_bundleAllocator;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	m_commandList;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	m_cubeBundle[10];
		Microsoft::WRL::ComPtr<ID3D12RootSignature>			m_rootSignature[10];
		Microsoft::WRL::ComPtr<ID3D12PipelineState>			m_pipelineState[10];

		Microsoft::WRL::ComPtr<ID3D12Resource>				m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D12Resource>				m_indexBuffer;
		Microsoft::WRL::ComPtr<ID3D12Resource>				m_constantBuffer;

		Microsoft::WRL::ComPtr<ID3D12Resource>				m_hitBuffer;
		Microsoft::WRL::ComPtr<ID3D12Resource>				m_rayBuffer;
		Microsoft::WRL::ComPtr<ID3D12Resource>				m_seedBuffer;
		Microsoft::WRL::ComPtr<ID3D12Resource>				m_accumBuffer;

		ModelViewProjectionConstantBuffer					m_constantBufferData;
		UINT8*												m_mappedConstantBuffer;
		D3D12_RECT											m_scissorRect;

		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW indexBufferView;

		std::vector<byte> quadShader;
		std::vector<byte> pixelShader[10];

		CDescriptorHeapWrapper m_cbvHeap[10];

		bool resized = false;

		ID3D12DescriptorHeap* ppCbvHeapsReserve[10];
		ID3D12DescriptorHeap* ppSamHeapsReserve[10];

		unsigned iterations = 0;

		// Variables used with the rendering loop.
		bool	m_loadingComplete;
		float	m_radiansPerSecond;
		float	m_angle;
		bool	m_tracking;
	};
}

