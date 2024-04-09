using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Windows.Forms;

namespace DataEncryptDecrypt
{
	public partial class Main : Form
	{
		string _FileFolder = Environment.CurrentDirectory;
		List<string> _FileList = new List<string>();
		List<string> _EncryptedData = new List<string>();

		public Main()
		{
			InitializeComponent();
			checkBox1.Visible = false;
			checkBox1.Checked = false;
			button5.Visible = false;
			textBox6.ReadOnly = true;
			checkBox2.Visible = false;
			checkBox2.Checked = false;
		}


		/// <summary>
		/// Open file for encryption button
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void button1_Click(object sender, EventArgs e)
		{
			OpenFileDialog dlg = null;

			try
			{
				dlg = new OpenFileDialog();
				dlg.InitialDirectory = _FileFolder;
				dlg.Filter = "XML File (*.xml)|*.xml|PNG File (*.png)|*.png|Info File (*.info)|*.info|Config File (*.cfg)|*.cfg|All Files (*.*)|*.*";
				if (dlg.ShowDialog() == DialogResult.OK)
				{
					textBox1.Text = string.Empty;
					textBox3.Text = string.Empty;
					textBox2.Text = string.Empty;
					pictureBox1.Image = null;

					textBox1.Text = dlg.FileName;
					_FileFolder = Path.GetFullPath(dlg.FileName);

					if (DataEncryptDecryptHandler.IsEncryptedFile(dlg.FileName))
					{
						// already encrypted
						textBox2.Visible = true;
						textBox2.Text = "Invalid File Selected To Encrypt: " + dlg.FileName;
						return;
					}

					string fileExtension = Path.GetExtension(dlg.FileName);
					if (fileExtension.Equals(".xml", StringComparison.InvariantCultureIgnoreCase) ||
						fileExtension.Equals(".info", StringComparison.InvariantCultureIgnoreCase))
					{
						textBox2.Visible = true;
						pictureBox1.Visible = false;
						using (StreamReader sr = new StreamReader(dlg.FileName))
						{
							textBox2.Text = sr.ReadToEnd();
						}
					}
					else if (fileExtension.Equals(".png", StringComparison.InvariantCultureIgnoreCase))
					{
						textBox2.Visible = false;
						pictureBox1.Visible = true;
						using (var fs = new FileStream(dlg.FileName, FileMode.Open, FileAccess.Read))
						{
							pictureBox1.Image = Image.FromStream(fs);
						}
						//pictureBox1.Image = Image.FromFile(dlg.FileName);
					}
				}
			}
			catch (Exception ex)
			{
				MessageBox.Show(ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
			}
			finally
			{
				if (dlg != null)
				{
					dlg.Dispose();
					dlg = null;
				}
			}
		}

		/// <summary>
		/// Encrypt file button
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void button2_Click(object sender, EventArgs e)
		{
			if (string.IsNullOrEmpty(textBox1.Text) == false)
			{
				string filename = textBox1.Text;

				textBox3.Text = string.Empty;
				label7.Text = string.Empty;

				if (DataEncryptDecryptHandler.IsEncryptedFile(filename))
				{
					// already encrypted
					textBox3.Text = "Invalid File Selected To Encrypt: " + filename;
					return;
				}

				Stopwatch encryptionTime = new Stopwatch();
				encryptionTime.Start();

				string encryptedfilename = DataEncryptDecryptHandler.createEncryptionFilePath(filename);
				if (DataEncryptDecryptHandler.EncryptFile(filename))
				{
					encryptionTime.Stop();
					label7.Text = string.Format("Elapsed Time: {0:d4} ms", encryptionTime.ElapsedMilliseconds);

					using (StreamReader sr = new StreamReader(encryptedfilename))
					{
						textBox3.Text = sr.ReadToEnd();
					}

					textBox4.Text = encryptedfilename;
					textBox5.Text = textBox3.Text;
					textBox6.Text = string.Empty;
				}
				else
					textBox3.Text = "Failed to Encrypt File";
			}
		}

		/// <summary>
		/// Open file for decryption button
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void button3_Click(object sender, EventArgs e)
		{
			OpenFileDialog dlg = null;

			try
			{
				dlg = new OpenFileDialog();
				dlg.InitialDirectory = Environment.CurrentDirectory;
				dlg.Filter = "Encrypted Files (*.e*)|*.e*";
				if (dlg.ShowDialog() == DialogResult.OK)
				{
					textBox4.Text = string.Empty;
					textBox5.Text = string.Empty;
					textBox6.Visible = true;
					textBox6.Text = string.Empty;
					pictureBox2.Image = null;
					checkBox1.Checked = false;

					textBox4.Text = dlg.FileName;
					_FileFolder = Path.GetFullPath(dlg.FileName);

					if (!DataEncryptDecryptHandler.IsEncryptedFile(dlg.FileName))
					{
						// already decrypted
						textBox5.Text = "Invalid File Selected To Decrypt: " + dlg.FileName;
						return;
					}

					using (StreamReader sr = new StreamReader(dlg.FileName))
					{
						textBox5.Text = sr.ReadToEnd();
					}
				}
			}
			catch (Exception ex)
			{
				MessageBox.Show(ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
			}
			finally
			{
				if (dlg != null)
				{
					dlg.Dispose();
					dlg = null;
				}
			}
		}

		/// <summary>
		/// Decrypt to text file button
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void button4_Click(object sender, EventArgs e)
		{
			if (string.IsNullOrEmpty(textBox4.Text) == false)
			{
				string encryptedfilename = textBox4.Text;

				textBox6.Text = string.Empty;
				pictureBox2.Image = null;
				label8.Text = string.Empty;

				if (!DataEncryptDecryptHandler.IsEncryptedFile(encryptedfilename))
				{
					// already decrypted
					textBox6.Visible = true;
					textBox6.Text = "Invalid File Selected To Decrypt: " + encryptedfilename;
					return;
				}

				Stopwatch decryptionTime = new Stopwatch();
				decryptionTime.Start();

				string decryptedfilename = DataEncryptDecryptHandler.createDecryptionFilePath(encryptedfilename);
				if (DataEncryptDecryptHandler.DecryptFile(encryptedfilename))
				{
					decryptionTime.Stop();
					label8.Text = string.Format("Elapsed Time: {0:d4} ms", decryptionTime.ElapsedMilliseconds);

					try
					{
						if (DataEncryptDecryptHandler.IsImageFile(decryptedfilename))
						{
							textBox6.Visible = false;
							pictureBox2.Visible = true;
							using (var fs = new FileStream(decryptedfilename, FileMode.Open, FileAccess.Read))
							{
								pictureBox2.Image = Image.FromStream(fs);
							}
						}
						else
						{
							textBox6.Visible = true;
							pictureBox2.Visible = false;
							using (StreamReader sr = new StreamReader(decryptedfilename))
							{
								textBox6.Text = sr.ReadToEnd();
							}
							checkBox1.Visible = true;
						}
					}
					catch (Exception ex)
					{
						textBox6.Visible = true;
						pictureBox2.Visible = false;
						textBox6.Text = ex.Message;
					}
				}
				else
					textBox6.Text = "Failed to Decrypt File";
			}
		}

		/// <summary>
		/// Folder selection for encrypting multiple files button
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void button7_Click(object sender, EventArgs e)
		{
			FolderBrowserDialog dlg = null;

			try
			{
				dlg = new FolderBrowserDialog();
				dlg.RootFolder = Environment.SpecialFolder.MyComputer;

				if (dlg.ShowDialog() == DialogResult.OK)
				{
					textBox7.Text = dlg.SelectedPath;
					textBox8.Text = string.Empty;
				}
			}
			catch (Exception ex)
			{
				MessageBox.Show(ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
			}
			finally
			{
				if (dlg != null)
				{
					dlg.Dispose();
					dlg = null;
				}
			}
		}

		/// <summary>
		/// Encrypt Multiple Files button
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void button6_Click(object sender, EventArgs e)
		{
			if (string.IsNullOrEmpty(textBox7.Text) == false)
			{
				this.Cursor = Cursors.WaitCursor;

				Stopwatch encryptTime = new Stopwatch();
				encryptTime.Start();

				_FileList.Clear();
				DataEncryptDecryptHandler.EncrypDirectories(textBox7.Text, ref _FileList);

				encryptTime.Stop();
				label11.Text = string.Format("Elapsed Time: {0:d4} ms", encryptTime.ElapsedMilliseconds);

				this.Cursor = null;
				textBox8.Text = string.Format("{0} files encrypted", _FileList.Count) + Environment.NewLine + string.Join(Environment.NewLine, _FileList);
			}
		}

		// <summary>
		/// Folder selection for decrypting multiple files button
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void button8_Click(object sender, EventArgs e)
		{
			FolderBrowserDialog dlg = null;

			try
			{
				dlg = new FolderBrowserDialog();
				dlg.RootFolder = Environment.SpecialFolder.MyComputer;

				if (dlg.ShowDialog() == DialogResult.OK)
				{
					textBox9.Text = dlg.SelectedPath;
					textBox10.Text = string.Empty;
				}
			}
			catch (Exception ex)
			{
				MessageBox.Show(ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
			}
			finally
			{
				if (dlg != null)
				{
					dlg.Dispose();
					dlg = null;
				}
			}
		}

		/// <summary>
		/// Decrypt Multiple Files button
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void button9_Click(object sender, EventArgs e)
		{
			if (string.IsNullOrEmpty(textBox9.Text) == false)
			{
				this.Cursor = Cursors.WaitCursor;

				Stopwatch decryptTime = new Stopwatch();
				decryptTime.Start();

				_FileList.Clear();
				DataEncryptDecryptHandler.DecrypDirectories(textBox9.Text, ref _FileList);

				decryptTime.Stop();
				label14.Text = string.Format("Elapsed Time: {0:d4} ms", decryptTime.ElapsedMilliseconds);

				this.Cursor = null;
				textBox10.Text = string.Format("{0} files decrypted", _FileList.Count) + Environment.NewLine + string.Join(Environment.NewLine, _FileList);
			}
		}

		private void Main_DragEnter(object sender, DragEventArgs e)
		{
			tabControl1.SelectedTab = tabControl1.TabPages[2];
			Encrypty_DragEnter(sender, e);
			Decrypt_DragEnter(sender, e);
		}

		private void Encrypty_DragEnter(object sender, DragEventArgs e)
		{
			if (e.Data.GetDataPresent(DataFormats.FileDrop))
			{
				string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);

				if (files.Any(x => DataEncryptDecryptHandler.IsEncryptedFile(x) == false)
					|| files.Any(x => Directory.Exists(x)))
				{
					e.Effect = DragDropEffects.Copy;
					return;
				}
			}
			e.Effect = DragDropEffects.None;
		}

		private void Encrypt_DragDrop(object sender, DragEventArgs e)
		{
			if (e.Data.GetDataPresent(DataFormats.FileDrop))
			{
				string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);
				if (files.Length <= 0)
				{
					return;
				}

				var filesToEncrypt = new List<string>();
				var foldersToEncrypt = new List<string>();
				foreach (var item in files)
				{
					if (Directory.Exists(item))
					{
						_FileList.Add(item);
						foldersToEncrypt.Add(item);
						continue;
					}

					if (!DataEncryptDecryptHandler.IsEncryptedFile(item))
					{
						_FileList.Add(item);
						filesToEncrypt.Add(item);
					}
				}

				textBox11.Text = string.Format("{0} total files/folders selected", _FileList.Count) + Environment.NewLine + string.Join(Environment.NewLine, _FileList);

				this.Cursor = Cursors.WaitCursor;

				_FileList.Clear();
				var unprocessedFiles = DataEncryptDecryptHandler.EncryptMultipleFiles(filesToEncrypt);
				_FileList.AddRange(files.Where(x => !unprocessedFiles.Contains(x)));

				foreach (var folder in foldersToEncrypt)
				{
					DataEncryptDecryptHandler.EncrypDirectories(folder, ref _FileList);
				}

				textBox13.Text = string.Format("{0} total files/folders encrypted", _FileList.Count) + Environment.NewLine + string.Join(Environment.NewLine, _FileList);
				this.Cursor = null;
				_FileList.Clear();
			}
		}

		private void Decrypt_DragEnter(object sender, DragEventArgs e)
		{
			string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);

			if (files.Any(x => DataEncryptDecryptHandler.IsEncryptedFile(x))
					|| files.Any(x => Directory.Exists(x)))
			{
				e.Effect = DragDropEffects.Copy;
				return;
			}
			e.Effect = DragDropEffects.None;
		}

		private void Decrypt_DragDrop(object sender, DragEventArgs e)
		{
			_FileList.Clear();

			if (e.Data.GetDataPresent(DataFormats.FileDrop))
			{
				string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);
				if (files.Length <= 0)
				{
					return;
				}

				var filesToDecrypt = new List<string>();
				var foldersToDecrypt = new List<string>();
				foreach (var item in files)
				{
					if (Directory.Exists(item))
					{
						_FileList.Add(item);
						foldersToDecrypt.Add(item);
						continue;
					}

					if (DataEncryptDecryptHandler.IsEncryptedFile(item))
					{
						_FileList.Add(item);
						filesToDecrypt.Add(item);
					}
				}

				textBox12.Text = string.Format("{0} total files/folders selected", _FileList.Count) + Environment.NewLine + string.Join(Environment.NewLine, _FileList);

				this.Cursor = Cursors.WaitCursor;

				_FileList.Clear();
				var unprocessedFiles = DataEncryptDecryptHandler.DecryptMultipleFiles(filesToDecrypt);
				_FileList.AddRange(files.Where(x => !unprocessedFiles.Contains(x)));

				foreach (var folder in foldersToDecrypt)
				{
					DataEncryptDecryptHandler.DecrypDirectories(folder, ref _FileList);
				}

				textBox14.Text = string.Format("{0} total files/folders decrypted", _FileList.Count) + Environment.NewLine + string.Join(Environment.NewLine, _FileList);
				this.Cursor = null;
				_FileList.Clear();
			}
		}

