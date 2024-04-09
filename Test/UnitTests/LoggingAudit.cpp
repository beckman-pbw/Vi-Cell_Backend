#include "stdafx.h"
#include <iostream>
#include <rng.h>
#include <afx.h>
#include <filesystem>

#include "gtest\gtest.h"

#include "Core\LogEntry.hpp"
#include "Core\LoggerSignature.hpp"
#include "Core\LoggerSignature_Impl.hxx"

#include "Logger.hpp"

#include "boost\filesystem.hpp"

#include "HawkeyeLogic.hpp"

namespace LoggingSignatureTests
{
	static const std::string MODULENAME = "UnitTest";

	using Logger1 = LoggerSignature<1>;
	using Logger2 = LoggerSignature<2>;

	/**
	* \brief – This test is meant to setup and initialize the logging system. 
	*/
	TEST(LogSign, InitializeTesting)
	{
		//start with a empty long to do the testing. 
		if (boost::filesystem::exists(".\\UnitTests1.log"))
			boost::filesystem::remove(".\\UnitTests1.log");
		if (boost::filesystem::exists(".\\UnitTests2.log"))
			boost::filesystem::remove(".\\UnitTests2.log");
		if (boost::filesystem::exists(".\\UnitTests3.log"))
			boost::filesystem::remove(".\\UnitTests3.log");

		boost::system::error_code ec;
		Logger1::L().Initialize(ec, "UnitTests1.info");
		EXPECT_EQ(boost::system::errc::success, ec);
		Logger1::L().Log("Test-UC_40_1_1", severity_level::normal, "Initialize for Testing");
		Logger1::L().Flush(); //so we have a log file ready for the next test
		
		Logger1::L().SetAlwaysFlush(false);//make sure the log will NOT flushed at every new log entry.
		Logger1::L().setAutoCompromisedBackup(false);
		
		Logger2::L().Initialize(ec, "UnitTests2.info");
		EXPECT_EQ(boost::system::errc::success, ec);		
		Logger2::L().SetAlwaysFlush(false);//make sure the log will NOT flushed at every new log entry.
		Logger2::L().setAutoCompromisedBackup(false);
	}

	/**
	* \brief – Log file must be re - readable from disk.
	* \test - The test would open the file of the log outside of the 
	* logging system. To check the update of the log when a new log entry 
	* is created. 
	*/
	TEST(LogSign, UC_40_1_1)
	{
		std::string line;
		std::ifstream instream(Logger1::L().LogFile());
		std::getline(instream, line); //get initial log entry
		
		ASSERT_TRUE(instream.is_open());
		
		Logger1::L().Log("Test-UC_40_1_1", severity_level::normal, "Line 1");
		Logger1::L().Flush();
		instream.sync();
		instream.seekg(0, std::ios::beg);
		
		std::getline(instream, line);//this is the initial log entry again
		std::getline(instream, line);

		EXPECT_LT(0,line.find("Line 1"));
	}

	/**
	* \brief – Authenticity of the log file must be checkable. That is: we must be able to detect
	*  if the log file has been altered from our last write to that file.
	* \test  - The test would create 3 log entries and then modify one of them.  Then check to 
	* see if the signature is still valid. Add 3 more entries and check the signature. 
	*/
	TEST(LogSign, UC_40_2_1)
	{
		std::fstream iostream(Logger1::L().LogFile(), std::fstream::in | std::fstream::out);
		ASSERT_TRUE(iostream.is_open());

		Logger1::L().Log("Test-UC_40_2_1", severity_level::normal, "Line 2");
		Logger1::L().Log("Test-UC_40_2_1", severity_level::normal, "Line 3");
		Logger1::L().Log("Test-UC_40_2_1", severity_level::normal, "Line 4");

		//it was found with one type signature algorithm was reproducing
		//deferent hash values when calling Flush more than once without adding new entry. 
		Logger1::L().Flush();
		EXPECT_TRUE(Logger1::L().CheckSignature());
		std::string line;
		std::getline(iostream, line);
		iostream.seekg(-1, std::ios::cur);
		iostream << "BLAH\n";
		iostream.seekg(0, std::ios::end);
		iostream.close();

		EXPECT_FALSE(Logger1::L().CheckSignature());

		Logger1::L().Log("Test-UC_40_2_1", severity_level::normal, "Line 5");
		Logger1::L().Log("Test-UC_40_2_1", severity_level::normal, "Line 6");
		Logger1::L().Log("Test-UC_40_2_1", severity_level::normal, "Line 7");
		Logger1::L().Flush();

		EXPECT_FALSE(Logger1::L().isValidSignature()); //should still be invalid signature
		EXPECT_FALSE(Logger1::L().CheckSignature());
	}

