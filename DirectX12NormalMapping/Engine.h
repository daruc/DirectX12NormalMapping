#pragma once

#define NOMINMAX

#include <d3d12.h>
#include "d3dx12.h"
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <chrono>
#include <wincodec.h>
#include <memory>
#include "Camera.h"
#include "Actor.h"
#include "Light.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "windowscodecs.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using std::chrono::high_resolution_clock;
using std::chrono::duration;

struct Wvp
{
	XMFLOAT4X4 world;
	XMFLOAT4X4 wvp;
	XMFLOAT3 cameraPos;
	BYTE padding[4];
	XMFLOAT4X4 lightWvp;
	XMFLOAT3 lightWorldPos;
	BYTE padding2[4];
	XMFLOAT3 lightDirection;
	float lightFov;
};

extern const XMVECTOR X_UNIT_VEC;
extern const XMVECTOR Y_UNIT_VEC;
extern const XMVECTOR Z_UNIT_VEC;

class Engine
{
private:
	UINT m_shadowMapRes;
	UINT m_resolutionWidth;
	UINT m_resolutionHeight;
	HWND m_hwnd;

	float m_mouseX;
	float m_mouseY;
	float m_mouseDeltaX;	// when right mouse button was pressed
	float m_mouseDeltaY;

	high_resolution_clock::time_point m_prevTime;

	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	ComPtr<ID3D12CommandAllocator> m_lightCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_lightCommandList;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12PipelineState> m_lightPipelineState;
	ComPtr<ID3D12Resource> m_renderTarget[2];
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12Fence> m_fence;
	ComPtr<ID3D12Device> m_device;

	// drawing triangles
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12RootSignature> m_lightRootSignature;

	ComPtr<ID3D12Resource> m_vertexBuffer;
	ComPtr<ID3D12Resource> m_vBufferUploadHeap;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	ComPtr<ID3D12Resource> m_indexBuffer;
	ComPtr<ID3D12Resource> m_iBufferUploadHeap;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	ComPtr<ID3DBlob> m_vertexShader;
	ComPtr<ID3DBlob> m_pixelShader;
	ComPtr<ID3DBlob> m_lightPixelShader;
	ComPtr<ID3DBlob> m_lightVertexShader;
	D3D12_VIEWPORT m_viewport;
	D3D12_VIEWPORT m_lightViewport;
	D3D12_RECT m_scissorRect;
	D3D12_RECT m_lightDepthScissorRect;

	// depth/stencil buffer
	ComPtr<ID3D12DescriptorHeap> m_dsDescriptorHeap;
	ComPtr<ID3D12Resource> m_dsBuffer;

	ComPtr<ID3D12DescriptorHeap> m_dsLightDescriptorHeap;
	ComPtr<ID3D12DescriptorHeap> m_lightSamplerDescriptorHeap;
	ComPtr<ID3D12Resource> m_dsLightBuffer;

	// constant buffers
	ComPtr<ID3D12DescriptorHeap> m_cbDescriptorHeap[2];

	ComPtr<ID3D12Resource> m_cbWvpUploadHeap[2];
	Wvp m_wvpData;
	UINT8* m_cbWvpGpuAddress[2];

	Actor m_actor;
	Light m_light;
	Camera m_camera;

	// textures
	std::unique_ptr<BYTE[]> m_textureData;
	ComPtr<ID3D12Resource> m_textureDefaultHeap;
	ComPtr<ID3D12Resource> m_textureUploadHeap;
	ComPtr<ID3D12DescriptorHeap> m_textureDescriptorHeap;

	int m_frameIndex;	// render target index
	UINT m_rtvDescriptorSize;	// Render Target View descriptor heap size
	UINT64 m_fenceValue;
	HANDLE m_fenceEvent;

	void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter);
	void WaitForPreviousFrame();

	void CreateRootSignature();
	void CreateLightRootSignature();
	void LoadShaders();
	void CreateLightDepthBuffer();
	void LoadTextures();
	void CreatePipelineStateObject();
	void CreateLightPso();
	void CreateVertexBuffer();
	void FillOutViewportAndScissorRect();
	void InitWvp();
	void UpdateWvp(float deltaSec);
	void CreateConstantBuffers();
	void CreateSamplers();

	void RenderLightDepth();
	void RenderScene();
public:

	Engine(UINT resolutionWidth, UINT resolutionHeight);
	~Engine();

	void Init(HWND hwnd);
	void Input(float mouseX, float mouseY, bool rightMouseBtnPressed);
	void Update();
	void ResizeViewport(UINT resolutionWidth, UINT resolutionHeight);
	void Render();

	void Destroy();
};