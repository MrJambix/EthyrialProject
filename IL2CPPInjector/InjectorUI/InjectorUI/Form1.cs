using System;
using System.Diagnostics;
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

        public Form1()
        {
            InitializeComponent();

            // Log welcome message and check for game on startup
            Log("Welcome, please select DLL path.");
            Log("Checking if game is running...");
            CheckGameOnStartup();
        }

        private void Log(string message)
        {
            richTextStatus.AppendText(message + Environment.NewLine);
        }

        private void CheckGameOnStartup()
        {
            var process = Process.GetProcessesByName("Ethyrial").FirstOrDefault();
            if (process == null)
            {
                Log("Game not found!");
            }
            else
            {
                Log("Game found!");
            }
        }

        private void buttonSelectDLL_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog();
            ofd.Filter = "DLL files (*.dll)|*.dll";
            if (ofd.ShowDialog() == DialogResult.OK)
            {
                selectedDLLPath = ofd.FileName;
                Log($"Selected DLL: {selectedDLLPath}");
            }
        }

        private async void buttonInject_Click(object sender, EventArgs e)
        {
            if (string.IsNullOrEmpty(selectedDLLPath) || !File.Exists(selectedDLLPath))
            {
                Log("Please select a valid DLL first.");
                return;
            }

            Log("Searching for Ethyrial.exe...");

            var process = Process.GetProcessesByName("Ethyrial").FirstOrDefault();
            if (process == null)
            {
                Log("Could not find Ethyrial.exe! Please start the game first.");
                return;
            }

            Log("Found Ethyrial.exe! Attempting injection...");

            bool result = await Task.Run(() => InjectDLL(process.Id, selectedDLLPath));
            if (result)
            {
                Log("DLL injected successfully!");
            }
            else
            {
                Log("DLL injection failed!");
            }
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
    }
}