#ifndef __UI_WIDGET_H
#define __UI_WIDGET_H

#include "common_video/interface/i420_video_frame.h"
#include <string>
#include <limits.h>

// forward declarations
class BjnRenderer;
class bjnpluginslaveAPI;

class UIWidget
{
public:
    static UIWidget* CreateUIWidget(void* window);
    static void DeleteUIWidget(UIWidget* widget);

    virtual bool Show(bool show, int x, int y, int cx, int cy, unsigned int flags) = 0;
    virtual bool GetPosition(int& x, int& y, int& cx, int& cy) = 0;
    virtual void SetButtonState(int button, int state) = 0;
    virtual void SetButtonVisible(int button, bool visible) = 0;
    virtual void SetButtonTooltipText(int button, int state, const std::string& text) = 0;
    virtual void SetRenderer(BjnRenderer* renderer) = 0;
    virtual void SetSlaveJSPtr(bjnpluginslaveAPI *slaveJSPtr) = 0;
    virtual void UpdateVideo(webrtc::BASEVideoFrame *videoFrame) = 0;
    virtual void ShowRecordingIndicator(bool show) = 0;

#if (defined FB_WIN) && (!defined FB_WIN_D3D)
	virtual HWND GetWindowHandle(void) = 0;
	virtual LRESULT HandleWindowMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
#endif
	enum {
        kUiWidgetDefaultPosition = INT_MAX,
    };

    enum {
        // default size (16x9)
        kUIWidgetDefaultWidth = 240,
        kUIWidgetDefaultHeight = 135,
        kUIWidgetToolbarHeight = 31,
        // default offset
        kUIWidgetDefaultOffsetX = 60,
        kUIWidgetDefaultOffsetY = 45,
        // misc
        kUIWidgetMaxTooltipTextLength = 256,
        kUIWidgetCornerRadius = 0,
        // color and tranparency (0 - 255 range)
        kUIWidgetToolbarColorRed = 48,
        kUIWidgetToolbarColorGreen = 48,
        kUIWidgetToolbarColorBlue = 48,
        kUIWidgetToolbarAlpha = 242,
        // no video text colors
        kUIWidgetNoVideoTextRed = 96,
        kUIWidgetNoVideoTextGreen = 96,
        kUIWidgetNoVideoTextBlue = 96,
        // margin
        kUIWidgetNoVideoTextMargin = 8,
        // recording indicator
        kUIWidgetRecIndicatorOnTime = 700,
        kUIWidgetRecIndicatorOffTime = 300,
    };

    // UI Toolbar button layout
    enum {
        kButtonWidth = 25,
        kButtonHeight = 23,
        kButtonWidth_JF3 = 24,//values for new Join flow 3 buttons.
        kButtonHeight_JF3 = 24,
        kSmallButtonWidth_JF3 = 16,
        kButtonMargin = 6,
        kSmallButtonMargin_JF3 = 4,
        kButtonTotalWidth = (kButtonWidth + (kButtonMargin * 2)),
        kButtonTotalWidth_JF3 = (kButtonWidth_JF3 + (kButtonMargin * 2)),
    };

    // UI buttons
    enum {
        // UI buttons
        kUIWidgetButtonSharing = 0,
        kUIWidgetButtonVideoMute = 1,
        kUIWidgetButtonAudioMute = 2,
        kUIWidgetCenterButtonCount = (kUIWidgetButtonAudioMute + 1),
        kUIWidgetButtonMinimize = 3,
        kUIWidgetButtonResize = 4,
        kUIWidgetButtonRecording = 5,
        kUIWidgetButtonCount = (kUIWidgetButtonRecording + 1),
        // other UI controls that we may need to set text strings for
        kUIWidgetNoVideo = 1000,
    };

    // UI button states
    enum {
        kUIWidgetButtonStateNormal = 0,
        kUIWidgetButtonStateOff = kUIWidgetButtonStateNormal,
        kUIWidgetButtonStateOn = 1,
        kUIWidgetButtonStateModeratorOn = 2,
    };
    
    // UI flags
    enum {
        kUIWidgetFlagsNormal = 0,
        kUIWidgetFlagsNoVideo = (1 << 0),
        kUIWidgetFlagsMinimize = (1 << 1),
    };
};


// base class for cross platform code
class UIWidgetBase: public UIWidget
{
public:
    UIWidgetBase()
        : _renderer(NULL)
    {
    }
    ~UIWidgetBase()
    {
    }
    void SetRenderer(BjnRenderer* renderer)
    {
        _renderer = renderer;
    }
    void SetSlaveJSPtr(bjnpluginslaveAPI *slaveJSPtr)
    {
    }

protected:
    BjnRenderer*        _renderer;
};

#endif // __UI_WIDGET_H
