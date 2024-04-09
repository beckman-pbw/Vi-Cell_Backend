#include "stdafx.h"
#include "SecurityHelpers.hpp"

namespace SecurityHelpers
{
	//Digital Signature Function
	RSADigitalSignature::RSADigitalSignature()
	{
		GenerateSignatureKeys();
	}

	RSADigitalSignature::~RSADigitalSignature()
	{
		if(pPrivateKey_)
			pPrivateKey_.reset();
		if (pPublicKey_)
			pPublicKey_.reset();
	}

	bool RSADigitalSignature::AreKeysValid()
	{
		return ValidateKey<CryptoPP::RSA::PrivateKey>(*pPrivateKey_);
	}

	// Generates Private and Public keys
	void RSADigitalSignature::GenerateSignatureKeys()
	{
		// Pseudo Random Number Generator
		CryptoPP::AutoSeededRandomPool rng;
		// Key Generation
		CryptoPP::InvertibleRSAFunction keys;
		keys.Initialize(rng, 2048);

		pPrivateKey_ = std::make_shared<CryptoPP::RSA::PrivateKey>(keys);
		pPublicKey_ = std::make_shared<CryptoPP::RSA::PublicKey>(keys);
	}

	bool RSADigitalSignature::ApplySignature(const std::string& dataToSign, std::string& signature)
	{
		if (dataToSign.empty())
			return false;

		// Convert the text to Data
		std::vector<byte> hashkey = TextToData(dataToSign);
		std::vector<byte> signatureVec;

		// Pseudo Random Number Generator
		CryptoPP::AutoSeededRandomPool rng;
		try
		{
			// Signer
			CryptoPP::RSASS<CryptoPP::PKCS1v15, CryptoPP::SHA256>::Signer signer(*(this->pPrivateKey_));

			// Signature
			// NOTE: Resize is mandatory here to avail Continuous memory
			signatureVec.resize(signer.MaxSignatureLength());

			// Signature Length is 256 bytes
			//Using the Raw Data 
			size_t length = signer.SignMessage(rng, (byte *)dataToSign.c_str(),
											   dataToSign.size(), signatureVec.data());
			signature = std::string(signatureVec.begin(), signatureVec.end()); // Retunining the Raw Signature.
		}
		catch (...)
		{
			return false;
		}

		return true;
	}

	bool RSADigitalSignature::VerifySignature(const std::string& hashKey, CryptoPP::RSA::PublicKey& publicKey, const std::string& signature)
	{
		// Verification
		CryptoPP::RSASS<CryptoPP::PKCS1v15, CryptoPP::SHA256>::Verifier verifier(publicKey);
		return verifier.VerifyMessage((byte *)hashKey.c_str(), hashKey.size(), (byte *)signature.c_str(), signature.size());
	}

	CryptoPP::RSA::PrivateKey RSADigitalSignature::GetPrivateKey()
	{
		return *pPrivateKey_;
	}

	CryptoPP::RSA::PublicKey RSADigitalSignature::GetPublicKey()
	{
		return *pPublicKey_;
	}
}