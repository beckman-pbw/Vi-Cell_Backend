using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace BackgroundUniformityCriteria
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private bool isFullScreen_ = false;
        public MainWindow()
        {
            InitializeComponent();
            this.DataContext = new BackgroundUniformityCriteriaViewModel();
            dataGrid.Visibility = Visibility.Collapsed;
            FullScreen.Visibility = Visibility.Collapsed;
            DisplayGrid.Visibility = Visibility.Visible;
        }

        private void OnBrowseButtonClick(object sender, RoutedEventArgs e)
        {
            if(this.DataContext is BackgroundUniformityCriteriaViewModel)
            {
                var vm = this.DataContext as BackgroundUniformityCriteriaViewModel;
                vm.loadImage();
                dataGrid.Visibility = Visibility.Visible;
                FullScreen.Visibility = Visibility.Collapsed;
                DisplayGrid.Visibility = Visibility.Visible;
            }
        }

        private void OnDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            bool isdoubleClick = e.ChangedButton == MouseButton.Left
                && e.ClickCount == 2;

            if (isdoubleClick == false)
            {
                return;
            }

            if (isFullScreen_)
            {
                FullImage.Source = null;
                FullScreen.Visibility = Visibility.Collapsed;
                DisplayGrid.Visibility = Visibility.Visible;
                isFullScreen_ = false;
                return;
            }
            isFullScreen_ = true;
            FullScreen.Visibility = Visibility.Visible;
            FullScreen.MaxWidth = this.ActualWidth;
            FullScreen.MaxHeight = this.ActualHeight;

            DisplayGrid.Visibility = Visibility.Collapsed;
            if (sender is Image)
            {
                var vm = this.DataContext as BackgroundUniformityCriteriaViewModel;
                FullImage.Source = (sender as Image).Source;                
                FullImage.Width = this.ActualWidth;
                FullImage.Width = this.ActualHeight;
            }
        }
    }
}
