#include "stdafx.h"
#include <comdef.h>
#include <iostream>
#include "Engine.h"

const XMFLOAT3 X_UNIT_VEC_FLOAT = XMFLOAT3(1.0f, 0.0f, 0.0f);
const XMFLOAT3 Y_UNIT_VEC_FLOAT = XMFLOAT3(0.0f, 1.0f, 0.0f);
const XMFLOAT3 Z_UNIT_VEC_FLOAT = XMFLOAT3(0.0f, 0.0f, 1.0f);

const XMVECTOR X_UNIT_VEC = XMLoadFloat3(&X_UNIT_VEC_FLOAT);
const XMVECTOR Y_UNIT_VEC = XMLoadFloat3(&Y_UNIT_VEC_FLOAT);
const XMVECTOR Z_UNIT_VEC = XMLoadFloat3(&Z_UNIT_VEC_FLOAT);

Engine::Engine(UINT resolutionWidth, UINT resolutionHeight)
	: m_resolutionWidth(resolutionWidth), m_resolutionHeight(resolutionHeight),
	m_actor(this)
{
	m_shadowMapRes = 1024;
}


Engine::~Engine()
{
}


_Use_decl_annotations_
void Engine::GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
	ComPtr<IDXGIAdapter1> adapter;
	*ppAdapter = nullptr;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			// If you want a software adapter, pass in "/warp" on the command line.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the
		// actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}

	*ppAdapter = adapter.Detach();
}

void Engine::WaitForPreviousFrame()
{
	// wait for previous frame

	// signal fence value
	const UINT64 fence = m_fenceValue;
	HRESULT hr = m_commandQueue->Signal(m_fence.Get(), fence);
	if (FAILED(hr))
	{
		exit(-1);
	}

	++m_fenceValue;

	if (m_fence->GetCompletedValue() < fence)
	{
		hr = m_fence->SetEventOnCompletion(fence, m_fenceEvent);
		if (FAILED(hr))
		{
			exit(-1);
		}
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void Engine::CreateRootSignature()
{
	// WVP matrix
	D3D12_ROOT_DESCRIPTOR rootDescriptor;
	rootDescriptor.ShaderRegister = 0;
	rootDescriptor.RegisterSpace = 0;

	D3D12_ROOT_PARAMETER rootParameters[3];

	// WVP matrix
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].Descriptor = rootDescriptor;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// albedo texture
	D3D12_DESCRIPTOR_RANGE descriptorRanges[3];
	descriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRanges[0].NumDescriptors = 1;
	descriptorRanges[0].BaseShaderRegister = 0;
	descriptorRanges[0].RegisterSpace = 0;
	descriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// normal texture
	descriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRanges[1].NumDescriptors = 1;
	descriptorRanges[1].BaseShaderRegister = 1;
	descriptorRanges[1].RegisterSpace = 0;
	descriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// depth buffer texture
	descriptorRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRanges[2].NumDescriptors = 1;
	descriptorRanges[2].BaseShaderRegister = 2;
	descriptorRanges[2].RegisterSpace = 0;
	descriptorRanges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
	descriptorTable.NumDescriptorRanges = _countof(descriptorRanges);
	descriptorTable.pDescriptorRanges = &descriptorRanges[0];

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].DescriptorTable = descriptorTable;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// sampler for shadow mapping

	D3D12_DESCRIPTOR_RANGE lightDepthRange;
	lightDepthRange.BaseShaderRegister = 1;
	lightDepthRange.NumDescriptors = 1;
	lightDepthRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	lightDepthRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	lightDepthRange.RegisterSpace = 0;

	D3D12_ROOT_DESCRIPTOR_TABLE lightDepthTable;
	lightDepthTable.NumDescriptorRanges = 1;
	lightDepthTable.pDescriptorRanges = &lightDepthRange;

	rootParameters[2].DescriptorTable = lightDepthTable;
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// sampler
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;

	rootSignatureDesc.Init(_countof(rootParameters),
		rootParameters,
		1,
		&sampler,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
	);

	ID3DBlob* errorBuffer;
	ID3DBlob* signature;
	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBuffer);
	if (FAILED(hr))
	{
		OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
		exit(-1);
	}
	
	hr = m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
	if (FAILED(hr))
	{
		exit(-1);
	}
}

