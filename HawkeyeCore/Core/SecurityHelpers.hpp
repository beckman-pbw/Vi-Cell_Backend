#pragma once

#include <string>
#include <vector>

#include <sha.h>
#include <chrono>
#include <gcm.h>
#include <aes.h>
#include <osrng.h>
#include <rsa.h>

enum eCONFIGDATATYPE
{
	eHashKey,
	eSignature,
	ePublicKey,
	eConfigData
};

enum eSECURITYTYPE
{
	eNoSecurity = 0,
	eLocalSecurity,
	eActiveDirectory
};

namespace SecurityHelpers
{
	#define  GCM_TAG_SIZE 12 /*96 bits*/
	static std::string HashHeader = "<---BEGIN-HASH--->";
	static std::string HashFooter = "<---END-HASH--->";
	static std::string SignatureHeader = "<---BEGIN-SIGNATURE--->";
	static std::string SignatureFooter = "<---END-SIGNATURE--->";
	static std::string PublicKeyHeader = "<---BEGIN-PUBLIC KEY--->";
	static std::string PublicKeyFooter = "<---END-PUBLIC KEY--->";
	static std::string ConfigurationHeader = "<---BEGIN-Configuration--->";
	static std::string ConfigurationFooter = "<---END-Configuration--->";

	// Converts Binary to Hex data
	std::string DataToText(const std::vector<unsigned char>& data);
	// Converts HEXDecimal to Binay Data
	std::vector<unsigned char> TextToData(const std::string& text);
	std::vector<unsigned char> GenerateRandomSalt(std::size_t length);
	std::string CalculateHash(const std::vector<unsigned char>& plainText, const std::vector<unsigned char>& saltValue, std::size_t repetitions);
	
	// Creates the File and writes the Given String
	bool WriteFile(const std::string& strData, const char *filename);
	// Fetches the Contents of the file and loads to the Given string.
	bool ReadFile(std::string& strData, const char *filename);

	class RunningSHADigest
	{
	public:
		RunningSHADigest(const std::vector<unsigned char>& saltValue, std::size_t repetitions);
		virtual ~RunningSHADigest() {};

		/// Call these functions to add additional data to the digest you're calculating.
		void UpdateDigest(const std::vector<unsigned char>& message);
		void UpdateDigest(const std::string& message);
		void UpdateDigest(const unsigned char* message, std::size_t length);

		/// NOTE: This is the FINAL call you should make to your running digest.  Do NOT call it multiple times.
		std::string FinalizeDigestToHash();

		/// This method will return the hash code for the input (UpdateDigest()) till now
		std::string GetTemporaryHash();

		/// To reuse this instance to calculate a hash for a new message using the same salt and repetition count
		void Reset();

	protected:
		std::vector<unsigned char> salt;
		std::size_t rep_count;
		CryptoPP::SHA256 sha;

	private:
		std::string finalizeDigestToHashInternal(CryptoPP::SHA256& shaObj);
	};

	std::chrono::system_clock::time_point getEpochTime();
	typedef std::function<uint64_t(std::chrono::system_clock::time_point)> timepoint_to_epoch_converter_t;

	template<typename T>
	uint64_t UnitsSinceEpoch(const std::chrono::system_clock::time_point& tp)
	{
		auto difference = tp - getEpochTime();

		return std::chrono::duration_cast<T>(difference).count();

	}

	extern const timepoint_to_epoch_converter_t HMAC_YEARS;
	extern const timepoint_to_epoch_converter_t HMAC_DAYS;
	extern const timepoint_to_epoch_converter_t HMAC_HOURS;
	extern const timepoint_to_epoch_converter_t HMAC_MINS;
	extern const timepoint_to_epoch_converter_t HMAC_SECS;

	std::string GenerateHMACPasscode(timepoint_to_epoch_converter_t scale);
	std::string GenerateHMACPasscode(int8_t epochoffset, timepoint_to_epoch_converter_t scale);
	std::string GenerateHMACPasscode(std::chrono::system_clock::time_point timepoint, timepoint_to_epoch_converter_t scale);
	std::string GenerateHMACPasscode(std::chrono::system_clock::time_point timepoint, timepoint_to_epoch_converter_t scale, const std::string& instrument_key );
	std::string GenerateHMACPasscode(std::chrono::system_clock::time_point timepoint, int8_t epochoffset, timepoint_to_epoch_converter_t scale);
	std::string GenerateHMACPasscode(std::chrono::system_clock::time_point timepoint, int8_t epochoffset, timepoint_to_epoch_converter_t scale, const std::string& keyval );


	bool ValidateHMACPasscode( const std::string& passcode, timepoint_to_epoch_converter_t scale, uint8_t unitsbefore = 0, uint8_t unitsafter = 0 );
	bool ValidateServicePasscode( const std::string& passcode, timepoint_to_epoch_converter_t scale );
	bool ValidateResetPasscode( const std::string& passcode, timepoint_to_epoch_converter_t scale, const std::string instkey );

	bool SecurityEncryptFile(std::ifstream &in_file, std::ostream & out_stream, const std::string & iv_string, std::string salt, std::string & keyout);
	bool SecurityDecryptFile(std::ifstream &in_file, std::ostream & out_stream, const std::string & iv_string, const std::string& keyin);
	bool SecurityEncryptString(std::string &in_str, std::string & out_str, const std::string& ivIn, const std::string& keyin);
	bool SecurityDecryptString(std::string &in_str, std::string & out_str, const std::string& ivIn, const std::string& keyin);

	class RSADigitalSignature
	{
	public:
		RSADigitalSignature();
		RSADigitalSignature(const RSADigitalSignature&) = delete;
		~RSADigitalSignature();
		
		bool AreKeysValid();
		// Inputs
		//1. String data for Signing
		//OutPut 
		//1. Signature
		bool ApplySignature(const std::string & dataToSign, std::string & signature);

		// Inputs
		//1. DataSigned
		//2. RSA::PublicKey
		//3. Signature
		//Output
		//true or false
		static bool VerifySignature(const std::string & hashKey, CryptoPP::RSA::PublicKey & publicKey, const std::string & signature);

		//T- Input: RSA::PublicKey or RSA::PrivateKey
		template<class T>
		static bool ValidateKey(const T& rsakey)
		{
			CryptoPP::AutoSeededRandomPool rnd;
			if (!rsakey.Validate(rnd, 3))
				return false;
			return true;
		}

		//T- Input: RSA::PublicKey or RSA::PrivateKey
		template<class T>
		static bool LoadKeyFromString(const std::string& keystr, T& rsaKey)
		{
			try
			{
				CryptoPP::StringSource stringSource(keystr, true);
				rsaKey.BERDecode(stringSource);
			}
			catch (...)
			{
				return false;
			}
			return true;
		}

		template<class T>
		static bool SaveKeyToString(std::string& keystr, T& rsaKey)
		{
			try
			{
				CryptoPP::StringSink stringSink(keystr);
				rsaKey.DEREncode(stringSink);
				stringSink.MessageEnd();

			}
			catch (...)
			{
				return false;
			}
			return true;
		}

		CryptoPP::RSA::PrivateKey GetPrivateKey();
		CryptoPP::RSA::PublicKey GetPublicKey();
	private:
		std::shared_ptr<CryptoPP::RSA::PrivateKey> pPrivateKey_;
		std::shared_ptr<CryptoPP::RSA::PublicKey> pPublicKey_;
		void GenerateSignatureKeys();
	};
}