	/**
	* \brief – Test the construction and destruction of classes that inherited Logger Signature class.
	* \test – Currently, this test case is disabled. Since the destruction of a singleton is unpredictable.  
	*/
	TEST(LogSign, DISABLED_UC_40_2_1_file_integrityCheck)
	{
		//
		class fake_signedlogger : public LoggerSignature<3>
		{
		public:
			fake_signedlogger() {}
		};

		//Create and initialize a new fake logger.
		boost::system::error_code ec;
		auto fakelog = std::make_shared<fake_signedlogger>();
		fakelog->L().Initialize(ec, "UnitTests3.info");
		EXPECT_EQ(boost::system::errc::success, ec);

		fakelog->L().Log("Test-UC_40_2_1", severity_level::normal, "Line 1");
		fakelog->L().Log("Test-UC_40_2_1", severity_level::normal, "Line 2");
		fakelog->L().Log("Test-UC_40_2_1", severity_level::normal, "Line 3");
		fakelog->L().Log("Test-UC_40_2_1", severity_level::normal, "Line 4");
		fakelog->L().Log("Test-UC_40_2_1", severity_level::normal, "Line 5");
		EXPECT_TRUE(fakelog->L().isValidSignature());
		fakelog.reset();//the destructor would be called.

		fakelog.reset(new fake_signedlogger);
		fakelog->L().Initialize(ec, "UnitTests3.info");
		EXPECT_EQ(boost::system::errc::success, ec);
		EXPECT_TRUE(fakelog->L().isValidSignature());
		EXPECT_TRUE(fakelog->L().CheckSignature());
		fakelog.reset();

		std::fstream iostream(".\\UnitTests3.log", std::fstream::in|std::fstream::out);
		ASSERT_TRUE(iostream.is_open());
		iostream.seekp(73, std::ios::beg);
		iostream.put('P');
		iostream.close();

		fakelog.reset(new fake_signedlogger);
		fakelog->L().Initialize(ec, "UnitTests3.info");
		EXPECT_EQ(boost::system::errc::success, ec);
		EXPECT_FALSE(fakelog->L().isValidSignature());
		EXPECT_FALSE(fakelog->L().CheckSignature());
		fakelog.reset();

		boost::filesystem::remove(".\\UnitTests3.log");//we do not need the log anymore
	}

	/**
	 * \brief 
	 * \param filename 
	 */
	static void compromise_file(std::string filename)
	{
		std::fstream iostream(filename, std::fstream::in | std::fstream::out);
		ASSERT_TRUE(iostream.is_open());
		iostream.seekg(0, std::ios::end);
		auto mid_point = iostream.tellg() / 2;
		iostream.seekp(mid_point, std::ios::beg);
		iostream << "BLAH";
		iostream.close();
	}
	
	/**
	* \brief – .
	* \test – .
	*/
	TEST(LogSign, UC_40_2_1_logfileHasBeenCompromised_manuallyBackupLog)
	{
		//we need a clean slate for this test case
		if (boost::filesystem::exists(".\\UnitTests3.log"))
			boost::filesystem::remove(".\\UnitTests3.log");

		boost::system::error_code ec;
		using logger3 = LoggerSignature<3>;
		logger3::L().Initialize(ec, "UnitTests3.info");
		EXPECT_EQ(boost::system::errc::success, ec);

		logger3::L().SetAlwaysFlush(true);//make sure the log is flushed at every new log entry.
		logger3::L().setAutoCompromisedBackup(false);
		logger3::L().Log("Test-UC_40_2_1", severity_level::normal, "Line 1");
		logger3::L().Log("Test-UC_40_2_1", severity_level::normal, "Line 2");
		logger3::L().Log("Test-UC_40_2_1", severity_level::normal, "Line 3");
		EXPECT_TRUE(logger3::L().CheckSignature());
		
		//Compromise the log file
		compromise_file(".\\UnitTests3.log");

		EXPECT_FALSE(logger3::L().CheckSignature());
		EXPECT_TRUE(logger3::L().hasLogsBeenCompromised());
		EXPECT_TRUE(logger3::L().BackupCompromisedLog());
		logger3::L().Log("Test-UC_40_2_1", severity_level::normal, "Line 1");
		logger3::L().Log("Test-UC_40_2_1", severity_level::normal, "Line 2");
		logger3::L().Log("Test-UC_40_2_1", severity_level::normal, "Line 3");
		EXPECT_TRUE(logger3::L().CheckSignature());
		EXPECT_TRUE(logger3::L().hasLogsBeenCompromised());//It would return true until there is are no compromised log files. 
		size_t files_count;
		EXPECT_TRUE(logger3::L().deleteCompromisedLogs(files_count));
		EXPECT_EQ(1, files_count);
		EXPECT_FALSE(logger3::L().hasLogsBeenCompromised());//It would return true unit there are no compromised logs backups. 

		logger3::L().CloseLogFile();
	}