void Engine::CreateLightRootSignature()
{
	// WVP matrix
	D3D12_ROOT_DESCRIPTOR rootDescriptor;
	rootDescriptor.ShaderRegister = 0;
	rootDescriptor.RegisterSpace = 0;

	D3D12_ROOT_PARAMETER rootParameters[1];

	// WVP matrix
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].Descriptor = rootDescriptor;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// sampler
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;

	rootSignatureDesc.Init(_countof(rootParameters),
		rootParameters,
		1,
		&sampler,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
	);

	ID3DBlob* errorBuffer;
	ID3DBlob* signature;
	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBuffer);
	if (FAILED(hr))
	{
		OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
		exit(-1);
	}

	hr = m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_lightRootSignature));
	if (FAILED(hr))
	{
		exit(-1);
	}
}

void Engine::LoadShaders()
{
	// load shaders

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	ComPtr<ID3DBlob> vsErrorMsgs;
	HRESULT hr = D3DCompileFromFile(
		TEXT("Shaders.hlsl"), nullptr, nullptr, "vsMain", "vs_5_0", compileFlags, 0, &m_vertexShader, &vsErrorMsgs);

	if (FAILED(hr))
	{
		char* errorMsgStr = reinterpret_cast<char*>(vsErrorMsgs->GetBufferPointer());
		OutputDebugStringA(errorMsgStr);
		exit(-1);
	}

	ComPtr<ID3DBlob> psErrorMsgs;
	hr = D3DCompileFromFile(
		TEXT("Shaders.hlsl"), nullptr, nullptr, "psMain", "ps_5_0", compileFlags, 0, &m_pixelShader, &psErrorMsgs);

	if (FAILED(hr))
	{
		char* errorMsgStr = reinterpret_cast<char*>(psErrorMsgs->GetBufferPointer());
		OutputDebugStringA(errorMsgStr);
		exit(-1);
	}

	ComPtr<ID3DBlob> psDepthErrorMsgs;
	hr = D3DCompileFromFile(
		TEXT("Shaders.hlsl"), nullptr, nullptr, "psDepth", "ps_5_0", compileFlags, 0, &m_lightPixelShader, &psDepthErrorMsgs);

	if (FAILED(hr))
	{
		char* errorMsgStr = reinterpret_cast<char*>(psDepthErrorMsgs->GetBufferPointer());
		OutputDebugStringA(errorMsgStr);
		exit(-1);
	}

	ComPtr<ID3DBlob> vsDepthErrorMsgs;
	hr = D3DCompileFromFile(
		TEXT("Shaders.hlsl"), nullptr, nullptr, "vsDepth", "vs_5_0", compileFlags, 0, &m_lightVertexShader, &vsDepthErrorMsgs);

	if (FAILED(hr))
	{
		char* errorMsgStr = reinterpret_cast<char*>(psDepthErrorMsgs->GetBufferPointer());
		OutputDebugStringA(errorMsgStr);
		exit(-1);
	}
}

