
using BackgroundUniformityCriteriaModel;
using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace BackgroundUniformityCriteria
{
    public class DisplayData : INotifyPropertyChanged
    {
        private string testName_;
        private string testResult_;
        private string passFail_;

        public DisplayData(
            string name, double result, bool success, string specialChars)
        {
            TestName = name;
            TestResult = result.ToString("0.##") + " " + specialChars;
            PassFail = success ? "Pass" : "Fail";
        }

        public string TestName
        {
            get { return testName_; }
            private set
            {
                if(testName_ == value)
                {
                    return;
                }
                testName_ = value;
                NotifyPropertyChanged("TestName");
            }
        }

        public string TestResult
        {
            get { return testResult_; }
            private set
            {
                if (testResult_ == value)
                {
                    return;
                }
                testResult_ = value;
                NotifyPropertyChanged("TestResult");
            }
        }

        public string PassFail
        {
            get { return passFail_; }
            private set
            {
                if (passFail_ == value)
                {
                    return;
                }
                passFail_ = value;
                NotifyPropertyChanged("PassFail");
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        private void NotifyPropertyChanged(string propName)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(propName));
            }
        }
    }

    public class BackgroundUniformityCriteriaViewModel 
        : INotifyPropertyChanged
    {
        private string imagePath_;
        private BackgroundUniformityCriteriaModel.ImageTestsModel model_;

        public BackgroundUniformityCriteriaViewModel()
        {            
            model_ = new BackgroundUniformityCriteriaModel.ImageTestsModel();
            Results = new ObservableCollection<DisplayData>();

            ImagePath = string.Empty;
            LabelOutput_1 = string.Empty;
            LabelOutput_2 = string.Empty;
            ImageOutput_1 = null;
            ImageOutput_2 = null;
        }        

        public void loadImage()
        {
            var dlg = new Microsoft.Win32.OpenFileDialog();

            // Set filter for file extension and default file extension 
            dlg.DefaultExt = "*.*";
            dlg.Filter = "PNG Files (*.png)|*.png|JPEG Files (*.jpeg)|*.jpeg|JPG Files (*.jpg)|*.jpg| BMP Files (*.bmp)|*.bmp| All Files (*.*)|*.*";
            dlg.Multiselect = false;

            var result = dlg.ShowDialog();
            if(result == true)
            {
                ImagePath = dlg.FileName;                
            }           
        }

        public string ImagePath
        {
            get { return imagePath_; }
            private set
            {
                if (imagePath_ == value
                    || string.IsNullOrEmpty(value))
                {
                    return;
                }
                imagePath_ = value;
                if (model_.loadImage(value) == false)
                {
                    MessageBox.Show("Invalid input image!");
                    return;
                }
                if (model_.execute() == false)
                {
                    MessageBox.Show("Error : cannot run the test at this moment!");
                    return;
                }
                //UnmanagedImageData imageData;
                //model_.getUnmanagedData(value, out imageData);
                //if (model_.loadDataAndExecute(imageData) == false)
                //{
                //    MessageBox.Show("Invalid input image or cannot run the test at this moment!");
                //    return;
                //}
                ResultDataComplete result;
                model_.getResult(out result);
                DisplayResultData(ref result);
                OnPropertyChanged("ImagePath");
            }
        }

        public string LabelOutput_1 { get; private set; }

        public ImageSource ImageOutput_1 { get; private set; }

        public string LabelOutput_2 { get; private set; }

        public ImageSource ImageOutput_2 { get; private set; }

        public ObservableCollection<DisplayData> Results { get; private set; }

        public event PropertyChangedEventHandler PropertyChanged;

        private void OnPropertyChanged(string propName)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(propName));
            }
        }

        private void DisplayResultData(ref ResultDataComplete result)
        {
            if (result == null || result.resultList == null)
            {
                return;
            }

            var length = result.resultList.Length;
            if (length <= 0)
            {
                return;
            }

            Results.Clear();

            for (int index = 0; index < length; index++)
            {
                var item = result.resultList[index];

                string specialChars = string.Empty;
                var stringToSearch = "Image";
                var testNames = item.testName.Split(new[] { stringToSearch }, StringSplitOptions.None);

                if (index == 0)
                {
                    ImageOutput_1 = getImageSource(ref item.resultImage);
                    LabelOutput_1 = testNames[0] + stringToSearch;
                    OnPropertyChanged("ImageOutput_1");
                    OnPropertyChanged("LabelOutput_1");

                    specialChars = "%";

                }
                if (index == 1)
                {
                    ImageOutput_2 = getImageSource(ref item.resultImage);
                    LabelOutput_2 = testNames[0] + stringToSearch;
                    OnPropertyChanged("ImageOutput_2");
                    OnPropertyChanged("LabelOutput_2");

                    specialChars = "%";
                }                

                Results.Add(new DisplayData(item.testName, item.value, item.success, specialChars));
            }
        }

        [DllImport("gdi32")]
        static extern int DeleteObject(IntPtr o);

        private static BitmapSource loadBitmap(Bitmap source)
        {
            IntPtr ip = source.GetHbitmap();
            BitmapSource bs = null;
            try
            {
                bs = System.Windows.Interop.Imaging.CreateBitmapSourceFromHBitmap(ip,
                   IntPtr.Zero, Int32Rect.Empty,
                   BitmapSizeOptions.FromEmptyOptions());
            }
            finally
            {
                DeleteObject(ip);
            }

            return bs;
        }

        private ImageSource getImageSource(ref ImageData imageData)
        {
            if (imageData == null
                || imageData.data == null
                || imageData.data.Length == 0
                || imageData.height * imageData.height <= 0)
            {
                return null;
            }

            var bitmap = new Bitmap(imageData.width, imageData.height);
            int index = 0;
            for (int i = 0; i < bitmap.Height; i++)
            {
                for (int j = 0; j < bitmap.Width; j++)
                {                   
                    if(index >= imageData.data.Length)
                    {
                        continue;
                    }

                    var value = (byte)imageData.data[index];
                    index++;
                    var nc = System.Drawing.Color.FromArgb(255, value, value, value);
                    bitmap.SetPixel(j, i, nc);
                }
            }

            int newWidth = 2048;
            int newHeight = 2048;            

            float scale = Math.Min(newWidth / bitmap.Width, newHeight / bitmap.Height);

            var scaleWidth = (int)(bitmap.Width * scale);
            var scaleHeight = (int)(bitmap.Height * scale);

            var resizedBitmap = new Bitmap(scaleWidth, scaleHeight);
            var graph = Graphics.FromImage(resizedBitmap);

            graph.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.NearestNeighbor;
            graph.DrawImage(bitmap, 
                new Rectangle(0, 0, scaleWidth, scaleHeight));

            //new Bitmap(resizedBitmap).Save("Save.tiff", ImageFormat.Tiff);

            //var palette = bitmap.Palette;
            //for (int i = 0; i < 256; i++)
            //{
            //    var b = i; // palette.Entries[i].B;
            //    palette.Entries[i] = System.Drawing.Color.FromArgb(255, b, b, b);
            //}
            //bitmap.Palette = palette;

            //new Bitmap(bitmap).Save("Save_VecImage.tiff", ImageFormat.Tiff);

            return loadBitmap(resizedBitmap);
        }
    }
}