	/**
	* \brief – .
	* \test – .
	*/
	TEST(LogSign, UC_40_2_1_logfileHasBeenCompromised_automaticallyBackupLog)
	{
		//we need a clean slate for this test case
		if (boost::filesystem::exists(".\\UnitTests3.log"))
			boost::filesystem::remove(".\\UnitTests3.log");

		boost::system::error_code ec;
		using logger3 = LoggerSignature<3>;
		logger3::L().Initialize(ec, "UnitTests3.info");
		EXPECT_EQ(boost::system::errc::success, ec);

		logger3::L().SetAlwaysFlush(true);//make sure the log is flushed at every new log entry.
		logger3::L().setAutoCompromisedBackup(true);//backup the log when it's been Compromised

		logger3::L().Log("Test-UC_40_2_1", severity_level::normal, "Line 1");
		EXPECT_TRUE(logger3::L().CheckSignature());
		EXPECT_FALSE(logger3::L().hasLogsBeenCompromised());

		//Compromise the log file
		compromise_file(".\\UnitTests3.log");
		EXPECT_FALSE(logger3::L().CheckSignature());
		EXPECT_TRUE(logger3::L().hasLogsBeenCompromised());

		logger3::L().Log("Test-UC_40_2_1", severity_level::normal, "Line 1");
		compromise_file(".\\UnitTests3.log");
		EXPECT_FALSE(logger3::L().CheckSignature());
		EXPECT_TRUE(logger3::L().hasLogsBeenCompromised());

		logger3::L().Log("Test-UC_40_2_1", severity_level::normal, "Line 1");
		compromise_file(".\\UnitTests3.log");
		EXPECT_FALSE(logger3::L().CheckSignature());
		std::vector <std::string>compromised_files;
		EXPECT_TRUE(logger3::L().hasLogsBeenCompromised(compromised_files));		
		EXPECT_EQ(3, compromised_files.size());

		size_t files_count;
		EXPECT_TRUE(logger3::L().deleteCompromisedLogs(files_count));
		EXPECT_EQ(3, files_count);
		logger3::L().Log("Test-UC_40_2_1", severity_level::normal, "Line 1");
		EXPECT_FALSE(logger3::L().hasLogsBeenCompromised());//It would return true unit there are no compromised logs backups. 

		logger3::L().CloseLogFile();
	}

	/**
	* \brief – Authenticity of the log file must be checkable.
	* \test – This test case would create a fake log file with correct hashes for each log entry and 
	* file hash. Then the test would replace one of the original log entry with a 
	* different but valid log entry.  The authenticity should fail. 
	*/
	TEST(LogSign, UC_40_2_1_hashcheck_suspectEntry)
	{
		//we need a clean slate for this test case
		if (boost::filesystem::exists(".\\UnitTests3.log"))
			boost::filesystem::remove(".\\UnitTests3.log");
		//there is no easy way to test invalid hashes 
		//We will create a fake log, manually make log entries.
		std::fstream iostream(".\\UnitTests3.log", std::fstream::out | std::fstream::app);
		ASSERT_TRUE(iostream.is_open());
		iostream << "#16012557B4382EF6140EFCC8824255D29D0269525A47F53544043D3F5A0669AB#[2000-Jan-01 00:00:00.000000][NORM]<UC_40_2_1> Line 1\n";
		iostream << "#99B27863F5887FB692E52E5A66EEDC969FD8509F5CB1176829AB945758DE2814#[2000-Jan-01 00:00:01.000000][NORM]<UC_40_2_1> FOO\n";
		//iostream << "#32602D051B6CF97BCD3C5DB114BF40CDA3CC9C96020EE4FDD7D3E1CEA8013255#[2000-Jan-01 00:00:02.000000][NORM]<UC_40_2_1> BAR\n";//this line was used to calculate the file hash
		iostream << "#BAA5F5AB70C922483251D72A89540A6A76B9B505A94B1B64F7557CF5117EF302#[2000-Jan-01 00:00:02.000000][NORM]<UC_40_2_1> FOOBAR\n";//this line has a valid entry hash
		iostream << "#3F83BACA18E9F0E3474D38D625169370CD900FA946D30E2911E444953F5B93C1#[2000-Jan-01 00:00:03.000000][NORM]<UC_40_2_1> Line 4\n";
		iostream << "C96A7D5B3104415652D9CF3CCD279556C61371CBE321DF5D8AF5D30F6AEC58F5";
		iostream.close();

		boost::system::error_code ec;
		using logger3 = LoggerSignature<3>;
		logger3::L().Initialize(ec, "UnitTests3.info");
		EXPECT_EQ(boost::system::errc::success, ec);
		logger3::L().SetAlwaysFlush(false);
		logger3::L().setAutoCompromisedBackup(false);

		std::vector<LogEntry> log_entries;
		logger3::L().getLogEntriesIndex(0, LAST_ENTRY_INDEX, log_entries); //"-1" Special value to indicate the last position.   
		//the entries have valid hashes 
		EXPECT_TRUE(log_entries[0].isValid);
		EXPECT_TRUE(log_entries[1].isValid);
		EXPECT_TRUE(log_entries[2].isValid);
		EXPECT_TRUE(log_entries[3].isValid);

		//but the file hash would be invalid
		//
		EXPECT_FALSE(logger3::L().CheckSignature()); 
		logger3::L().CloseLogFile();
	}

