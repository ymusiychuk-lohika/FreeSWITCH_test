using System;

namespace WpfFiber.Fiber
{
    //is called from Fiber to init D3DSurface and its resolution
    public delegate void InitDxWidgetCallback(int idxStream, IntPtr surface, int width, int heigth);
    //is called from fiber when new frame has arrived
    public delegate void UpdateDxViewCallback(int idxStream);

    public delegate void ShowWindowCallback(int window);
    public delegate void MessageCallback(string window);
}
