using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WpfFiber
{
    internal class StreamInfo
    {
        public int StreamId;
        public int Width;
        public int Height;
        public bool IsLocal;
        public bool IsRunning;
        public bool IsNewFrameReady;
        public FpsCounter Fps = new FpsCounter();
    }
}
