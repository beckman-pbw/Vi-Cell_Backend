using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace DataEncryptDecrypt
{
	public static class DataEncryptDecryptHandler
	{
		const string HAWKEYEDATAACCESS_DLL = "HawkeyeDataAccess.dll";
		static string EncryptionFileExtensionKeyword = "e";

		[DllImport(HAWKEYEDATAACCESS_DLL, CharSet = CharSet.Ansi)]
		static extern bool HDA_FileEncrypt (string decryptedfilename, string encryptedfilename);

		[DllImport(HAWKEYEDATAACCESS_DLL, CharSet = CharSet.Ansi)]
		static extern bool HDA_FileDecrypt (string encryptedfilename, string decryptedfilename);

		public static bool IsEncryptedFile(string file)
		{
			return Path.GetExtension(file).Contains("." + EncryptionFileExtensionKeyword);
		}

		public static bool IsImageFile(string file)
		{
			var fileExtUpper = Path.GetExtension(file).ToUpper();
			return fileExtUpper.Contains("PNG");
		}

		public static string createEncryptionFilePath(string fullPath)
		{
			if (string.IsNullOrEmpty(fullPath))
			{
				return "";
			}
			string currentExtension = Path.GetExtension(fullPath);
			string newExtension = string.Format(".{0}{1}", EncryptionFileExtensionKeyword, currentExtension.Replace(".", ""));
			return fullPath.Replace(currentExtension, newExtension);
		}

		public static string createDecryptionFilePath(string fullPath)
		{
			if (string.IsNullOrEmpty(fullPath))
			{
				return "";
			}
			string currentExtension = Path.GetExtension(fullPath);
			string newExtension = currentExtension.Replace(("." + EncryptionFileExtensionKeyword), ".");
			return fullPath.Replace(currentExtension, newExtension);
		}

		public static List<string> filterAllowedFiles(List<string> fullFileNames, List<string> allowedExtensions)
		{
			if(allowedExtensions == null || allowedExtensions.Count <= 0)
			{
				return fullFileNames;
			}

			var keyWord = EncryptionFileExtensionKeyword.ToUpper();
			return fullFileNames.Where(
				fl => allowedExtensions.Any(
					x => x.ToUpper().Contains(Path.GetExtension(fl).ToUpper().Replace(("." + keyWord), ".")))).ToList();
		}

		public static bool EncryptFile(string fullFileName)
		{
			if(string.IsNullOrEmpty(fullFileName))
			{
				return false;
			}

			// Check if file is already encrypted
			if (IsEncryptedFile(fullFileName))
			{
				return false;
			}

			try
			{
				string encryptedfilename = createEncryptionFilePath(fullFileName);
				return HDA_FileEncrypt (fullFileName, encryptedfilename);
			}
			catch (Exception ex)
			{
				Console.WriteLine(string.Format("Failed to encrypt file : {0}, Exception occurred : {1}", fullFileName, ex.Message));
			}
			return false;
		}

		public static List<string> EncryptMultipleFiles(List<string> fullFilePaths)
		{
			var unprocessedFiles = new List<string>();
			if (fullFilePaths?.Count <= 0)
			{
				Console.WriteLine("DataEncryptDecryptHandler : EncryptMultipleFiles : list is empty");
				return unprocessedFiles;
			}

			foreach (var fullFileName in fullFilePaths)
			{
				if (!EncryptFile(fullFileName))
				{
					unprocessedFiles.Add(fullFileName);
				}
			}
			return unprocessedFiles;
		}

		public static void EncrypDirectories(
			string directoryPath, ref List<string> encryptedFiles, List<string> allowedExtensions = null)
		{
			if(!Directory.Exists(directoryPath))
			{
				return;
			}

			DirectoryInfo startFolder = new DirectoryInfo(directoryPath);
			if (startFolder == null)
			{
				return;
			}

			// Encrypt files in current folder
			var files = startFolder.GetFiles().Select(x => x.FullName).ToList();
			var unprocessedFiles = EncryptMultipleFiles(filterAllowedFiles(files, allowedExtensions));

			encryptedFiles.AddRange(files.Where(x => !unprocessedFiles.Contains(x)));

			foreach (var folder in startFolder.GetDirectories())
			{
				EncrypDirectories(folder.FullName, ref encryptedFiles, allowedExtensions);
			}
		}

		public static bool DecryptFile(string fullFileName)
		{
			if (string.IsNullOrEmpty(fullFileName))
			{
				return false;
			}

			// Check if file is already decrypted
			if (!IsEncryptedFile(fullFileName))
			{
				return false;
			}

			try
			{
				string decryptedfilename = createDecryptionFilePath(fullFileName);
				return HDA_FileDecrypt(fullFileName, decryptedfilename);
			}
			catch (Exception ex)
			{
				Console.WriteLine(string.Format("Failed to decrypt file : {0}, Exception occurred : {1}", fullFileName, ex.Message));
			}
			return false;
		}

		public static List<string> DecryptMultipleFiles(List<string> fullFilePaths)
		{
			var unprocessedFiles = new List<string>();
			if (fullFilePaths?.Count <= 0)
			{
				return unprocessedFiles;
			}

			foreach (var fullFileName in fullFilePaths)
			{
				if (!DecryptFile(fullFileName))
				{
					unprocessedFiles.Add(fullFileName);
				}
			}
			return unprocessedFiles;
		}

		public static void DecrypDirectories(
			string directoryPath, ref List<string> decryptedFiles, List<string> allowedExtensions = null)
		{
			if (!Directory.Exists(directoryPath))
			{
				return;
			}

			DirectoryInfo startFolder = new DirectoryInfo(directoryPath);
			if (startFolder == null)
			{
				return;
			}

			// Encrypt files in current folder
			var files = startFolder.GetFiles().Select(x => x.FullName).ToList();
			var unprocessedFiles = DecryptMultipleFiles(filterAllowedFiles(files, allowedExtensions));

			decryptedFiles.AddRange(files.Where(x => !unprocessedFiles.Contains(x)));

			foreach (var folder in startFolder.GetDirectories())
			{
				DecrypDirectories(folder.FullName, ref decryptedFiles, allowedExtensions);
			}
		}
	}
}
