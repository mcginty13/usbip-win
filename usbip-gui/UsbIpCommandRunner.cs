using Renci.SshNet;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;

namespace usbip_gui
{
    class UsbIpCommandRunner
    {
        public List<UsbDevice> GetFilteredRemoteUsbDevices(string serverip)
        {
            List<UsbDevice> allRemote = GetAllRemoteDevices(serverip, "pi", "raspberry");
            List<UsbDevice> boundRemote = GetBoundRemoteUsbDevices(serverip);

            foreach (var device in allRemote)
            {
                if (boundRemote.Where(x=>x.Id==device.Id).Count()> 0)
                {
                    device.Bound = true;
                }
            }
            return allRemote;
        }
        public List<UsbDevice> GetBoundRemoteUsbDevices(string serverip)
        {
            var result = RunCommand("list --parsable -r " + serverip).Result;
            result.WaitForExit();
            var output = result.StandardOutput.ReadToEnd();
            List<UsbDevice> usbDevices = ProcessRemoteListResult(output, serverip);
            return usbDevices;
        }

        public List<UsbDevice> GetLocalUsbDevices()
        {
            Process result = RunCommand("list -l").Result;
            result.WaitForExit();
            var output = result.StandardOutput.ReadToEnd();
            List<UsbDevice> usbDevices = ProcessLocalListResult(output);
            return usbDevices;
        }


        public List<UsbDevice> GetAllRemoteDevices(string serverip, string username, string password)
        {
            List<UsbDevice> zzx = new List<UsbDevice>();
            using (var client = new SshClient(serverip, username, password))
            {
                client.HostKeyReceived += (sender, e) =>
                {
                    e.CanTrust = true;
                };
                client.Connect();
                var command = client.CreateCommand("sudo usbip list -l");
                command.Execute();
                var result = command.Result;
                List<UsbDevice> usbDevices = ProcessLocalListResult(result);
                return usbDevices;
            }
        }
        public Task<Process> AttachDevice(UsbDevice device)
        {
            var result = RunCommand("attach -r " + device.IpAddress + " -b " + device.BusId);
            return result;
        }
        //public string DetachDevice(UsbDevice device, List<Process> procesess)
        //{
        //    //var attachedDevices = GetLocalUsbDevices();
        //    //var localDeviceToDetach = attachedDevices.Where(x => x.Id == device.Id).FirstOrDefault();
        //    //if (localDeviceToDetach != null)
        //    //{
        //    //    var result = RunCommand("detach -p " + localDeviceToDetach.BusId);
        //    //    return result;

        //    //}
        //    var taskToEnd = tasks.Where(t => t.Id == device.ProcessId).FirstOrDefault();
        //    taskToEnd.
        //    else return "device not attached";
        //}
        private Task<Process> RunCommand(string command)
        {
            Task<Process> task = null;
            task = Task.Run(() =>
            {
                Process cmd = new Process();
                cmd.StartInfo.FileName = "cmd.exe";
                cmd.StartInfo.RedirectStandardInput = true;
                cmd.StartInfo.RedirectStandardOutput = true;
                cmd.StartInfo.CreateNoWindow = true;
                cmd.StartInfo.UseShellExecute = false;
                cmd.Start();

                cmd.StandardInput.WriteLine("usbip.exe " + command);
                cmd.StandardInput.Flush();
                cmd.StandardInput.Close();

                return cmd;
            });
            return task;

        }
        private List<UsbDevice> ProcessLocalListResult(string input)
        {
            var lines = input.Split("\n\r".ToCharArray(), StringSplitOptions.RemoveEmptyEntries);
            List<UsbDevice> devices = new List<UsbDevice>();
            for (int i = 0; i < lines.Length; i++)
            {

                var trimmedLine1 = lines[i].Trim();

                if (trimmedLine1.StartsWith("-"))
                {
                    var trimmedLine2 = lines[i + 1].Trim();
                    var strArr1 = trimmedLine1.Split(' ');
                    var strArr2 = Regex.Split(trimmedLine2, @":\s");
                    devices.Add(new UsbDevice
                    {
                        BusId = strArr1[2],
                        Id = strArr1[3].Replace("(", "").Replace(")", ""),
                        Name = strArr2[1],
                        Manufacturer = strArr2[0],

                    });

                }
            }
            return devices;
        }
        private List<UsbDevice> ProcessRemoteListResult(string input, string serverip)
        {
            List<int> indexes = input.AllIndexesOf(serverip);
            StringBuilder sb = new StringBuilder();
            input = input.Remove(0, indexes[1]);
            var lines = input.Split("\n\r".ToCharArray(), StringSplitOptions.RemoveEmptyEntries);
            List<UsbDevice> devices = new List<UsbDevice>();
            foreach (var line in lines)
            {
                var trimmedLine = line.Trim();
                if (!trimmedLine.StartsWith(":"))
                {
                    var strs = Regex.Split(trimmedLine, @":\s");
                    var idRegex = Regex.Match(trimmedLine, @"\w{4}:\w{4}");
                    string id = "";
                    if (idRegex.Success)
                    {
                        id = idRegex.Groups[0].Value;
                    }

                    if (strs.Length >= 3)
                    {
                        devices.Add(new UsbDevice
                        {
                            BusId = strs[0],
                            Manufacturer = strs[1],
                            Name = strs[2],
                            IpAddress = serverip,
                            Id = id,
                            Remote = true
                        });
                    }

                }
            }
            return devices;

        }
    }
}
