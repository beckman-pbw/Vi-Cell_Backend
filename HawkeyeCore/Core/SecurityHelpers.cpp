#include "stdafx.h"

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include "ChronoUtilities.hpp"
#include "Logger.hpp"
#include "SecurityHelpers.hpp"

/*
 * for Crypto system:
 */
#include <hmac.h>
#include <chrono>
#include <ctime>
#include <default.h>
#include <pwdbased.h>

#include <files.h>
 #include <filters.h>
#include <hex.h>

static const char MODULENAME[] = "SecurityHelpers";

const SecurityHelpers::timepoint_to_epoch_converter_t SecurityHelpers::HMAC_YEARS = std::bind(&SecurityHelpers::UnitsSinceEpoch<std::chrono::duration<int, std::ratio<31557600 >>>, std::placeholders::_1);// "julian" years - 365.25 days @ 86400 seconds/day.  A good enough "year" as anything...
const SecurityHelpers::timepoint_to_epoch_converter_t SecurityHelpers::HMAC_HOURS = std::bind(&SecurityHelpers::UnitsSinceEpoch<std::chrono::hours>, std::placeholders::_1);
const SecurityHelpers::timepoint_to_epoch_converter_t SecurityHelpers::HMAC_MINS = std::bind(&SecurityHelpers::UnitsSinceEpoch<std::chrono::minutes>, std::placeholders::_1);
const SecurityHelpers::timepoint_to_epoch_converter_t SecurityHelpers::HMAC_SECS = std::bind(&SecurityHelpers::UnitsSinceEpoch<std::chrono::seconds>, std::placeholders::_1);

namespace SecurityHelpers
{
	std::chrono::system_clock::time_point getEpochTime()
	{
		std::tm fixedTime = { 0 };

		// Set fixed time as 01/01/1970 00:00:00
		fixedTime.tm_year = 70;
		fixedTime.tm_mon = 0;
		fixedTime.tm_mday = 1;
		fixedTime.tm_hour = 0;
		fixedTime.tm_min = 0;
		fixedTime.tm_sec = 0;

		auto epochTime_ = (std::chrono::system_clock::from_time_t(mktime(&fixedTime)));
		return epochTime_;
	}

	// Performs HEXEncoding on the given data
	std::string DataToText(const std::vector<unsigned char>& data)
	{
		CryptoPP::HexEncoder encoder;
		std::string output;

		encoder.Attach(new CryptoPP::StringSink(output));
		encoder.Put(data.data(), data.size());
		encoder.MessageEnd();

		return output;
	}
	
	// Performs HEXDecoding on the Given text.
	std::vector<unsigned char> TextToData(const std::string& text)
	{
		std::vector<unsigned char> decoded;
		CryptoPP::HexDecoder decoder;

		decoder.Put((byte*)text.data(), text.size());
		decoder.MessageEnd();

		std::size_t size = decoder.MaxRetrievable();
		if (size && size <= SIZE_MAX)
		{
			decoded.resize(size);
			decoder.Get((byte*)decoded.data(), decoded.size());
		}

		return decoded;
	}

	// Copies the contents of a string in to File
	bool WriteFile(const std::string & strData, const char * filename)
	{
		std::ofstream dataFile(filename, std::ofstream::binary | std::ofstream::out);
		if(!dataFile.is_open())
			return false;
		dataFile << strData;
		dataFile.close();

		return true;
	}

	// Copies the Contents of a file in to string
	bool ReadFile(std::string & strData, const char * filename)
	{

		std::ifstream dataFile(filename, std::ofstream::binary | std::ofstream::in);
		if (!dataFile.is_open())
			return false;

		strData = std::string((std::istreambuf_iterator<char>(dataFile)),
									std::istreambuf_iterator<char>());
		return true;
	}

	std::vector<unsigned char> GenerateRandomSalt(std::size_t length)
	{
		std::vector<unsigned char> retval(length, 0);
		CryptoPP::OS_GenerateRandomBlock(false, retval.data(), length);
		return retval;
	}

	std::string CalculateHash(const std::vector<unsigned char>& plainText, const std::vector<unsigned char>& saltValue, std::size_t repetitions)
	{
		RunningSHADigest sha(saltValue, repetitions);

		sha.UpdateDigest(plainText);
		return sha.FinalizeDigestToHash();
	}


