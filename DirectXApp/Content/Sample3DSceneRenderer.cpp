#include "pch.h"

#include "Sample3DSceneRenderer.h"

#include "..\Common\DirectXHelper.h"
#include <ppltasks.h>
#include <synchapi.h>

#include <string>

using namespace DirectXApp;

using namespace Concurrency;
using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Storage;

Platform::String^ AngleKey = "Angle";
Platform::String^ TrackingKey = "Tracking";

Sample3DSceneRenderer::Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_radiansPerSecond(XM_PIDIV4),
	m_angle(0),
	m_tracking(false),
	m_mappedConstantBuffer(nullptr),
	m_deviceResources(deviceResources)
{
	LoadState();
	ZeroMemory(&m_constantBufferData, sizeof(m_constantBufferData));

	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

DirectXApp::Sample3DSceneRenderer::~Sample3DSceneRenderer()
{
	m_constantBuffer->Unmap(0, nullptr);
	m_mappedConstantBuffer = nullptr;
}

DXGI_SAMPLE_DESC sampleDesc;
CD3DX12_RESOURCE_DESC resourceDescSource;
D3D12_BUFFER_UAV bufferDescSource;
D3D12_UNORDERED_ACCESS_VIEW_DESC uavDescSource;
D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
	D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
	D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
	D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
	D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

void initDescriptorSouces() {
	sampleDesc.Count = 1;
	sampleDesc.Quality = 0;

	resourceDescSource.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resourceDescSource.Buffer(1);
	resourceDescSource.Format = DXGI_FORMAT_UNKNOWN;
	resourceDescSource.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDescSource.Alignment = 0;
	resourceDescSource.DepthOrArraySize = 1;
	resourceDescSource.Height = 1;
	resourceDescSource.Width = 1;
	resourceDescSource.MipLevels = 1;
	resourceDescSource.SampleDesc = sampleDesc;
	resourceDescSource.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	bufferDescSource.FirstElement = 0;
	bufferDescSource.NumElements = 1;
	bufferDescSource.StructureByteStride = 1;
	bufferDescSource.CounterOffsetInBytes = 0;
	bufferDescSource.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	uavDescSource.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDescSource.Format = DXGI_FORMAT_UNKNOWN;
}

void Sample3DSceneRenderer::CreateDeviceDependentResources()
{
	initDescriptorSouces();
	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&m_bundleAllocator)));

	Concurrency::task<void> quadTask = DX::ReadDataAsync(L"quad.cso").then([this](std::vector<byte>& fileData) {quadShader = fileData;});

	Concurrency::task<void> createTask[20];
	Concurrency::task<void> loadTask[20];

	std::wstring shaders[] = { L"camera.cso", L"begin.cso", L"intersection.cso", L"background.cso", L"diffuse.cso", L"light.cso",L"final.cso", L"render.cso", L"clear.cso", L"refraction.cso", L"reflection.cso", L"reset.cso" };
	shaderCount = _countof(shaders);

	int it;
	for (it = 0;it < shaderCount;it++) {

		Concurrency::task<void> * task = (it-1 < 0) ? &quadTask : &(createTask[it-1]);
		loadTask[it] = DX::ReadDataAsync(shaders[it]).then([this, it](std::vector<byte>& fileData) { 
			const int &i = it; 
			pixelShader[i] = fileData;
		});
		createTask[it] = (loadTask[it] && *task).then([this, it]() {
			const int &i = it;

			static const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			{
				CD3DX12_DESCRIPTOR_RANGE range[7];
				CD3DX12_ROOT_PARAMETER parameter[1];

				range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
				range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
				range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2);
				range[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 3);
				range[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 4);
				range[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 5);
				range[6].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 6);
				parameter[0].InitAsDescriptorTable(_countof(range), range, D3D12_SHADER_VISIBILITY_ALL);

				CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
				descRootSignature.Init(1, parameter, 0, nullptr, rootSignatureFlags);

				ComPtr<ID3DBlob> pSignature;
				ComPtr<ID3DBlob> pError;
				DX::ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, pSignature.GetAddressOf(), pError.GetAddressOf()));
				DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature[i])));
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC state = {};
			state.InputLayout = { inputLayout, _countof(inputLayout) };
			state.pRootSignature = m_rootSignature[i].Get();
			state.VS = { &quadShader[0], quadShader.size() };
			state.PS = { &pixelShader[i][0], pixelShader[i].size() };
			state.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			state.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			state.DepthStencilState.DepthEnable = FALSE;
			state.DepthStencilState.StencilEnable = FALSE;
			state.SampleMask = UINT_MAX;
			state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			state.NumRenderTargets = 1;
			state.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			state.SampleDesc.Count = 1;

			DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateGraphicsPipelineState(&state, IID_PPV_ARGS(&m_pipelineState[i])));
			DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_deviceResources->GetCommandAllocator(), m_pipelineState[i].Get(), IID_PPV_ARGS(&m_commandList)));

			pixelShader[i].clear();
		});
	}

	///////////////////////////////////////
	////////////INIT RESOURCES/////////////
	///////////////////////////////////////

	//Can and must be used in many shaders

	auto createAssetsTask = (createTask[it-1]).then([this]() {
		quadShader.clear();
		auto d3dDevice = m_deviceResources->GetD3DDevice();
		
		//////////////////////////////////////////VERTICES///////////////////////////////////

		PixelPosition cubeVertices[] =
		{
			{ XMFLOAT2(-1.0f,  1.0f) },
			{ XMFLOAT2(-1.0f, -1.0f) },
			{ XMFLOAT2(1.0f,  1.0f) },
			{ XMFLOAT2(1.0f, -1.0f) }
		};
		const UINT vertexBufferSize = sizeof(cubeVertices);

		DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_vertexBuffer)));
		m_vertexBuffer->SetName(L"Vertex Buffer Resource");

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferUpload;
		DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBufferUpload)));
		vertexBufferUpload->SetName(L"Vertex Buffer Upload Resource");

		{
			D3D12_SUBRESOURCE_DATA vertexData = {};
			vertexData.pData = reinterpret_cast<BYTE*>(cubeVertices);
			vertexData.RowPitch = vertexBufferSize;
			vertexData.SlicePitch = vertexData.RowPitch;

			UpdateSubresources(m_commandList.Get(), m_vertexBuffer.Get(), vertexBufferUpload.Get(), 0, 0, 1, &vertexData);
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
		}

		vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		vertexBufferView.StrideInBytes = sizeof(PixelPosition);
		vertexBufferView.SizeInBytes = sizeof(cubeVertices);


		////////////////////////////////INDICES//////////////////////////////////////

		unsigned short cubeIndices[] =
		{
			0,2,1,
			1,2,3
		};
		const UINT indexBufferSize = sizeof(cubeIndices);
		
		DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_indexBuffer)));
		m_indexBuffer->SetName(L"Index Buffer Resource");

		Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferUpload;
		DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBufferUpload)));
		indexBufferUpload->SetName(L"Index Buffer Upload Resource");

		{
			D3D12_SUBRESOURCE_DATA indexData = {};
			indexData.pData = reinterpret_cast<BYTE*>(cubeIndices);
			indexData.RowPitch = indexBufferSize;
			indexData.SlicePitch = indexData.RowPitch;

			UpdateSubresources(m_commandList.Get(), m_indexBuffer.Get(), indexBufferUpload.Get(), 0, 0, 1, &indexData);
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
		}

		indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
		indexBufferView.SizeInBytes = sizeof(cubeIndices);
		indexBufferView.Format = DXGI_FORMAT_R16_UINT;

		//////////////////////////CONSTANT//////////////////////

		DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(m_constantBufferData)), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_constantBuffer)));
		m_constantBuffer->SetName(L"Constant Buffer");

		D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
		desc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
		desc.SizeInBytes = (sizeof(ModelViewProjectionConstantBuffer) + 255) & ~255;	// the size of the constant buffer must be a multiple of 256 bytes.

		DX::ThrowIfFailed(m_constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedConstantBuffer)));
		memcpy(m_mappedConstantBuffer, &m_constantBufferData, sizeof(m_constantBufferData));



		//////////////////////////RAYS UAV//////////////////////////

		const unsigned rayCount = 2048*1024;
		D3D12_UNORDERED_ACCESS_VIEW_DESC raysDesc(uavDescSource);

		{
			CD3DX12_RESOURCE_DESC resourceDesc(resourceDescSource);
			resourceDesc.Buffer(rayCount * sizeof(Ray));
			resourceDesc.Width = rayCount * sizeof(Ray);

			DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_rayBuffer)));
			m_rayBuffer->SetName(L"Ray Buffer");
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_rayBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

			D3D12_BUFFER_UAV bufferDesc(bufferDescSource);
			bufferDesc.NumElements = rayCount;
			bufferDesc.StructureByteStride = sizeof(Ray);
			raysDesc.Buffer = bufferDesc;
		}



		//////////////////////////HITS UAV//////////////////////////
		D3D12_UNORDERED_ACCESS_VIEW_DESC hitsDesc(uavDescSource);
		{
			CD3DX12_RESOURCE_DESC resourceDesc(resourceDescSource);
			resourceDesc.Buffer(rayCount * sizeof(Hit));
			resourceDesc.Width = rayCount * sizeof(Hit);

			DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_hitBuffer)));
			m_hitBuffer->SetName(L"Hit Buffer");
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_hitBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

			D3D12_BUFFER_UAV bufferDesc(bufferDescSource);
			bufferDesc.NumElements = rayCount;
			bufferDesc.StructureByteStride = sizeof(Hit);

			hitsDesc.Buffer = bufferDesc;
		}



		/////////////////////// SEED UAV /////////////////////
		D3D12_UNORDERED_ACCESS_VIEW_DESC seedDesc(uavDescSource);
		{
			CD3DX12_RESOURCE_DESC resourceDesc(resourceDescSource);
			resourceDesc.Buffer(rayCount * sizeof(Typed));
			resourceDesc.Width = rayCount * sizeof(Typed);

			DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_seedBuffer)));
			m_seedBuffer->SetName(L"Seed Buffer");
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_seedBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

			D3D12_BUFFER_UAV bufferDesc(bufferDescSource);
			bufferDesc.NumElements = rayCount;
			bufferDesc.StructureByteStride = sizeof(Typed);

			seedDesc.Buffer = bufferDesc;
		}
		
		Vertice vertices[] =
		{
			{ XMFLOAT3(-0.5f, -0.5f, -0.5f) },
			{ XMFLOAT3(-0.5f, -0.5f,  0.5f) },
			{ XMFLOAT3(-0.5f,  0.5f, -0.5f) },
			{ XMFLOAT3(-0.5f,  0.5f,  0.5f) },
			{ XMFLOAT3(0.5f, -0.5f, -0.5f) },
			{ XMFLOAT3(0.5f, -0.5f,  0.5f) },
			{ XMFLOAT3(0.5f,  0.5f, -0.5f) },
			{ XMFLOAT3(0.5f,  0.5f,  0.5f) }
		};

		
		Indice indices[] =
		{
			{ 0,2,1 }, // -x
			{ 1,2,3 },

			{ 4,5,6 }, // +x
			{ 5,7,6 },

			{ 0,1,5 }, // -y
			{ 0,5,4 },

			{ 2,6,7 }, // +y
			{ 2,7,3 },

			{ 0,4,6 }, // -z
			{ 0,6,2 },

			{ 1,3,7 }, // +z
			{ 1,7,5 }
		};

		/////////////////////// TRIANGLES UAV /////////////////////
		D3D12_UNORDERED_ACCESS_VIEW_DESC verticeDesc(uavDescSource);
		{
			CD3DX12_RESOURCE_DESC resourceDesc(resourceDescSource);
			resourceDesc.Buffer(sizeof(vertices));
			resourceDesc.Width = sizeof(vertices);

			DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_verticeBuffer)));
			m_verticeBuffer->SetName(L"Vertice Buffer");
			
			DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&verticeBufferUpload)));
			verticeBufferUpload->SetName(L"Vertice upload buffer");

			{
				D3D12_SUBRESOURCE_DATA data = {};
				data.pData = reinterpret_cast<BYTE*>(vertices);
				data.RowPitch = resourceDesc.Width;
				data.SlicePitch = data.RowPitch;

				UpdateSubresources(m_commandList.Get(), m_verticeBuffer.Get(), verticeBufferUpload.Get(), 0, 0, 1, &data);
				m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_verticeBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
			}

			D3D12_BUFFER_UAV bufferDesc(bufferDescSource);
			bufferDesc.NumElements = _countof(vertices);
			bufferDesc.StructureByteStride = sizeof(Vertice);

			verticeDesc.Buffer = bufferDesc;
		}

		/////////////////////// Indices UAV /////////////////////
		D3D12_UNORDERED_ACCESS_VIEW_DESC indiceDesc(uavDescSource);
		{
			CD3DX12_RESOURCE_DESC resourceDesc(resourceDescSource);
			resourceDesc.Buffer(sizeof(indices));
			resourceDesc.Width = sizeof(indices);

			DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_indiceBuffer)));
			m_indiceBuffer->SetName(L"Indice Buffer");
			
			DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indiceBufferUpload)));
			verticeBufferUpload->SetName(L"Indice upload buffer");

			{
				D3D12_SUBRESOURCE_DATA data = {};
				data.pData = reinterpret_cast<BYTE*>(indices);
				data.RowPitch = resourceDesc.Width;
				data.SlicePitch = data.RowPitch;

				UpdateSubresources(m_commandList.Get(), m_indiceBuffer.Get(), indiceBufferUpload.Get(), 0, 0, 1, &data);
				m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_indiceBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
			}

			D3D12_BUFFER_UAV bufferDesc(bufferDescSource);
			bufferDesc.NumElements = _countof(indices);
			bufferDesc.StructureByteStride = sizeof(Indice);

			indiceDesc.Buffer = bufferDesc;
		}




		//////////////////////////Header UAV//////////////////////////
		D3D12_UNORDERED_ACCESS_VIEW_DESC headerDesc(uavDescSource);
		{
			CD3DX12_RESOURCE_DESC resourceDesc(resourceDescSource);
			resourceDesc.Buffer(rayCount * sizeof(unsigned));
			resourceDesc.Width = rayCount * sizeof(unsigned);

			DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_headerBuffer)));
			m_headerBuffer->SetName(L"Hit Buffer");
			m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_headerBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

			D3D12_BUFFER_UAV bufferDesc(bufferDescSource);
			bufferDesc.NumElements = rayCount;
			bufferDesc.StructureByteStride = sizeof(unsigned);

			headerDesc.Buffer = bufferDesc;
		}

		//Counter
		{
			CD3DX12_RESOURCE_DESC resourceDesc(resourceDescSource);
			resourceDesc.Buffer(sizeof(unsigned));
			resourceDesc.Width = sizeof(unsigned);

			DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_headerBufferCounter)));
			m_headerBufferCounter->SetName(L"Counter Buffer");

			DX::ThrowIfFailed(m_headerBufferCounter->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedHeaderBufferCounter)));
			memcpy(m_mappedHeaderBufferCounter, &counter, sizeof(unsigned));
		}






		//////////////////////EXECUTE UPLOADING///////////////////////

		DX::ThrowIfFailed(m_commandList->Close());
		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		(m_deviceResources->GetCommandQueue())->SetName(L"render");
		m_deviceResources->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		m_deviceResources->WaitForGpu();


		ID3D12DescriptorHeap* ppHeaps[1];


		//////////////////
		//Shader Related//
		//////////////////

		for (int i = 0;i < shaderCount;i++) {
			////////////////////// VIEWS AND HEAPS (Shader 0) ////////////////
			m_cbvHeap[i].Create(d3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 7, true);
			m_cbvHeap[i].pDH->SetName(L"Heap");

			d3dDevice->CreateConstantBufferView(&desc, m_cbvHeap[i].hCPUHeapStart);
			d3dDevice->CreateUnorderedAccessView(m_rayBuffer.Get(), nullptr, &raysDesc, m_cbvHeap[i].hCPU(1)); 
			d3dDevice->CreateUnorderedAccessView(m_hitBuffer.Get(), nullptr, &hitsDesc, m_cbvHeap[i].hCPU(2));
			d3dDevice->CreateUnorderedAccessView(m_seedBuffer.Get(), nullptr, &seedDesc, m_cbvHeap[i].hCPU(3)); 
			d3dDevice->CreateUnorderedAccessView(m_verticeBuffer.Get(), nullptr, &verticeDesc, m_cbvHeap[i].hCPU(4));
			d3dDevice->CreateUnorderedAccessView(m_indiceBuffer.Get(), nullptr, &indiceDesc, m_cbvHeap[i].hCPU(5)); 
			d3dDevice->CreateUnorderedAccessView(m_headerBuffer.Get(), m_headerBufferCounter.Get(), &headerDesc, m_cbvHeap[i].hCPU(6)); 

			ppCbvHeapsReserve[i] = m_cbvHeap[i].pDH.Get();

			////////////////////////// BUNDLE (Shader 0) /////////////////////////
			ppHeaps[0] = ppCbvHeapsReserve[i];
			DX::ThrowIfFailed(d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, m_bundleAllocator.Get(), m_pipelineState[i].Get(), IID_PPV_ARGS(&m_cubeBundle[i])));
			m_cubeBundle[i]->SetName(L"Quad");
			m_cubeBundle[i]->SetGraphicsRootSignature(m_rootSignature[i].Get());
			m_cubeBundle[i]->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
			m_cubeBundle[i]->SetGraphicsRootDescriptorTable(0, m_cbvHeap[i].hGPUHeapStart);
			m_cubeBundle[i]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			m_cubeBundle[i]->IASetVertexBuffers(0, 1, &vertexBufferView);
			m_cubeBundle[i]->IASetIndexBuffer(&indexBufferView);
			m_cubeBundle[i]->DrawIndexedInstanced(_countof(cubeIndices), 1, 0, 0, 0);

			DX::ThrowIfFailed(m_cubeBundle[i]->Close());
		}

	});

	createAssetsTask.then([this]() {
		m_loadingComplete = true;
	});
}

