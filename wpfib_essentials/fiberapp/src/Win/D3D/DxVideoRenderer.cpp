#include "talk/base/logging_libjingle.h" //should be very first!!!
#include "DxVideoRenderer.h"
#include "..\winbjnrenderer.h"
#include <assert.h>


template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		ULONG count = (*ppT)->Release();
		*ppT = NULL;
	}
}

inline UINT GetRefCounter(IUnknown *pUnk)
{
#ifdef _DEBUG
	pUnk->AddRef();
	return pUnk->Release();
#else
	return 0;
#endif
}


DxVideoRenderer::DxVideoRenderer()
	: hWnd(NULL),
	pDeviceSurface(NULL),
	pOffScreenSurface(NULL),
	_pRenderFrame(NULL),
	pDX(NULL), pDevice(NULL),
	format(D3DFMT_UNKNOWN),
	_isYUVSupported(false)
{

}

DxVideoRenderer::~DxVideoRenderer()
{
	DestroyDevice();
}

HRESULT DxVideoRenderer::CreateDevice(HWND hWnd, int width, int height)
{
	if (pDevice)
		return S_OK;

	HRESULT hr = D3D_OK;
	if (NULL == pDX)
	{
#ifdef DX9EX
		hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &pDX);
#else
		pDX = Direct3DCreate9(D3D_SDK_VERSION);
#endif

		assert(pDX != NULL);
		if (pDX == NULL)
			return D3DERR_DRIVERINTERNALERROR;
	}

	D3DPRESENT_PARAMETERS pp = { 0 };
	D3DDISPLAYMODE mode = { 0 };
	IDirect3DDevice9Ex *pDeviceEx = NULL;

	hr = pDX->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode);

	if (FAILED(hr))
		goto done;

	hr = pDX->CheckDeviceType(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		mode.Format,
		D3DFMT_X8R8G8B8,
		TRUE);    // windowed

	if (FAILED(hr))
		goto done;

	pp.BackBufferFormat = D3DFMT_X8R8G8B8;
	pp.SwapEffect = D3DSWAPEFFECT_FLIP;
	pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	pp.Windowed = TRUE;
	pp.hDeviceWindow = hWnd;
	pp.Flags = D3DPRESENTFLAG_VIDEO | D3DPRESENTFLAG_DEVICECLIP;
	pp.BackBufferCount = NumberBackBuffers;
#ifdef DX9EX
	hr = pDX->CreateDeviceEx(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING |
		D3DCREATE_FPU_PRESERVE |
		D3DCREATE_MULTITHREADED,
		&pp,
		NULL,
		&pDeviceEx);

	if (FAILED(hr))
		goto done;
	pDeviceEx->QueryInterface(__uuidof(IDirect3DDevice9), reinterpret_cast<void**>(&pDevice));
#else
	hr = pDX->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING |
		D3DCREATE_FPU_PRESERVE |
		D3DCREATE_MULTITHREADED,
		&pp,
		&pDevice);
#endif
	this->hWnd = hWnd;
	d3dParams = pp;
	d3dParams.BackBufferWidth = width;
	d3dParams.BackBufferHeight = height;

	_isYUVSupported = isHwColorConversionEnabled(mode);

	CreateSurfaces();

	CreateFrameBuffer();

done:
	SafeRelease(&pDeviceEx);
	return hr;
}

IDirect3DSurface9 *DxVideoRenderer::GetSurface() const
{
	assert(pDeviceSurface != NULL);
	return pDeviceSurface;
}

void DxVideoRenderer::Draw(webrtc::BASEVideoFrame * pVideoFrame)
{
	assert(pDeviceSurface != NULL);
	assert(_pRenderFrame != NULL);

	HRESULT hr = S_OK;

	if (_pRenderFrame)
	{
		_pRenderFrame->CopyFrame(pVideoFrame);   // Copy the videoFrame to the render frame buffer to share with the render thread.
		UpdateOffScreenSurface();
	}
	assert(hr == S_OK);
}

void DxVideoRenderer::DestroyDevice()
{
	SafeRelease(&pDeviceSurface);
	SafeRelease(&pOffScreenSurface);
	SafeRelease(&pDevice);
	SafeRelease(&pDX);
}

HRESULT DxVideoRenderer::CreateSurfaces()
{
	HRESULT hr = CreateDeviceSurface();
	assert(hr == S_OK);
	if (FAILED(hr))
		goto done;

	if (_isYUVSupported)
		hr = CreateOffScreenSurface();
	assert(hr == S_OK);

done:
	return hr;

}

HRESULT DxVideoRenderer::CreateDeviceSurface()
{
	SafeRelease(&pDeviceSurface);

	HRESULT hr = pDevice->CreateRenderTarget(
		d3dParams.BackBufferWidth,
		d3dParams.BackBufferHeight,
		d3dParams.BackBufferFormat,
		D3DMULTISAMPLE_NONE,
		0,
		FALSE,	//Lockable buffer!
		&pDeviceSurface,
		NULL);

	assert(hr == S_OK);
	if (FAILED(hr))
		goto done;

	hr = pDevice->SetRenderTarget(0, pDeviceSurface);

done:
	return hr;
}