	RunningSHADigest::RunningSHADigest(const std::vector<unsigned char>& saltValue, std::size_t repetitions) : salt(saltValue), rep_count(repetitions)
	{

	}

	void RunningSHADigest::UpdateDigest(const std::vector<unsigned char>& message)
	{
		sha.Update(message.data(), message.size());
	}

	void RunningSHADigest::UpdateDigest(const std::string& message)
	{
		sha.Update((unsigned char*)message.data(), message.length());
	}

	void RunningSHADigest::UpdateDigest(const unsigned char* message, std::size_t length)
	{
		sha.Update(message, length);
	}

	std::string RunningSHADigest::FinalizeDigestToHash()
	{
		return finalizeDigestToHashInternal(sha);
	}

	std::string RunningSHADigest::GetTemporaryHash()
	{
		// Create a clone of "sha" so that calling 
		// "finalizeDigestToHashInternal" won't affect the original hash-key update
		auto pTemp = (CryptoPP::SHA256*)sha.Clone();
		if (!pTemp)
		{
			return{};
		}

		auto output = finalizeDigestToHashInternal(*pTemp);
		delete pTemp;

		return output;
	}

	void RunningSHADigest::Reset()
	{
		sha.Restart();
	}

	std::string RunningSHADigest::finalizeDigestToHashInternal(CryptoPP::SHA256& shaObj)
	{
		std::vector<unsigned char> digest(CryptoPP::SHA256::DIGESTSIZE, 0);

		// Take whatever we've been given to date and add the salt.
		shaObj.Update(salt.data(), salt.size());
		shaObj.Final(digest.data());

		//
		// And now repeat the operation as requested.
		//
		{
			std::vector<unsigned char> digest_2(CryptoPP::SHA256::DIGESTSIZE, 0);

			for (std::size_t i = 0; i < rep_count; i++)
			{
				// Calculate the digest of the digest
				shaObj.CalculateDigest(digest_2.data(), digest.data(), digest.size());

				// Swap values so that "digest" always has the digested value
				std::swap(digest, digest_2);
			}
		}

		//
		// Stringify it for output.
		//
		return DataToText(digest);
	}

	std::string GenerateHMACPasscode(timepoint_to_epoch_converter_t scale)
	{
		return GenerateHMACPasscode(ChronoUtilities::CurrentTime(), 0, scale);
	}
	
	std::string GenerateHMACPasscode(int8_t epochoffset, timepoint_to_epoch_converter_t scale)
	{
		return GenerateHMACPasscode(ChronoUtilities::CurrentTime(), epochoffset, scale);
	}
	
	std::string GenerateHMACPasscode(std::chrono::system_clock::time_point timepoint, timepoint_to_epoch_converter_t scale)
	{
		return GenerateHMACPasscode(timepoint, 0, scale);
	}

	std::string GenerateHMACPasscode(std::chrono::system_clock::time_point timepoint, int8_t epochoffset, timepoint_to_epoch_converter_t scale, const std::string& keyval)
	{
		uint64_t UNITS;
		{
			UNITS = scale(timepoint);
		}
		UNITS += epochoffset;

		std::vector<unsigned char> encoded;
		{
			std::string plain;
			{
				CryptoPP::HexEncoder encoder;
				encoder.Attach(new CryptoPP::StringSink(plain));
				encoder.Put((unsigned char*)(&UNITS), sizeof(uint64_t));
				encoder.MessageEnd();
			}

			std::string mac;
			{
				CryptoPP::SecByteBlock key(keyval.size());
				memcpy_s((void*)key.data(), keyval.size(), (void*)keyval.data(), keyval.size());


				CryptoPP::HMAC< CryptoPP::SHA256 > hmac(key, key.size());
				CryptoPP::StringSource ss2(plain, true,
										   new CryptoPP::HashFilter(hmac,
																	new CryptoPP::StringSink(mac)
										   ) // HashFilter      
				); // StringSource
			}

			{
				plain.clear();
				CryptoPP::StringSource ss3(mac, true,
										   new CryptoPP::HexEncoder(
											   new CryptoPP::StringSink(plain)
										   ) // HexEncoder
				); // StringSource
			}

			encoded = TextToData(plain);
		}

		// Offset = lowest 4 bits of encoded.
		uint8_t offset = *(encoded.rbegin()) & 0x0F;

		uint32_t* iptr = (uint32_t*)(encoded.data() + offset);
		uint32_t i_val = *iptr;

		i_val = i_val & 0x8FFFFFFF;

		std::string passcode;
		for (int16_t i = 0; i < 6; i++)
		{
			uint32_t remainder = i_val % 10;
			i_val /= 10;
			char c = remainder + '0';
			passcode = c + passcode;
		}
		return passcode;
	}
	std::string GenerateHMACPasscode(std::chrono::system_clock::time_point timepoint, int8_t epochoffset, timepoint_to_epoch_converter_t scale)
	{

		return GenerateHMACPasscode(timepoint, epochoffset, scale, "\"Phil, Dennis and Perry are pretty neat guys\"");
		// Shared key "Phil, Dennis and Perry are pretty neat guys"
		// Message: UNITS since Epoch (determined by "scale")
		// Crypto: SHA256
		// 
		// Generate HMAC hash H of C+K
		// Offset O = H & 0x0F
		// Integer I = 4 bytes of H beginning at O bytes, & 0x8FFF
		// Token = lowest  6 digits of I (expressed in base 10) padded with leading zeroes.
	}

