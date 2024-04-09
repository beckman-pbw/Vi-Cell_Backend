#include <iostream>
#include <boost\filesystem.hpp>
#include "HawkeyeDataAccess.h"

int main(int argc, char* argv[])
{
	if (argc == 1)
	{
		std::cout << "Invalid arguments" << std::endl;
		std::cout << "Usage EncryptResultBinaries.exe <Input File Path> <Output directory>" << std::endl;
		getchar();
		return 0;
	}
	std::string input_directory_path = std::string(argv[1]);
	std::string output_directory_path = input_directory_path;
	if (argc > 2)
	{
		output_directory_path = std::string(argv[2]);
	}

	if (boost::filesystem::is_directory(input_directory_path) && boost::filesystem::exists(input_directory_path))
	{

		if (!boost::filesystem::exists(output_directory_path))
		{
			boost::system::error_code ec;
			boost::filesystem::create_directories(output_directory_path, ec);

			if (ec)
			{
				std::cout << "Failed to create the output directory :" << output_directory_path << std::endl;
				getchar();
				return 0;
			}
		}

		std::cout << "Input dir path: " << input_directory_path << "\nOutput dir path : " << output_directory_path << std::endl;
		char flag;
		std::cout << "Enter y to continue : n to cancel" << std::endl;
		std::cin >> flag;

		if (flag != 'y')
		{
			std::cout << "Cancelling the operation:" << std::endl;
			getchar();
			return 0;
		}

		boost::filesystem::directory_iterator itr(input_directory_path);
		for (itr; itr != boost::filesystem::directory_iterator(); itr++)
		{
			boost::filesystem::path ip(*itr);
			
			if (!boost::filesystem::is_regular_file(ip))
				continue;

			std::string inputfilename = ip.string();
			std::string outputfilename = output_directory_path + "\\" + ip.filename().string();

			std::string tmpfile = input_directory_path + "\\tmp.bin";

			if (HDA_FileEncrypt (inputfilename.c_str(), tmpfile.c_str()))
			{
				boost::system::error_code ec;
				boost::filesystem::copy_file(tmpfile, outputfilename, boost::filesystem::copy_option::overwrite_if_exists, ec);

				if (ec)
				{
					std::cout << " Failed to copy file from : " << tmpfile << " To " << outputfilename << std::endl;
				}
				ec.clear();
				if(boost::filesystem::exists(tmpfile))
				boost::filesystem::remove(tmpfile, ec);
			}
			else
			{
				std::cout << " Failed to encrypt : " << inputfilename << std::endl;
			}
		}
	}
	else
	{
		std::cout << "Invalid Directory input : " << input_directory_path << std::endl;
	}

	std::cout << "Completed !!!!" << std::endl;
	system("pause");
	return 0;
}