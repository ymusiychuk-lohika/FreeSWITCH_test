using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace WpfFiber
{
    internal class MainWindowViewModel : INotifyPropertyChanged
    {
        private bool _isStreaming;
        public bool IsStreaming
        {
            get { return _isStreaming; }
            private set
            {
                if (_isStreaming != value)
                {
                    _isStreaming = value;
                    NotifyPropertyChanged();
                }
            }
        }

        private bool _isJoinMeetingEnabled;
        public bool IsJoinMeetingEnabled
        {
            get { return _isJoinMeetingEnabled; }
            private set
            {
                if (_isJoinMeetingEnabled != value)
                {
                    _isJoinMeetingEnabled = value;
                    NotifyPropertyChanged();
                }
            }
        }

        private bool _isDisconnectEnabled;
        public bool IsDisconnectEnabled
        {
            get { return _isDisconnectEnabled; }
            private set
            {
                if (_isDisconnectEnabled != value)
                {
                    _isDisconnectEnabled = value;
                    NotifyPropertyChanged();
                }
            }
        }

        private bool _isLoadingVisible;
        public bool IsLoadingVisible
        {
            get { return _isLoadingVisible; }
            private set
            {
                if (_isLoadingVisible != value)
                {
                    _isLoadingVisible = value;
                    NotifyPropertyChanged();
                }
            }
        }

        public MainWindowViewModel()
        {
            SetState(FiberConnectionState.Disconnected);
        }

        public event PropertyChangedEventHandler PropertyChanged;
        private void NotifyPropertyChanged([CallerMemberName] string propertyName = "")
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        public void SetState(FiberConnectionState state)
        {
            IsStreaming = (state == FiberConnectionState.Connected);
            IsJoinMeetingEnabled = (state == FiberConnectionState.Disconnected);
            IsDisconnectEnabled = (state != FiberConnectionState.Disconnected);
            IsLoadingVisible = (state == FiberConnectionState.Connecting);
        }
    }
}