	//HMAC passcode based on with a +/- grace period.
	bool ValidateHMACPasscode(const std::string& passcode, timepoint_to_epoch_converter_t scale, uint8_t UNITSbefore, uint8_t UNITSafter)
	{
//NOTE: leaving the commented out logging statements here in case they are needed later.	
//		Logger::L().Log (MODULENAME, severity_level::debug1,
//			boost::str (boost::format ("ValidateHMACPasscode: <enter, passcode: %s, before: %d, after: %d>") 
//			% passcode
//			% (int)UNITSbefore
//			% (int)UNITSafter));

		for (uint8_t i = UNITSbefore; i > 0; i--)
		{
			std::string genCode = GenerateHMACPasscode(-1 * i, scale);
//			Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("ValidateHMACPasscode: <UNITSbefore, genCode: %s>") % genCode));
			if (passcode == genCode)
				return true;
		}

		std::string genCode2 = GenerateHMACPasscode(scale);
//		Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("ValidateHMACPasscode: <genCode2: %s>") % genCode2));
		if (passcode == genCode2)
			return true;
		
		for (uint8_t i = 1; i <= UNITSafter; i++)
		{
			std::string genCode3 = GenerateHMACPasscode(i, scale);
//			Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("ValidateHMACPasscode: <UNITSafter, genCode: %s>") % genCode3));
			if (passcode == genCode3)
				return true;
		}
		
		return false;
	}

	bool SecurityEncryptFile(std::ifstream &in_file, std::ostream & out_stream, const std::string & iv_string, const std::string  salt, std::string & keyout)// , const std::string& iv)
	{
		if (!in_file.is_open())
			return false;

		try
		{
			//byte key[CryptoPP::AES::DEFAULT_KEYLENGTH];
			CryptoPP::AutoSeededRandomPool random_pool;

			std::vector<byte> passcode(CryptoPP::AES::DEFAULT_KEYLENGTH);
			random_pool.GenerateBlock(passcode.data(), passcode.size());

			CryptoPP::SHA256 hash;
			std::vector<byte> key(CryptoPP::SHA256::DIGESTSIZE);
			hash.Update(passcode.data(), passcode.size());
			hash.Update((const byte*)salt.data(), salt.size());
			hash.Final(key.data());
			key.resize(CryptoPP::AES::DEFAULT_KEYLENGTH);//Do not think we need the maximum AES key length. 

			std::vector<byte> iv = TextToData(iv_string);

			CryptoPP::GCM<CryptoPP::AES >::Encryption encryptor;
			encryptor.SetKeyWithIV(key.data(), key.size(), iv.data(), iv.size());

			CryptoPP::FileSource(in_file, true,	// NOLINT(misc-unused-raii)
								 new CryptoPP::AuthenticatedEncryptionFilter(
									 encryptor, new CryptoPP::FileSink(out_stream), false, GCM_TAG_SIZE
								 ));

			keyout = DataToText(key);
		}
		catch (CryptoPP::Exception& ex)
		{
			std::cerr << "Encryption Exception Occured.. error mesg: " << std::endl;
			std::cerr << ex.what() << std::endl;
			std::cerr << std::endl;
			return false;
		}

		return true;
	}

