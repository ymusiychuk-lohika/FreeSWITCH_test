#ifndef __I_DX_PROXY_WIDGET_H
#define __I_DX_PROXY_WIDGET_H

typedef void(__stdcall *fnInitWidget)(int, void*, int, int); //init DxImage (idxStream, pSurface, width, height)
typedef void(__stdcall *fnUpdateView)(int); //update view with new frame (number of view)

class IDxProxyWidget
{
public:
	IDxProxyWidget()
		: _initDxWidget(nullptr),
		_updateView(nullptr),
		_hwndMainWindow(nullptr)
	{}
	virtual ~IDxProxyWidget()
	{}
	
	virtual void RegisterWidget(fnInitWidget cbInit, fnUpdateView cbUpdate)
	{
		_initDxWidget = cbInit;
		_updateView = cbUpdate;
	}

	virtual void RegisterMainWindow(HWND hwnd)
	{
		_hwndMainWindow = hwnd;
	}

	virtual HWND GetMainWindow() const
	{
		return _hwndMainWindow;
	}

	virtual void InitDxWidget(int idx, void * pD3DSurface, int width, int heigth)
	{
		assert(_initDxWidget);
		_initDxWidget(idx, pD3DSurface, width, heigth);
	}

	virtual void UpdateView(int n)
	{
		assert(_updateView);
		_updateView(n);
	}

protected:
	fnInitWidget _initDxWidget;
	fnUpdateView _updateView;
	HWND	_hwndMainWindow;
};

#endif
