using System;
using System.Diagnostics;
using System.ServiceProcess;
using System.Timers;
using System.Threading;
using System.Runtime.InteropServices;
using Shared;
using System.Collections.Generic;
using System.Runtime.Remoting.Channels;

namespace WindowsService1
{
    public partial class URemoteControlService : ServiceBase
    {
        [DllImport("kernel32", CharSet = CharSet.Unicode)]
        private static extern bool GetExitCodeProcess(IntPtr Result, out int ExitCode);

        [DllImport("kernel32", CharSet = CharSet.Unicode)]
        private static extern bool TerminateProcess(IntPtr handle, uint exitCode);

        private const int STILL_ACTIVE = 0x00000103;

        private string _Hash;

        private Common.ClientInfo _ClientInfo;

        public URemoteControlService()
        {
            _ClientInfo.client_type = Common.ControlledClientType;

            InitializeComponent();
            eventLog1 = new EventLog();

            eventLog1.Source = "URemoteControlWindowsService";
            eventLog1.Log = "URemoteControlWindowsServiceLog";

        }

        private IntPtr GetControlledProcessHandle()
        {
            Process[] processes;

            processes = Process.GetProcessesByName(
                Common.ControlledExeName);

            if (processes.Length == 0)
            {
                return IntPtr.Zero;
            }

            IntPtr processHandle = processes[0].Handle;

            return processHandle;
        }

        private bool Disconnect()
        {
            int response;
            bool res = Common.PostRequest(
                    Common.DisconnectRoutName,
                    _ClientInfo,
                    new Dictionary<string, string>
                    {{Common.AuthorizationHeaderName , _Hash }},
                    out response);

            return res;
        }

        private void Restart()
        {
            bool res = Disconnect();
            if (!res)
            {
                Log("Failed to disconnect");
                OnStop();

            }

            Thread.Sleep(3000);

            OnStart(null);
        }

        private void Log(string message)
        {
            eventLog1.WriteEntry(message);
        }

        public static void WaitProcessExit(
            IntPtr processHandle,
            Action callback)
        {

            System.Timers.Timer timer = new System.Timers.Timer();

            timer.Elapsed += ((sender, e) =>
            {


                int exitCode;
                bool res = GetExitCodeProcess(
                    processHandle,
                    out exitCode);

                if (exitCode != STILL_ACTIVE)
                {
                    timer.Enabled = false;
                    callback();
                }
            });

            timer.Interval = 1000;
            timer.Enabled = true;
        }

        private void HandleErrorInOnStart()
        {
            var timer = new System.Timers.Timer();
            timer.Elapsed += (sender, o) =>
            {
                timer.Enabled = false;
                OnStart(null);
            };
            timer.AutoReset = false;
            timer.Enabled = true;
            timer.Interval = 1000;
        }

        protected override void OnStart(string[] args)
        {

            Common.LoadInfo();

            string computerName;
            string password;


            Common.GetComputerMacAddressAndPassword(
                out computerName,
                out password);

            _Hash = Common.MakeHah(
                computerName,
                password);


            int response;
            bool res = Common.PostRequest(
                    Common.ConnectRoutName,
                    _ClientInfo,
                    new Dictionary<string, string>
                    {{Common.AuthorizationHeaderName , _Hash }},
                    out response);

            if (!res)
            {
                HandleErrorInOnStart();
                return;
            }

            Dictionary<string, string> sessionInfo;
            res = Common.GetRequest(
                Common.GetSessionInfoRout,
                _Hash,
                out sessionInfo);
            if (!res)
            {
                HandleErrorInOnStart();
                return;
            }

            var t = new Thread(() =>
            {
                Common.StartMainAppProcess(
            sessionInfo[Common.SessionPortDictionaryKey],
            Common.ControlledClientType,
            true);

                IntPtr processHandle;

                do
                {
                    Thread.Sleep(1000);
                    processHandle = GetControlledProcessHandle();

                } while (processHandle == IntPtr.Zero);


                res =
                    Common.PostRequest(
                        String.Format(
                            Common.TryConnectToSessionRout,
                            _Hash),
                            null,
                            null,
                            out response);
                if (!res)
                {
                    HandleErrorInOnStart();
                    return;
                }


                WaitProcessExit(
                    processHandle,
                    Restart);
            });

            t.Start();


        }

        protected override void OnStop()
        {
            IntPtr processHandle =
                GetControlledProcessHandle();


            if (processHandle != IntPtr.Zero)
            {
                TerminateProcess(processHandle, 0);
            }

            Disconnect();
            base.OnStop();
        }



    }
}