	bool SecurityDecryptFile(std::ifstream &in_file, std::ostream & out_stream, const std::string & iv_string, const std::string& keyin)
	{
		if (!in_file.is_open())
			return false;
		try
		{
			CryptoPP::GCM<CryptoPP::AES >::Decryption decryption;
			std::vector<byte> key = TextToData(keyin);
			std::vector<byte> iv = TextToData(iv_string);
			decryption.SetKeyWithIV(key.data(), key.size(), iv.data(), iv.size());

			CryptoPP::AuthenticatedDecryptionFilter decryption_filter(decryption, 
                                                                      new CryptoPP::FileSink(out_stream), 
                                                                      CryptoPP::AuthenticatedDecryptionFilter::DEFAULT_FLAGS,
                                                                      GCM_TAG_SIZE);
			CryptoPP::FileSource(in_file, true,	// NOLINT(misc-unused-raii)
                         new CryptoPP::Redirector(decryption_filter /*, PASS_EVERYTHING */));

			   // If the object does not throw, here's the only
			   //  opportunity to check the data's integrity
			bool pass = decryption_filter.GetLastResult();
			if (!pass) return false;
			
		}
		//need a better way to report a failure.
		catch (CryptoPP::HashVerificationFilter::HashVerificationFailed& e)
		{
			std::cerr << "Caught HashVerificationFailed..." << std::endl;
			std::cerr << e.what() << std::endl;
			std::cerr << std::endl;
			return false;
		}
		catch (CryptoPP::InvalidArgument& e)
		{
			std::cerr << "Caught InvalidArgument..." << std::endl;
			std::cerr << e.what() << std::endl;
			std::cerr << std::endl;
			return false;
		}
		catch (CryptoPP::Exception& e)
		{
			std::cerr << "Caught Exception..." << std::endl;
			std::cerr << e.what() << std::endl;
			std::cerr << std::endl;
			return false;
		}
		return true;
	}

	bool SecurityEncryptString(std::string &in_str, std::string & out_str, const std::string& ivIn, const std::string& keyin)
	{
		try
		{
			CryptoPP::CBC_Mode<CryptoPP::AES >::Encryption encryptor;
			std::vector<byte> key = TextToData(keyin);
			std::vector<byte> iv = TextToData(ivIn);
			encryptor.SetKeyWithIV(key.data(), key.size(), iv.data(), iv.size());

			if ((in_str.length() % 16) == 0)
				CryptoPP::StringSource(in_str, true, new CryptoPP::StreamTransformationFilter(encryptor, new CryptoPP::StringSink(out_str), CryptoPP::StreamTransformationFilter::NO_PADDING));
			else
				CryptoPP::StringSource(in_str, true, new CryptoPP::StreamTransformationFilter(encryptor, new CryptoPP::StringSink(out_str)));
		}
		catch (CryptoPP::Exception& ex)
		{
			std::cerr << "Encryption Exception Occured.. error mesg: " << std::endl;
			std::cerr << ex.what() << std::endl;
			std::cerr << std::endl;
			return false;
		}

		return true;
	}

	bool SecurityDecryptString(std::string &in_str, std::string & out_str, const std::string& ivIn, const std::string& keyin)
	{
		try
		{
			CryptoPP::CBC_Mode<CryptoPP::AES >::Decryption decryptor;
			std::vector<byte> key = TextToData(keyin);
			std::vector<byte> iv = TextToData(ivIn);
			decryptor.SetKeyWithIV(key.data(), key.size(), iv.data(), iv.size());

			if ((in_str.length() % 16) == 0)
				CryptoPP::StringSource(in_str, true, new CryptoPP::StreamTransformationFilter(decryptor, new CryptoPP::StringSink(out_str), CryptoPP::StreamTransformationFilter::NO_PADDING));
			else
				CryptoPP::StringSource(in_str, true, new CryptoPP::StreamTransformationFilter(decryptor, new CryptoPP::StringSink(out_str)));
		}
		catch (CryptoPP::Exception& ex)
		{
			std::cerr << "Decryption Exception Occured.. error mesg: " << std::endl;
			std::cerr << ex.what() << std::endl;
			std::cerr << std::endl;
			return false;
		}

		return true;
	}


} // namespace SecurityHelpers
