using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Printing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace URemoteControlGUI
{
    internal class SpecialLabel : System.Windows.Forms.Label
    {

        public delegate void OnRenameCallback(
            string oldName, 
            string newName);

        public delegate void OnDeleteCallback(string name);

        public delegate void OnDoubleClickCallback(string name);

        private Color _DefaultColor = Color.White;
        private Color _ClickColor = Color.Gray;
        private Color _OnMouseHoverColor = Color.LightGray;

        private bool _IsSelected = false;

        private const string _RenameItemName = "remane";
        private const string _DeleteItemName = "delete";
        
        private OnRenameCallback _RenameCallback;
        private OnDeleteCallback _DeleteCallback;
        private OnDoubleClickCallback _DoubleClickCallback;

        public SpecialLabel(
            string text, 
            OnRenameCallback onRename,
            OnDeleteCallback onDelete,
            OnDoubleClickCallback onDoubleClick)
        {
            this._RenameCallback = onRename;
            this._DeleteCallback = onDelete;
            this._DoubleClickCallback = onDoubleClick;

            this.BackColor = Color.White;
            this.Text = text;

            this.TextAlign = ContentAlignment.MiddleCenter;

            ContextMenu contectMenu = new ContextMenu();
            
            var renameMenuIntem = new MenuItem();
            var deleteMenueItem = new MenuItem();
            
            renameMenuIntem.Text = _RenameItemName;
            renameMenuIntem.Click += HandleRemname;

            contectMenu.MenuItems.Add(renameMenuIntem);

            deleteMenueItem.Text = _DeleteItemName;
            deleteMenueItem.Click += HandleDelete;

            contectMenu.MenuItems.Add(renameMenuIntem);
            contectMenu.MenuItems.Add(deleteMenueItem);


            this.ContextMenu = contectMenu;
        }

        private void HandleRemname(
            object sender, 
            EventArgs e)
        {
            var mbForm = new TextMbForm((res, text) =>
            {
                if (!res)
                {
                    return;
                }

                _RenameCallback(this.Text, text);
                this.Text = text; 
            });

            mbForm.ShowDialog();
        }

        private void HandleDelete(
            object sender, 
            EventArgs e)
        {
            _DeleteCallback(this.Text); 
        }

        protected override void OnClick(EventArgs e)
        {
            base.OnClick(e);
            
            this.BackColor = _ClickColor;
            _IsSelected= true;
        }

        public void OnLostFocus()
        {
            this.BackColor = _DefaultColor;
            _IsSelected = false;
        }


        protected override void OnMouseEnter(EventArgs e)
        {
            base.OnMouseEnter(e);
            if (_IsSelected)
            {
                return;
            }

            this.BackColor = _OnMouseHoverColor;
        }

        protected override void OnMouseLeave(EventArgs e)
        {
            base.OnMouseLeave(e);
            if (_IsSelected)
            {
                return;
            }

            this.BackColor = _DefaultColor;
        }
        protected override void OnDoubleClick(EventArgs e)
        {
            base.OnDoubleClick(e);
            _DoubleClickCallback(this.Text);
        }

        public string GetText()
        {
            return this.Text;
        }



    }
}
