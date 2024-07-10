using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace SHOELauncher
{
    /// <summary>
    /// Interaction logic for NewProjectWindow.xaml
    /// </summary>
    public partial class NewProjectWindow : Window
    {
        SHOEProject newProject;

        public NewProjectWindow()
        {
            InitializeComponent();

            newProject = new SHOEProject("", "");

            List<DXVersionChoice> dXVersionChoices = new List<DXVersionChoice>();
            dXVersionChoices.Add(new DXVersionChoice() { Title="DirectX 11", Value=DXVersion.DX11 });
            dXVersionChoices.Add(new DXVersionChoice() { Title="DirectX 12", Value=DXVersion.DX12 });
            DirectXChoiceListBox.ItemsSource = dXVersionChoices;
        }

        private void CreateProjectButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ((MainWindow)this.Owner).CreateNewProjectFromData(newProject);
                this.Close();
            } catch (Exception ex)
            {
                MessageBox.Show($"Error passing new project details to main window: {ex}");
            }

        }

        private void DirectXChoiceListBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (DirectXChoiceListBox.SelectedItem != null)
            {
                newProject.DirectXVersion = (DirectXChoiceListBox.SelectedItem as DXVersionChoice).Value;
                if (newProject.DirectXVersion == DXVersion.DX12) {
                    DirectXWarning.Visibility = Visibility.Visible;
                } else
                {
                    DirectXWarning.Visibility = Visibility.Hidden;
                }
            }
        }

        private void SelectProjectDirectory_Click(object sender, RoutedEventArgs e)
        {
            var folderDialog = new OpenFolderDialog
            {
                Title = "Select Folder For Project",
                InitialDirectory = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments)
            };

            if (folderDialog.ShowDialog() == true)
            {
                var folderName = folderDialog.FolderName;
                newProject.ProjectPath = folderName;
                ProjectDirectory.Text = folderName;
            }

            if (newProject.ProjectName != "" &&  newProject.ProjectPath != "")
            {
                CreateProjectButton.IsEnabled = true;
            }
        }

        private void ProjectName_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (ProjectName == null || newProject == null) {
                return;
            }

            if (!string.IsNullOrEmpty(ProjectName.Text))
            {
                newProject.ProjectName = ProjectName.Text;
            }

            if (newProject.ProjectName != "" && newProject.ProjectPath != "")
            {
                CreateProjectButton.IsEnabled = true;
            }
        }

        private void StarterAssetsChecked_Checked(object sender, RoutedEventArgs e)
        {
            if (newProject == null) { return; }
            newProject.StarterAssets = true;
        }

        private void StarterAssetsChecked_Unchecked(object sender, RoutedEventArgs e)
        {
            if (newProject == null) { return; }
            newProject.StarterAssets = false;
        }
    }
}
