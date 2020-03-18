#include "Actor.h"


void Actor::UpdateTransformationMat()
{
	m_worldMat = XMMatrixScalingFromVector(m_scaleVec) *
		XMMatrixRotationRollPitchYawFromVector(m_rotationVec) *
		XMMatrixTranslationFromVector(m_translationVec);
}

Actor::Actor()
{
	XMFLOAT3 scaleFloat3 = XMFLOAT3(1.0f, 1.0f, 1.0f);
	m_scaleVec = XMLoadFloat3(&scaleFloat3);

	m_rotationVec = XMVectorZero();

	m_translationVec = XMVectorZero();

	UpdateTransformationMat();
}

void Actor::SetScale(const XMFLOAT3 * const scaleVec)
{
	m_scaleVec = XMLoadFloat3(scaleVec);
	UpdateTransformationMat();
}

void Actor::SetRotation(const XMFLOAT3 * const rotationVec)
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

void Actor::SetTranslation(const XMFLOAT3 * const translationVec)
{
	m_translationVec = XMLoadFloat3(translationVec);
	UpdateTransformationMat();
}

XMMATRIX Actor::GetWorldMat() const
{
	return m_worldMat;
}

void Actor::LoadObjFromFile(const wchar_t * const fileName)
{
	HRESULT hr = waveFrontReader.Load(fileName);
	if (FAILED(hr))
	{
		exit(-1);
	}
}

void Actor::ClearObj()
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
