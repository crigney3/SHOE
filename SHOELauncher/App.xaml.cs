using System.Configuration;
using System.Data;
using System.Windows;
using System.Windows.Threading;

namespace SHOELauncher
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        void App_DispatcherUnhandledException(object sender, DispatcherUnhandledExceptionEventArgs args)
        {
            MessageBox.Show($"Error occurred before launch: {args.Exception}");

            // Prevent default unhandled exception processing
            args.Handled = true;

            Environment.Exit(0);
        }
    }

}
