namespace URemoteControlGUI
{
    partial class TextMbForm
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
            this.renameTextBox = new System.Windows.Forms.TextBox();
            this.RenameLable = new System.Windows.Forms.Label();
            this.finishButton = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // renameTextBox
            // 
            this.renameTextBox.Location = new System.Drawing.Point(120, 78);
            this.renameTextBox.Name = "renameTextBox";
            this.renameTextBox.Size = new System.Drawing.Size(100, 20);
            this.renameTextBox.TabIndex = 0;
            // 
            // RenameLable
            // 
            this.RenameLable.AutoSize = true;
            this.RenameLable.Font = new System.Drawing.Font("Microsoft Sans Serif", 20.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.RenameLable.Location = new System.Drawing.Point(102, 24);
            this.RenameLable.Name = "RenameLable";
            this.RenameLable.Size = new System.Drawing.Size(138, 31);
            this.RenameLable.TabIndex = 1;
            this.RenameLable.Text = "new name";
            // 
            // finishButton
            // 
            this.finishButton.Location = new System.Drawing.Point(134, 131);
            this.finishButton.Name = "finishButton";
            this.finishButton.Size = new System.Drawing.Size(75, 23);
            this.finishButton.TabIndex = 2;
            this.finishButton.Text = "finish";
            this.finishButton.UseVisualStyleBackColor = true;
            this.finishButton.Click += new System.EventHandler(this.finishButton_Click);
            // 
            // TextMbForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(322, 176);
            this.Controls.Add(this.finishButton);
            this.Controls.Add(this.RenameLable);
            this.Controls.Add(this.renameTextBox);
            this.Name = "TextMbForm";
            this.Text = "Form2";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox renameTextBox;
        private System.Windows.Forms.Label RenameLable;
        private System.Windows.Forms.Button finishButton;
    }
}