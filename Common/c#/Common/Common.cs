using murrayju.ProcessExtensions;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Net.NetworkInformation;
using System.Runtime.InteropServices;
using System.Text;
using System.Security.Cryptography;
using System.Net;
using Newtonsoft.Json;
using System.Dynamic;
using System.ComponentModel;

namespace Shared
{


    public class Common
    {

        public const string InIFileName = "config.ini";

        public static string InIFullPath =
            Path.Combine(AppDomain.CurrentDomain.BaseDirectory, InIFileName);
        public static string HTTPServerAddress;

        public const string ConnectRoutName = "connect/";
        public const string DisconnectRoutName = "disconnect/";
        public const string GetSessionInfoRout = "get_session_info/{0}/";
        public const string TryConnectToSessionRout = "try_connect_to_session/{0}/";

        public const string HTTPServerUrlInIKey = "http_server_url";
        public const string MainServerAddressInIKey = "main_server_address";
        public const string MacAddressInIKey = "mac_address";
        public const string PasswordInIKey = "password";

        public const string AuthorizationHeaderName = "Authorization";

        public const int ControlledClientType = 2;
        public const int ControllerClientType = 1;

        public const string ClientTypeDictionaryKey = "client_type";
        public const string SessionPortDictionaryKey = "port";

        public const string ServerConnectInISection = "ServerConnectInfo";
        public const string SavedPeopleSection = "SavedPeople";
        public const string GuiInfoSection = "GuiInfo";

        public const string NextSavedIndexInIKeyName = "NextSavedIndex";

        public const string ControllerExeName = "ControllerApp";
        public const string ControlledExeName = "ControlledApp";

        public const string ConsoleAppLauncherExeName = "ConsoleAppLauncher.exe";

        public const int Error = -1;

        private const int _NumOfRetries = 3;

        private const int _ComputerNameLen = 8;
        private const int _HashLen = 6;

        public const int MaxSavedPeople = 80;
        public const int MaxNameLen = 80;

        public struct ClientInfo
        {
            public int client_type;
        }

        [DllImport("kernel32", CharSet = CharSet.Unicode)]
        private static extern int
            GetPrivateProfileString(
            string Section,
            string Key,
            string Default,
            StringBuilder RetVal,
            int Size,
            string FilePath);


        [DllImport("kernel32", CharSet = CharSet.Unicode)]

        static extern int
            WritePrivateProfileString(
            string Section,
            string Key,
            string Value,
            string FilePath);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetPrivateProfileSection(
            string section,
            byte[] buffer,
            int size,
            string filePath);

        public static void LoadInfo()
        {
            HTTPServerAddress =
                ReadFromInI(
                    HTTPServerUrlInIKey,
                    ServerConnectInISection);
        }
        public static IntPtr StartSubProcess(
            int port,
            int appType,
            bool StartFromService)
        {


            string appPath =
                Path.Combine(
                    AppDomain.CurrentDomain.BaseDirectory,
                    ConsoleAppLauncherExeName);

            string mainServerAddress = ReadFromInI(
                MainServerAddressInIKey,
                ServerConnectInISection);

            string programArgs =
                appType.ToString() + " " +
                mainServerAddress + " " +
                port.ToString();


            IntPtr outProcess;
            if (StartFromService)
            {
                programArgs = appPath + " " + programArgs;
                outProcess =
                    ProcessExtensions.StartProcessAsCurrentUser(
                        appPath,
                        programArgs);

            }
            else
            {
                outProcess =
                    Process.Start(
                        appPath,
                        programArgs).Handle;
            }

            return outProcess;



        }





        public static string ReadFromInI(
            string Key,
            string section)
        {

            var RetVal = new StringBuilder(256);
            GetPrivateProfileString(
                section,
                Key,
                "",
                RetVal,
                256,
                InIFullPath);

            return RetVal.ToString();
        }

        public static void ReplaceKeyNameInInI(
            string oldKey,
            string newKey,
            string section)
        {
            string value = ReadFromInI(
                oldKey,
                section);

            WriteToInI(
                oldKey,
                null,
                section);

            WriteToInI(
                newKey,
                value,
                section);
        }

        public static long WriteToInI(
            string key,
            string value,
            string section)
        {

            return WritePrivateProfileString(
                section,
                key,
                value,
                InIFullPath);
        }

        public static bool GetRequest<T>(
            string rout,
            string parameter,
            out T response)
        {
            for (int i = 0; i < _NumOfRetries; i++)
            {
                try
                {
                    var request =
                        (HttpWebRequest)WebRequest.Create(
                            HTTPServerAddress +
                            String.Format(rout, parameter));

                    request.ContentType = "application/json";
                    request.Method = "GET";

                    WebResponse webResponse = request.GetResponse();

                    using (var reader = new StreamReader(webResponse.GetResponseStream()))
                    {
                        response =
                            JsonConvert.DeserializeObject<T>(reader.ReadToEnd());
                    }

                    return true;
                }
                // if catch any exception retry
                catch (WebException)
                {
                    continue;
                }
                catch (Exception)
                {

                    response = default(T);
                    return false;
                }

            }

            response = default(T);
            return false;
        }