		private void editEncrypt_checkedChanged(object sender, EventArgs e)
		{
			checkBox2.Checked = true;
			if (checkBox1.Checked)
			{
				button5.Visible = true;
				checkBox2.Visible = true;
				textBox6.ReadOnly = false;
			}
			else
			{
				button5.Visible = false;
				checkBox2.Visible = false;
				textBox6.ReadOnly = true;
			}
		}

		private void button5_Click(object sender, EventArgs e)
		{
			string decryptedfilename = string.Empty;
			try
			{
				decryptedfilename = DataEncryptDecryptHandler.createDecryptionFilePath(textBox4.Text);

				FileStream file = File.Open(decryptedfilename, FileMode.Create);
				StreamWriter sw = new StreamWriter(file);
				sw.WriteLine(textBox6.Text);
				sw.Close();

				if (!DataEncryptDecryptHandler.EncryptFile(decryptedfilename))
				{
					MessageBox.Show("Failed to encrypt edited file");
					return;
				}
			}
			catch (Exception ex)
			{
				MessageBox.Show(ex.Message, "Failed to encrypt edited file");
			}

			try
			{
				if (!string.IsNullOrEmpty(decryptedfilename) && checkBox2.Checked)
				{
					File.Delete(decryptedfilename);
				}
			}
			catch (Exception ex)
			{
				MessageBox.Show(ex.Message, "Failed to deleted decrypted file");
			}
		}
	}
}