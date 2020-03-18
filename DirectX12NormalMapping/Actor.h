#pragma once

#define NOMINMAX

#include <d3d12.h>
#include <DirectXMath.h>
#include <DirectXMesh.h>
#include <WaveFrontReader.h>

using namespace DirectX;

class Actor
{
private:
	XMVECTOR m_scaleVec;
	XMVECTOR m_rotationVec;
	XMVECTOR m_translationVec;

	XMMATRIX m_worldMat;

	WaveFrontReader<DWORD> waveFrontReader;

	void UpdateTransformationMat();

public:
	Actor();
	void SetScale(const XMFLOAT3* const scaleVec);
	void SetRotation(const XMFLOAT3* const rotationVec);
	void RotateRoll(float radians);
	void RotateYaw(float radians);
	void SetTranslation(const XMFLOAT3* const translationVec);
	XMMATRIX GetWorldMat() const;
	void LoadObjFromFile(const wchar_t* const fileName);
	void ClearObj();
	std::vector<WaveFrontReader<DWORD>::Vertex>& GetVerticles();
	std::vector<DWORD>& GetIndices();
};

