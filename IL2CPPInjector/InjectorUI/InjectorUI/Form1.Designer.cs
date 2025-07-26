namespace InjectorUI
{
    partial class Form1
    {
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
            // Title Bar
            this.panelTitleBar = new System.Windows.Forms.Panel();
            this.labelAppName = new System.Windows.Forms.Label();
            this.buttonMinimize = new System.Windows.Forms.Button();
            this.buttonClose = new System.Windows.Forms.Button();

            // Sidebar
            this.panelSidebar = new System.Windows.Forms.Panel();
            this.buttonHome = new System.Windows.Forms.Button();
            this.buttonSettings = new System.Windows.Forms.Button();
            this.buttonAbout = new System.Windows.Forms.Button();

            // Main Panel
            this.panelMain = new System.Windows.Forms.Panel();

            // Dashboard/Home Panel
            this.panelDashboard = new System.Windows.Forms.Panel();
            this.groupBoxDashboard = new System.Windows.Forms.GroupBox();
            this.buttonLaunchGame = new System.Windows.Forms.Button();
            this.buttonRefresh = new System.Windows.Forms.Button();
            this.labelGameStatus = new System.Windows.Forms.Label();
            this.buttonSelectDLL = new System.Windows.Forms.Button();
            this.labelDLL = new System.Windows.Forms.Label();
            this.buttonInject = new System.Windows.Forms.Button();
            this.labelStatus = new System.Windows.Forms.Label();

            // Console Log
            this.labelConsoleLog = new System.Windows.Forms.Label();
            this.groupBoxLog = new System.Windows.Forms.GroupBox();
            this.richTextStatus = new System.Windows.Forms.RichTextBox();
            this.buttonClearLog = new System.Windows.Forms.Button();

            // Settings Panel
            this.panelSettings = new System.Windows.Forms.Panel();
            this.lblSettingsTitle = new System.Windows.Forms.Label();

            this.lblPrimaryColor = new System.Windows.Forms.Label();
            this.buttonPrimaryColor = new System.Windows.Forms.Button();
            this.lblBackgroundColor = new System.Windows.Forms.Label();
            this.buttonBackgroundColor = new System.Windows.Forms.Button();
            this.lblAccentColor = new System.Windows.Forms.Label();
            this.buttonAccentColor = new System.Windows.Forms.Button();
            this.buttonSaveColors = new System.Windows.Forms.Button();
            this.lblSettingsSaved = new System.Windows.Forms.Label();

            // About Panel
            this.panelAbout = new System.Windows.Forms.Panel();
            this.lblAboutTitle = new System.Windows.Forms.Label();
            this.lblAboutVersion = new System.Windows.Forms.Label();
            this.lblAboutAuthor = new System.Windows.Forms.Label();
            this.lblAboutThanks = new System.Windows.Forms.Label();
            this.lblAboutCopyright = new System.Windows.Forms.Label();
            this.linkLabelGithub = new System.Windows.Forms.LinkLabel();

            // Footer
            this.panelFooter = new System.Windows.Forms.Panel();
            this.labelCredits = new System.Windows.Forms.Label();

            // ================= Layout & Properties =================

            // Title Bar
            this.panelTitleBar.BackColor = System.Drawing.Color.FromArgb(36, 42, 56);
            this.panelTitleBar.Controls.Add(this.labelAppName);
            this.panelTitleBar.Controls.Add(this.buttonMinimize);
            this.panelTitleBar.Controls.Add(this.buttonClose);
            this.panelTitleBar.Dock = System.Windows.Forms.DockStyle.Top;
            this.panelTitleBar.Size = new System.Drawing.Size(850, 42);
            this.panelTitleBar.MouseDown += new System.Windows.Forms.MouseEventHandler(this.TitleBar_MouseDown);

            this.labelAppName.Font = new System.Drawing.Font("Segoe UI", 18F, System.Drawing.FontStyle.Bold);
            this.labelAppName.ForeColor = System.Drawing.Color.FromArgb(81, 200, 255);
            this.labelAppName.Location = new System.Drawing.Point(14, 6);
            this.labelAppName.Size = new System.Drawing.Size(500, 32);
            this.labelAppName.Text = "ETHYRIAL INJECTOR PRO";

            this.buttonMinimize.Anchor = System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right;
            this.buttonMinimize.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.buttonMinimize.Font = new System.Drawing.Font("Segoe UI", 14F, System.Drawing.FontStyle.Bold);
            this.buttonMinimize.ForeColor = System.Drawing.Color.LightGray;
            this.buttonMinimize.Location = new System.Drawing.Point(760, 0);
            this.buttonMinimize.Size = new System.Drawing.Size(44, 42);
            this.buttonMinimize.Text = "–";
            this.buttonMinimize.Click += new System.EventHandler(this.buttonMinimize_Click);

            this.buttonClose.Anchor = System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right;
            this.buttonClose.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.buttonClose.Font = new System.Drawing.Font("Segoe UI", 14F, System.Drawing.FontStyle.Bold);
            this.buttonClose.ForeColor = System.Drawing.Color.LightGray;
            this.buttonClose.Location = new System.Drawing.Point(804, 0);
            this.buttonClose.Size = new System.Drawing.Size(44, 42);
            this.buttonClose.Text = "×";
            this.buttonClose.Click += new System.EventHandler(this.buttonClose_Click);

            // Sidebar
            this.panelSidebar.BackColor = System.Drawing.Color.FromArgb(31, 36, 46);
            this.panelSidebar.Controls.Add(this.buttonHome);
            this.panelSidebar.Controls.Add(this.buttonSettings);
            this.panelSidebar.Controls.Add(this.buttonAbout);
            this.panelSidebar.Dock = System.Windows.Forms.DockStyle.Left;
            this.panelSidebar.Size = new System.Drawing.Size(120, 488);

            this.buttonHome.BackColor = System.Drawing.Color.FromArgb(38, 120, 255);
            this.buttonHome.FlatAppearance.BorderSize = 0;
            this.buttonHome.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.buttonHome.Font = new System.Drawing.Font("Segoe UI Semibold", 11F, System.Drawing.FontStyle.Bold);
            this.buttonHome.ForeColor = System.Drawing.Color.White;
            this.buttonHome.Location = new System.Drawing.Point(12, 24);
            this.buttonHome.Size = new System.Drawing.Size(96, 38);
            this.buttonHome.Text = "Home";
            this.buttonHome.Click += new System.EventHandler(this.buttonHome_Click);

            this.buttonSettings.BackColor = System.Drawing.Color.FromArgb(31, 36, 46);
            this.buttonSettings.FlatAppearance.BorderSize = 0;
            this.buttonSettings.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.buttonSettings.Font = new System.Drawing.Font("Segoe UI", 10F);
            this.buttonSettings.ForeColor = System.Drawing.Color.FromArgb(150, 200, 255);
            this.buttonSettings.Location = new System.Drawing.Point(12, 72);
            this.buttonSettings.Size = new System.Drawing.Size(96, 34);
            this.buttonSettings.Text = "Settings";
            this.buttonSettings.Click += new System.EventHandler(this.buttonSettings_Click);

            this.buttonAbout.BackColor = System.Drawing.Color.FromArgb(31, 36, 46);
            this.buttonAbout.FlatAppearance.BorderSize = 0;
            this.buttonAbout.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.buttonAbout.Font = new System.Drawing.Font("Segoe UI", 10F);
            this.buttonAbout.ForeColor = System.Drawing.Color.FromArgb(150, 200, 255);
            this.buttonAbout.Location = new System.Drawing.Point(12, 112);
            this.buttonAbout.Size = new System.Drawing.Size(96, 34);
            this.buttonAbout.Text = "About";
            this.buttonAbout.Click += new System.EventHandler(this.buttonAbout_Click);

            // Main Panel
            this.panelMain.BackColor = System.Drawing.Color.FromArgb(34, 38, 49);
            this.panelMain.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panelMain.Location = new System.Drawing.Point(120, 42);
            this.panelMain.Size = new System.Drawing.Size(730, 488);

            // Dashboard/Home Panel
            this.panelDashboard.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panelDashboard.Visible = true;

            // Dashboard Group
            this.groupBoxDashboard.BackColor = System.Drawing.Color.Transparent;
            this.groupBoxDashboard.Controls.Add(this.buttonLaunchGame);
            this.groupBoxDashboard.Controls.Add(this.buttonRefresh);
            this.groupBoxDashboard.Controls.Add(this.labelGameStatus);
            this.groupBoxDashboard.Controls.Add(this.buttonSelectDLL);
            this.groupBoxDashboard.Controls.Add(this.labelDLL);
            this.groupBoxDashboard.Controls.Add(this.buttonInject);
            this.groupBoxDashboard.Controls.Add(this.labelStatus);
            this.groupBoxDashboard.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Bold);
            this.groupBoxDashboard.ForeColor = System.Drawing.Color.FromArgb(81, 200, 255);
            this.groupBoxDashboard.Location = new System.Drawing.Point(28, 15);
            this.groupBoxDashboard.Size = new System.Drawing.Size(674, 195);
            this.groupBoxDashboard.TabStop = false;
            this.groupBoxDashboard.Text = "Dashboard";

            this.buttonLaunchGame.BackColor = System.Drawing.Color.FromArgb(81, 200, 255);
            this.buttonLaunchGame.FlatAppearance.BorderSize = 0;
            this.buttonLaunchGame.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.buttonLaunchGame.Font = new System.Drawing.Font("Segoe UI", 11F, System.Drawing.FontStyle.Bold);
            this.buttonLaunchGame.ForeColor = System.Drawing.Color.White;
            this.buttonLaunchGame.Location = new System.Drawing.Point(15, 40);
            this.buttonLaunchGame.Size = new System.Drawing.Size(150, 38);
            this.buttonLaunchGame.Text = "Launch Game";
            this.buttonLaunchGame.Click += new System.EventHandler(this.buttonLaunchGame_Click);

            this.buttonRefresh.BackColor = System.Drawing.Color.FromArgb(62, 69, 85);
            this.buttonRefresh.FlatAppearance.BorderSize = 0;
            this.buttonRefresh.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.buttonRefresh.Font = new System.Drawing.Font("Segoe UI", 11F, System.Drawing.FontStyle.Bold);
            this.buttonRefresh.ForeColor = System.Drawing.Color.FromArgb(81, 200, 255);
            this.buttonRefresh.Location = new System.Drawing.Point(175, 40);
            this.buttonRefresh.Size = new System.Drawing.Size(40, 38);
            this.buttonRefresh.Text = "⟳";
            this.buttonRefresh.Click += new System.EventHandler(this.buttonRefresh_Click);

            this.labelGameStatus.Font = new System.Drawing.Font("Segoe UI", 11F, System.Drawing.FontStyle.Bold);
            this.labelGameStatus.ForeColor = System.Drawing.Color.LimeGreen;
            this.labelGameStatus.Location = new System.Drawing.Point(230, 40);
            this.labelGameStatus.Size = new System.Drawing.Size(200, 38);
            this.labelGameStatus.Text = "Game: Not Running";
            this.labelGameStatus.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;

            this.buttonSelectDLL.BackColor = System.Drawing.Color.FromArgb(38, 120, 255);
            this.buttonSelectDLL.FlatAppearance.BorderSize = 0;
            this.buttonSelectDLL.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.buttonSelectDLL.Font = new System.Drawing.Font("Segoe UI", 11F, System.Drawing.FontStyle.Bold);
            this.buttonSelectDLL.ForeColor = System.Drawing.Color.White;
            this.buttonSelectDLL.Location = new System.Drawing.Point(15, 90);
            this.buttonSelectDLL.Size = new System.Drawing.Size(150, 38);
            this.buttonSelectDLL.Text = "Select DLL";
            this.buttonSelectDLL.Click += new System.EventHandler(this.buttonSelectDLL_Click);

            this.labelDLL.Font = new System.Drawing.Font("Segoe UI", 11F, System.Drawing.FontStyle.Italic);
            this.labelDLL.ForeColor = System.Drawing.Color.FromArgb(220, 220, 220);
            this.labelDLL.Location = new System.Drawing.Point(175, 90);
            this.labelDLL.Size = new System.Drawing.Size(470, 38);
            this.labelDLL.Text = "No DLL selected.";
            this.labelDLL.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;

            this.buttonInject.BackColor = System.Drawing.Color.FromArgb(60, 220, 120);
            this.buttonInject.FlatAppearance.BorderSize = 0;
            this.buttonInject.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.buttonInject.Font = new System.Drawing.Font("Segoe UI", 11F, System.Drawing.FontStyle.Bold);
            this.buttonInject.ForeColor = System.Drawing.Color.White;
            this.buttonInject.Location = new System.Drawing.Point(15, 140);
            this.buttonInject.Size = new System.Drawing.Size(150, 38);
            this.buttonInject.Text = "Inject";
            this.buttonInject.Click += new System.EventHandler(this.buttonInject_Click);

            this.labelStatus.Font = new System.Drawing.Font("Segoe UI", 11F, System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Italic);
            this.labelStatus.ForeColor = System.Drawing.Color.Gold;
            this.labelStatus.Location = new System.Drawing.Point(175, 140);
            this.labelStatus.Size = new System.Drawing.Size(470, 38);
            this.labelStatus.Text = "Status: Waiting...";
            this.labelStatus.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;

            // Console Log Label
            this.labelConsoleLog.Text = "Console Log:";
            this.labelConsoleLog.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Bold);
            this.labelConsoleLog.ForeColor = System.Drawing.Color.FromArgb(81, 200, 255);
            this.labelConsoleLog.Location = new System.Drawing.Point(40, 220);
            this.labelConsoleLog.Size = new System.Drawing.Size(140, 28);

            // Console Log (GroupBox)
            this.groupBoxLog.Location = new System.Drawing.Point(28, 250);
            this.groupBoxLog.Size = new System.Drawing.Size(674, 160);
            this.groupBoxLog.BackColor = System.Drawing.Color.Transparent;
            this.groupBoxLog.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.groupBoxLog.TabStop = false;
            this.groupBoxLog.Text = "";

            // Console Log RichTextBox
            this.richTextStatus.Location = new System.Drawing.Point(12, 18);
            this.richTextStatus.Size = new System.Drawing.Size(650, 100);
            this.richTextStatus.ReadOnly = true;
            this.richTextStatus.BackColor = System.Drawing.Color.FromArgb(28, 32, 39);
            this.richTextStatus.ForeColor = System.Drawing.Color.FromArgb(199, 255, 190);
            this.richTextStatus.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.richTextStatus.Font = new System.Drawing.Font("Cascadia Mono", 10.5F);

            // Button to clear log
            this.buttonClearLog.Text = "Clear Log";
            this.buttonClearLog.BackColor = System.Drawing.Color.FromArgb(64, 69, 85);
            this.buttonClearLog.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.buttonClearLog.ForeColor = System.Drawing.Color.FromArgb(120, 180, 255);
            this.buttonClearLog.Font = new System.Drawing.Font("Segoe UI", 10F, System.Drawing.FontStyle.Bold);
            this.buttonClearLog.Location = new System.Drawing.Point(560, 120);
            this.buttonClearLog.Size = new System.Drawing.Size(90, 28);
            this.buttonClearLog.Click += new System.EventHandler(this.buttonClearLog_Click);

            this.groupBoxLog.Controls.Add(this.richTextStatus);
            this.groupBoxLog.Controls.Add(this.buttonClearLog);

            // Add Console Log label and log groupBox to panelDashboard
            this.panelDashboard.Controls.Add(this.labelConsoleLog);
            this.panelDashboard.Controls.Add(this.groupBoxLog);

            // Add dashboard group
            this.panelDashboard.Controls.Add(this.groupBoxDashboard);

            // Settings Panel (Appearance Only)
            this.panelSettings.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panelSettings.Visible = false;
            this.panelSettings.BackColor = System.Drawing.Color.FromArgb(34, 38, 49);

            this.lblSettingsTitle.Text = "Settings";
            this.lblSettingsTitle.Font = new System.Drawing.Font("Segoe UI", 20F, System.Drawing.FontStyle.Bold);
            this.lblSettingsTitle.ForeColor = System.Drawing.Color.FromArgb(81, 200, 255);
            this.lblSettingsTitle.Location = new System.Drawing.Point(40, 30);
            this.lblSettingsTitle.Size = new System.Drawing.Size(200, 40);

            // Primary Color
            this.lblPrimaryColor.Text = "Primary Color:";
            this.lblPrimaryColor.Font = new System.Drawing.Font("Segoe UI", 11F);
            this.lblPrimaryColor.ForeColor = System.Drawing.Color.WhiteSmoke;
            this.lblPrimaryColor.Location = new System.Drawing.Point(60, 90);
            this.lblPrimaryColor.Size = new System.Drawing.Size(150, 25);

            this.buttonPrimaryColor.Text = "Choose...";
            this.buttonPrimaryColor.Font = new System.Drawing.Font("Segoe UI", 11F);
            this.buttonPrimaryColor.BackColor = System.Drawing.Color.FromArgb(81, 200, 255);
            this.buttonPrimaryColor.ForeColor = System.Drawing.Color.White;
            this.buttonPrimaryColor.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.buttonPrimaryColor.Location = new System.Drawing.Point(220, 90);
            this.buttonPrimaryColor.Size = new System.Drawing.Size(90, 27);
            this.buttonPrimaryColor.Click += new System.EventHandler(this.buttonPrimaryColor_Click);

            // Background Color
            this.lblBackgroundColor.Text = "Background Color:";
            this.lblBackgroundColor.Font = new System.Drawing.Font("Segoe UI", 11F);
            this.lblBackgroundColor.ForeColor = System.Drawing.Color.WhiteSmoke;
            this.lblBackgroundColor.Location = new System.Drawing.Point(60, 130);
            this.lblBackgroundColor.Size = new System.Drawing.Size(150, 25);

            this.buttonBackgroundColor.Text = "Choose...";
            this.buttonBackgroundColor.Font = new System.Drawing.Font("Segoe UI", 11F);
            this.buttonBackgroundColor.BackColor = System.Drawing.Color.FromArgb(34, 38, 49);
            this.buttonBackgroundColor.ForeColor = System.Drawing.Color.White;
            this.buttonBackgroundColor.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.buttonBackgroundColor.Location = new System.Drawing.Point(220, 130);
            this.buttonBackgroundColor.Size = new System.Drawing.Size(90, 27);
            this.buttonBackgroundColor.Click += new System.EventHandler(this.buttonBackgroundColor_Click);

            // Accent Color
            this.lblAccentColor.Text = "Accent Color:";
            this.lblAccentColor.Font = new System.Drawing.Font("Segoe UI", 11F);
            this.lblAccentColor.ForeColor = System.Drawing.Color.WhiteSmoke;
            this.lblAccentColor.Location = new System.Drawing.Point(60, 170);
            this.lblAccentColor.Size = new System.Drawing.Size(150, 25);

            this.buttonAccentColor.Text = "Choose...";
            this.buttonAccentColor.Font = new System.Drawing.Font("Segoe UI", 11F);
            this.buttonAccentColor.BackColor = System.Drawing.Color.FromArgb(120, 180, 255);
            this.buttonAccentColor.ForeColor = System.Drawing.Color.White;
            this.buttonAccentColor.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.buttonAccentColor.Location = new System.Drawing.Point(220, 170);
            this.buttonAccentColor.Size = new System.Drawing.Size(90, 27);
            this.buttonAccentColor.Click += new System.EventHandler(this.buttonAccentColor_Click);

            // Save Colors Button
            this.buttonSaveColors.Text = "Save Colors";
            this.buttonSaveColors.Font = new System.Drawing.Font("Segoe UI", 11F, System.Drawing.FontStyle.Bold);
            this.buttonSaveColors.BackColor = System.Drawing.Color.FromArgb(60, 220, 120);
            this.buttonSaveColors.ForeColor = System.Drawing.Color.White;
            this.buttonSaveColors.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.buttonSaveColors.Location = new System.Drawing.Point(220, 210);
            this.buttonSaveColors.Size = new System.Drawing.Size(120, 36);
            this.buttonSaveColors.Click += new System.EventHandler(this.buttonSaveColors_Click);

            this.lblSettingsSaved.Text = "";
            this.lblSettingsSaved.Font = new System.Drawing.Font("Segoe UI", 10F, System.Drawing.FontStyle.Italic);
            this.lblSettingsSaved.ForeColor = System.Drawing.Color.LimeGreen;
            this.lblSettingsSaved.Location = new System.Drawing.Point(360, 218);
            this.lblSettingsSaved.Size = new System.Drawing.Size(300, 22);

            // Add Controls
            this.panelSettings.Controls.Clear();
            this.panelSettings.Controls.Add(this.lblSettingsTitle);
            this.panelSettings.Controls.Add(this.lblPrimaryColor);
            this.panelSettings.Controls.Add(this.buttonPrimaryColor);
            this.panelSettings.Controls.Add(this.lblBackgroundColor);
            this.panelSettings.Controls.Add(this.buttonBackgroundColor);
            this.panelSettings.Controls.Add(this.lblAccentColor);
            this.panelSettings.Controls.Add(this.buttonAccentColor);
            this.panelSettings.Controls.Add(this.buttonSaveColors);
            this.panelSettings.Controls.Add(this.lblSettingsSaved);

            // About Panel
            this.panelAbout.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panelAbout.Visible = false;
            this.panelAbout.BackColor = System.Drawing.Color.FromArgb(34, 38, 49);

            this.lblAboutTitle.Text = "About Ethyrial Injector Pro";
            this.lblAboutTitle.Font = new System.Drawing.Font("Segoe UI", 20F, System.Drawing.FontStyle.Bold);
            this.lblAboutTitle.ForeColor = System.Drawing.Color.FromArgb(81, 200, 255);
            this.lblAboutTitle.Location = new System.Drawing.Point(40, 30);
            this.lblAboutTitle.Size = new System.Drawing.Size(400, 40);

            this.lblAboutVersion.Text = "Version 2.0";
            this.lblAboutVersion.Font = new System.Drawing.Font("Segoe UI", 13F, System.Drawing.FontStyle.Bold);
            this.lblAboutVersion.ForeColor = System.Drawing.Color.White;
            this.lblAboutVersion.Location = new System.Drawing.Point(45, 80);
            this.lblAboutVersion.Size = new System.Drawing.Size(200, 28);

            this.lblAboutAuthor.Text = "Created by MrJambix";
            this.lblAboutAuthor.Font = new System.Drawing.Font("Segoe UI", 12F);
            this.lblAboutAuthor.ForeColor = System.Drawing.Color.WhiteSmoke;
            this.lblAboutAuthor.Location = new System.Drawing.Point(45, 110);
            this.lblAboutAuthor.Size = new System.Drawing.Size(220, 25);

            this.lblAboutThanks.Text = "Special thanks to the Ethyrial Development Team,\nand especially Ryan for his amazing dedication and work on Ethyrial: Echoes of Yore.";
            this.lblAboutThanks.Font = new System.Drawing.Font("Segoe UI", 11F, System.Drawing.FontStyle.Bold);
            this.lblAboutThanks.ForeColor = System.Drawing.Color.Gold;
            this.lblAboutThanks.Location = new System.Drawing.Point(45, 150);
            this.lblAboutThanks.Size = new System.Drawing.Size(600, 60);

            this.lblAboutCopyright.Text = "© 2025 MrJambix. All rights reserved.";
            this.lblAboutCopyright.Font = new System.Drawing.Font("Segoe UI", 10F, System.Drawing.FontStyle.Italic);
            this.lblAboutCopyright.ForeColor = System.Drawing.Color.Silver;
            this.lblAboutCopyright.Location = new System.Drawing.Point(45, 220);
            this.lblAboutCopyright.Size = new System.Drawing.Size(400, 20);

            this.linkLabelGithub.Text = "GitHub: https://github.com/MrJambix";
            this.linkLabelGithub.Font = new System.Drawing.Font("Segoe UI", 10F, System.Drawing.FontStyle.Underline);
            this.linkLabelGithub.LinkColor = System.Drawing.Color.FromArgb(81, 200, 255);
            this.linkLabelGithub.Location = new System.Drawing.Point(45, 250);
            this.linkLabelGithub.Size = new System.Drawing.Size(400, 20);
            this.linkLabelGithub.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabelGithub_LinkClicked);

            this.panelAbout.Controls.Add(this.lblAboutTitle);
            this.panelAbout.Controls.Add(this.lblAboutVersion);
            this.panelAbout.Controls.Add(this.lblAboutAuthor);
            this.panelAbout.Controls.Add(this.lblAboutThanks);
            this.panelAbout.Controls.Add(this.lblAboutCopyright);
            this.panelAbout.Controls.Add(this.linkLabelGithub);

            // Add all main sub-panels to main panel
            this.panelMain.Controls.Add(this.panelDashboard);
            this.panelMain.Controls.Add(this.panelSettings);
            this.panelMain.Controls.Add(this.panelAbout);

            // Footer
            this.panelFooter.BackColor = System.Drawing.Color.FromArgb(27, 32, 40);
            this.panelFooter.Controls.Add(this.labelCredits);
            this.panelFooter.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panelFooter.Size = new System.Drawing.Size(850, 34);

            this.labelCredits.Font = new System.Drawing.Font("Segoe UI", 10.5F, System.Drawing.FontStyle.Italic);
            this.labelCredits.ForeColor = System.Drawing.Color.FromArgb(130, 180, 200);
            this.labelCredits.Location = new System.Drawing.Point(14, 7);
            this.labelCredits.Size = new System.Drawing.Size(500, 24);
            this.labelCredits.Text = "© 2025 MrJambix | All rights reserved | v2.0 Pro";

            // Compose main form
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None;
            this.BackColor = System.Drawing.Color.FromArgb(34, 38, 49);
            this.ClientSize = new System.Drawing.Size(850, 564);
            this.Controls.Add(this.panelMain);
            this.Controls.Add(this.panelSidebar);
            this.Controls.Add(this.panelTitleBar);
            this.Controls.Add(this.panelFooter);
            this.Font = new System.Drawing.Font("Segoe UI", 11.25F, System.Drawing.FontStyle.Regular);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.None;
            this.MaximizeBox = false;
            this.Name = "Form1";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Ethyrial Injector Pro | MrJambix";
        }

        #endregion

        // Title bar
        private System.Windows.Forms.Panel panelTitleBar;
        private System.Windows.Forms.Label labelAppName;
        private System.Windows.Forms.Button buttonMinimize;
        private System.Windows.Forms.Button buttonClose;

        // Sidebar
        private System.Windows.Forms.Panel panelSidebar;
        private System.Windows.Forms.Button buttonHome;
        private System.Windows.Forms.Button buttonSettings;
        private System.Windows.Forms.Button buttonAbout;

        // Main
        private System.Windows.Forms.Panel panelMain;

        // Dashboard
        private System.Windows.Forms.Panel panelDashboard;
        private System.Windows.Forms.GroupBox groupBoxDashboard;
        private System.Windows.Forms.Button buttonLaunchGame;
        private System.Windows.Forms.Button buttonRefresh;
        private System.Windows.Forms.Label labelGameStatus;
        private System.Windows.Forms.Button buttonSelectDLL;
        private System.Windows.Forms.Label labelDLL;
        private System.Windows.Forms.Button buttonInject;
        private System.Windows.Forms.Label labelStatus;

        // Console Log
        private System.Windows.Forms.Label labelConsoleLog;
        private System.Windows.Forms.GroupBox groupBoxLog;
        private System.Windows.Forms.RichTextBox richTextStatus;
        private System.Windows.Forms.Button buttonClearLog;

        // Settings
        private System.Windows.Forms.Panel panelSettings;
        private System.Windows.Forms.Label lblSettingsTitle;
        private System.Windows.Forms.Label lblPrimaryColor;
        private System.Windows.Forms.Button buttonPrimaryColor;
        private System.Windows.Forms.Label lblBackgroundColor;
        private System.Windows.Forms.Button buttonBackgroundColor;
        private System.Windows.Forms.Label lblAccentColor;
        private System.Windows.Forms.Button buttonAccentColor;
        private System.Windows.Forms.Button buttonSaveColors;
        private System.Windows.Forms.Label lblSettingsSaved;

        // About
        private System.Windows.Forms.Panel panelAbout;
        private System.Windows.Forms.Label lblAboutTitle;
        private System.Windows.Forms.Label lblAboutVersion;
        private System.Windows.Forms.Label lblAboutAuthor;
        private System.Windows.Forms.Label lblAboutThanks;
        private System.Windows.Forms.Label lblAboutCopyright;
        private System.Windows.Forms.LinkLabel linkLabelGithub;

        // Footer
        private System.Windows.Forms.Panel panelFooter;
        private System.Windows.Forms.Label labelCredits;
    }
}