	/**
	* \brief – Authenticity of the log file must be checkable.
	* \test – This test case would create a fake log file with correct hashes for each log entry 
	* and file hash. Then the test would replace the string of one of the original log entry with a 
	* different string. But log entry hash would stay the same.  The authenticity should fail.
	*/
	TEST(LogSign, UC_40_2_1_hashcheck_modifiedEntry)
	{
		//we need a clean slate for this test case
		if (boost::filesystem::exists(".\\UnitTests3.log"))
			boost::filesystem::remove(".\\UnitTests3.log");
		//there is no easy way to test invalid hashes 
		//We will create a fake log, manually make log entries.
		std::fstream iostream(".\\UnitTests3.log", std::fstream::out);
		ASSERT_TRUE(iostream.is_open());
		iostream << "#05BBD12D2B59D5206B7D4F46923881B2ACBF0BF3905E71E94438AAB46F3B6810#[2000-Jan-01 00:00:00.000000][NORM]<UC_40_2_1> Line 1\n";
		iostream << "#9610927B4BEA2330F2B951500FE1C71132C70C8CB66EB62791CBECF506CC4A17#[2000-Jan-01 00:00:01.000000][NORM]<UC_40_2_1> FOO\n";
		iostream << "#F4202100580659B8399F5891EDAADA3CCF7A30C1AD6E21E7E88BA11276D7D01A#[2000-Jan-01 00:00:02.000000][NORM]<UC_40_2_1> Bar\n";//originally, this read "BAR" it was changed to "Bar"
		iostream << "#6A7AFAD1ECC9142D760B1E258D44DF4C7BDF40AB8DFF156C85ECF8447D835791#[2000-Jan-01 00:00:03.000000][NORM]<UC_40_2_1> Line 4\n";
		iostream << "E8869034A87BA72CD04B79A2CE671DB132F4E29E05032458A06E2DBF0AFE0A4A";

		iostream.close();

		boost::system::error_code ec;
		using logger3 = LoggerSignature<3>;
		logger3::L().Initialize(ec, "UnitTests3.info");
		EXPECT_EQ(boost::system::errc::success, ec);
		logger3::L().SetAlwaysFlush(false);
		logger3::L().setAutoCompromisedBackup(false);

		std::vector<LogEntry> log_entries;
		logger3::L().getLogEntriesIndex(0, LAST_ENTRY_INDEX, log_entries);
		//the entries have valid hashes , except 3rd line
		EXPECT_TRUE(log_entries[0].isValid);
		EXPECT_TRUE(log_entries[1].isValid);
		EXPECT_FALSE(log_entries[2].isValid);
		EXPECT_TRUE(log_entries[3].isValid);

		//The file is invalid even if the line hashes still adds up to the file hash.
		//Since there is an invalid entry.
		EXPECT_FALSE(logger3::L().isValidSignature());
		EXPECT_FALSE(logger3::L().CheckSignature());
		logger3::L().CloseLogFile();
	}

	/**
	* \brief –  Log entries should be represented as objects so that we can pass them around / smartly
	*           extract information from them - a plain old string is probably insufficient.
	* \test  -  The test would create 5 log entries. Then get these entries and confirm the entries 
	*           are correct and valid. 
	*/
	TEST(LogSign, UC_40_3_1)
	{
		Logger2::L().Log("UC_40_3_1", severity_level::normal, "Line 1");
		Logger2::L().Log("UC_40_3_1", severity_level::normal, "Line 2");
		Logger2::L().Log("UC_40_3_1", severity_level::normal, "Line 3\n");
		Logger2::L().Log("UC_40_3_1", severity_level::normal, "Line 4");
		Logger2::L().Log("UC_40_3_1", severity_level::normal, "Line 5");

		EXPECT_EQ(5, Logger2::L().getTotalEntries());

		LogEntry log_entry;
		Logger2::L().getLogEntryIndex(2, log_entry);
		EXPECT_EQ("Line 3\n", log_entry.LogMessage);

		std::vector<LogEntry> log_entries;
		Logger2::L().getLogEntriesIndex(1, LAST_ENTRY_INDEX, log_entries);
		EXPECT_EQ(4, log_entries.size());
		EXPECT_EQ("Line 2", log_entries[0].LogMessage);
		EXPECT_EQ("Line 3\n", log_entries[1].LogMessage);
		EXPECT_EQ("Line 4", log_entries[2].LogMessage);

		log_entries.clear();
		Logger2::L().getLogEntriesIndex(2, LAST_ENTRY_INDEX, log_entries);
		EXPECT_EQ("Line 3\n", log_entries[0].LogMessage);
		EXPECT_EQ("Line 4", log_entries[1].LogMessage);
		EXPECT_EQ("Line 5", log_entries[2].LogMessage);
	}

