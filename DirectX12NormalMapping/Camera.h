#pragma once

#include <DirectXMath.h>

using namespace DirectX;

class Camera
{
private:
	const float m_NEAR_Z;
	const float m_FAR_Z;

	XMVECTOR m_translationVec;
	XMVECTOR m_rotationVec;
	XMMATRIX m_viewMat;

	float m_fov;
	float m_aspectRatio;
	XMMATRIX m_projectionMat;

	void UpdateViewMat();
	void UpdateProjectionMat();
	XMVECTOR CalculateForwardVec();
	XMVECTOR CalculateRightVec();
	XMVECTOR CalculateUpVec();

public:
	void SetTranslation(const XMFLOAT3* const position);
	void SetRotation(const XMFLOAT3* const rotation);
	void SetFov(float fov);
	void SetAspectRatio(float aspectRatio);
	void RotatePitch(float radians);
	void RotateYaw(float radians);
	void MoveForward(float units);
	void MoveRight(float units);
	void MoveUp(float units);
	XMVECTOR GetPosition() const;

	Camera();
	XMMATRIX GetViewProjectionMat() const;
};