void Engine::LoadTextures()
{
	// SRV descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC srvDescriptorHeapDesc = {};
	srvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvDescriptorHeapDesc.NumDescriptors = 3;
	srvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	HRESULT hr = m_device->CreateDescriptorHeap(&srvDescriptorHeapDesc, IID_PPV_ARGS(&m_textureDescriptorHeap));

	if (FAILED(hr))
	{
		exit(-1);
	}

	m_textureDescriptorHeap->SetName(TEXT("SRV Descriptor Heap"));

	m_actor.LoadAlbedoFromFile(TEXT("Assets\\color.png"));
	m_actor.UploadAlbedoResource(m_textureDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_CPU_DESCRIPTOR_HANDLE textureDescriptorHeapStart = m_textureDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	UINT srvHandleDescriptorIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE normalCPUDescriptorHandle(
		textureDescriptorHeapStart,
		1,
		srvHandleDescriptorIncrementSize
	);

	m_actor.LoadNormalFromFile(TEXT("Assets\\normal.png"));
	m_actor.UploadNormalResource(normalCPUDescriptorHandle);

	// light depth
	// SRV descriptor
	D3D12_SHADER_RESOURCE_VIEW_DESC srvLightDepthTextDesc = {};
	srvLightDepthTextDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvLightDepthTextDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvLightDepthTextDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvLightDepthTextDesc.Texture2D.MipLevels = 1;

	CD3DX12_CPU_DESCRIPTOR_HANDLE lightCPUDescriptorHandle(
		textureDescriptorHeapStart,
		2,
		srvHandleDescriptorIncrementSize
	);
	m_device->CreateShaderResourceView(
		m_dsLightBuffer.Get(),
		&srvLightDepthTextDesc,
		lightCPUDescriptorHandle
	);
}

void Engine::CreatePipelineStateObject()
{
	// input layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements = _countof(inputLayout);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	// shaders
	D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
	vertexShaderBytecode.BytecodeLength = m_vertexShader->GetBufferSize();
	vertexShaderBytecode.pShaderBytecode = m_vertexShader->GetBufferPointer();

	D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
	pixelShaderBytecode.BytecodeLength = m_pixelShader->GetBufferSize();
	pixelShaderBytecode.pShaderBytecode = m_pixelShader->GetBufferPointer();

	// sample desc
	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = vertexShaderBytecode;
	psoDesc.PS = pixelShaderBytecode;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc = sampleDesc;
	psoDesc.SampleMask = 0xffffffff;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.NumRenderTargets = 1;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	HRESULT hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
	if (FAILED(hr))
	{
		exit(-1);
	}
}

void Engine::CreateLightPso()
{
	// input layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements = _countof(inputLayout);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	// shaders
	D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
	vertexShaderBytecode.BytecodeLength = m_lightVertexShader->GetBufferSize();
	vertexShaderBytecode.pShaderBytecode = m_lightVertexShader->GetBufferPointer();

	D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
	pixelShaderBytecode.BytecodeLength = m_lightPixelShader->GetBufferSize();
	pixelShaderBytecode.pShaderBytecode = m_lightPixelShader->GetBufferPointer();

	// sample desc
	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.pRootSignature = m_lightRootSignature.Get();
	psoDesc.VS = vertexShaderBytecode;
	psoDesc.PS = pixelShaderBytecode;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc = sampleDesc;
	psoDesc.SampleMask = 0xffffffff;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.NumRenderTargets = 1;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	HRESULT hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_lightPipelineState));
	if (FAILED(hr))
	{
		exit(-1);
	}
}

void Engine::CreateVertexBuffer()
{
	m_actor.LoadObjFromFile(TEXT("Assets\\model.obj"));

	std::vector<WaveFrontReader<DWORD>::Vertex>& verticles = m_actor.GetVerticles();
	int vBufferSize = verticles.size() * sizeof(WaveFrontReader<DWORD>::Vertex);

	// create default heap - memory on GPU. Only GPU has access to it.
	HRESULT hr = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vBufferSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer)
	);
	if (FAILED(hr))
	{
		exit(-1);
	}

	m_vertexBuffer->SetName(L"Vertex Buffer Resource Type");
	
	// create upload heap - cpu can write to it, gpu can read from it
	hr = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vBufferUploadHeap)
	);
	if (FAILED(hr))
	{
		exit(-1);
	}

	m_vBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = reinterpret_cast<BYTE*>(&verticles[0]);
	vertexData.RowPitch = verticles.size();
	vertexData.SlicePitch = verticles.size();

	// copy from upload heap to default heap
	UpdateSubresources(m_commandList.Get(), m_vertexBuffer.Get(), m_vBufferUploadHeap.Get(), 0, 0, 1, &vertexData);
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	// index buffer
	UINT iBufferSize = m_actor.GetIndices().size() * sizeof(DWORD);

	// create deafult heap
	hr = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(iBufferSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_indexBuffer)
	);
	if (FAILED(hr))
	{
		exit(-1);
	}
	m_indexBuffer->SetName(L"Index buffer default heap");

	// create upload heap
	hr = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(iBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_iBufferUploadHeap)
	);
	if (FAILED(hr))
	{
		exit(-1);
	}

	// store index data in upload heap
	D3D12_SUBRESOURCE_DATA indexData = {};
	indexData.pData = reinterpret_cast<BYTE*>(&m_actor.GetIndices()[0]);
	indexData.RowPitch = iBufferSize;
	indexData.SlicePitch = iBufferSize;

	UpdateSubresources(m_commandList.Get(), m_indexBuffer.Get(), m_iBufferUploadHeap.Get(), 0, 0, 1, &indexData);

	m_commandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
	);

	// create depth/stencil descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsDescriptorHeap));
	if (FAILED(hr))
	{
		exit(-1);
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	hr = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_resolutionWidth, m_resolutionHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&m_dsBuffer)
	);
	if (FAILED(hr))
	{
		exit(-1);
	}

	m_dsBuffer->SetName(TEXT("DS Buffer"));

	m_dsDescriptorHeap->SetName(L"Depth Stencil Resource Heap");

	m_device->CreateDepthStencilView(m_dsBuffer.Get(), &depthStencilDesc, m_dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// execute command list to upload initial assets
	m_commandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_lightCommandList.Get(), m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	m_fenceValue++;
	hr = m_commandQueue->Signal(m_fence.Get(), m_fenceValue);
	if (FAILED(hr))
	{
		exit(-1);
	}

	// create vertex buffer view
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(WaveFrontReader<DWORD>::Vertex);
	m_vertexBufferView.SizeInBytes = vBufferSize;

	// create index buffer view
	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_indexBufferView.SizeInBytes = iBufferSize;
}

