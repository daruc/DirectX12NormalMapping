#include "Actor.h"
#include <comdef.h>
#include <wrl.h>
#include <wincodec.h>


void Actor::UpdateTransformationMat()
{
	m_worldMat = XMMatrixScalingFromVector(m_scaleVec) *
		XMMatrixRotationRollPitchYawFromVector(m_rotationVec) *
		XMMatrixTranslationFromVector(m_translationVec);
}

Texture Actor::LoadTextureFromFile(const wchar_t* const fileName)
{
	using namespace Microsoft::WRL;

	Texture texture;

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

	hr = frame->GetSize(&texture.width, &texture.height);

	if (FAILED(hr))
	{
		exit(-1);
	}

	const UINT bytesPerPixel = 4;

	UINT textureSize = texture.width * texture.height * bytesPerPixel;
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

	texture.data = std::make_unique<BYTE[]>(textureSize);

	UINT bytesPerRow = texture.width * bytesPerPixel;
	hr = frame->CopyPixels(nullptr, bytesPerRow, textureSize, texture.data.get());

	if (FAILED(hr))
	{
		exit(-1);
	}

	return texture;
}

Actor::Actor()
{
	XMFLOAT3 scaleFloat3 = XMFLOAT3(1.0f, 1.0f, 1.0f);
	m_scaleVec = XMLoadFloat3(&scaleFloat3);

	m_rotationVec = XMVectorZero();

	m_translationVec = XMVectorZero();

	UpdateTransformationMat();
}

void Actor::SetScale(const XMFLOAT3* const scaleVec)
{
	m_scaleVec = XMLoadFloat3(scaleVec);
	UpdateTransformationMat();
}

void Actor::SetRotation(const XMFLOAT3* const rotationVec)
{
	m_rotationVec = XMLoadFloat3(rotationVec);
	UpdateTransformationMat();
}

void Actor::RotateRoll(float radians)
{
	float previousRoll = XMVectorGetZ(m_rotationVec);
	float newRoll = previousRoll + radians;
	m_rotationVec = XMVectorSetZ(m_rotationVec, newRoll);
	UpdateTransformationMat();
}

void Actor::RotateYaw(float radians)
{
	float previousYaw = XMVectorGetY(m_rotationVec);
	float newYaw = previousYaw + radians;
	m_rotationVec = XMVectorSetY(m_rotationVec, newYaw);
	UpdateTransformationMat();
}

void Actor::SetTranslation(const XMFLOAT3* const translationVec)
{
	m_translationVec = XMLoadFloat3(translationVec);
	UpdateTransformationMat();
}

XMMATRIX Actor::GetWorldMat() const
{
	return m_worldMat;
}

void Actor::LoadObjFromFile(const wchar_t* const fileName)
{
	HRESULT hr = waveFrontReader.Load(fileName);
	if (FAILED(hr))
	{
		exit(-1);
	}
}

void Actor::ReleaseObj()
{
	waveFrontReader.Clear();
}

std::vector<WaveFrontReader<DWORD>::Vertex>& Actor::GetVerticles()
{
	return waveFrontReader.vertices;
}

std::vector<DWORD>& Actor::GetIndices()
{
	return waveFrontReader.indices;
}

void Actor::LoadAlbedoFromFile(const wchar_t* const fileName)
{
	m_albedoTex = LoadTextureFromFile(fileName);
}

void Actor::LoadNormalFromFile(const wchar_t* const fileName)
{
	m_normalTex = LoadTextureFromFile(fileName);
}

void Actor::ReleaseAlbedo()
{
	m_albedoTex.data.release();
}

void Actor::ReleaseNormal()
{
	m_normalTex.data.release();
}

Texture& Actor::GetAlbedo()
{
	return m_albedoTex;
}

Texture& Actor::GetNormal()
{
	return m_normalTex;
}
