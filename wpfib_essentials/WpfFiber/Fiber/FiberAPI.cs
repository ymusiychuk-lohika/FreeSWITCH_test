using System.Runtime.InteropServices;
using System;
using System.Diagnostics;

namespace WpfFiber.Fiber
{
    public static class FiberAPI
    { 
        static IntPtr fiberHandle = IntPtr.Zero;

        static public bool IsFiberActive
        {
            get { return fiberHandle != IntPtr.Zero; }
        }

        #region Interface
        static public void CreateFiber()
        {
            if (IsFiberActive)
                return;

            fiberHandle = CreateFiberApiHandle();
            Debug.Assert(IsFiberActive);
        }

        static public void DestroyFiber()
        {
            if (IsFiberActive)
            {
                DestroyFiberApiHandle(fiberHandle);
                fiberHandle = IntPtr.Zero;
            }
        }

        static public void Init(string name, string uri, IntPtr handle)
        {
            Debug.Assert(IsFiberActive);

            Init(fiberHandle, handle, name, uri, String.Empty, false, "0.2");
        }

        static public void InitializeWidget(int widget, int width, int height, int screenWidth, int screenHeight)
        {
            Debug.Assert(IsFiberActive);
            InitializeWidget(fiberHandle, widget, width, height, screenWidth, screenHeight);
        }

        static public void RegisterDxWidgetCallback(InitDxWidgetCallback dxInitDelegate, UpdateDxViewCallback dxUpdateDelegate)
        {
            Debug.Assert(IsFiberActive);
            registerDxWidgetCallback(fiberHandle, dxInitDelegate, dxUpdateDelegate);
        }

        public static void RegisterMessageCallback(MessageCallback callback)
        {
            registerMessageCallback(fiberHandle, callback);
        }

        public static void RegisterShowWindowCallback(ShowWindowCallback messageCallbackDelegate)
        {
            registerShowWindowCallback(fiberHandle, messageCallbackDelegate);
        }
        #endregion

        #region Fiber import
        [DllImport("winfbrapp.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern IntPtr CreateFiberApiHandle();

        [DllImport("winfbrapp.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern void DestroyFiberApiHandle(IntPtr fiberApiHandle);



        [DllImport("winfbrapp.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private static extern bool Init(IntPtr fiberApiHandle, IntPtr handle, string name, string uri, string turnservers, bool forceTurn, string huddleVersion);

        [DllImport("winfbrapp.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public static extern IntPtr HandleWindowMessage(IntPtr fiberApiHandle, IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam);

        [DllImport("winfbrapp.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool hangupCall(IntPtr fiberApiHandle);

        [DllImport("winfbrapp.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool InitializeWidget(IntPtr fiberApiHandle, int widget, int width, int height, int screenWidth, int screenHeight);

        [DllImport("winfbrapp.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool muteAudio(IntPtr fiberApiHandle, bool muteState);

        [DllImport("winfbrapp.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool muteVideo(IntPtr fiberApiHandle, bool muteState);

        [DllImport("winfbrapp.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool toggleSelfView(IntPtr fiberApiHandle, bool show);

        [DllImport("winfbrapp.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern bool registerMessageCallback(IntPtr fiberApiHandle, MessageCallback callback);

        [DllImport("winfbrapp.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void registerShowWindowCallback(IntPtr fiberApiHandle, ShowWindowCallback messageCallbackDelegate);

        [DllImport("winfbrapp.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool ShowRemoteVideo(IntPtr fiberApiHandle, int width, int height, int posX, int posY, IntPtr remoteViewWindow);

        [DllImport("winfbrapp.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool EnableMeetingFeatures(IntPtr fiberApiHandle, string features);

        [DllImport("winfbrapp.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool registerDxWidgetCallback(IntPtr fiberApiHandle, InitDxWidgetCallback cbInit, UpdateDxViewCallback cbUpdate);
        #endregion
    }
}