	/**
	* \brief – This test case would confirm be able to obtain entries objects by using timestamps.  
	* \test – The tests would create 10 log entries that are 1 second apart. 
	* The times are captured of the particular log  entries. These times are used 
	* to retrieve log entries.
	*/
	TEST(LogSign, UC_40_3_1_TimeStamp)
	{
		boost::posix_time::time_duration diff;
		boost::posix_time::ptime pt_line6;
		boost::posix_time::ptime pt_line8;
		boost::posix_time::ptime pt_line10;
		
		//the logger timestamp only has resolution of a second.
		//I would be adding additional 5 lines to the log file
		//for the next series of tests. 
		for(auto i=6;i<=10;i++)
		{
			::Sleep(1000);
			if(i==6) pt_line6 = boost::posix_time::second_clock::local_time();
			if(i==8) pt_line8 = boost::posix_time::second_clock::local_time();
			Logger2::L().Log("UC_40_3_1", severity_level::normal, "Line "+std::to_string(i));
		}
		pt_line10 = boost::posix_time::second_clock::local_time();
		EXPECT_EQ(5, Logger2::L().getIndexFromTimestamp(pt_line6));
		EXPECT_EQ(7, Logger2::L().getIndexFromTimestamp(pt_line8));
		diff = pt_line10 - Logger2::L().getTimestampFromIndex(9);
		EXPECT_EQ(0, diff.total_seconds());//the difference could be off by microseconds, but that's ok

		LogEntry log_entry;
		Logger2::L().getLogEntryTimestamp(pt_line8, log_entry);
		EXPECT_EQ("Line 8", log_entry.LogMessage);
		std::vector<LogEntry> log_entries;
		Logger2::L().getLogEntriesTimestamp(pt_line6, pt_line8, log_entries);
		//I just need to know we got the right count of log entries
		EXPECT_EQ(3, log_entries.size());

		log_entries.clear();
		Logger2::L().getLogEntriesAfterTimestamp(pt_line6, log_entries);
		EXPECT_EQ(5, log_entries.size());

		Logger2::L().Flush();
	}

	/**
	* \brief – Should be able to append data to an existing log (with file signature being reset as appropriate)
	* \test  - This test would create a fake log file with valid entry and file hashes. A newly created signed
	*  logger will open this log file and append to it. The test will then create new log entries 
	*  and confirm if is able to do maintenance on them.  
	*/
	TEST(LogSign, UC_40_4_1)
	{
		//we need a clean slate for this test case
		if (boost::filesystem::exists(".\\UnitTests3.log"))
			boost::filesystem::remove(".\\UnitTests3.log");
		//there is no easy way to test the appending of logs. 
		//We will create a fake log, then open it through the logger.
		std::string line;
		std::fpos<_Mbstatet> end_of_fakelog;
		std::fstream iostream(".\\UnitTests3.log", std::fstream::out);
		ASSERT_TRUE(iostream.is_open());
		line = "#A7A0264D32E6BD66F23211E5A983B2E1DE0C1457B44F1F46644F24297D352724#[2000-Jan-01 00:00:00.000000][NORM]<UC_40_4_1> Line 1\n";
		iostream << line;
		line = "CE42879F5DE87ED03A9E0D8F47C482DD0CB043A8D04E25D05AFC5EB3F6C89235"; 
		iostream << line;

		end_of_fakelog = iostream.tellp();//save the position so I can use it later
		iostream.close();

		boost::system::error_code ec;
		using logger3 = LoggerSignature<3>;
		logger3::L().Initialize(ec, "UnitTests3.info");
		EXPECT_EQ(boost::system::errc::success, ec);
		logger3::L().SetAlwaysFlush(false);
		logger3::L().setAutoCompromisedBackup(false);
		EXPECT_TRUE(logger3::L().CheckSignature()); //this confirms that we were able to open the fake log.
		
		LogEntry entry;
		logger3::L().getLogEntryIndex(0, entry);
		EXPECT_TRUE(entry.isValid);//this validates the fake log entry.

		logger3::L().Log("UC_40_4_1", severity_level::normal, "Line 2");
		logger3::L().Log("UC_40_4_1", severity_level::normal, "Line 3");
		logger3::L().Log("UC_40_4_1", severity_level::normal, "Line 4");
		logger3::L().Log("UC_40_4_1", severity_level::normal, "Line 5");
		logger3::L().Flush();
		EXPECT_TRUE(logger3::L().CheckSignature());

		iostream.open(".\\UnitTests3.log", std::fstream::in | std::fstream::out);
		auto newline_count = std::count(std::istreambuf_iterator<char>(iostream), std::istreambuf_iterator<char>(), '\n');
		EXPECT_EQ(5, newline_count);//this we were able to append to the fake log.
		EXPECT_EQ(logger3::L().getTotalEntries(), newline_count);//confirms the internal matches.

		iostream.seekp(end_of_fakelog, std::ios::beg);
		iostream << "BLAH";
		iostream.close();
		EXPECT_FALSE(logger3::L().CheckSignature());//This confirms operation on the fake log.
		logger3::L().CloseLogFile();
	}

