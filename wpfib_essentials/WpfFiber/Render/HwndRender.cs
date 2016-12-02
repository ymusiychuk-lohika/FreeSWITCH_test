using System.Diagnostics;
using System.Windows.Threading;
using WpfFiber.Fiber;

namespace WpfFiber.Render
{
    class HwndRender
    {
        public ShowWindowCallback showWindowCallbackDelegate;
        public delegate void CreateWindow(int window, int width, int height, int screenWidth, int screenHeight);

        public HwndRender()
        {
            showWindowCallbackDelegate = new ShowWindowCallback(showWindowCallback);
            FiberAPI.RegisterShowWindowCallback(showWindowCallbackDelegate);
        }
        private void showWindowCallback(int window)
        {
            Debug.Print($"Create renderer [{window}]");
            CreateWindow handler = CreateWindowHandler;

            int selfViewWidth = (window == 0) ? 320 : 640; // Screen.PrimaryScreen.Bounds.Width / 8;
            int selfViewHeight = (window == 0) ?240 : 480; // selfViewWidth * 9 / 16;
            int selfViewX = (window == 0) ? 50 : 5;// Screen.PrimaryScreen.Bounds.Width - selfViewWidth - margin;
            int selfViewY = (window == 0) ? 150 : 5;// margin;
            App.Current.Dispatcher.Invoke(handler, DispatcherPriority.Normal, window, selfViewWidth, selfViewHeight, selfViewX, selfViewY);

        }

        private void CreateWindowHandler(int window, int width, int height, int x, int y)
        {
            FiberAPI.InitializeWidget(window, width, height, x, y);
        }
    }
}
