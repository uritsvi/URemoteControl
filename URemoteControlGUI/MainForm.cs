using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Printing;
using System.Reflection;
using System.Windows.Forms;
using Shared;

namespace URemoteControlGUI
{
    public partial class MainForm : Form
    {
        private string _Hash;

        private Common.ClientInfo _ClientInfo;
        private Dictionary<string, string> _CurrentSessionInfo;
        private string _CurrentHash;
        private int _NextSaveIndex = 0;

        private const string _SavedText = "Saved {0}";

        private const int _LabelsDefaultPadding = 5;

        private SpecialLabel _LastSelect;

        public MainForm()
        {
            Common.LoadInfo();
            _ClientInfo.client_type = Common.ControllerClientType;


            InitializeComponent();
            this.FormClosing += new FormClosingEventHandler(MainForm_Closing);
        }

        private void UIResetError()
        {
            statuseLable.Visible = false;
        }
        private void UIHandleError()
        {
            statuseLable.Visible = true;
        }

        private void RefreshButton_Click(object sender, EventArgs e)
        {
            UIResetError();

            _CurrentSessionInfo = null;

            _CurrentHash = Common.MakeHah(
                computerNameTextBox.Text, 
                passwordTextBox.Text);

            bool res = Common.GetRequest(
                Common.GetSessionInfoRout,
                _CurrentHash, 
                out _CurrentSessionInfo);

            if (!res)
            {
                UIHandleError();
                return;
            }

            if (_CurrentSessionInfo != null)
            {
                ConnectButton.Enabled = true;
            }
            else
            {
                ConnectButton.Enabled = false;
            }
        }

        private void ConnectButton_Click(object sender, EventArgs e)
        {
            try
            {
                UIResetError();

                int response;
                bool res =
                    Common.PostRequest(
                        String.Format(
                            Common.TryConnectToSessionRout,
                            _CurrentHash),
                            null,
                            null,
                            out response);
                if (!res || response != 100)
                {
                    UIHandleError();
                    return;
                }

                Common.StartMainAppProcess(
                    _CurrentSessionInfo[Common.SessionPortDictionaryKey],
                    Common.ControllerClientType,
                    false);
            }
            catch(Exception ex)
            {
                UIHandleError();
            }
           

        }

        private void MainForm_Closing(object sender, CancelEventArgs e)
        {
            UIResetError();
            int response;
            bool res =
                Common.PostRequest(
                    Common.DisconnectRoutName,
                    _ClientInfo,
                    new Dictionary<string, string>
                    {{Common.AuthorizationHeaderName , _Hash }},
                    out response);

            if (!res)
            {
                UIHandleError();
            }

            Common.WriteToInI(
                Common.NextSavedIndexInIKeyName, 
                _NextSaveIndex.ToString(), 
                Common.GuiInfoSection);

        }

        private void MainForm_Load(object sender, EventArgs e)
        {

            string computerName;
            string password;

            Common.GetComputerMacAddressAndPassword(
                out computerName, 
                out password);

            computerNameLable.Text = computerName;
            passwordLable.Text = password;

            _Hash = Common.MakeHah(
                computerName,
                password);

            var saved = new List<string>();
            Common.ReadFullKeysSection(
                Common.SavedPeopleSection, 
                Common.MaxNameLen * Common.MaxSavedPeople, 
                saved);

            foreach(var key in saved)
            {
                AddElementToConnectList(key);
            }

            string nextIndex = Common.ReadFromInI(
                Common.NextSavedIndexInIKeyName, 
                Common.GuiInfoSection);

            if(nextIndex != "")
            {
                _NextSaveIndex = Int32.Parse(nextIndex);
            }



            UIResetError();
            int response;
            bool res =
                Common.PostRequest(
                    Common.ConnectRoutName, 
                    _ClientInfo,
                    new Dictionary<string, string>
                    {{Common.AuthorizationHeaderName , _Hash }}, 
                    out response);

            if (!res)
            {
                UIHandleError();
            }

        }

        private void AddElementToConnectList(string text)
        {
            var control = new SpecialLabel(
                text, 
                HandleRename,
                HandleDelete,
                HandleDoubleClick);

            control.Margin = new Padding(_LabelsDefaultPadding);

            control.MouseClick += (sender, e) =>
            {
                if (_LastSelect != null && _LastSelect != control)
                {
                    _LastSelect.OnLostFocus();
                }
                _LastSelect = control;

            };
            
            savedPeople.Controls.Add(control);


        }

        private void DeleteElementFromConnectList(string text)
        {
            foreach(SpecialLabel control in savedPeople.Controls)
            {
                if(control.Text != text)
                {
                    continue;
                }
                savedPeople.Controls.Remove(control);

            }
        }

        private void saveButton_Click(object sender, EventArgs e)
        {
            UIResetError();
            if(savedPeople.Controls.Count >= Common.MaxSavedPeople)
            {
                UIHandleError();
            }

            string connectInfo = 
                computerNameTextBox.Text + " " + passwordTextBox.Text;

            string name = String.Format(_SavedText, _NextSaveIndex++);

            Common.WriteToInI(
                name, 
                connectInfo, 
                Common.SavedPeopleSection);
            AddElementToConnectList(name);
        }

        private void HandleRename(
            string oldText, 
            string newText)
        {
            Common.ReplaceKeyNameInInI(
                oldText, 
                newText,
                Common.SavedPeopleSection);
        }

        private void HandleDelete(string name)
        {
            Common.WriteToInI(
                name, 
                null, 
                Common.SavedPeopleSection);

            DeleteElementFromConnectList(name);
        }

        private void HandleDoubleClick(string name)
        {
            UIResetError();
            string connectInfo = 
                Common.ReadFromInI(
                    name, 
                    Common.SavedPeopleSection);
           
            var parsed = connectInfo.Split(' ');
            if(parsed.Length != 2 ) 
            {
                UIHandleError();
                return;
            }

            computerNameTextBox.Text = parsed[0];
            passwordTextBox.Text = parsed[1];
        }

        private void MainForm_Click(object sender, EventArgs e)
        {
            if(_LastSelect == null) {
                return;
            }

            _LastSelect.OnLostFocus();
            _LastSelect= null;
        }
    }
}