void Sample3DSceneRenderer::CreateWindowSizeDependentResources()
{
	resized = true;

	Size outputSize = m_deviceResources->GetOutputSize();
	m_scissorRect = { 0, 0, static_cast<LONG>(outputSize.Width), static_cast<LONG>(outputSize.Height) };

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();
	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	float aspectRatio = max(outputSize.Width / outputSize.Height, outputSize.Height / outputSize.Width);
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(fovAngleY, aspectRatio, 0.0001f, 10000.0f);
	XMStoreFloat4x4(&m_constantBufferData.projection, XMMatrixTranspose(XMMatrixInverse(nullptr, perspectiveMatrix * orientationMatrix)));

	static const XMVECTORF32 eye = { 6.0f, 0.0f, -1.0f, 1.0f };
	static const XMVECTORF32 at = { 0.0f, 0.0f, 1.0f, 1.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 1.0f };
	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(XMMatrixInverse(nullptr, XMMatrixLookAtRH(eye, at, up))));

	XMUINT2 resolution = { static_cast<unsigned>(outputSize.Width) , static_cast<unsigned>(outputSize.Height) };
	m_constantBufferData.resolution = resolution;
}

void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
	if (m_loadingComplete)
	{
		memcpy(m_mappedConstantBuffer, &m_constantBufferData, sizeof(m_constantBufferData));
	}
}





