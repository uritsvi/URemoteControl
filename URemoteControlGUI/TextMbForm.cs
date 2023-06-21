using Shared;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace URemoteControlGUI
{
    public partial class TextMbForm : Form
    {
        public delegate void GetTextCallback(
            bool res, 
            string text);

        private GetTextCallback _Callback;
        private bool _Finshed;

        public TextMbForm(GetTextCallback callback)
        {
            InitializeComponent();
            _Callback = callback;
        }

        private void finishButton_Click(object sender, EventArgs e)
        {
            string _out = renameTextBox.Text;

            if(_out.Length > Common.MaxNameLen)
            {
                _out = renameTextBox.Text.Substring(0, Common.MaxNameLen);
            }

            _Callback(true, _out);
            _Finshed = true;

            Close();
        }

        protected override void OnClosed(EventArgs e)
        {
            if (_Finshed)
            {
                return;
            }

            _Callback(false, null);
        }
    }
}
