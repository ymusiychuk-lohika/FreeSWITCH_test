#include "talk/base/logging_libjingle.h" //should be very first!!!
#include "DxBjnRenderer.h"
#include "D3DProxyWidget.h"

DxBjnRenderer::DxBjnRenderer(int index)
	: _pView(nullptr), 
	_pRenderer(nullptr),
	_idxStream(index)
{
	::InitializeCriticalSection(&_cs);
	LOG(LS_INFO) << "DxBjnRenderer created";
}
DxBjnRenderer::~DxBjnRenderer()
{
	::DeleteCriticalSection(&_cs);
	delete _pRenderer;
	delete _pView;
}

int DxBjnRenderer::FrameSizeChange(unsigned int width,
	unsigned int height,
	unsigned int number_of_streams,
	webrtc::RawVideoType videoType)
{
	//if (_idxStream == 0) return 0;
	if (!_pView)
	{
		_pView = getView();
		assert(_pView);
	}
	::EnterCriticalSection(&_cs);
	auto pRenderer = new DxVideoRenderer();
	
	HRESULT hr = pRenderer->CreateDevice(_pView->GetMainWindow(), width, height);
	assert(hr == S_OK);
	if (FAILED(hr))
	{
		LOG(LS_ERROR) << "Can't create DX device: " << hr;
		return -1;
	}
	_pView->InitDxWidget(_idxStream, pRenderer->GetSurface(), width, height);

	delete _pRenderer;
	_pRenderer = pRenderer;
	::LeaveCriticalSection(&_cs);
	return 0;
}

// This method is called when a new frame should be rendered.
int DxBjnRenderer::DeliverFrame(webrtc::BASEVideoFrame *videoFrame)
{
	//if (_idxStream == 0) return 0;
	::EnterCriticalSection(&_cs);
	_pRenderer->Draw(videoFrame);
	::LeaveCriticalSection(&_cs);
	_pView->UpdateView(_idxStream);
	return 0;
}

IDxProxyWidget* DxBjnRenderer::getView() const
{
	return dynamic_cast<IDxProxyWidget*>(_uiwidget);
}