void Sample3DSceneRenderer::useProgram(unsigned i) {
	ID3D12DescriptorHeap* ppHeaps[1];
	ID3D12CommandList* ppCommandLists[1];

	unsigned shaderPointer = i;

	ppHeaps[0] = ppCbvHeapsReserve[shaderPointer];

	DX::ThrowIfFailed(m_deviceResources->GetCommandAllocator()->Reset());
	DX::ThrowIfFailed(m_commandList->Reset(m_deviceResources->GetCommandAllocator(), m_pipelineState[shaderPointer].Get()));

	PIXBeginEvent(m_commandList.Get(), 0, L"Draw the cube");
	{
		m_commandList->SetName(L"Proccess");
		m_commandList->SetPipelineState(m_pipelineState[shaderPointer].Get());
		m_commandList->SetGraphicsRootSignature(m_rootSignature[shaderPointer].Get());
		m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		m_commandList->RSSetViewports(1, &m_deviceResources->GetScreenViewport());
		m_commandList->RSSetScissorRects(1, &m_scissorRect);
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
		m_commandList->ClearRenderTargetView(m_deviceResources->GetRenderTargetView(), DirectX::Colors::CornflowerBlue, 0, nullptr);
		m_commandList->OMSetRenderTargets(1, &m_deviceResources->GetRenderTargetView(), false, nullptr);
		m_commandList->ExecuteBundle(m_cubeBundle[shaderPointer].Get());
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	}

	PIXEndEvent(m_commandList.Get());
	DX::ThrowIfFailed(m_commandList->Close());

	ppCommandLists[0] = { m_commandList.Get() };
	m_deviceResources->GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	m_deviceResources->WaitForGpu();
}


