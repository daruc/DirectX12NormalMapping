#include "Actor.h"
#include <comdef.h>
#include <wrl.h>
#include <wincodec.h>
#include "Engine.h"


void Actor::UpdateTransformationMat()
{
	m_worldMat = XMMatrixScalingFromVector(m_scaleVec) *
		XMMatrixRotationRollPitchYawFromVector(m_rotationVec) *
		XMMatrixTranslationFromVector(m_translationVec);
}

Actor::Actor(Engine* const engine)
	: m_albedoTex(engine),
	m_normalTex(engine)
{
	m_engine = engine;

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
	m_albedoTex.LoadFromFile(fileName);
}

void Actor::LoadNormalFromFile(const wchar_t* const fileName)
{
	m_normalTex.LoadFromFile(fileName);
}

void Actor::UploadAlbedoResource(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle)
{
	m_albedoTex.CreateResource(L"Albedo", cpuDescriptorHandle);
	m_albedoTex.UploadToResource();
}

void Actor::UploadNormalResource(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle)
{
	m_normalTex.CreateResource(L"Normal", cpuDescriptorHandle);
	m_normalTex.UploadToResource();
}

void Actor::ReleaseAlbedo()
{
	m_albedoTex.Release();
}

void Actor::ReleaseNormal()
{
	m_normalTex.Release();
}

Texture& Actor::GetAlbedo()
{
	return m_albedoTex;
}

Texture& Actor::GetNormal()
{
	return m_normalTex;
}
