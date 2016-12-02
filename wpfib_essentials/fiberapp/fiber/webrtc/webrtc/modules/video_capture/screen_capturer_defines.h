#define ENABLE_SCREEN_SHARE 1

#define SCREEN_SHARING_FRIENDLY_NAME "Screen Share"
#define SCREEN_SHARING_UNIQUE_ID "{7306149c-b8c7-4227-9946-6d6316edc64f}"
#define SCREEN_SHARING_MAX_WIDTH 1920
#define SCREEN_SHARING_MAX_HEIGHT 1080
#define SCREEN_SHARING_LESS_THAN_QUAD_CORE_FPS 2
#define SCREEN_SHARING_QUAD_CORE_FPS 5
#define SCREEN_SHARING_WIDGET_NAME "BJNPresenterWidget"
#define SCREEN_SHARING_BORDER_INFOBAR_NAME "BJNSharingBorderInfoBar"

//Mac only
#define SCREEN_SHARING_QUEUE_NAME "com.screencapture.cgdisplayqueue"
// screen sharing border <-> widget communications through NSNotificationCenter
// widget to border communications
#define SCREEN_SHARING_WIDGET_CREATED_ID "bjn.sharing.widget.created.id"
#define SCREEN_SHARING_DISPLAY_STRINGS_ID "bjn.sharing.display.strings.id"
// border to widget communications
#define SCREEN_SHARING_BORDER_CREATED_ID "bjn.sharing.border.created.id"
#define SCREEN_SHARING_STOP_CLICKED_ID "bjn.sharing.stop.clicked.id"
// dictionary keys for the displa strings
#define SCREEN_SHARING_KEY_STOP             "stop"
#define SCREEN_SHARING_KEY_TITLE            "title"

// Windows only
// Universal Windows Message to be registered with Operating System
// for communication from sharing border to widget
// TODO(Suyash): Should the string be made longer?
#define WM_BJN_SHARING_BORDER_TO_WIDGET     "WM_BJN_SHARING_BORDER_TO_WIDGET"
// commands
#define SHARING_BORDER_CMD_INIT         1
#define SHARING_BORDER_CMD_STOP         2
// message use for communication from widget to sharing border
#define WM_WIDGET_TO_SHARING_BORDER     (WM_APP + 50)
// commands
#define WIDGET_CMD_STOP_TEXT            1
#define WIDGET_CMD_TITLE_TEXT           2
