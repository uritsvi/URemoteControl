using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace URemoteControlGUI
{
    internal class Utils
    {
        public static void ExitWithError(string errorMsg)
        {
            MessageBox.Show(errorMsg);
            Environment.Exit(-1);
        }
    }
}
