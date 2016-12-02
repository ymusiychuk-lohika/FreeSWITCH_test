using System;
using System.Diagnostics;
using WpfFiber.Fiber;

namespace WpfFiber.Render
{
    class D3DRender
    {
        public InitDxWidgetCallback dxInitDelegate;
        public UpdateDxViewCallback dxUpdateView;


        public event InitDxWidgetCallback Init;
        public event UpdateDxViewCallback Update;

        public D3DRender()
        {
            dxInitDelegate = new InitDxWidgetCallback(dxInitCallback);
            dxUpdateView = new UpdateDxViewCallback(dxUpdateCallback);
            FiberAPI.RegisterDxWidgetCallback(dxInitDelegate, dxUpdateView);
        }
        private void dxInitCallback(int idxStream, IntPtr surface, int width, int heigth)
        {
            Debug.Print($"Stream: {idxStream} has resolution: {width} x {heigth}");

            Init?.Invoke(idxStream, surface, width, heigth);
        }

        private void dxUpdateCallback(int idxStream)
        {
            //Debug.Print($"Stream {idxStream} has updated!");

            Update?.Invoke(idxStream);
        }
    }
}