        public static bool PostRequest<T>(
            string rout,
            object parameters,
            Dictionary<string, string> headers,
            out T response)
        {
            for (int i = 0; i < _NumOfRetries; i++)
            {
                try
                {
                    var request =
                        (HttpWebRequest)WebRequest.Create(HTTPServerAddress + rout);

                    request.ContentType = "application/json";
                    request.Method = "POST";

                    if (headers != null)
                    {
                        foreach (var headerName in headers.Keys)
                        {
                            request.Headers[headerName] =
                                headers[headerName];
                        }
                    }

                    using (var streamWriter = new StreamWriter(request.GetRequestStream()))
                    {
                        streamWriter.Write(JsonConvert.SerializeObject(parameters));
                    }

                    var webResponse = request.GetResponse();

                    using (var streamReader = new StreamReader(webResponse.GetResponseStream()))
                    {
                        response =
                            JsonConvert.DeserializeObject<T>(streamReader.ReadToEnd());

                        return true;
                    }


                }
                // if catch any exception retry
                catch (Exception)
                {
                    response = default(T);
                    return false;
                }
            }

            response = default(T);
            return false;
        }

        public static bool ReadFullKeysSection(
            string sectionName,
            int bufferSize,
            List<string> outKeys)
        {

            byte[] buffer = new byte[bufferSize];

            GetPrivateProfileSection(
                sectionName,
                buffer,
                bufferSize,
                InIFullPath);

            char[] charBuffer =
                System.Text.Encoding.Unicode.GetString(buffer).ToCharArray();

            StringBuilder builder = new StringBuilder();
            for (int i = 0; i < charBuffer.Length; i++)
            {
                if (charBuffer[i] == '=')
                {
                    outKeys.Add(builder.ToString());
                    while (charBuffer[i] != '\0')
                    {
                        i++;
                    }

                    builder.Clear();

                    continue;
                }

                builder.Append(charBuffer[i]);
            }

            return true;
        }


        private static void _CalculateMacAddressAndPassword(
            out string macAddress,
            out string hash)
        {

            hash = "";
            macAddress = "";

            string _macAddress = "";

            NetworkInterface[] interfaces = NetworkInterface.GetAllNetworkInterfaces();

            foreach (NetworkInterface networkInterface in interfaces)
            {
                if (networkInterface.NetworkInterfaceType != NetworkInterfaceType.Ethernet &&
                    networkInterface.NetworkInterfaceType != NetworkInterfaceType.Wireless80211)
                    continue;

                if (networkInterface.OperationalStatus == OperationalStatus.Up)
                {
                    PhysicalAddress physicalAddress = networkInterface.GetPhysicalAddress();
                    byte[] bytes = physicalAddress.GetAddressBytes();

                    for (int i = 0; i < bytes.Length; i++)
                    {
                        _macAddress += bytes[i].ToString("X2");
                    }


                }

                using (var hashObject = SHA256.Create())
                {
                    byte[] buffer = Encoding.UTF8.GetBytes(_macAddress);
                    byte[] hashBuffer = hashObject.ComputeHash(buffer);


                    hash = BuildString(hashBuffer);
                }

            }

            if (_macAddress.Length > _ComputerNameLen)
            {
                _macAddress = _macAddress.Substring(0, _ComputerNameLen);
            }
            if (hash.Length > _HashLen)
            {
                hash = hash.Substring(0, _HashLen);
            }

            macAddress = _macAddress.ToLower();
            hash = hash.ToLower();


        }

        public static string MakeHah(
            string computerName,
            string password)
        {
            using (var hashObject = SHA256.Create())
            {
                byte[] buffer = Encoding.UTF8.GetBytes(computerName + password);
                byte[] target = hashObject.ComputeHash(buffer);

                return BuildString(target);
            }
        }

        private static string BuildString(byte[] buffer)
        {
            StringBuilder builder = new StringBuilder(buffer.Length * 2);

            foreach (byte b in buffer)
            {
                builder.Append(b.ToString("x2"));
            }

            return builder.ToString();
        }

        public static void GetComputerMacAddressAndPassword(
            out string macAddress,
            out string hash)
        {
            macAddress = ReadFromInI(
                MacAddressInIKey,
                GuiInfoSection);
            if (macAddress == "")
            {
                _CalculateMacAddressAndPassword(
                    out macAddress, 
                    out hash);

                WriteToInI(
                    MacAddressInIKey, 
                    macAddress, 
                    GuiInfoSection);
                
                WriteToInI(
                    PasswordInIKey, 
                    hash, 
                    GuiInfoSection);

                return;
            }

            hash = ReadFromInI(
                PasswordInIKey, 
                GuiInfoSection);


        }




        public static void StartMainAppProcess(
            string port,
            int appType,
            bool StartFromService)
        {


            string appPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, ConsoleAppLauncherExeName);
            string controllerPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, ControllerExeName);

            string mainServerAddress = ReadFromInI(
                MainServerAddressInIKey,
                ServerConnectInISection);

            string programArgs =
                mainServerAddress + " " +
                port;



            if (StartFromService)
            {
                programArgs = "\"" + appPath + "\"" + " " + appType.ToString() + " " + programArgs;
                ProcessExtensions.StartProcessAsCurrentUser(appPath, programArgs);

            }
            else
            {
                Process.Start(controllerPath, programArgs);
            }


        }


    }
}