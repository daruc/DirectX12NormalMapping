#include "Texture.h"
#include <wrl.h>
#include <wincodec.h>
#include "Engine.h"
#include <wchar.h>

Texture::Texture(Engine * const engine)
{
	m_engine = engine;
}

void Texture::LoadFromFile(const wchar_t * const fileName)
{
	using namespace Microsoft::WRL;

	m_textureDesc = {};
	m_textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	m_textureDesc.Alignment = 0;
	m_textureDesc.DepthOrArraySize = 1;
	m_textureDesc.MipLevels = 1;
	m_textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	m_textureDesc.SampleDesc.Count = 1;
	m_textureDesc.SampleDesc.Quality = 0;
	m_textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	m_textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	CoInitialize(nullptr);
	ComPtr<IWICImagingFactory> imagingFactory = nullptr;

	HRESULT hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&imagingFactory)
	);

	if (FAILED(hr))
	{
		exit(-1);
	}

	IWICBitmapDecoder* bitmapDecoder = nullptr;	// TODO: release

	hr = imagingFactory->CreateDecoderFromFilename(
		fileName,
		nullptr,
		GENERIC_READ,
		WICDecodeMetadataCacheOnDemand,
		&bitmapDecoder
	);

	if (FAILED(hr))
	{
		exit(-1);
	}

	IWICBitmapFrameDecode* frame = nullptr;	// TODO: release

	hr = bitmapDecoder->GetFrame(0, &frame);

	if (FAILED(hr))
	{
		exit(-1);
	}

	UINT textureWidth, textureHeight;
	hr = frame->GetSize(&textureWidth, &textureHeight);
	if (FAILED(hr))
	{
		exit(-1);
	}

	m_textureDesc.Width = textureWidth;
	m_textureDesc.Height = textureHeight;

	const UINT bytesPerPixel = 4;

	UINT textureSize = textureWidth * textureHeight * bytesPerPixel;
	if (textureSize == 0)
	{
		exit(-1);
	}

	WICPixelFormatGUID pixelFormat;
	hr = frame->GetPixelFormat(&pixelFormat);
	if (FAILED(hr))
	{
		exit(-1);
	}

	m_data = std::make_unique<BYTE[]>(textureSize);

	UINT bytesPerRow = textureWidth * bytesPerPixel;
	hr = frame->CopyPixels(nullptr, bytesPerRow, textureSize, m_data.get());

	if (FAILED(hr))
	{
		exit(-1);
	}
}

void Texture::CreateResource(const wchar_t * const textureName, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle)
{
	//deafult heap

	HRESULT hr = m_engine->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&m_textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_textureDefaultHeap)
	);

	if (FAILED(hr))
	{
		exit(-1);
	}

	wstring defaultHeapName(textureName);
	defaultHeapName += L" - DefaultHeap";
	m_textureDefaultHeap->SetName(defaultHeapName.c_str());

	// upload heap

	UINT64 textureBufferUploadSize;
	m_engine->GetDevice()->GetCopyableFootprints(&m_textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureBufferUploadSize);

	hr = m_engine->GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(textureBufferUploadSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_textureUploadHeap)
	);

	if (FAILED(hr))
	{
		exit(-1);
	}

	wstring uploadHeapName(textureName);
	uploadHeapName += L" - Upload Heap";
	m_textureUploadHeap->SetName(uploadHeapName.c_str());

	// SRV descriptor
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	m_engine->GetDevice()->CreateShaderResourceView(
		m_textureDefaultHeap.Get(),
		&srvDesc,
		cpuDescriptorHandle
	);
}

void Texture::UploadToResource()
{
	D3D12_SUBRESOURCE_DATA textureSubresource = {};
	textureSubresource.pData = m_data.get();
	textureSubresource.RowPitch = 4 * m_textureDesc.Width;
	textureSubresource.SlicePitch = textureSubresource.RowPitch * m_textureDesc.Height;

	UpdateSubresources(m_engine->GetCommandList().Get(), 
		m_textureDefaultHeap.Get(), m_textureUploadHeap.Get(), 0, 0, 1, &textureSubresource);

	m_engine->GetCommandList()->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			m_textureDefaultHeap.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		)
	);
}

void Texture::Release()
{
	m_data.release();
}

UINT Texture::GetWidth() const
{
	return m_textureDesc.Width;
}

UINT Texture::GetHeight() const
{
	return m_textureDesc.Height;
}

BYTE* Texture::GetData() const
{
	return m_data.get();
}
