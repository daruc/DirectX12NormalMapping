#include "Camera.h"
#include "Engine.h"

void Camera::UpdateViewMat()
{
	XMMATRIX rotationMat = XMMatrixRotationRollPitchYawFromVector(m_rotationVec);
	XMMATRIX translationMat = XMMatrixTranslationFromVector(m_translationVec);
	m_viewMat = rotationMat * translationMat;
}

void Camera::UpdateProjectionMat()
{
	m_projectionMat = XMMatrixPerspectiveFovLH(m_fov, m_aspectRatio, m_NEAR_Z, m_FAR_Z);
}

XMVECTOR Camera::CalculateForwardVec()
{
	XMMATRIX rotationMat = XMMatrixRotationRollPitchYawFromVector(m_rotationVec);
	return XMVector4Transform(Z_UNIT_VEC, rotationMat);
}

XMVECTOR Camera::CalculateRightVec()
{
	XMMATRIX rotationMat = XMMatrixRotationRollPitchYawFromVector(m_rotationVec);
	return XMVector4Transform(X_UNIT_VEC, rotationMat);
}

XMVECTOR Camera::CalculateUpVec()
{
	XMMATRIX rotationMat = XMMatrixRotationRollPitchYawFromVector(m_rotationVec);
	return XMVector4Transform(Y_UNIT_VEC, rotationMat);
}

void Camera::SetTranslation(const XMFLOAT3 * const position)
{
	m_translationVec = XMLoadFloat3(position);
	UpdateViewMat();
}

void Camera::SetRotation(const XMFLOAT3 * const rotation)
{
	m_rotationVec = XMLoadFloat3(rotation);
	UpdateViewMat();
}

void Camera::SetFov(float fov)
{
	m_fov = fov;
	UpdateProjectionMat();
}

void Camera::SetAspectRatio(float aspectRatio)
{
	m_aspectRatio = aspectRatio;
	UpdateProjectionMat();
}

void Camera::RotatePitch(float radians)
{
	if (radians == 0.0f)
	{
		return;
	}

	float previousPitch = XMVectorGetX(m_rotationVec);
	float newPitch = previousPitch - radians;

	if (newPitch > XM_PIDIV2)
	{
		newPitch = XM_PIDIV2;
	}
	else if (newPitch < -XM_PIDIV2)
	{
		newPitch = -XM_PIDIV2;
	}

	m_rotationVec = XMVectorSetX(m_rotationVec, newPitch);
	UpdateViewMat();
}

void Camera::RotateYaw(float radians)
{
	if (radians == 0.0f)
	{
		return;
	}

	float previousYaw = XMVectorGetY(m_rotationVec);
	float newYaw = previousYaw - radians;

	if (newYaw > XM_2PI)
	{
		newYaw -= XM_2PI;
	}
	else if (newYaw < -XM_2PI)
	{
		newYaw += XM_2PI;
	}

	m_rotationVec = XMVectorSetY(m_rotationVec, newYaw);
	UpdateViewMat();
}

void Camera::MoveForward(float units)
{
	XMVECTOR translationVec = CalculateForwardVec() * units;
	m_translationVec += translationVec;
	UpdateViewMat();
}

void Camera::MoveRight(float units)
{
	XMVECTOR translationVec = CalculateRightVec() * units;
	m_translationVec += translationVec;
	UpdateViewMat();
}

void Camera::MoveUp(float units)
{
	XMVECTOR translationVec = CalculateUpVec() * units;
	m_translationVec += translationVec;
	UpdateViewMat();
}

XMVECTOR Camera::GetPosition() const
{
	return m_translationVec;
}

Camera::Camera()
	: m_translationVec(XMVectorZero()),
	m_rotationVec(XMVectorZero()),

	m_NEAR_Z(0.1f),
	m_FAR_Z(1000.0f),

	m_fov(90.0f),
	m_aspectRatio(1.0f)
{
}

XMMATRIX Camera::GetViewProjectionMat() const
{
	return XMMatrixInverse(nullptr, m_viewMat) * m_projectionMat;
}
