using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Diagnostics;
using usbip_gui;
using System.Text.RegularExpressions;
using System.Management;

namespace usbip_gui
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        UsbIpCommandRunner commandRunner = new UsbIpCommandRunner();
        List<UsbDevice> remoteDevices;
        List<Process> Processes = new List<Process>();
        public MainWindow()
        {
            InitializeComponent();
        }

        private void Btn_GetBoundDevices_Click(object sender, RoutedEventArgs e)
        {
            var newRemoteDevices = commandRunner.GetBoundRemoteUsbDevices(TB_ServerIP.Text);
            if (remoteDevices == null || newRemoteDevices.Count >= remoteDevices.Count)
            {
                remoteDevices = newRemoteDevices;
                dataGrid1.ItemsSource = remoteDevices;
            }
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {

        }

        private void Btn_ChangeDeviceBindStatus_Click(object sender, RoutedEventArgs e)
        {
            Button b = (Button)sender;
            UsbDevice senderDevice = (UsbDevice)b.DataContext;
            UsbDevice device = remoteDevices.Where(x => x.Id == senderDevice.Id).FirstOrDefault();
            string result = "";
            if (device.Attached)
            {
                var p = System.Diagnostics.Process.GetProcessById(device.ProcessId);
                KillProcessAndChildrens(p.Id);
                //  result = commandRunner.DetachDevice(device,Processes);
                device.Attached = false;
                b.Content = "Attach";
            }
            else
            {
                var t = Task.Run(() => commandRunner.AttachDevice(device));
                var p = t.Result;

                Processes.Add(p);
                device.Attached = true;
                device.ProcessId = p.Id;
                b.Content = "Detach";
            }
            TB_Output.Text = result;
        }

        private static void KillProcessAndChildrens(int pid)
        {
            ManagementObjectSearcher processSearcher = new ManagementObjectSearcher
              ("Select * From Win32_Process Where ParentProcessID=" + pid);
            ManagementObjectCollection processCollection = processSearcher.Get();

            try
            {
                Process proc = Process.GetProcessById(pid);
                if (!proc.HasExited) proc.Kill();
            }
            catch (ArgumentException)
            {
                // Process already exited.
            }

            if (processCollection != null)
            {
                foreach (ManagementObject mo in processCollection)
                {
                    KillProcessAndChildrens(Convert.ToInt32(mo["ProcessID"])); //kill child processes(also kills childrens of childrens etc.)
                }
            }
        }

        private void Btn_GetUnboundDevices_Click(object sender, RoutedEventArgs e)
        {
            var result = commandRunner.GetAllRemoteDevices(TB_ServerIP.Text, "pi", "raspberry");
        }

        private void Btn_GetRemoteDevices_Click(object sender, RoutedEventArgs e)
        {
            var result = commandRunner.GetFilteredRemoteUsbDevices(TB_ServerIP.Text);
            remoteDevices = result;
            dataGrid1.ItemsSource = remoteDevices;

        }
    }
}
