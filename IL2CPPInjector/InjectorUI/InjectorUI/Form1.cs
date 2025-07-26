using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace InjectorUI
{
    public partial class Form1 : Form
    {
        private string selectedDLLPath = "";
        private string settingsFile = "settings.cfg";

        // Ethyrial Steam AppID from the official Steam page
        private const string ETHYRIAL_STEAM_APPID = "1277920";

        public Form1()
        {
            InitializeComponent();
            LoadSettings();
            ShowDashboard();
            Log("Welcome to Ethyrial Injector Pro!");
            Log("Ready. Select a DLL and launch or inject.");
            CheckGameOnStartup();
        }

        // ---------------------- NAVIGATION ----------------------
        private void ShowDashboard()
        {
            panelDashboard.Visible = true;
            panelSettings.Visible = false;
            panelAbout.Visible = false;
        }
        private void ShowSettings()
        {
            panelDashboard.Visible = false;
            panelSettings.Visible = true;
            panelAbout.Visible = false;
        }
        private void ShowAbout()
        {
            panelDashboard.Visible = false;
            panelSettings.Visible = false;
            panelAbout.Visible = true;
        }

        private void buttonHome_Click(object sender, EventArgs e) => ShowDashboard();
        private void buttonSettings_Click(object sender, EventArgs e) => ShowSettings();
        private void buttonAbout_Click(object sender, EventArgs e) => ShowAbout();

        // ---------------------- TITLE BAR ----------------------
        private void buttonMinimize_Click(object sender, EventArgs e)
        {
            this.WindowState = FormWindowState.Minimized;
        }
        private void buttonClose_Click(object sender, EventArgs e)
        {
            this.Close();
        }
        private void TitleBar_MouseDown(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                NativeMethods.ReleaseCapture();
                NativeMethods.SendMessage(this.Handle, 0xA1, 0x2, 0);
            }
        }
        private static class NativeMethods
        {
            [DllImport("user32.dll")]
            public static extern bool ReleaseCapture();
            [DllImport("user32.dll")]
            public static extern int SendMessage(IntPtr hWnd, int Msg, int wParam, int lParam);
        }

        // ---------------------- LOG ----------------------
        private void Log(string message)
        {
            if (richTextStatus.InvokeRequired)
            {
                richTextStatus.Invoke(new Action(() =>
                {
                    richTextStatus.AppendText($"[{DateTime.Now:HH:mm:ss}] {message}{Environment.NewLine}");
                    richTextStatus.ScrollToCaret();
                }));
            }
            else
            {
                richTextStatus.AppendText($"[{DateTime.Now:HH:mm:ss}] {message}{Environment.NewLine}");
                richTextStatus.ScrollToCaret();
            }
        }

        private void buttonClearLog_Click(object sender, EventArgs e)
        {
            richTextStatus.Clear();
        }

        // ---------------------- DASHBOARD CORE ----------------------

        private string GetEthyrialExePath()
        {
            if (!string.IsNullOrWhiteSpace(txtExePath.Text))
                return txtExePath.Text.Trim();
            // fallback
            return @"C:\Program Files\Ethyrial\Ethyrial.exe";
        }

        private void UpdateSelectedDLLLabel()
        {
            labelDLL.Text = string.IsNullOrEmpty(selectedDLLPath) ? "No DLL selected." : selectedDLLPath;
        }

        private void UpdateStatusLabel(string status, Color? color = null)
        {
            labelStatus.Text = status;
            labelStatus.ForeColor = color ?? Color.Gold;
        }

        private void UpdateGameStatusLabel(bool running)
        {
            if (running)
            {
                labelGameStatus.Text = "Game: Running";
                labelGameStatus.ForeColor = Color.LimeGreen;
            }
            else
            {
                labelGameStatus.Text = "Game: Not Running";
                labelGameStatus.ForeColor = Color.OrangeRed;
            }
        }

        private void CheckGameOnStartup()
        {
            var process = Process.GetProcessesByName("Ethyrial").FirstOrDefault();
            UpdateGameStatusLabel(process != null);
            if (process == null)
            {
                UpdateStatusLabel("Status: Game not found.", Color.OrangeRed);
            }
            else
            {
                UpdateStatusLabel("Status: Game found!", Color.LimeGreen);
            }
        }

        private void buttonSelectDLL_Click(object sender, EventArgs e)
        {
            using (OpenFileDialog ofd = new OpenFileDialog())
            {
                ofd.Filter = "DLL files (*.dll)|*.dll";
                ofd.Title = "Select DLL to Inject";
                if (ofd.ShowDialog() == DialogResult.OK)
                {
                    selectedDLLPath = ofd.FileName;
                    Log($"Selected DLL: {selectedDLLPath}");
                    UpdateSelectedDLLLabel();
                }
            }
        }

        private async void buttonInject_Click(object sender, EventArgs e)
        {
            if (string.IsNullOrEmpty(selectedDLLPath) || !File.Exists(selectedDLLPath))
            {
                Log("Please select a valid DLL first.");
                UpdateStatusLabel("Status: Invalid DLL!", Color.OrangeRed);
                return;
            }

            Log("Searching for Ethyrial.exe...");
            UpdateStatusLabel("Status: Searching for Ethyrial.exe...", Color.Gold);

            var process = Process.GetProcessesByName("Ethyrial").FirstOrDefault();
            if (process == null)
            {
                Log("Could not find Ethyrial.exe! Please launch the game first.");
                UpdateGameStatusLabel(false);
                UpdateStatusLabel("Status: Ethyrial.exe not found!", Color.OrangeRed);
                return;
            }

            Log("Found Ethyrial.exe! Attempting injection...");
            UpdateGameStatusLabel(true);
            UpdateStatusLabel("Status: Injecting...", Color.DeepSkyBlue);

            bool result = await Task.Run(() => InjectDLL(process.Id, selectedDLLPath));
            if (result)
            {
                Log("DLL injected successfully!");
                UpdateStatusLabel("Status: DLL injected successfully!", Color.LimeGreen);
            }
            else
            {
                Log("DLL injection failed!");
                UpdateStatusLabel("Status: DLL injection failed!", Color.OrangeRed);
            }
        }

        private void buttonLaunchGame_Click(object sender, EventArgs e)
        {
            try
            {
                Process.Start(new ProcessStartInfo
                {
                    FileName = $"steam://run/{ETHYRIAL_STEAM_APPID}",
                    UseShellExecute = true
                });
                Log("Requested launch via Steam.");
                // Wait a few seconds and refresh status
                Task.Delay(4000).ContinueWith(_ => Invoke(new Action(CheckGameOnStartup)));
            }
            catch (Exception ex)
            {
                Log("Failed to launch game via Steam: " + ex.Message);
            }
        }

        private void buttonRefresh_Click(object sender, EventArgs e)
        {
            CheckGameOnStartup();
        }

        // DLL Injection logic
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr OpenProcess(uint dwDesiredAccess, bool bInheritHandle, int dwProcessId);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr VirtualAllocEx(IntPtr hProcess, IntPtr lpAddress, uint dwSize, uint flAllocationType, uint flProtect);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool WriteProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, uint nSize, out UIntPtr lpNumberOfBytesWritten);

        [DllImport("kernel32.dll")]
        static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr GetModuleHandle(string lpModuleName);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr CreateRemoteThread(IntPtr hProcess, IntPtr lpThreadAttributes,
            uint dwStackSize, IntPtr lpStartAddress, IntPtr lpParameter, uint dwCreationFlags, IntPtr lpThreadId);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool CloseHandle(IntPtr hObject);

        private bool InjectDLL(int processId, string dllPath)
        {
            const uint PROCESS_ALL_ACCESS = 0x1F0FFF;
            const uint MEM_COMMIT = 0x1000;
            const uint PAGE_READWRITE = 0x04;

            IntPtr hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, processId);
            if (hProcess == IntPtr.Zero) return false;

            IntPtr addr = VirtualAllocEx(hProcess, IntPtr.Zero, (uint)((dllPath.Length + 1) * Marshal.SizeOf(typeof(char))),
                MEM_COMMIT, PAGE_READWRITE);
            if (addr == IntPtr.Zero) return false;

            byte[] bytes = System.Text.Encoding.ASCII.GetBytes(dllPath);
            if (!WriteProcessMemory(hProcess, addr, bytes, (uint)bytes.Length, out _)) return false;

            IntPtr loadLibraryAddr = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
            if (loadLibraryAddr == IntPtr.Zero) return false;

            IntPtr hThread = CreateRemoteThread(hProcess, IntPtr.Zero, 0, loadLibraryAddr, addr, 0, IntPtr.Zero);
            if (hThread == IntPtr.Zero) return false;

            WaitForSingleObject(hThread, 5000);

            CloseHandle(hThread);
            CloseHandle(hProcess);
            return true;
        }

        // ---------------------- SETTINGS ----------------------
        private void LoadSettings()
        {
            try
            {
                if (File.Exists(settingsFile))
                {
                    var lines = File.ReadAllLines(settingsFile);
                    foreach (var line in lines)
                    {
                        var kv = line.Split(new[] { '=' }, 2);
                        if (kv.Length == 2)
                        {
                            if (kv[0].Trim() == "EthyrialExePath")
                                txtExePath.Text = kv[1].Trim();
                        }
                    }
                }
            }
            catch
            {
                txtExePath.Text = @"C:\Program Files\Ethyrial\Ethyrial.exe";
            }
        }

        private void SaveSettings()
        {
            try
            {
                File.WriteAllText(settingsFile, $"EthyrialExePath={txtExePath.Text.Trim()}\n");
                lblSettingsSaved.Text = "Settings saved!";
            }
            catch
            {
                lblSettingsSaved.Text = "Error saving settings!";
            }
        }

        private void buttonBrowseExe_Click(object sender, EventArgs e)
        {
            using (OpenFileDialog ofd = new OpenFileDialog())
            {
                ofd.Filter = "EXE files (*.exe)|*.exe";
                ofd.Title = "Select Ethyrial.exe";
                if (ofd.ShowDialog() == DialogResult.OK)
                {
                    txtExePath.Text = ofd.FileName;
                }
            }
        }
        private void buttonSaveSettings_Click(object sender, EventArgs e)
        {
            SaveSettings();
        }

        // ---------------------- ABOUT ----------------------
        private void linkLabelGithub_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            Process.Start(new ProcessStartInfo
            {
                FileName = "https://github.com/MrJambix",
                UseShellExecute = true
            });
        }
    }
}