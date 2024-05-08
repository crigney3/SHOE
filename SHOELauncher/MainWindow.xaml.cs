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
using Microsoft.Win32;
using System.Net.Http;

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
        private string SHOELocalZipName;
        private string SHOEBuildPath;
        private string SHOEOnlineZipPath;
        private string projectsFilePath;

        private LauncherState launcherStatus;
        internal LauncherState LauncherStatus
        {
            get => launcherStatus;
            set
            {
                launcherStatus = value;
                switch(launcherStatus)
                {
                    case LauncherState.Initializing:
                        UpdateButton.Content = "Starting up";
                        break;
                    case LauncherState.Running:
                        UpdateButton.Content = "Check for Updates";
                        break;
                    case LauncherState.Failed:
                        UpdateButton.Content = "Update failed!";
                        break;
                    case LauncherState.FirstSHOEInstall:
                        UpdateButton.Content = "Installing SHOE";
                        break;
                    case LauncherState.UpdatingSHOE:
                        UpdateButton.Content = "Updating SHOE";
                        break;
                    default:
                        break;
                }
            }
        }

        private SHOEProject selectedProject;
        internal SHOEProject SelectedProject
        {
            get => selectedProject;
            set
            {
                // Maybe need to add other triggers here
                selectedProject = value;
            }
        }

        private List<SHOEProject> projects;
        private AppRing selectedAppRing;

        public static MainWindow windowRef;

        public MainWindow()
        {
            InitializeComponent();

            windowRef = this;

            launcherPath = Directory.GetCurrentDirectory();
            versionFilePath = Path.Combine(launcherPath, "version.txt");
            projectsFilePath = Path.Combine(launcherPath, "projects.txt");
            SHOEExecPath = "";
            selectedProjectPath = "";
            SHOELocalZipName = "SHOE.zip";
            SHOEOnlineZipPath = "";
            SHOEBuildPath = "";

            List<AppRing> appRings = new List<AppRing>();
            appRings.Add(new AppRing() { Title = "Stable", LocalPath = "", OnlinePath = "http://dionysus.headass.house:3000/Stable/" });
            appRings.Add(new AppRing() { Title = "Canary", LocalPath = "", OnlinePath = "http://dionysus.headass.house:3000/Canary/" });
            appRings.Add(new AppRing() { Title = "Experimental", LocalPath = "", OnlinePath = "http://dionysus.headass.house:3000/Experimental/" });
            RingChoiceListBox.ItemsSource = appRings;
            selectedAppRing = appRings[0];

            if (File.Exists("EngineInstallLocation.txt"))
            {
                SHOEBuildPath = File.ReadAllText("EngineInstallLocation.txt");
                SHOEExecPath = Path.Combine(SHOEBuildPath, selectedAppRing.Title) + "\\SHOE.exe";
            }

            LoadProjects();
            ProjectsListView.ItemsSource = projects;
        }

        private void LoadProjects()
        {
            projects = new List<SHOEProject>();
            // If no projects exist, then the listview should just have the "new project" button
            if(File.Exists(projectsFilePath))
            {
                string[] allProjStrings = File.ReadAllLines(projectsFilePath);
                foreach (string projString in allProjStrings)
                {
                    string[] projData = projString.Split(',');
                    projects.Add(new SHOEProject(projData[0], projData[1], (DXVersion)int.Parse(projData[2])));
                }
            }
        }

        private void CheckForUpdates()
        {
            if(SHOEBuildPath == "")
            {
                UpdateButton.Content = "Please Select a Directory First";
            }
            if (File.Exists(versionFilePath))
            {
                SHOEVersion localVersion = new SHOEVersion(File.ReadAllText(versionFilePath));
                EngineVersionText.Text = localVersion.ToString();

                try
                {
                    WebClient webClient = new WebClient();
                    string versionString = selectedAppRing.OnlinePath + "version.txt";
                    SHOEVersion onlineVersion = new SHOEVersion(webClient.DownloadString(versionString));

                    if (onlineVersion.HasVersionChanged(localVersion))
                    {
                        InstallSHOEFiles(true, SHOEVersion.zero);
                    } else
                    {
                        LauncherStatus = LauncherState.Running;
                        UpdateButton.Content = "No updates on " + selectedAppRing.Title;
                    }
                } catch (Exception ex)
                {
                    LauncherStatus = LauncherState.Failed;
                    MessageBox.Show($"Error checking for SHOE Updates: {ex}");
                }
            } else
            {
                // SHOE isn't installed at all. Install it
                InstallSHOEFiles(false, SHOEVersion.zero);
            }
        }

        private async void InstallSHOEFiles(bool _isUpdate, SHOEVersion _onlineVersion)
        {
            try
            {
                WebClient webClient = new WebClient();

                string versionString = selectedAppRing.OnlinePath + "version.txt";

                if (_isUpdate)
                {
                    LauncherStatus = LauncherState.UpdatingSHOE;
                }
                else
                {
                    LauncherStatus = LauncherState.FirstSHOEInstall;
                }

                _onlineVersion = new SHOEVersion(webClient.DownloadString(versionString));

                Uri engineString = new Uri(selectedAppRing.OnlinePath + "SHOE.zip");

                using (HttpClient client = new HttpClient())
                {
                    using (HttpResponseMessage response = await client.GetAsync(engineString))
                    {
                        using (var stream = await response.Content.ReadAsStreamAsync())
                        {
                            using (var byteStream = new MemoryStream())
                            {
                                stream.CopyTo(byteStream);
                                File.WriteAllBytes(SHOEBuildPath + "\\SHOE.zip", byteStream.ToArray());
                                DownloadSHOECompletedCallback(_onlineVersion);
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                launcherStatus = LauncherState.Failed;
                MessageBox.Show($"Error installing SHOE files: {ex}");
            }
        }

        private void DownloadSHOECompletedCallback(SHOEVersion _onlineVersion)
        {
            try
            {
                Console.WriteLine("Installing");
                string ringFilePath = SHOEBuildPath + "\\" + selectedAppRing.Title;
                if (!Directory.Exists(ringFilePath))
                {
                    Directory.CreateDirectory(ringFilePath);
                } else
                {
                    Directory.Delete(ringFilePath, true);
                    Directory.CreateDirectory(ringFilePath);
                }
                ZipFile.ExtractToDirectory(SHOEBuildPath + "\\SHOE.zip", ringFilePath, true);
                File.Delete(SHOEBuildPath + "\\SHOE.zip");

                File.WriteAllText(versionFilePath, _onlineVersion.ToString());

                EngineVersionText.Text = _onlineVersion.ToString();
                SHOEExecPath = Path.Combine(SHOEBuildPath, selectedAppRing.Title) + "\\SHOE.exe";
                LauncherStatus = LauncherState.Running;
            } catch (Exception ex)
            {
                LauncherStatus = LauncherState.Failed;
                MessageBox.Show($"Error finishing SHOE download: {ex}");
            }
        }

        private void Launcher_ContentRendered(object sender, EventArgs e)
        {

        }

        private void LaunchSHOEButton_Click(object sender, RoutedEventArgs e)
        {
            if (File.Exists(SHOEExecPath))
            {
                ProcessStartInfo startInfo = new ProcessStartInfo(SHOEExecPath);
                startInfo.WorkingDirectory = SHOEBuildPath;
                startInfo.Arguments = $"/DXVersion:{selectedProject.DirectXVersion} /ProjectPath:{selectedProject.ProjectPath}";
                Process.Start(startInfo);

                Close();
            }
        }

        private void RingChoiceListBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if(RingChoiceListBox.SelectedItem != null)
            {
                this.SHOEOnlineZipPath = (RingChoiceListBox.SelectedItem as AppRing).OnlinePath;
            }
        }
        
        private void ProjectsListView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if(ProjectsListView.SelectedItem != null)
            {
                this.selectedProject = ProjectsListView.SelectedItem as SHOEProject;
                LaunchSHOEButton.Content = "Launch " + this.selectedProject.ProjectName;
            }
        }

        private void UpdateButton_Click(object sender, RoutedEventArgs e)
        {
            CheckForUpdates();
        }

        private void SelectInstallDirectory_Click(object sender, RoutedEventArgs e)
        {
            var folderDialog = new OpenFolderDialog
            {
                Title = "Select Folder For Install",
                InitialDirectory = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles)
            };

            if (folderDialog.ShowDialog() == true)
            {
                var folderName = folderDialog.FolderName;
                SHOEBuildPath = folderName;
            }

            File.WriteAllText("EngineInstallLocation.txt", SHOEBuildPath);
        }
    }

    public struct SHOEVersion
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

    public enum DXVersion
    {
        DX11,
        DX12
    }

    public class SHOEProject
    {
        private string projectName;
        private string projectPath;
        private DXVersion dxVersion;

        public string ProjectName
        {
            get => projectName;
            set => projectName = value;
        }

        public string ProjectPath
        {
            get => projectPath;
            set
            {
                // Make any other adjustments that changing the project path needs here
                projectPath = value;
            }
        }

        public DXVersion DirectXVersion
        {
            get => dxVersion;
            set
            {
                dxVersion = value;
            }
        }

        internal SHOEProject(string _projectName, string _projectPath, DXVersion _dxVersion = DXVersion.DX11)
        {
            projectName = _projectName;
            projectPath = _projectPath;
            dxVersion = _dxVersion;
        }
    }

    public class AppRing
    {
        public string Title { get; set; }
        public string OnlinePath { get; set; }
        public string LocalPath { get; set; }

    }
}