void Engine::CreateLightDepthBuffer()
{
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	HRESULT hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsLightDescriptorHeap));
	if (FAILED(hr))
	{
		exit(-1);
	}

	m_dsLightDescriptorHeap->SetName(L"Depth Stencil Light Descriptor Heap");

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	hr = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_shadowMapRes, m_shadowMapRes, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&m_dsLightBuffer)
	);
	if (FAILED(hr))
	{
		exit(-1);
	}

	m_dsLightBuffer->SetName(TEXT("Light DS buffer"));

	m_device->CreateDepthStencilView(m_dsLightBuffer.Get(), &depthStencilDesc, m_dsLightDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void Engine::FillOutViewportAndScissorRect()
{
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	m_viewport.Width = static_cast<float>(m_resolutionWidth);
	m_viewport.Height = static_cast<float>(m_resolutionHeight);
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	m_lightViewport.TopLeftX = 0;
	m_lightViewport.TopLeftY = 0;
	m_lightViewport.Width = static_cast<float>(m_shadowMapRes);
	m_lightViewport.Height = static_cast<float>(m_shadowMapRes);
	m_lightViewport.MinDepth = 0.0f;
	m_lightViewport.MaxDepth = 1.0f;

	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = m_resolutionWidth;
	m_scissorRect.bottom = m_resolutionHeight;

	m_lightDepthScissorRect.left = 0;
	m_lightDepthScissorRect.top = 0;
	m_lightDepthScissorRect.right = m_shadowMapRes;
	m_lightDepthScissorRect.bottom = m_shadowMapRes;
}

void Engine::CreateConstantBuffers()
{
	HRESULT hr;
	// descriptor heap
	for (int i = 0; i < 2; ++i)
	{
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
		descriptorHeapDesc.NumDescriptors = 2;
		descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		hr = m_device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_cbDescriptorHeap[i]));
		if (FAILED(hr))
		{
			exit(-1);
		}
	}

	// resource heap
	for (int i = 0; i < 2; ++i)
	{
		// WVP matrix

		hr = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(1024 * 64),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_cbWvpUploadHeap[i]));

		if (FAILED(hr))
		{
			exit(-1);
		}

		m_cbWvpUploadHeap[i]->SetName(L"Constant buffer WVP upload heap");

		// WVP
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbWvpDesc;
		cbWvpDesc.BufferLocation = m_cbWvpUploadHeap[i]->GetGPUVirtualAddress();
		cbWvpDesc.SizeInBytes = (sizeof(Wvp) + 255) & ~255;

		m_device->CreateConstantBufferView(&cbWvpDesc, m_cbDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart());
		CD3DX12_RANGE readRange(0, 0);
		m_cbWvpUploadHeap[i]->Map(0, &readRange, reinterpret_cast<void**>(&m_cbWvpGpuAddress[i]));

		memcpy(m_cbWvpGpuAddress[i], &m_wvpData, sizeof(Wvp));
	}
}