HRESULT DxVideoRenderer::CreateOffScreenSurface()
{
	assert(_isYUVSupported);
	SafeRelease(&pOffScreenSurface);

	return pDevice->CreateOffscreenPlainSurface(
		d3dParams.BackBufferWidth,
		d3dParams.BackBufferHeight,
		(D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2'),
		D3DPOOL_DEFAULT,
		&pOffScreenSurface,
		NULL);
}

bool DxVideoRenderer::isHwColorConversionEnabled(D3DDISPLAYMODE mode)
{
	HRESULT hr = pDX->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, mode.Format, 0, D3DRTYPE_SURFACE, (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2'));

	if (SUCCEEDED(hr))
	{
		hr = pDX->CheckDeviceFormatConversion(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2'), d3dParams.BackBufferFormat);  // Checking direct YV12 rendering capability
		if (SUCCEEDED(hr))
		{
			LOG(LS_INFO) << "D3D HW Color Conversion capability check complete YU12 render is enabled";
			return true;
		}
		else
		{
			LOG(LS_INFO) << "D3D HW Color Conversion capability check failed from YV12 to back buffer format:" << d3dParams.BackBufferFormat << " hr:" << hr;
		}
	}
	else
	{
		LOG(LS_ERROR) << "D3D HW Color Conversion capability check failed from " << mode.Format << " to YV12 hr:" << hr;
	}

	return false;
}

HRESULT DxVideoRenderer::CreateFrameBuffer()
{
	if (_isYUVSupported)   // YV12 render
	{
		LOG(LS_INFO) << "D3D HW color conversion is initiated";
		if (NULL == _pRenderFrame)
			_pRenderFrame = new WinBjnRenderBuffer(kVideoI420);
		else
			_pRenderFrame->Reset(kVideoI420);
	}
	else            // RGB render
	{
		LOG(LS_INFO) << "D3D RGB render is initiated";
		if (NULL == _pRenderFrame)
			_pRenderFrame = new WinBjnRenderBuffer(kVideoARGB);
		else
			_pRenderFrame->Reset(kVideoARGB);
	}

	return S_OK;
}

HRESULT DxVideoRenderer::UpdateOffScreenSurface()
{
	HRESULT hr = E_FAIL;

	if (_pRenderFrame)
	{
		int frameWidth = 0, frameHeight = 0;
		if (_isYUVSupported)
		{
			I420VideoFrame* renderI420Buffer = dynamic_cast<I420VideoFrame*>(_pRenderFrame->LockBuffer(&frameWidth, &frameHeight, NULL));
			if (renderI420Buffer && pOffScreenSurface && pDeviceSurface)
			{
				D3DLOCKED_RECT lr;
				hr = pOffScreenSurface->LockRect(&lr, NULL, D3DLOCK_NOSYSLOCK);
				if (SUCCEEDED(hr))
				{
					int y_stride = renderI420Buffer->stride(kYPlane);
					int u_stride = renderI420Buffer->stride(kUPlane);
					int v_stride = renderI420Buffer->stride(kVPlane);
					UCHAR* pRect = (UCHAR*)lr.pBits;

					if (lr.Pitch == y_stride)
					{

						memcpy(pRect, renderI420Buffer->buffer(kYPlane), y_stride*frameHeight);  // y-plane
						pRect += y_stride * frameHeight;

						memcpy(pRect, renderI420Buffer->buffer(kVPlane), v_stride*(frameHeight >> 1));  //v-plane
						pRect += v_stride * (frameHeight >> 1);

						memcpy(pRect, renderI420Buffer->buffer(kUPlane), u_stride*(frameHeight >> 1));  //u-plane
					}
					else
					{
						CopyWithStride(pRect, lr.Pitch, renderI420Buffer->buffer(kYPlane), y_stride, frameHeight);
						pRect += lr.Pitch * frameHeight;

						CopyWithStride(pRect, lr.Pitch >> 1, renderI420Buffer->buffer(kVPlane), v_stride, frameHeight >> 1);
						pRect += (lr.Pitch >> 1) * (frameHeight >> 1);

						CopyWithStride(pRect, lr.Pitch >> 1, renderI420Buffer->buffer(kUPlane), u_stride, frameHeight >> 1);
					}
					pOffScreenSurface->UnlockRect();

					hr = pDevice->StretchRect(pOffScreenSurface, NULL, pDeviceSurface, NULL, D3DTEXF_LINEAR);
				}
				_pRenderFrame->UnlockBuffer();
			}
		}
		else
		{
			RGBVideoFrame* renderI420Buffer = dynamic_cast<RGBVideoFrame*>(_pRenderFrame->LockBuffer(NULL, NULL, NULL));
			if (renderI420Buffer && pDeviceSurface)
			{
				D3DLOCKED_RECT lr;
				hr = pDeviceSurface->LockRect(&lr, NULL, D3DLOCK_NOSYSLOCK);
				if (SUCCEEDED(hr))
				{
					UCHAR* pRect = (BYTE*)lr.pBits;
					if (lr.Pitch == renderI420Buffer->width() * 4)
					{
						memcpy(pRect, renderI420Buffer->buffer(), renderI420Buffer->width() * renderI420Buffer->height() * 4);
					}
					else
					{
						CopyWithStride(pRect, lr.Pitch, renderI420Buffer->buffer(), renderI420Buffer->width(), renderI420Buffer->height());
					}
					hr = pDeviceSurface->UnlockRect();
				}
			}
			_pRenderFrame->UnlockBuffer();
		}
	}

	return hr;
}


void DxVideoRenderer::CopyWithStride(BYTE *pDest, int lengthWithPitch, BYTE *pSrc, int width, int height)
{
	int k = _isYUVSupported ? 1 : 4;
	int lengthRow = width * k;
	for (int i = 0; i < height; ++i)
	{
		memcpy(pDest, pSrc, lengthRow);
		pSrc += lengthRow;
		pDest += lengthWithPitch;
	}
}
