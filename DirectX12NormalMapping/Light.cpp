#include "Light.h"
#include "Engine.h"


void Light::UpdateViewProjectionMat()
{
	XMMATRIX rotationMat = XMMatrixRotationRollPitchYawFromVector(m_rotationVec);
	XMMATRIX translationMat = XMMatrixTranslationFromVector(m_translationVec);
	XMMATRIX viewMat = XMMatrixInverse(nullptr, rotationMat * translationMat);
	const float aspectRatio = 1.0f;
	XMMATRIX projectionMat = XMMatrixPerspectiveFovLH(m_fov, aspectRatio, 0.1f, m_range);
	m_viewProjectionMat = viewMat * projectionMat;
}

Light::Light()
{
	m_rotationVec = XMVectorZero();
	m_directionVec = Z_UNIT_VEC;
	m_range = 1000.0f;
	m_fov = XM_PIDIV4;

	UpdateViewProjectionMat();
}

void Light::SetProperties(float fov, float range)
{
	m_fov = fov;
	m_range = range;

	UpdateViewProjectionMat();
}

void Light::SetTranslation(const XMFLOAT3 * const translationVec)
{
	m_translationVec = XMLoadFloat3(translationVec);

	UpdateViewProjectionMat();
}

XMVECTOR Light::GetTranslation() const
{
	return m_translationVec;
}

void Light::SetRotation(const XMFLOAT3 * const rotationVec)
{
	m_rotationVec = XMLoadFloat3(rotationVec);
	XMMATRIX rotationMat = XMMatrixRotationRollPitchYawFromVector(m_rotationVec);
	m_directionVec = XMVector3Transform(Z_UNIT_VEC, rotationMat);

	UpdateViewProjectionMat();
}

XMMATRIX Light::GetViewProjectionMat() const
{
	return m_viewProjectionMat;
}

float Light::GetFov() const
{
	return m_fov;
}

XMVECTOR Light::GetDirectionVec() const
{
	return m_directionVec;
}
