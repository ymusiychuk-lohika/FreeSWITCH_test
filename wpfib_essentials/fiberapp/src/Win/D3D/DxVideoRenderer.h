#ifndef __DX_VIDEORENDERER_H
#define __DX_VIDEORENDERER_H

#include <d3d9.h>
#include "..\winbjnrenderer.h"
#define DX9EX

class DxVideoRenderer
{
public:
	DxVideoRenderer();
	~DxVideoRenderer();
	IDirect3DSurface9 *GetSurface() const;
	HRESULT CreateDevice(HWND hWnd, int width, int height);
	void Draw(webrtc::BASEVideoFrame * pVideoFrame);
	void DestroyDevice();

private:
	static const int NumberBackBuffers = 1;
	HWND hWnd;
	bool _isYUVSupported;

#ifdef DX9EX
	IDirect3D9Ex *pDX;
#else
	IDirect3D9 *pDX;
#endif
	IDirect3DDevice9 *pDevice;
	IDirect3DSurface9 *pDeviceSurface;		//Surface that is represent phisical video device
	IDirect3DSurface9 *pOffScreenSurface;	//OffScreen surface for color conversion YV12 > RGB
	D3DPRESENT_PARAMETERS  d3dParams;
	D3DFORMAT format;

	WinBjnRenderBuffer*       _pRenderFrame;  //frame to be rendered

	HRESULT TestCooperativeLevel();
	HRESULT CreateSurfaces();
	HRESULT CreateFrameBuffer();
	bool isHwColorConversionEnabled(D3DDISPLAYMODE mode);
	HRESULT UpdateOffScreenSurface();

	HRESULT CreateDeviceSurface();
	HRESULT CreateOffScreenSurface();

	void CopyWithStride(BYTE *pDest, int lengthWithPitch, BYTE *pSrc, int width, int height);
};
#endif