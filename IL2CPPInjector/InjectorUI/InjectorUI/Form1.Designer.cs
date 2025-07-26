namespace InjectorUI
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        private void InitializeComponent()
        {
            this.labelTitle = new System.Windows.Forms.Label();
            this.richTextStatus = new System.Windows.Forms.RichTextBox();
            this.buttonSelectDLL = new System.Windows.Forms.Button();
            this.buttonInject = new System.Windows.Forms.Button();
            this.labelCredits = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // labelTitle
            // 
            this.labelTitle.AutoSize = true;
            this.labelTitle.Font = new System.Drawing.Font("Segoe UI", 16F, System.Drawing.FontStyle.Bold);
            this.labelTitle.Location = new System.Drawing.Point(30, 20);
            this.labelTitle.Name = "labelTitle";
            this.labelTitle.Size = new System.Drawing.Size(363, 37);
            this.labelTitle.TabIndex = 0;
            this.labelTitle.Text = "Ethyrial Injector by MrJambix";
            // 
            // richTextStatus
            // 
            this.richTextStatus.Location = new System.Drawing.Point(30, 70);
            this.richTextStatus.Name = "richTextStatus";
            this.richTextStatus.ReadOnly = true;
            this.richTextStatus.Size = new System.Drawing.Size(420, 160);
            this.richTextStatus.TabIndex = 1;
            this.richTextStatus.Text = "";
            // 
            // buttonSelectDLL
            // 
            this.buttonSelectDLL.Location = new System.Drawing.Point(30, 240);
            this.buttonSelectDLL.Name = "buttonSelectDLL";
            this.buttonSelectDLL.Size = new System.Drawing.Size(130, 40);
            this.buttonSelectDLL.TabIndex = 2;
            this.buttonSelectDLL.Text = "Select DLL";
            this.buttonSelectDLL.UseVisualStyleBackColor = true;
            this.buttonSelectDLL.Click += new System.EventHandler(this.buttonSelectDLL_Click);
            // 
            // buttonInject
            // 
            this.buttonInject.Location = new System.Drawing.Point(180, 240);
            this.buttonInject.Name = "buttonInject";
            this.buttonInject.Size = new System.Drawing.Size(130, 40);
            this.buttonInject.TabIndex = 3;
            this.buttonInject.Text = "Inject";
            this.buttonInject.UseVisualStyleBackColor = true;
            this.buttonInject.Click += new System.EventHandler(this.buttonInject_Click);
            // 
            // labelCredits
            // 
            this.labelCredits.AutoSize = true;
            this.labelCredits.Font = new System.Drawing.Font("Segoe UI", 10F, System.Drawing.FontStyle.Italic);
            this.labelCredits.Location = new System.Drawing.Point(30, 300);
            this.labelCredits.Name = "labelCredits";
            this.labelCredits.Size = new System.Drawing.Size(153, 23);
            this.labelCredits.TabIndex = 4;
            this.labelCredits.Text = "Made by MrJambix";
            // 
            // Form1
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(500, 350);
            this.Controls.Add(this.labelCredits);
            this.Controls.Add(this.buttonInject);
            this.Controls.Add(this.buttonSelectDLL);
            this.Controls.Add(this.richTextStatus);
            this.Controls.Add(this.labelTitle);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.Name = "Form1";
            this.Text = "Ethyrial Injector";
            this.ResumeLayout(false);
            this.PerformLayout();
        }

        #endregion

        private System.Windows.Forms.Label labelTitle;
        private System.Windows.Forms.RichTextBox richTextStatus;
        private System.Windows.Forms.Button buttonSelectDLL;
        private System.Windows.Forms.Button buttonInject;
        private System.Windows.Forms.Label labelCredits;
    }
}