	/**
	* \brief – Interface to discard data before a given date/time. Naturally, use of this interface should
	*          create an automatic entry in the log file :smile:
	* \test - The test case would use previous test logs to confirm deletion of these log entries. 
	*/
	TEST(LogSign, UC_40_5_1)
	{
		LogEntry log_entry;
		Logger2::L().getLogEntryIndex(2, log_entry);
		EXPECT_EQ("Line 3\n", log_entry.LogMessage);

		//removed the 3rd entry, so the 4th entry 
		//is the new 3rd
		Logger2::L().deleteLogEntryIndex(2);
		EXPECT_TRUE(Logger2::L().CheckSignature());
		Logger2::L().getLogEntryIndex(2, log_entry);
		EXPECT_EQ("Line 4", log_entry.LogMessage);

		Logger2::L().deleteLogEntriesIndex(3, 7);
		EXPECT_TRUE(Logger2::L().CheckSignature());
		EXPECT_EQ(6, Logger2::L().getTotalEntries()); //The 6th entry is the message about previous deletion

		Logger2::L().deleteLogEntriesThroughIndex(2);
		EXPECT_EQ(4, Logger2::L().getTotalEntries()); //The last 2 log entries are  about previous deletions
		EXPECT_TRUE(Logger2::L().CheckSignature());

		//clean up the log
		Logger2::L().deleteLogEntriesIndex(0, LAST_ENTRY_INDEX);
		//make sure we are able to write back to the log
		Logger2::L().Log("UC_40_5_1", severity_level::normal, "Line 1");
		Logger2::L().Flush();
		EXPECT_TRUE(Logger2::L().CheckSignature());
	}

	/**
	* \brief – Test the ability to perform maintenance on the entered log entries. 
	* \test – The tests would create 10 log entries that are 1 second apart. 
	* The  times are captured of the particular log  entries. These times are used 
	* to retrieve and do maintenance of these log entries.
	*/
	TEST (LogSign, UC_40_5_1_TimeStamp)
	{
		LogEntry log_entry;
		Logger2::L().deleteLogEntriesIndex(0, LAST_ENTRY_INDEX);//I would like to have no lines entered

		boost::posix_time::ptime pt_top;
		boost::posix_time::ptime pt_line3;
		boost::posix_time::ptime pt_line5;
		boost::posix_time::ptime pt_line8;
		boost::posix_time::ptime pt_line10;

		//the logger timestamp only has resolution of a second.
		for (auto i = 1; i <= 10; i++)
		{
			::Sleep(1000);
			if (i == 3) pt_line3 = boost::posix_time::second_clock::local_time();
			if (i == 5) pt_line5 = boost::posix_time::second_clock::local_time();
			if (i == 8) pt_line8 = boost::posix_time::second_clock::local_time();
			Logger2::L().Log("UC_40_5_1", severity_level::normal, "Line " + std::to_string(i));
		}
		pt_line10 = boost::posix_time::second_clock::local_time();

		EXPECT_EQ(11, Logger2::L().getTotalEntries());//this includes the deletion message from previous

		Logger2::L().getLogEntryIndex(8, log_entry);//index 9 is line 8;
		EXPECT_EQ("Line 8", log_entry.LogMessage);
		Logger2::L().deleteLogEntryTimestamp(pt_line8);
		Logger2::L().getLogEntryIndex(8, log_entry);
		EXPECT_EQ("Line 9", log_entry.LogMessage);//line 8 is removed, it should be line 9 now
		Logger2::L().Flush();
		
		Logger2::L().deleteLogEntriesTimestamp(pt_line5, pt_line10);
		EXPECT_EQ(7, Logger2::L().getTotalEntries());//4 inserted lines but 3 deletion notices.
		Logger2::L().Flush();

		Logger2::L().deleteLogEntriesPriorToTimestamp(pt_line3);
		Logger2::L().getLogEntryIndex(0, log_entry);
		EXPECT_EQ("Line 3", log_entry.LogMessage);//the first entry should be line 3 now
		Logger2::L().Flush();

		pt_top = Logger2::L().getTimestampFromIndex(0);
		::Sleep(1000); //wait a second so we would be outside the last log entry
		Logger2::L().deleteLogEntriesTimestamp( pt_top, boost::posix_time::second_clock::local_time());
		EXPECT_EQ(1, Logger2::L().getTotalEntries());//you just have one entry, the last deletion message
		Logger2::L().Flush();
	}

