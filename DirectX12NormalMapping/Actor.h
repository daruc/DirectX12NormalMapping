#pragma once

#define NOMINMAX

#include <d3d12.h>
#include <DirectXMath.h>
#include <DirectXMesh.h>
#include <WaveFrontReader.h>
#include <vector>
#include "Texture.h"

using namespace DirectX;
using namespace std;

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT3 tangent;
	XMFLOAT2 textureCoordinate;
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
	std::vector<Vertex> m_verticesWithTangents;

	class Engine* m_engine;

	void UpdateTransformationMat();
	void CalculateTangents();

public:
	Actor(class Engine* const engine);
	void SetScale(const XMFLOAT3* const scaleVec);
	void SetRotation(const XMFLOAT3* const rotationVec);
	void RotateRoll(float radians);
	void RotateYaw(float radians);
	void SetTranslation(const XMFLOAT3* const translationVec);
	XMMATRIX GetWorldMat() const;
	void LoadObjFromFile(const wchar_t* const fileName);
	void ReleaseObj();
	vector<Vertex>& GetVertices();
	vector<DWORD>& GetIndices();
	void LoadAlbedoFromFile(const wchar_t* const fileName);
	void LoadNormalFromFile(const wchar_t* const fileName);
	void UploadAlbedoResource(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle);
	void UploadNormalResource(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle);
	void ReleaseAlbedo();
	void ReleaseNormal();
	Texture& GetAlbedo();
	Texture& GetNormal();
};