void Engine::CreateSamplers()
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.NumDescriptors = 1;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

	m_device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_lightSamplerDescriptorHeap));

	D3D12_SAMPLER_DESC lightSamplerDesc = {};
	lightSamplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	lightSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	lightSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	lightSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	lightSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
	lightSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

	m_device->CreateSampler(&lightSamplerDesc, m_lightSamplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void Engine::InitWvp()
{
	// model
	const XMFLOAT3 modelScale = XMFLOAT3(100.0f, 100.0f, 100.0f);
	m_actor.SetScale(&modelScale);

	// view
	const XMFLOAT3 cameraPosition = XMFLOAT3(0.0f, 0.0f, -150.0f);
	m_camera.SetTranslation(&cameraPosition);

	// projection
	const float fov = 60.0f * (XM_PI / 180.0f);
	m_camera.SetFov(fov);

	// light direction
	const XMFLOAT3 lightRotation(XM_PIDIV4, XM_PIDIV4, 0.0f);
	m_light.SetRotation(&lightRotation);
	m_light.SetProperties(60.0f * (XM_PI / 180.0f), 1000.0f);

	const XMFLOAT3 lightTranslation(-80.0f, 120.0f, -80.0f);
	m_light.SetTranslation(&lightTranslation);

	const float aspectRatio = static_cast<float>(m_resolutionWidth) / static_cast<float>(m_resolutionHeight);
	m_camera.SetAspectRatio(aspectRatio);

	// World-View-Projection matrix
	XMMATRIX worldTransposedMat = XMMatrixTranspose(m_actor.GetWorldMat());
	XMMATRIX wvpMat = m_actor.GetWorldMat() * m_camera.GetViewProjectionMat();
	XMMATRIX wvpTransposedMat = XMMatrixTranspose(wvpMat);

	XMMATRIX lightWvpMat = m_actor.GetWorldMat() * m_light.GetViewProjectionMat();
	XMMATRIX lightWvpTransposedMat = XMMatrixTranspose(lightWvpMat);

	XMStoreFloat4x4(&m_wvpData.world, worldTransposedMat);
	XMStoreFloat4x4(&m_wvpData.wvp, wvpTransposedMat);
	XMStoreFloat3(&m_wvpData.cameraPos, m_camera.GetPosition());
	XMStoreFloat4x4(&m_wvpData.lightWvp, lightWvpTransposedMat);
	XMStoreFloat3(&m_wvpData.lightWorldPos, m_light.GetTranslation());
	XMStoreFloat3(&m_wvpData.lightDirection, m_light.GetDirectionVec());
	m_wvpData.lightFov = m_light.GetFov();
}

void Engine::UpdateWvp(float deltaSec)
{
	const float movementSpeed = 50.0f;
	const float rotationSpeed = 0.005f;
	const SHORT keyStatePressedFlag = 0x800;
	bool viewHasChanged = false;

	if (GetKeyState(((int) 'W')) & keyStatePressedFlag)
	{
		m_camera.MoveForward(movementSpeed * deltaSec);
	}

	if (GetKeyState(((int) 'S')) & keyStatePressedFlag)
	{
		m_camera.MoveForward(-movementSpeed * deltaSec);
	}

	if (GetKeyState(((int) 'A')) & keyStatePressedFlag)
	{
		m_camera.MoveRight(-movementSpeed * deltaSec);
	}

	if (GetKeyState(((int) 'D')) & keyStatePressedFlag)
	{
		m_camera.MoveRight(movementSpeed * deltaSec);
	}

	if (GetKeyState(((int) 'Q')) & keyStatePressedFlag)
	{
		m_actor.RotateRoll(-deltaSec);
	}

	if (GetKeyState(((int) 'E')) & keyStatePressedFlag)
	{
		m_actor.RotateRoll(deltaSec);
	}

	if (GetKeyState(((int) 'Z')) & keyStatePressedFlag)
	{
		m_actor.RotateYaw(-deltaSec);
	}

	if (GetKeyState(((int) 'C')) & keyStatePressedFlag)
	{
		m_actor.RotateYaw(deltaSec);
	}

	if (m_mouseDeltaX != 0.0f || m_mouseDeltaY != 0.0f)
	{
		m_camera.RotateYaw(-rotationSpeed * m_mouseDeltaX);
		m_camera.RotatePitch(-rotationSpeed * m_mouseDeltaY);
	}

	//m_actor.RotateRoll(deltaSec);

	XMMATRIX worldTransposedMat = XMMatrixTranspose(m_actor.GetWorldMat());
	XMMATRIX wvpMat = m_actor.GetWorldMat() * m_camera.GetViewProjectionMat();
	XMMATRIX wvpMatTransposed = XMMatrixTranspose(wvpMat);

	XMMATRIX lightWvpMat = m_actor.GetWorldMat() * m_light.GetViewProjectionMat();
	XMMATRIX lightWvpTransposedMat = XMMatrixTranspose(lightWvpMat);

	XMStoreFloat4x4(&m_wvpData.world, worldTransposedMat);
	XMStoreFloat4x4(&m_wvpData.wvp, wvpMatTransposed);
	XMStoreFloat3(&m_wvpData.cameraPos, m_camera.GetPosition());
	XMStoreFloat4x4(&m_wvpData.lightWvp, lightWvpTransposedMat);
	XMStoreFloat3(&m_wvpData.lightWorldPos, m_light.GetTranslation());
	XMStoreFloat3(&m_wvpData.lightDirection, m_light.GetDirectionVec());
	m_wvpData.lightFov = m_light.GetFov();
}

void Engine::Init(HWND hwnd)
{
	m_hwnd = hwnd;

	// debug
#if defined(_DEBUG)
	UINT dxgiFactoryFlags = 0;

	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	// dx factory
	ComPtr<IDXGIFactory4> factory;
	if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory))))
	{
		exit(-1);
	}


	// find graphic card

	ComPtr<IDXGIAdapter1> hardwareAdapter;
	GetHardwareAdapter(factory.Get(), &hardwareAdapter);

	// create device
	if (FAILED(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device))))
	{
		exit(-1);
	}

	// create command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	
	if (FAILED(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue))))
	{
		exit(-1);
	}

	// create swap chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Height = m_resolutionHeight;
	swapChainDesc.Width = m_resolutionWidth;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;	// double buffering
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	ComPtr<IDXGISwapChain1> swapChain;
	if (FAILED(factory->CreateSwapChainForHwnd(m_commandQueue.Get(), m_hwnd, &swapChainDesc, nullptr, nullptr, &swapChain)))
	{
		exit(-1);
	}

	if (FAILED(factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER)))
	{
		exit(-1);
	}

	swapChain.As(&m_swapChain);
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// create swap chain for light
	DXGI_SWAP_CHAIN_DESC1 lightSwapChainDesc = {};
	lightSwapChainDesc.Height = m_shadowMapRes;
	lightSwapChainDesc.Width = m_shadowMapRes;
	lightSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	lightSwapChainDesc.Stereo = FALSE;
	lightSwapChainDesc.SampleDesc.Count = 1;
	lightSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	lightSwapChainDesc.BufferCount = 2;	// double buffering
	lightSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	

	// create descriptor heaps
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = 2;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		if (FAILED(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap))))
		{
			exit(-1);
		}

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// create frame resources
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		for (UINT i = 0; i < 2; ++i)
		{
			if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTarget[i]))))
			{
				exit(-1);
			}

			m_device->CreateRenderTargetView(m_renderTarget[i].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

	// create command allocator
	HRESULT hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));
	if (FAILED(hr))
	{
		exit(-1);
	}

	// create command list
	hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList));
	if (FAILED(hr))
	{
		exit(-1);
	}

	// create light command allocator
	hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_lightCommandAllocator));
	if (FAILED(hr))
	{
		exit(-1);
	}

	// create light command list
	hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_lightCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_lightCommandList));
	if (FAILED(hr))
	{
		exit(-1);
	}

	m_lightCommandList->Close();

	// create fence
	hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
	if (FAILED(hr))
	{
		exit(-1);
	}

	m_fenceValue = 1;
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr)
	{
		exit(-1);
	}

	CreateRootSignature();
	CreateLightRootSignature();
	LoadShaders();
	CreatePipelineStateObject();
	CreateLightPso();
	CreateLightDepthBuffer();
	LoadTextures();
	InitWvp();
	CreateConstantBuffers();
	CreateVertexBuffer();
	CreateSamplers();
	FillOutViewportAndScissorRect();

	WaitForPreviousFrame();

	m_prevTime = high_resolution_clock::now();
}

