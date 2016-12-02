#ifndef __D3D_PROXY_WIDGET_H
#define __D3D_PROXY_WIDGET_H

#include <Windows.h>
#include "uiwidget.h"
#include "IDxProxyWidget.h"


//Proxy for D3D renderer that is used by WPF for render image

class D3DProxyWidget : public UIWidgetBase, public IDxProxyWidget
{
public:
	D3DProxyWidget();
	virtual ~D3DProxyWidget();
	
	//Implementation UIWidget interface
	virtual bool Show(bool show, int x, int y, int cx, int cy, unsigned int flags);
	virtual bool GetPosition(int& x, int& y, int& cx, int& cy);
	virtual void SetButtonState(int button, int state);
	virtual void SetButtonVisible(int button, bool visible);
	virtual void SetButtonTooltipText(int button, int state, const std::string& text);
	virtual void UpdateVideo(webrtc::BASEVideoFrame *videoFrame);
	virtual void ShowRecordingIndicator(bool show);

	//Implementation IDxProxyWidget interface
};

#endif