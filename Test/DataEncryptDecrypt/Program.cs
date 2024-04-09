using System;
using System.CodeDom;
using System.Collections.Generic;
using System.Diagnostics;
using System.Diagnostics.Eventing.Reader;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace DataEncryptDecrypt
{
	static class Program
	{
		private static bool? _Encrypt = null;
		private static List<string> folders_ = new List<string>();
		private static List<string> files_ = new List<string>();
		private static List<string> allowedExtensions_ = new List<string>();

		static void print(string message)
		{
			Console.WriteLine(message);
			Debug.WriteLine(message);
		}

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main(string[] args)
		{
			allowedExtensions_.Add(".XML");
			allowedExtensions_.Add(".BIN");
			allowedExtensions_.Add(".INFO");
			allowedExtensions_.Add(".CFG");
			allowedExtensions_.Add(".TXT");
			allowedExtensions_.Add(".PNG");

			if (args.Length == 0)
			{
				Application.EnableVisualStyles();
				Application.SetCompatibleTextRenderingDefault(false);
				Application.Run(new Main());
			}
			else
			{
				if (String.Compare(args[0], "-h", true) == 0 || String.Compare(args[0], "-?") == 0 ||
				    String.Compare(args[0], "/h", true) == 0 || String.Compare(args[0], "/?") == 0)
				{
					#region Display Help

					string szHelp = "Encrypts or decrypts the specified file(s), directories and Subdirectories.";
					szHelp += Environment.NewLine +Environment.NewLine;
					szHelp += "Usage: DataEncryptDecrypt";
					szHelp += Environment.NewLine;
					szHelp += Environment.NewLine;

					szHelp += "-? | -h            \tShow help menu";
					szHelp += Environment.NewLine;

					szHelp += "-E | -D filename   \tEncrypt/Decrypt the specified file.";
					szHelp += Environment.NewLine;

					szHelp += "-E | -D file file  \tEncrypt/Decrypt the specified files";
					szHelp += Environment.NewLine;

					szHelp += "-E | -D folder     \tEncrypt/Decrypt the specified folder(s)";
					szHelp += Environment.NewLine;

					szHelp += "-IE fExt | fExt    \tInclude new extensions";
					szHelp += Environment.NewLine;

					szHelp += "-EE fExt | fExt    \tExclude existing extensions";
					szHelp += Environment.NewLine;

					szHelp += "-SE                \tDisplay existing extensions";
					szHelp += Environment.NewLine;

					szHelp += Environment.NewLine;
					szHelp += "Encrypt will generate the output file with same name";
					szHelp += "and its file extension will have a leading [.e]";
					szHelp += Environment.NewLine;
					szHelp += Environment.NewLine;
					szHelp += "Decrypt will generate the output file with same name";
					szHelp += "and its file extension will be without leading [.e]";
					szHelp += Environment.NewLine;
					MessageBox.Show(szHelp, "DataEncryptDecrypt Help", MessageBoxButtons.OK, MessageBoxIcon.Information);

					#endregion
				}
				else
				{
					Application.UseWaitCursor = true;

					var stdout = Console.OpenStandardOutput();
					StreamWriter stdOutWriter = new StreamWriter(stdout);
					stdOutWriter.AutoFlush = true;
					AttachConsole(-1);

					#region Read Command Line Arguments

					folders_.Clear();
					files_.Clear();

					// Read each argument
					for(var index = 0; index < args.Length; ++index)
					{
						var szData = args[index];

						// Decrypt the file
						if (String.Compare(szData, "-D", true) == 0)
						{
							_Encrypt = false;
						}
						// Encrypt the file
						else if (String.Compare(szData, "-E", true) == 0)
						{
							_Encrypt = true;
						}
						// Include new extension
						else if (String.Compare(szData, "-IE", true) == 0)
						{
							if ((index + 1) < args.Length)
							{
								szData = args[index + 1];
							}
							var extList = szData.ToUpper().Split('|').ToList();
							if (extList.Count > 0)
							{
								allowedExtensions_.AddRange(extList.Where(ext => ext.Contains(".")));
								index++;
							}
							else
							{
								print("You must specify a valid file extension : " + szData);
							}
						}
						// Exclude existing extension
						else if (String.Compare(szData, "-EE", true) == 0)
						{
							if ((index + 1) < args.Length)
							{
								szData = args[index + 1];
							}
							var extList = szData.ToUpper().Split('|').ToList();
							if (extList.Count > 0)
							{
								foreach (var ext in extList.Where(ext => ext.Contains(".")))
								{
									allowedExtensions_.RemoveAll(x => x.Contains(ext));
								}
								index++;
							}
							else
							{
								print("You must specify a valid file extension : " + szData);
							}
						}
						// Display  existing extension
						else if (String.Compare(szData, "-SE", true) == 0)
						{
							var extString = "Existing Extensions" + Environment.NewLine;
							foreach (var ext in allowedExtensions_)
							{
								extString += ext + Environment.NewLine;
							}
							MessageBox.Show(extString);
						}
						// Encrypt/Decrypt folder
						else if (String.Compare(szData, "-F", true) == 0)
						{
							if ((index + 1) < args.Length)
							{
								szData = args[index + 1];
							}

							if (Directory.Exists(szData))
							{
								folders_.Add(szData);
								index++;
							}
							else
							{
								print("You must specify a valid folder path : " + szData);
							}
						}
						// Encrypt/Decrypt file
						else
						{
							if (String.Compare(szData, "[", true) == 0
								|| String.Compare(szData, "]", true) == 0
								|| String.Compare(szData, "|", true) == 0)
							{
								continue;
							}

							string directory = Path.GetDirectoryName(szData);
							if (!Directory.Exists(directory))
							{
								print("Invalid file name : " + szData);
								continue;
							}

							// Handler special wildcard (* and ?) character
							string filename = Path.GetFileName(szData);
							if(filename.Contains("*") || filename.Contains("?"))
							{
								files_.AddRange(Directory.GetFiles(directory, filename).ToList());
							}

							if (!File.Exists(szData))
							{
								print("Invalid file name : " + szData);
								continue;
							}
							files_.Add(szData);
						}
					}

					#endregion

					if(!_Encrypt.HasValue)
					{
						print("You must specify mode of operation : Encryption(-E) or Decryption(-D)");
						return;
					}

					try
					{
						#region Perform Encrypt/Decrypt Action

						if (_Encrypt.Value)
						{
							var encryptedFiles = new List<string>();
							foreach (var folder in folders_)
							{
								encryptedFiles.Clear();

								print("Encrypting folder : " + folder);
								DataEncryptDecryptHandler.EncrypDirectories(
									folder, ref encryptedFiles, allowedExtensions_);

								foreach (var file in encryptedFiles)
								{
									print("File encrypted : " + file);
								}
							}

							if(files_.Count > 0)
							{
								print(" Encrypt : input list\n" + files_.Aggregate((i, j) => i + j + Environment.NewLine));
								var unprocessedFiles = DataEncryptDecryptHandler.EncryptMultipleFiles(files_);
								foreach (var file in unprocessedFiles)
								{
									print("Failed to encrypt files : " + file);
								}
							}
						}
						else
						{
							var decryptedFiles = new List<string>();
							foreach (var folder in folders_)
							{
								decryptedFiles.Clear();

								print("Decrypting folder : " + folder);
								DataEncryptDecryptHandler.DecrypDirectories(
									folder, ref decryptedFiles, allowedExtensions_);

								foreach (var file in decryptedFiles)
								{
									print("File decrypted : " + file);
								}
							}

							if (files_.Count > 0)
							{
								print(" Decrypt : input list\n" + files_.Aggregate((i, j) => i + j + Environment.NewLine));
								var unprocessedFiles = DataEncryptDecryptHandler.DecryptMultipleFiles(files_);
								foreach (var file in unprocessedFiles)
								{
									print("Failed to decrypt files : " + file);
								}
							}
						}

						#endregion
					}
					catch (Exception e)
					{
						print(e.Message);
					}
					finally
					{
						stdOutWriter.Close();
						stdout.Close();

						Application.UseWaitCursor = false;
					}
				}
			}
		}

		[DllImport("kernel32.dll")]
		private static extern bool AttachConsole(int dwProcessId);
	}
}
