using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace usbip_gui
{
    public class UsbDevice : INotifyPropertyChanged
    {
        public string Id { get; set; }
        public string Name { get; set; }
        public string Manufacturer { get; set; }
        public string BusId { get; set; }
        private bool _attached;
        public bool Attached
        {
            get { return _attached; }
            set { _attached = value; NotifyPropertyChanged(); }
        }
        public bool Bound { get; set; }
        public bool Remote { get; set; }
        public string IpAddress { get; set; }
        public int ProcessId { get; set; }

        public event PropertyChangedEventHandler PropertyChanged;

        private void NotifyPropertyChanged([CallerMemberName] String propertyName = "")
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
