namespace URemoteControlGUI
{
    partial class MainForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.RefreshButton = new System.Windows.Forms.Button();
            this.ConnectButton = new System.Windows.Forms.Button();
            this.computerNameLable = new System.Windows.Forms.Label();
            this.computerNameTextBox = new System.Windows.Forms.TextBox();
            this.passwordLable = new System.Windows.Forms.Label();
            this.statuseLable = new System.Windows.Forms.Label();
            this.passwordTextBox = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.saveNewButton = new System.Windows.Forms.Button();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.savedPeople = new System.Windows.Forms.FlowLayoutPanel();
            this.SuspendLayout();
            // 
            // RefreshButton
            // 
            this.RefreshButton.Location = new System.Drawing.Point(283, 208);
            this.RefreshButton.Name = "RefreshButton";
            this.RefreshButton.Size = new System.Drawing.Size(102, 32);
            this.RefreshButton.TabIndex = 0;
            this.RefreshButton.Text = "&Refresh";
            this.RefreshButton.UseVisualStyleBackColor = true;
            this.RefreshButton.Click += new System.EventHandler(this.RefreshButton_Click);
            // 
            // ConnectButton
            // 
            this.ConnectButton.Enabled = false;
            this.ConnectButton.Location = new System.Drawing.Point(165, 208);
            this.ConnectButton.Name = "ConnectButton";
            this.ConnectButton.Size = new System.Drawing.Size(102, 32);
            this.ConnectButton.TabIndex = 1;
            this.ConnectButton.Text = "&Connect";
            this.ConnectButton.UseVisualStyleBackColor = true;
            this.ConnectButton.Click += new System.EventHandler(this.ConnectButton_Click);
            // 
            // computerNameLable
            // 
            this.computerNameLable.AutoSize = true;
            this.computerNameLable.Font = new System.Drawing.Font("Microsoft Sans Serif", 24F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.computerNameLable.Location = new System.Drawing.Point(333, 21);
            this.computerNameLable.Name = "computerNameLable";
            this.computerNameLable.Size = new System.Drawing.Size(207, 37);
            this.computerNameLable.TabIndex = 2;
            this.computerNameLable.Text = "0000000000";
            this.computerNameLable.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.computerNameLable.UseMnemonic = false;
            // 
            // computerNameTextBox
            // 
            this.computerNameTextBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 24F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.computerNameTextBox.Location = new System.Drawing.Point(116, 126);
            this.computerNameTextBox.Name = "computerNameTextBox";
            this.computerNameTextBox.Size = new System.Drawing.Size(151, 44);
            this.computerNameTextBox.TabIndex = 3;
            this.computerNameTextBox.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            // 
            // passwordLable
            // 
            this.passwordLable.AutoSize = true;
            this.passwordLable.Font = new System.Drawing.Font("Microsoft Sans Serif", 24F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.passwordLable.Location = new System.Drawing.Point(333, 68);
            this.passwordLable.Name = "passwordLable";
            this.passwordLable.Size = new System.Drawing.Size(169, 37);
            this.passwordLable.TabIndex = 4;
            this.passwordLable.Text = "00000000";
            this.passwordLable.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.passwordLable.UseMnemonic = false;
            // 
            // statuseLable
            // 
            this.statuseLable.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.statuseLable.AutoSize = true;
            this.statuseLable.ForeColor = System.Drawing.Color.Red;
            this.statuseLable.Location = new System.Drawing.Point(224, 253);
            this.statuseLable.Name = "statuseLable";
            this.statuseLable.Size = new System.Drawing.Size(85, 13);
            this.statuseLable.TabIndex = 5;
            this.statuseLable.Text = "opperation failed";
            this.statuseLable.Visible = false;
            // 
            // passwordTextBox
            // 
            this.passwordTextBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 24F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.passwordTextBox.Location = new System.Drawing.Point(283, 126);
            this.passwordTextBox.Name = "passwordTextBox";
            this.passwordTextBox.PasswordChar = '*';
            this.passwordTextBox.Size = new System.Drawing.Size(151, 44);
            this.passwordTextBox.TabIndex = 6;
            this.passwordTextBox.TextAlign = System.Windows.Forms.HorizontalAlignment.Center;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(316, 173);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(84, 13);
            this.label1.TabIndex = 7;
            this.label1.Text = "Guest Password";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(143, 173);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(97, 13);
            this.label2.TabIndex = 8;
            this.label2.Text = "Guest Computer ID";
            // 
            // saveNewButton
            // 
            this.saveNewButton.Location = new System.Drawing.Point(577, 208);
            this.saveNewButton.Name = "saveNewButton";
            this.saveNewButton.Size = new System.Drawing.Size(102, 32);
            this.saveNewButton.TabIndex = 10;
            this.saveNewButton.Text = "&Save";
            this.saveNewButton.UseVisualStyleBackColor = true;
            this.saveNewButton.Click += new System.EventHandler(this.saveButton_Click);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Microsoft Sans Serif", 24F, System.Drawing.FontStyle.Bold);
            this.label3.Location = new System.Drawing.Point(35, 21);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(292, 37);
            this.label3.TabIndex = 12;
            this.label3.Text = "Your Computer ID";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Font = new System.Drawing.Font("Microsoft Sans Serif", 24F, System.Drawing.FontStyle.Bold);
            this.label4.Location = new System.Drawing.Point(78, 68);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(249, 37);
            this.label4.TabIndex = 12;
            this.label4.Text = "Your Password";
            // 
            // savedPeople
            // 
            this.savedPeople.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.savedPeople.AutoScroll = true;
            this.savedPeople.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.savedPeople.BackColor = System.Drawing.SystemColors.ControlLight;
            this.savedPeople.Location = new System.Drawing.Point(573, 25);
            this.savedPeople.Name = "savedPeople";
            this.savedPeople.Size = new System.Drawing.Size(128, 177);
            this.savedPeople.TabIndex = 13;
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(734, 286);
            this.Controls.Add(this.savedPeople);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.saveNewButton);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.passwordTextBox);
            this.Controls.Add(this.statuseLable);
            this.Controls.Add(this.passwordLable);
            this.Controls.Add(this.computerNameTextBox);
            this.Controls.Add(this.computerNameLable);
            this.Controls.Add(this.ConnectButton);
            this.Controls.Add(this.RefreshButton);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "MainForm";
            this.Text = "URemoteControl";
            this.Load += new System.EventHandler(this.MainForm_Load);
            this.Click += new System.EventHandler(this.MainForm_Click);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button RefreshButton;
        private System.Windows.Forms.Button ConnectButton;
        private System.Windows.Forms.Label computerNameLable;
        private System.Windows.Forms.TextBox computerNameTextBox;
        private System.Windows.Forms.Label passwordLable;
        private System.Windows.Forms.Label statuseLable;
        private System.Windows.Forms.TextBox passwordTextBox;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Button saveNewButton;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.FlowLayoutPanel savedPeople;
    }
}

