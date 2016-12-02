#ifndef __DX_BJN_RENDERER_H
#define __DX_BJN_RENDERER_H

#include "bjnrenderer.h"
#include "DxVideoRenderer.h"

class IDxProxyWidget;
/*
	Renderer thai is used to render video frame directly to DX surface
*/
class DxBjnRenderer : public BjnRenderer
{
public:
	DxBjnRenderer(int index);
	virtual ~DxBjnRenderer();

	virtual int FrameSizeChange(unsigned int width,
		unsigned int height,
		unsigned int number_of_streams,
		webrtc::RawVideoType videoType) ;

	// This method is called when a new frame should be rendered.
	virtual int DeliverFrame(webrtc::BASEVideoFrame *videoFrame);

private:
	int _idxStream;
	IDxProxyWidget* _pView;
	DxVideoRenderer* _pRenderer;
	CRITICAL_SECTION _cs;

	IDxProxyWidget* getView() const;
};
#endif
