using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;

namespace WpfFiber
{
    class FpsCounter
    {
        public void Restart()
        {
            _fpsRenderCounter = 0;
            _fpsRenderCounter = 0;
            FpsRender = 0;
            FpsStream = 0;
            _timer.Restart();
        }

        public void RenderIncrement()
        {
            ++_fpsRenderCounter;
            AdjustTimer();
        }

        public void StreamIncrement()
        {
            ++_fpsStreamCounter;
            AdjustTimer();
        }

        private void AdjustTimer()
        {
            if (_timer.ElapsedMilliseconds >= 1000)
            {
                _timer.Restart();
                FpsRender = _fpsRenderCounter;
                FpsStream = _fpsStreamCounter;
                _fpsRenderCounter = 0;
                _fpsStreamCounter = 0;
            }
        }

        public int FpsRender { get; private set; }
        public int FpsStream { get; private set; }

        private int _fpsRenderCounter = 0;
        private int _fpsStreamCounter = 0;
        private Stopwatch _timer = Stopwatch.StartNew();
    }
}