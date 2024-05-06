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
    FirstSHOEInstall,
    UpdatingSHOE,
    Running,
    LaunchingProject,
    Failed
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
        private string SHOEOnlineZipPath;
        private string SHOELocalZipPath;
        private string SHOELocalZipName;
        private string SHOEBuildPath;

        private LauncherState launcherStatus;
        internal LauncherState LauncherStatus
        {
            get => launcherStatus;
            set
            {
                launcherStatus = value;
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

        private void CheckForUpdates()
        {
            if (File.Exists(versionFilePath))
            {
                SHOEVersion localVersion = new SHOEVersion(File.ReadAllText(versionFilePath));
                EngineVersionText.Text = localVersion.ToString();

                try
                {
                    WebClient webClient = new WebClient();
                    SHOEVersion onlineVersion = new SHOEVersion(webClient.DownloadString("Version File"));

                    if (onlineVersion.HasVersionChanged(localVersion))
                    {
                        // Offer option to update to new version of SHOE
                    } else
                    {
                        launcherStatus = LauncherState.Running;
                    }
                } catch (Exception ex)
                {
                    launcherStatus = LauncherState.Failed;
                    MessageBox.Show($"Error checking for SHOE Updates: {ex}");
                }
            } else
            {
                // SHOE isn't installed at all. Install it
                InstallSHOEFiles(false, SHOEVersion.zero);
            }
        }

        private void InstallSHOEFiles(bool _isUpdate, SHOEVersion _onlineVersion)
        {
            try
            {
                WebClient webClient = new WebClient();
                if (_isUpdate)
                {
                    launcherStatus = LauncherState.UpdatingSHOE;
                }
                else
                {
                    launcherStatus = LauncherState.FirstSHOEInstall;
                    _onlineVersion = new SHOEVersion(webClient.DownloadString("Version File Link"));
                }

                webClient.DownloadFileCompleted += new AsyncCompletedEventHandler(DownloadSHOECompletedCallback);
                webClient.DownloadFileAsync(new Uri("SHOE main link"), SHOEOnlineZipPath, _onlineVersion);
            }
            catch (Exception ex)
            {
                launcherStatus = LauncherState.Failed;
                MessageBox.Show($"Error installing SHOE files: {ex}");
            }
        }

        private void DownloadSHOECompletedCallback(object sender, AsyncCompletedEventArgs e)
        {
            try
            {
                string onlineVersion = ((SHOEVersion)e.UserState).ToString();
                ZipFile.ExtractToDirectory(SHOELocalZipName, SHOEBuildPath, true);
                File.Delete(SHOELocalZipName);

                File.WriteAllText(versionFilePath, onlineVersion);

                EngineVersionText.Text = onlineVersion;
                launcherStatus = LauncherState.Running;
            } catch (Exception ex)
            {
                launcherStatus = LauncherState.Failed;
                MessageBox.Show($"Error finishing SHOE download: {ex}");
            }
        }

        private void Launcher_ContentRendered(object sender, EventArgs e)
        {
            CheckForUpdates();
        }

        private void LaunchSHOEButton_Click(object sender, RoutedEventArgs e)
        {
            if (File.Exists(SHOEExecPath) && launcherStatus == LauncherState.Running)
            {

            }
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