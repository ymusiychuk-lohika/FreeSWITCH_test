#include "talk/base/logging_libjingle.h" //should be very first!!!
#include "d3dproxywidget.h"

D3DProxyWidget::D3DProxyWidget()
{
	LOG(LS_INFO) << "Created D3D Proxy";
}

D3DProxyWidget::~D3DProxyWidget()
{

}

bool D3DProxyWidget::Show(bool show, int x, int y, int cx, int cy, unsigned int flags)
{
	//looks like do not need this stuff
	return true;
}

bool D3DProxyWidget::GetPosition(int& x, int& y, int& cx, int& cy)
{
	LOG(LS_WARNING) << "Do not handle GetPosition (should?)";
	assert(false);
	return false;
}

void D3DProxyWidget::SetButtonState(int button, int state)
{
	LOG(LS_WARNING) << "Do not handle SetButtonState (should?)";
	assert(false);
}

void D3DProxyWidget::SetButtonVisible(int button, bool visible)
{
	LOG(LS_WARNING) << "Do not handle SetButtonVisible (should?)";
	assert(false);
}

void D3DProxyWidget::SetButtonTooltipText(int button, int state, const std::string& text)
{
	LOG(LS_WARNING) << "Do not handle SetButtonTooltip (should?)";
	assert(false);
}

void D3DProxyWidget::ShowRecordingIndicator(bool show)
{
	LOG(LS_WARNING) << "Do not handle ShowRecord (should?)";
	assert(false);
}

void D3DProxyWidget::UpdateVideo(webrtc::BASEVideoFrame *videoFrame)
{
	return;
}