void Engine::Input(int mouseX, int mouseY, bool rightMouseBtnIsDown)
{
	if (rightMouseBtnIsDown)
	{
		m_mouseDeltaX += (static_cast<float>(mouseX) - m_mouseX);
		m_mouseDeltaY += (static_cast<float>(mouseY) - m_mouseY);
	}

	m_mouseX = static_cast<float>(mouseX);
	m_mouseY = static_cast<float>(mouseY);
}

void Engine::Update()
{
	high_resolution_clock::time_point now = high_resolution_clock::now();
	float deltaSec = duration<float>(now - m_prevTime).count();
	m_prevTime = now;

	// WVP matrix
	UpdateWvp(deltaSec);

	memcpy(m_cbWvpGpuAddress[m_frameIndex], &m_wvpData, sizeof(Wvp));

	m_mouseDeltaX = 0.0f;
	m_mouseDeltaY = 0.0f;
}

void Engine::ResizeViewport(UINT resolutionWidth, UINT resolutionHeight)
{
	m_resolutionWidth = resolutionWidth;
	m_resolutionHeight = resolutionHeight;
	float aspectRatio = static_cast<float>(resolutionWidth) / static_cast<float>(resolutionHeight);
	m_camera.SetAspectRatio(aspectRatio);
}

void Engine::Render()
{
	RenderLightDepth();
	RenderScene();

	// execute command list
	ID3D12CommandList* ppCommandLists[]{ m_lightCommandList.Get(), m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// present the frame
	HRESULT hr = m_swapChain->Present(1, 0);
	if (FAILED(hr))
	{
		exit(-1);
	}

	WaitForPreviousFrame();
}

void Engine::RenderScene()
{
	HRESULT hr = m_commandAllocator->Reset();
	if (FAILED(hr))
	{
		exit(-1);
	}

	hr = m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get());
	if (FAILED(hr))
	{
		exit(-1);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// record commands
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);

	// indicate that the back buffer will be used as a render target
	m_commandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_renderTarget[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	const float clearColor[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->ClearDepthStencilView(m_dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// draw triangle
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	// constant buffer descriptor heap
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_textureDescriptorHeap.Get(), m_lightSamplerDescriptorHeap.Get() };
	m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	m_commandList->SetGraphicsRootConstantBufferView(0, m_cbWvpUploadHeap[m_frameIndex]->GetGPUVirtualAddress());
	m_commandList->SetGraphicsRootDescriptorTable(1, m_textureDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	m_commandList->SetGraphicsRootDescriptorTable(2, m_lightSamplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->IASetIndexBuffer(&m_indexBufferView);
	m_commandList->DrawIndexedInstanced(m_actor.GetIndices().size(), 1, 0, 0, 0);

	// indicate that the back buffer will be used to present
	m_commandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(m_renderTarget[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	hr = m_commandList->Close();
	if (FAILED(hr))
	{
		exit(-1);
	}
}

void Engine::RenderLightDepth()
{
	HRESULT hr = m_lightCommandAllocator->Reset();
	if (FAILED(hr))
	{
		exit(-1);
	}

	hr = m_lightCommandList->Reset(m_lightCommandAllocator.Get(), m_lightPipelineState.Get());
	if (FAILED(hr))
	{
		exit(-1);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE lightDsvHandle(m_dsLightDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// record commands
	m_lightCommandList->OMSetRenderTargets(0, nullptr, false, &lightDsvHandle);
	m_lightCommandList->ClearDepthStencilView(m_dsLightDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	m_lightCommandList->SetGraphicsRootSignature(m_lightRootSignature.Get());

	// constant buffer descriptor heap

	m_lightCommandList->SetGraphicsRootConstantBufferView(0, m_cbWvpUploadHeap[m_frameIndex]->GetGPUVirtualAddress());

	m_lightCommandList->RSSetViewports(1, &m_lightViewport);
	m_lightCommandList->RSSetScissorRects(1, &m_lightDepthScissorRect);
	m_lightCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_lightCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_lightCommandList->IASetIndexBuffer(&m_indexBufferView);
	m_lightCommandList->DrawIndexedInstanced(m_actor.GetIndices().size(), 1, 0, 0, 0);


	hr = m_lightCommandList->Close();
	if (FAILED(hr))
	{
		exit(-1);
	}
}

void Engine::Destroy()
{
	CloseHandle(m_fenceEvent);
	m_actor.ReleaseObj();
}

ComPtr<ID3D12Device> Engine::GetDevice() const
{
	return m_device;
}

ComPtr<ID3D12GraphicsCommandList> Engine::GetCommandList() const
{
	return m_commandList;
}

