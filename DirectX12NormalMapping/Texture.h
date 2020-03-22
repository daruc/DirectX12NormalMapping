#pragma once

#define NOMINMAX

#include <memory>
#include <d3d12.h>
#include <wrl.h>

using namespace std;
using Microsoft::WRL::ComPtr;

class Texture
{
private:
	class Engine* m_engine;

	unique_ptr<BYTE[]> m_data;
	D3D12_RESOURCE_DESC m_textureDesc;

	ComPtr<ID3D12Resource> m_textureDefaultHeap;
	ComPtr<ID3D12Resource> m_textureUploadHeap;

public:
	Texture(Engine* const engine);

	void LoadFromFile(const wchar_t* const fileName);
	void CreateResource(const wchar_t * const textureName, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle);
	void UploadToResource();
	void Release();

	UINT GetWidth() const;
	UINT GetHeight() const;
	BYTE* GetData() const;
};

