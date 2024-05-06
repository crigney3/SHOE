using System.IO;
using System.IO.Compression;
using System.Net;
using System.Text;
using System.ComponentModel;
using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;

enum LauncherState
{
    Initializing,
    UpdatingLauncher,
    UpdatingSHOE,
    Running,
    LaunchingProject
}

namespace SHOELauncher
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private string launcherPath;
        private string versionFilePath;
        private string SHOEExecPath;
        private string selectedProjectPath;

        private LauncherState launcherState;
        internal LauncherState LauncherState
        {
            get => launcherState;
            set
            {
                launcherState = value;
                // Add switch here to update UI elements when launcher state changes
                // not important for now
            }
        }

        public MainWindow()
        {
            InitializeComponent();

            launcherPath = Directory.GetCurrentDirectory();
            versionFilePath = Path.Combine(launcherPath, "version.txt");
            SHOEExecPath = Path.Combine(launcherPath, "SHOE.exe");
        }

        private void Launcher_ContentRendered(object sender, EventArgs e)
        {

        }

        private void LaunchSHOEButton_Click(object sender, RoutedEventArgs e)
        {

        }
    }

    struct SHOEVersion
    {
        internal static SHOEVersion zero = new SHOEVersion(0, 0, 0);

        private short major, minor, subMinor;

        internal SHOEVersion(short _major, short _minor, short _subMinor) 
        {
            major = _major;
            minor = _minor;
            subMinor = _subMinor;
        }

        internal SHOEVersion(string _versionString)
        {
            string[] _versionStrings = _versionString.Split('.');
            if (_versionStrings.Length != 3 )
            {
                major = 0;
                minor = 0;
                subMinor = 0;
                return;
            }

            major = short.Parse(_versionStrings[0]);
            minor = short.Parse(_versionStrings[1]);
            subMinor = short.Parse(_versionStrings[2]);
        }

        internal bool HasVersionChanged(SHOEVersion _versionToCheck)
        {
            if (major != _versionToCheck.major)
            {
                return true;
            }
            if (minor != _versionToCheck.minor)
            {
                return true;
            }
            if (subMinor != _versionToCheck.subMinor)
            {
                return true;
            }
            return false;
        }

        public override string ToString()
        {
            return $"{major}.{minor}.{subMinor}";
        }
    }
}