	/**
	* \brief – These tests would confirm the behavior of the signature logging system 
	* would be able to handle inputs that are outside of rage of log entries.
	* \test – The tests would create 10 log entries that are 1 second apart. The 
	* times are captured before, after and middle of the inserting of these log entries. 
	* These times are used to retrieve and do maintenance of these log entries. .
	*/
	TEST(LogSign, UC_40_5_1_Exceptions)
	{
		LogEntry log_entry;
		Logger2::L().deleteLogEntriesIndex(0, LAST_ENTRY_INDEX);//I would like to have no lines entered

		boost::posix_time::ptime pt_invalid;
		boost::posix_time::ptime pt_mid_skip;
		boost::posix_time::ptime pt_line1;
		boost::posix_time::ptime pt_line6;
		boost::posix_time::ptime pt_outside_upper;
		boost::posix_time::ptime pt_outside_lower;

		//the logger timestamp only has resolution of a second.
		pt_outside_upper = boost::posix_time::second_clock::local_time();
		for (auto i = 1; i <= 3; i++)
		{
			::Sleep(1000);
			if(i==1) pt_line1 = boost::posix_time::second_clock::local_time();
			Logger2::L().Log("UC_40_5_1", severity_level::normal, "Line " + std::to_string(i));
		}

		::Sleep(1000);
		pt_mid_skip = boost::posix_time::second_clock::local_time();
		::Sleep(1000);

		for (auto i = 4; i <= 6; i++)
		{
			::Sleep(1000);
			Logger2::L().Log("UC_40_5_1", severity_level::normal, "Line " + std::to_string(i));
		}
		pt_line6 = boost::posix_time::second_clock::local_time();
		::Sleep(1000);
		pt_outside_lower = boost::posix_time::second_clock::local_time();

		std::vector<LogEntry> log_entries;
		Logger2::L().getLogEntriesTimestamp(pt_mid_skip, pt_line6, log_entries);
		EXPECT_EQ(7, Logger2::L().getTotalEntries());//

		EXPECT_FALSE(Logger2::L().getIndexFromTimestamp(pt_mid_skip));
		
		Logger2::L().deleteLogEntryTimestamp(pt_mid_skip);
		EXPECT_EQ(7, Logger2::L().getTotalEntries());//

		Logger2::L().deleteLogEntriesTimestamp(pt_line1, pt_outside_upper);
		EXPECT_EQ(7, Logger2::L().getTotalEntries());//

		Logger2::L().deleteLogEntriesTimestamp(pt_outside_upper, pt_line1);
		EXPECT_EQ(6, Logger2::L().getTotalEntries());

		Logger2::L().deleteLogEntriesTimestamp(pt_line1, pt_outside_lower);
		EXPECT_EQ(1, Logger2::L().getTotalEntries());//

		Logger2::L().deleteLogEntriesTimestamp(pt_invalid, pt_outside_lower);
		EXPECT_EQ(1, Logger2::L().getTotalEntries());//

		Logger2::L().Log("UC_40_5_1", severity_level::normal, "i need extra log entry for the next tests to pass.");
	}
	
	/**
	* \brief –  The incoming log message should be able to keep all its formatting and any
	*  new lines it might have. 
	* \test – The test will create 3 log entries that will have newlines and tabs. Then
	*  3 more log entries that are simple with no extra formatting. The tests would confirm
	*  retrieval and maintenance of log entries. 
	*/
	TEST(LogSign, UC_40_6_1)
	{
		std::string muli_line_1 = 
			"Line 1\n" 
			"lin1.1\ttab 1\ttab 2\ttab 3\n"
			"lin1.2\ttab 1\ttab 2\ttab 3\n"
			"lin1.3\ttab 1\ttab 2\ttab 3\n";
		std::string muli_line_2 =
			"Line 2\n"
			"lin2.1\ttab 1\ttab 2\ttab 3\n"
			"lin2.2\ttab 1\ttab 2\ttab 3\n"
			"lin2.3\ttab 1\ttab 2\ttab 3\n";
		std::string muli_line_3 =
			"Line 3\n"
			"lin3.1\ttab 1\ttab 2\ttab 3\n"
			"lin3.2\ttab 1\ttab 2\ttab 3\n"
			"lin3.3\ttab 1\ttab 2\ttab 3\n";
		Logger2::L().Log("UC_40_6_1", severity_level::normal, muli_line_1);
		Logger2::L().Log("UC_40_6_1", severity_level::normal, muli_line_2);
		Logger2::L().Log("UC_40_6_1", severity_level::normal, muli_line_3);
		Logger2::L().Log("UC_40_6_1", severity_level::normal, "Line 4");
		Logger2::L().Log("UC_40_6_1", severity_level::normal, "Line 5");
		Logger2::L().Log("UC_40_6_1", severity_level::normal, "Line 6");
		Logger2::L().Flush();

		EXPECT_EQ(8, Logger2::L().getTotalEntries()); //+1 line from previous test
		EXPECT_FALSE(Logger1::L().CheckSignature());

		LogEntry log_entry;
		Logger2::L().getLogEntryIndex(2, log_entry);
		EXPECT_STREQ(muli_line_1.c_str(), log_entry.LogMessage.c_str());

		std::vector<LogEntry> log_entries;
		Logger2::L().getLogEntriesIndex(2, LAST_ENTRY_INDEX, log_entries);
		EXPECT_STREQ(muli_line_1.c_str(), log_entries[0].LogMessage.c_str());
		EXPECT_STREQ(muli_line_2.c_str(), log_entries[1].LogMessage.c_str());
		EXPECT_EQ("Line 4", log_entries[3].LogMessage);

		Logger2::L().deleteLogEntriesIndex(3, 5);
		EXPECT_FALSE(Logger1::L().CheckSignature());
		EXPECT_EQ(6, Logger2::L().getTotalEntries());

		//Logger2::L().Flush();
		EXPECT_FALSE(Logger2::L().CheckSignature());
		Logger2::L().getLogEntryIndex(2, log_entry);
		EXPECT_STREQ(muli_line_1.c_str(), log_entry.LogMessage.c_str());

		log_entries.clear();
		Logger2::L().getLogEntriesIndex(0, LAST_ENTRY_INDEX, log_entries);
		EXPECT_EQ(6, log_entries.size());
		Logger2::L().deleteLogEntriesIndex(0, LAST_ENTRY_INDEX);
		EXPECT_EQ(1, Logger2::L().getTotalEntries()); //one log entry of the removal of other entries
	}

