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

void Actor::CalculateTangents()
{
	typedef WaveFrontReader<DWORD>::Vertex ReaderVertex;

	vector<DWORD>& indices = waveFrontReader.indices;
	vector<ReaderVertex>& vertices = waveFrontReader.vertices;

	for (DWORD indexOfIndex = 0; indexOfIndex < indices.size(); indexOfIndex += 3)
	{
		DWORD vertexIndex0 = indices[indexOfIndex];
		ReaderVertex& readerVertex0 = vertices[vertexIndex0];
		XMVECTOR vertexPos0 = XMLoadFloat3(&readerVertex0.position);
		XMVECTOR texCoord0 = XMLoadFloat2(&readerVertex0.textureCoordinate);

		DWORD vertexIndex1 = indices[indexOfIndex + 1];
		ReaderVertex& readerVertex1 = vertices[vertexIndex1];
		XMVECTOR vertexPos1 = XMLoadFloat3(&readerVertex1.position);
		XMVECTOR texCoord1 = XMLoadFloat2(&readerVertex1.textureCoordinate);

		DWORD vertexIndex2 = indices[indexOfIndex + 2];
		ReaderVertex& readerVertex2 = vertices[vertexIndex2];
		XMVECTOR vertexPos2 = XMLoadFloat3(&readerVertex2.position);
		XMVECTOR texCoord2 = XMLoadFloat2(&readerVertex2.textureCoordinate);

		XMVECTOR deltaPos0Vec = vertexPos1 - vertexPos0;
		deltaPos0Vec = XMVector3Normalize(deltaPos0Vec);
		XMFLOAT3 deltaPos0;
		XMStoreFloat3(&deltaPos0, deltaPos0Vec);

		XMVECTOR deltaPos1Vec = vertexPos2 - vertexPos0;
		deltaPos1Vec = XMVector3Normalize(deltaPos1Vec);
		XMFLOAT3 deltaPos1;
		XMStoreFloat3(&deltaPos1, deltaPos1Vec);

		XMVECTOR deltaTexCoordVec0 = texCoord1 - texCoord0;
		deltaTexCoordVec0 = XMVector2Normalize(deltaTexCoordVec0);
		XMFLOAT2 deltaTexCoord0;
		XMStoreFloat2(&deltaTexCoord0, deltaTexCoordVec0);

		XMVECTOR deltaTexCoord1Vec = texCoord2 - texCoord0;
		deltaTexCoord1Vec = XMVector2Normalize(deltaTexCoord1Vec);
		XMFLOAT2 deltaTexCoord1;
		XMStoreFloat2(&deltaTexCoord1, deltaTexCoord1Vec);

		float det = (deltaTexCoord0.x * deltaTexCoord1.y - deltaTexCoord0.y * deltaTexCoord1.x);

		XMVECTOR tangent = (deltaPos0Vec * deltaTexCoord1.y - deltaPos1Vec * deltaTexCoord0.y) / det;
		tangent = XMVector3Normalize(tangent);
		XMVECTOR bitangent = (deltaPos1Vec * deltaTexCoord0.x - deltaPos0Vec * deltaTexCoord1.x) / det;
		bitangent = XMVector3Normalize(bitangent);

		XMStoreFloat3(&m_verticesWithTangents[vertexIndex0].tanget, tangent);
		XMStoreFloat3(&m_verticesWithTangents[vertexIndex0].bitangent, bitangent);

		XMStoreFloat3(&m_verticesWithTangents[vertexIndex1].tanget, tangent);
		XMStoreFloat3(&m_verticesWithTangents[vertexIndex1].bitangent, bitangent);

		XMStoreFloat3(&m_verticesWithTangents[vertexIndex2].tanget, tangent);
		XMStoreFloat3(&m_verticesWithTangents[vertexIndex2].bitangent, bitangent);
	}
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
	typedef WaveFrontReader<DWORD>::Vertex ReaderVertex;

	HRESULT hr = waveFrontReader.Load(fileName);
	if (FAILED(hr))
	{
		exit(-1);
	}

	for (const ReaderVertex& readerVertex : waveFrontReader.vertices)
	{
		Vertex vertex;
		vertex.position = readerVertex.position;
		vertex.normal = readerVertex.normal;
		vertex.textureCoordinate = readerVertex.textureCoordinate;

		m_verticesWithTangents.push_back(vertex);
	}
	
	CalculateTangents();
}

void Actor::ReleaseObj()
{
	waveFrontReader.Clear();
}

vector<Vertex>& Actor::GetVertices()
{
	return m_verticesWithTangents;
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