void Sample3DSceneRenderer::Render()
{
	if (!m_loadingComplete){
		return;
	}

	if (first) {
		//first = false;

		if (resized) {
			resized = false;
			iterations = 0;
			useProgram(8);
		}

		//Generate camera
		useProgram(11);

		counter = 0;
		memcpy(m_mappedHeaderBufferCounter, &counter, sizeof(unsigned));
		useProgram(0);

		for (int i = 0;i < 8;i++) {
			useProgram(1);


			m_constantBufferData.origin = { 0.0f, -1.0f, 0.0f };
			m_constantBufferData.normal = { 0.0f, 1.0f, 0.0f };
			m_constantBufferData.primitiveType = 1;
			m_constantBufferData.primitiveID = 1;
			memcpy(m_mappedConstantBuffer, &m_constantBufferData, sizeof(m_constantBufferData));
			useProgram(2); //Interection shader (diffuse plane)

			m_constantBufferData.origin = { 1.0f, 0.0f, 0.0f };
			m_constantBufferData.radius = 0.5;
			m_constantBufferData.primitiveType = 0;
			m_constantBufferData.primitiveID = 2;
			memcpy(m_mappedConstantBuffer, &m_constantBufferData, sizeof(m_constantBufferData));
			useProgram(2); //Interection shader (sphere)


			m_constantBufferData.origin = { 0.0f, 0.0f, 2.0f };
			m_constantBufferData.radius = 0.9;
			m_constantBufferData.primitiveType = 0;
			m_constantBufferData.primitiveID = 3;
			memcpy(m_mappedConstantBuffer, &m_constantBufferData, sizeof(m_constantBufferData));
			useProgram(2); //Interection shader (sphere)

			m_constantBufferData.origin = { 0.0f, 0.0f, -2.0f };
			m_constantBufferData.radius = 0.9;
			m_constantBufferData.primitiveType = 0;
			m_constantBufferData.primitiveID = 4;
			memcpy(m_mappedConstantBuffer, &m_constantBufferData, sizeof(m_constantBufferData));
			useProgram(2); //Interection shader (sphere)

			m_constantBufferData.origin = { 0.0f, 2.0f, 1.0f };
			m_constantBufferData.count = 36 / 3;
			m_constantBufferData.primitiveType = 2;
			m_constantBufferData.primitiveID = 5;
			memcpy(m_mappedConstantBuffer, &m_constantBufferData, sizeof(m_constantBufferData));
			useProgram(2); //Interection shader (polygon)

			useProgram(3); //Background shader

			m_constantBufferData.primitiveID = 1;
			m_constantBufferData.mcolor = { 1.0f, 1.0f, 1.0f };
			memcpy(m_mappedConstantBuffer, &m_constantBufferData, sizeof(m_constantBufferData));
			useProgram(4); //Diffuse shader

			m_constantBufferData.primitiveID = 2;
			memcpy(m_mappedConstantBuffer, &m_constantBufferData, sizeof(m_constantBufferData));
			useProgram(5); //Light shader


			m_constantBufferData.primitiveID = 3;
			m_constantBufferData.mcolor = { 0.9f, 0.2f, 0.2f };
			memcpy(m_mappedConstantBuffer, &m_constantBufferData, sizeof(m_constantBufferData));
			useProgram(4); //Diffuse shader

			m_constantBufferData.primitiveID = 4;
			m_constantBufferData.mcolor = { 1.0f, 1.0f, 1.0f };
			memcpy(m_mappedConstantBuffer, &m_constantBufferData, sizeof(m_constantBufferData));
			//useProgram(4); //Diffuse shader
			useProgram(9); //Refraction shader

			m_constantBufferData.primitiveID = 5;
			m_constantBufferData.mcolor = { 0.1f, 0.9f, 0.1f };
			memcpy(m_mappedConstantBuffer, &m_constantBufferData, sizeof(m_constantBufferData));
			useProgram(4); //Diffuse shader


		}

		useProgram(6); //Accumulation

		iterations++;
		m_constantBufferData.iterations = iterations;
		memcpy(m_mappedConstantBuffer, &m_constantBufferData, sizeof(m_constantBufferData));
		useProgram(7); //Render
	}
}




void Sample3DSceneRenderer::SaveState()
{
	auto state = ApplicationData::Current->LocalSettings->Values;

	if (state->HasKey(AngleKey))
	{
		state->Remove(AngleKey);
	}
	if (state->HasKey(TrackingKey))
	{
		state->Remove(TrackingKey);
	}

	state->Insert(AngleKey, PropertyValue::CreateSingle(m_angle));
	state->Insert(TrackingKey, PropertyValue::CreateBoolean(m_tracking));
}

void Sample3DSceneRenderer::LoadState()
{
	auto state = ApplicationData::Current->LocalSettings->Values;
	if (state->HasKey(AngleKey))
	{
		m_angle = safe_cast<IPropertyValue^>(state->Lookup(AngleKey))->GetSingle();
		state->Remove(AngleKey);
	}
	if (state->HasKey(TrackingKey))
	{
		m_tracking = safe_cast<IPropertyValue^>(state->Lookup(TrackingKey))->GetBoolean();
		state->Remove(TrackingKey);
	}
}