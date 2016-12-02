#include "talk/base/logging_libjingle.h"
#include "talk/base/common.h"
#include "uiwidget.h"
#if defined FB_WIN
#ifdef FB_WIN_D3D
#include "Win/D3D/d3dproxywidget.h"
#else
#include "win/winuiwidget.h"
#endif
#elif defined FB_MACOSX
#include "Mac/macuiwidget.h"
#endif


UIWidget* UIWidget::CreateUIWidget(void* window)
{
	UIWidget* widget = NULL;

#if defined FB_WIN
#ifdef FB_WIN_D3D
	widget = new D3DProxyWidget();
#else
	widget = new WinUIWidget(window);
#endif
#elif defined FB_MACOSX
	widget = dynamic_cast<UIWidget*>(new MacUIWidget(window));
#elif defined FB_X11
#endif

	return widget;
}


void UIWidget::DeleteUIWidget(UIWidget* widget)
{
#if defined FB_WIN
	delete widget;
#elif defined FB_MACOSX
	MacUIWidget* mac = dynamic_cast<MacUIWidget*>(widget);
	delete mac;
#elif defined FB_X11
#endif
}
