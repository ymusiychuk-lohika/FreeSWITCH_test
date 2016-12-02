using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Interop;
using System.Windows.Media;
using WpfFiber.Fiber;
using WpfFiber.Render;

namespace WpfFiber
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private const string FiberMessageDisconnected = "DISCONNECTED";
        private const string FiberMessageShowRemote = "SHOWREMOTE";
        private const string FiberMessageVideoMuted = "VIDEO_MUTED";
        private const string FiberMessageVideoUnmuted = "VIDEO_UNMUTED";
        private const string FiberMessageAudioMuted = "AUDIO_MUTED";
        private const string FiberMessageAudioUnmuted = "AUDIO_UNMUTED";

#if HWND_RENDER
        private HwndRender _hwndRenderer;
        private D3DRender _localRenderer = null;
#else
        private D3DRender _localRenderer;
#endif


        private readonly MainWindowViewModel _dataContext = new MainWindowViewModel();
        
        private Stopwatch _frameSync = Stopwatch.StartNew();
        private int _minTicksPerFrame;

        private bool IsStreaming
        {
            get { return _dataContext.IsStreaming; }
        }

        public MainWindow()
        {
            InitializeComponent();

            CompositionTarget.Rendering += CompositionTarget_Rendering;
            _messageCallback = this.FiberMessageCallback;

            this.DataContext = _dataContext;
        }

        private void CompositionTarget_Rendering(object sender, EventArgs e)
        {
            foreach (StreamInfo si in _streams.Values)
            {
                if (si.IsLocal)
                {
                    tbLocalRenderFps.Text = $"Render: {si.Fps.FpsRender} fps.";
                    tbLocalStreamFps.Text = $"Stream: {si.Fps.FpsStream} fps.";
                }
                else
                {
                    tbRemoteRenderFps.Text = $"Render: {si.Fps.FpsRender} fps.";
                    tbRemoteStreamFps.Text = $"Stream: {si.Fps.FpsStream} fps.";
                }

                si.Fps.RenderIncrement();
            }

            if (!IsStreaming)
                return;

            _frameSync.Restart();

            foreach (StreamInfo si in _streams.Values)
            {
                if (si.IsNewFrameReady)
                {
                    si.IsNewFrameReady = false;
                    si.Fps.StreamIncrement();
                    UpdateImage(si);
                }
            }
        }

        private void UpdateImage(StreamInfo si)
        {
            D3DImage img = si.IsLocal ? imgLocalStream : imgRemoteStream;

            img.Lock();
            img.AddDirtyRect(new Int32Rect(0, 0, si.Width, si.Height));
            img.Unlock();
        }


        private void fiberJoinMeeting(string name, string meetingId)
        {
            try
            {
                _dataContext.SetState(FiberConnectionState.Connecting);
#if HWND_RENDER
                _hwndRenderer = new HwndRender();
#else
                _localRenderer = new D3DRender();

                _localRenderer.Init += LocalRenderer_Init;
                _localRenderer.Update += LocalRenderer_Update;
#endif
                FiberMessageCallback("Start call");
                FiberAPI.Init(
                    name,
                    $"sip:{meetingId}@172.23.86.16:5060;transport=tcp",
                    new WindowInteropHelper(this).Handle
                    );
            }
            catch (Exception ex)
            {
                _dataContext.SetState(FiberConnectionState.Disconnected);
                Debug.Print($"Can't create fiber: {ex.Message}");
            }
        }


        private MessageCallback _messageCallback;
        private void FiberMessageCallback(string message)
        {
            Debug.WriteLine(message);

            tbFiberMessageOutput.Text += message + Environment.NewLine;

            switch (message.ToUpper())
            {
                case FiberMessageDisconnected:
                    _dataContext.SetState(FiberConnectionState.Disconnected);
                    break;
                case FiberMessageShowRemote:
                    Console.WriteLine("Connected");
                    _dataContext.SetState(FiberConnectionState.Connected);
                    break;
                default:
                    break;
            }
        }

        private Dictionary<int, StreamInfo> _streams = new Dictionary<int, StreamInfo>()
        {
            { 0, new StreamInfo { IsLocal = true } },
            { 1, new StreamInfo() }
        };

        private void LocalRenderer_Init(int idxStream, IntPtr surface, int width, int heigth)
        {
            StreamInfo si;

            if (!_streams.TryGetValue(idxStream, out si))
            {
                throw new InvalidOperationException();
            }

            si.StreamId = idxStream;
            si.IsLocal = idxStream == 0;

            si.Width = width;
            si.Height = heigth;

            Dispatcher.Invoke(() =>
            {
                if (si.IsLocal)
                    tbLocalResolution.Text = $"Cam Res: {width} x {heigth}";
                else
                    tbRemoteResolution.Text = $"Stream Res: {width} x {heigth}";

                InitImage(si, surface);
            }
            );
        }

        private void InitImage(StreamInfo si, IntPtr surface)
        {
            D3DImage img = si.IsLocal ? imgLocalStream : imgRemoteStream;

            img.Lock();
            img.SetBackBuffer(D3DResourceType.IDirect3DSurface9, surface, true);
            img.Unlock();
        }

        private void LocalRenderer_Update(int idxStream)
        {
            StreamInfo si = _streams[idxStream];
            si.IsNewFrameReady = true;
        }


        private void setFiberOff()
        {
            _dataContext.SetState(FiberConnectionState.Disconnected);

            FiberMessageCallback("Drop call");

            if (_localRenderer != null)
            {
                _localRenderer.Init -= LocalRenderer_Init;
                _localRenderer.Update -= LocalRenderer_Update;
            }

            EmptyImage(imgLocalStream);

            FiberAPI.DestroyFiber();
        }

        private void EmptyImage(D3DImage image)
        {
            image.Lock();
            image.SetBackBuffer(D3DResourceType.IDirect3DSurface9, IntPtr.Zero);
            image.Unlock();
        }

        private void InitFiber()
        {
            FiberAPI.CreateFiber();
            FiberAPI.RegisterMessageCallback(_messageCallback);
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
                     
        }

        private void btnJoinMeeting_Click(object sender, RoutedEventArgs e)
        {
            InitFiber();
            fiberJoinMeeting(tbName.Text, tbMeetingId.Text);
        }

        private void btnDisconnect_Click(object sender, RoutedEventArgs e)
        {
            setFiberOff();
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            setFiberOff();
        }

        private void Window_ContentRendered(object sender, EventArgs e)
        {
            if (App.AutoConnect)
            {
                Console.WriteLine("AutoConnect");
                btnJoinMeeting.RaiseEvent(new RoutedEventArgs(ButtonBase.ClickEvent));
            }
        }
    }
}
