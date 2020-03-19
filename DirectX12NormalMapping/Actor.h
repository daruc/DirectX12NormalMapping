#pragma once

#define NOMINMAX

#include <d3d12.h>
#include <DirectXMath.h>
#include <DirectXMesh.h>
#include <WaveFrontReader.h>
#include <vector>

using namespace DirectX;
using namespace std;

struct Texture
{
	unique_ptr<BYTE[]> data;
	UINT width;
	UINT height;
};

class Actor
{
private:
	XMVECTOR m_scaleVec;
	XMVECTOR m_rotationVec;
	XMVECTOR m_translationVec;

	XMMATRIX m_worldMat;

	WaveFrontReader<DWORD> waveFrontReader;

	Texture m_albedoTex;
	Texture m_normalTex;

	void UpdateTransformationMat();
	Texture LoadTextureFromFile(const wchar_t* const fileName);

public:
	Actor();
	void SetScale(const XMFLOAT3* const scaleVec);
	void SetRotation(const XMFLOAT3* const rotationVec);
	void RotateRoll(float radians);
	void RotateYaw(float radians);
	void SetTranslation(const XMFLOAT3* const translationVec);
	XMMATRIX GetWorldMat() const;
	void LoadObjFromFile(const wchar_t* const fileName);
	void ReleaseObj();
	std::vector<WaveFrontReader<DWORD>::Vertex>& GetVerticles();
	std::vector<DWORD>& GetIndices();
	void LoadAlbedoFromFile(const wchar_t* const fileName);
	void LoadNormalFromFile(const wchar_t* const fileName);
	void ReleaseAlbedo();
	void ReleaseNormal();
	Texture& GetAlbedo();
	Texture& GetNormal();
};