	/**
	* \brief –  Need to test the original logging system.
	*/
	TEST(LogSign, OrginalLogger)
	{
		boost::system::error_code ec;
		//I want to start fresh with empty logger logs. 
		if (boost::filesystem::exists(".\\HawkeyeDLL.log"))
			boost::filesystem::remove(".\\HawkeyeDLL.log");
		if (boost::filesystem::exists(".\\HawkeyeDLL.log.1"))
			boost::filesystem::remove(".\\HawkeyeDLL.log.1");

		//crate an fake log file to check the logger system would roll this file.
		std::fstream iostream(".\\HawkeyeDLL.log", std::fstream::out|std::fstream::app);
		iostream.close();
		ASSERT_TRUE(boost::filesystem::exists(".\\HawkeyeDLL.log"));

		//start the logger and check to see it would append and roll the fake log.
		Logger::L().Initialize(ec,"HawkeyeDLL.info");
		EXPECT_EQ(boost::system::errc::success, ec);
		EXPECT_TRUE(boost::filesystem::exists(".\\HawkeyeDLL.log.1"));
		EXPECT_EQ(0, boost::filesystem::file_size(".\\HawkeyeDLL.log.1"));// make sure is the fake log file was the file that was rolled

		Logger::L().Log("OriginalLogger", severity_level::normal, "Line 1");
		Logger::L().Log("OriginalLogger", severity_level::normal, "Line 2");
		Logger::L().Log("OriginalLogger", severity_level::normal, "Line 3");
		Logger::L().Flush();
		EXPECT_LT(0, boost::filesystem::file_size(".\\HawkeyeDLL.log"));// the logger was able to append if the file size is grater than 0
	}

	/**
	* \brief – Return a string of human readable size of the bytes.
	* \note - Adapted from http://stackoverflow.com/questions/3758606/how-to-convert-byte-size-into-human-readable-format-in-java
	*/
	static std::string to_human_readable_byte_count(size_t bytes)
	{
		// Static lookup table of byte-based SI units
		static const std::string suffix[] = 
		{ "B", "kB", "MB", "GB", "TB", "EB", "ZB", "YB"};
		const int unit = 1024;
		int exp = 0;
		if (bytes > 0)
		{
			exp = min((int)(log(bytes) / log(unit)),
				(int) sizeof(suffix) / sizeof(suffix[0]) - 1);
		}
		return std::to_string(bytes / pow(unit, exp)) + suffix[exp];
	}

	/**
	* \brief –  This test is use to profile the memory and execution. 
	* \note - This test is disable since is an infinite loop. 
	*/
	TEST(LogSign, DISABLED_Profiling)
	{
		boost::system::error_code ec;
		Logger1::L().Initialize(ec,"UnitTests1.info");
		Logger1::L().deleteLogEntriesIndex(0, LAST_ENTRY_INDEX);
		std::ifstream instream(Logger1::L().LogFile(), std::ifstream::ate | std::ifstream::binary);
		ASSERT_TRUE(instream.is_open());
		
		std::stringstream ss;
		auto file_size_str = ::LoggingSignatureTests::to_human_readable_byte_count(instream.tellg());
		//Logger1::L().SetAlwaysFlush(false);
		while (true)
		{
			::Sleep(100);
			ss.str("");
			instream.seekg(0, std::ios::end);
			file_size_str = ::LoggingSignatureTests::to_human_readable_byte_count(instream.tellg());
			ss << "Total Entries:" << std::hex << Logger1::L().getTotalEntries() << "\t File Size: " << file_size_str;
			Logger1::L().Log("Test-Profiling", severity_level::normal, ss.str());
		}

	}


	
	/**
	* \brief –  This test was use to check to see the signed logger would start correctly
	*           with in the DLL.
	* \note - This test is disable since the Hawkeye logic runs in the background.
	*/
	TEST(LogSign, DISABLE_HawkeyeLogic_SignLogging_init)
	{
		Initialize();
		do
		{

		} while (true);
	}
}
