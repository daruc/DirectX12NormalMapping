#pragma once

#include <DirectXMath.h>

using namespace DirectX;

class Light
{

private:
	XMVECTOR m_translationVec;
	XMVECTOR m_rotationVec;
	XMVECTOR m_directionVec;
	float m_fov;
	float m_range;
	XMMATRIX m_viewProjectionMat;

	void UpdateViewProjectionMat();

public:
	Light();
	void SetProperties(float fov, float range);
	void SetTranslation(const XMFLOAT3* const translationVec);
	void SetRotation(const XMFLOAT3* const rotationVec);
	XMVECTOR GetTranslation() const;
	XMVECTOR GetDirectionVec() const;
	XMMATRIX GetViewProjectionMat() const;
	float GetFov() const;
};

