#include "stdafx.h"

#include "HawkeyeUser.hpp"
#include "Logger.hpp"
#include "SecurityHelpers.hpp"

static std::string MODULENAME = "HawkeyeUser";

#define REPETITIONS 543
#define SALTSIZE 20
#define MAXPWQUEUESIZE 10

//*****************************************************************************
HawkeyeUser::HawkeyeUser()
	: validated(false), userName_("")
	, userLevel(UserPermissionLevel::eNormal)
	, isActive(false)
{
	pwHash.clear();
}

//*****************************************************************************
HawkeyeUser::HawkeyeUser (const std::string& username, const std::string& password, UserPermissionLevel permissionLevel, bool isItActive)
{
	// Create a new user from the provided data.
	userName_ = username;
	userLevel = permissionLevel;
	isActive = isItActive;
	pwSalt = SecurityHelpers::GenerateRandomSalt(SALTSIZE);
	
	std::vector<unsigned char> pwtemp(password.begin(), password.end());
	pwHash.push_front(SecurityHelpers::CalculateHash(pwtemp, pwSalt, REPETITIONS) );

	validated = true;
}

//*****************************************************************************
HawkeyeUser::~HawkeyeUser()
{
}

//*****************************************************************************
bool HawkeyeUser::ValidatePassword(const std::string plainText)
{
	// Can't validate on a bad user or empty PW set.
	if (!this->IsValid() || pwHash.empty())
		return false;

	/*
	 * Convert string to a v<uchar>
	 */
	std::vector<unsigned char> pT(plainText.begin(), plainText.end());

	/*
	 * Calculate the hash value for the supplied PW
	 */
	std::string calculatedHash = SecurityHelpers::CalculateHash(pT, pwSalt, REPETITIONS);
	
	/*
	 * See if it matches what we expect
	 */
	return (calculatedHash == pwHash.front());
}

//*****************************************************************************
std::string HawkeyeUser::SignText(const std::string& freeText)
{
	if (!IsValid())
		return "";

	/*
	 * Sign text based on username, current PW and salt.
	 */

	std::vector<unsigned char> message(freeText.begin(), freeText.end());
	
	std::vector<unsigned char> saltA = SecurityHelpers::TextToData(pwHash.front());
	std::vector<unsigned char> saltB(userName_.begin(), userName_.end());

	std::vector<unsigned char> salt;
	salt.insert(salt.end(), saltA.begin(), saltA.end());
	salt.insert(salt.end(), saltB.begin(), saltB.end());
	
	return SecurityHelpers::CalculateHash(message, salt, REPETITIONS);
}

//*****************************************************************************
bool HawkeyeUser::ValidateText(const std::string& freeText, const std::string& signature)
{
	if (!IsValid())
		return false;

	std::vector<unsigned char> saltA = SecurityHelpers::TextToData(pwHash.front());
	std::vector<unsigned char> saltB(userName_.begin(), userName_.end());

	std::vector<unsigned char> salt;
	salt.insert(salt.end(), saltA.begin(), saltA.end());
	salt.insert(salt.end(), saltB.begin(), saltB.end());

	std::vector<unsigned char> message(freeText.begin(), freeText.end());
	std::string hash = SecurityHelpers::CalculateHash(message, salt, REPETITIONS);
	
	return (hash == signature);
}

//*****************************************************************************
bool HawkeyeUser::FromStorageString(const std::string& fromTextLine)
{
	// Bogus until proven otherwise.
	{
		this->validated = false;
		this->isActive = false;
		this->userLevel = UserPermissionLevel::eNormal;
		this->userName_.clear();
		this->pwHash.clear();
		this->pwSalt.clear();
	}

	//     1         2       3          4            5           {6..N}
	// [username]:[salt]:[UL Hash]:[ActivationHash]:[PW Hash]{:[Old PW Hash N]}*
	if ((std::count(fromTextLine.begin(), fromTextLine.end(), ':') < 4) ||
		(std::count(fromTextLine.begin(), fromTextLine.end(), ':') > 4+(MAXPWQUEUESIZE-1)))
		return false;

	//1
	std::string::const_iterator st_iter = fromTextLine.begin();
	std::string::const_iterator en_iter = std::find(st_iter, fromTextLine.end(), ':');
	std::string _UN_(st_iter, en_iter);

	//2
	st_iter = en_iter + 1;
	en_iter = std::find(st_iter, fromTextLine.end(), ':');
	std::string _SALT_(st_iter, en_iter);
	
	//3
	st_iter = en_iter + 1;
	en_iter = std::find(st_iter, fromTextLine.end(), ':');
	std::string _ULHASH_(st_iter, en_iter);

	//4
	st_iter = en_iter + 1;
	en_iter = std::find(st_iter, fromTextLine.end(), ':');
	std::string _ACTHASH_(st_iter, en_iter);

	//5..N
	std::vector<std::string> _PWHASHLIST_;
	while (en_iter != fromTextLine.end())
	{
		st_iter = en_iter + 1;
		en_iter = std::find(st_iter, fromTextLine.end(), ':');
		std::string _PWHASH_(st_iter, en_iter);
		_PWHASHLIST_.push_back(_PWHASH_);
	}
	if (_PWHASHLIST_.empty())
		return false;

	// Record the user name.  If the rest fails, we can still assist the caller in getting display text for errors.
	this->userName_ = _UN_;

	/*
	 * Validation:
	 *   UserLevel - we should get a match on one of our user-level strings salted with the PW hash.
	 */
	std::vector<unsigned char> saltA = SecurityHelpers::TextToData(_PWHASHLIST_.front());
	std::vector<unsigned char> saltB(_UN_.begin(), _UN_.end());
	UserPermissionLevel plTEMP;
	{
		
		std::vector<unsigned char> hash = SecurityHelpers::TextToData(_ULHASH_);
		if (!MatchPermissionLevel(hash, saltA, saltB, plTEMP))
			return false;
	}

	bool actTEMP;
	{
		std::vector<unsigned char> hash = SecurityHelpers::TextToData(_ACTHASH_);
		if (!MatchActivationState(hash, saltA, saltB, actTEMP))
			return false;
	}

	// Validated.
	{
		this->userLevel = plTEMP;
		this->userName_ = _UN_;
		for (auto pwh : _PWHASHLIST_)
		{
			pwHash.push_back(pwh);
		}
		this->pwSalt = SecurityHelpers::TextToData(_SALT_);
		this->isActive = actTEMP;
		this->validated = true;
	}

//NOTE: Keep for debugging...
//std::string tSaltB;
//for (auto& v : saltB)
//{
//	tSaltB += v;
//}
//
//Logger::L().Log (MODULENAME, severity_level::debug1, "FromStorageString: isActive: " + std::to_string(isActive));
//Logger::L().Log (MODULENAME, severity_level::debug1, "FromStorageString: pwHash: " + pwHash.front());
//Logger::L().Log (MODULENAME, severity_level::debug1, "FromStorageString:  saltA: " + SecurityHelpers::DataToText (saltA));
//Logger::L().Log (MODULENAME, severity_level::debug1, "FromStorageString:  saltB: " + tSaltB + "\n");

	return true;
}

//*****************************************************************************
std::string HawkeyeUser::ToStorageString()
{
	//   [username]:[salt]:[PW Hash]:[UL Hash]:[ActivationHash]{:[Old PW Hash]}*
	if (!validated)
		return "";

	std::vector<unsigned char> saltA = SecurityHelpers::TextToData(pwHash.front());
	std::vector<unsigned char> saltB(userName_.begin(), userName_.end());

//NOTE: Keep for debugging...
//std::string tSaltB;
//for (auto& v : saltB)
//{
//	tSaltB += v;
//}
//
//Logger::L().Log (MODULENAME, severity_level::debug1, "ToStorageString: pwHash: " + pwHash.front());
//Logger::L().Log (MODULENAME, severity_level::debug1, "ToStorageString:  saltA: " + SecurityHelpers::DataToText (saltA));
//Logger::L().Log (MODULENAME, severity_level::debug1, "ToStorageString:  saltB: " + tSaltB + "\n");
//Logger::L().Log (MODULENAME, severity_level::debug1, "ToStorageString: isActive: " + std::to_string(isActive));

	std::string storage = userName_ 
	                      + ":" + SecurityHelpers::DataToText(pwSalt)
	                      + ":" + HashPermissionLevel(userLevel, saltA, saltB) 
	                      + ":" + HashActivationState(isActive, saltA, saltB);
	for (auto pwh : pwHash)
	{
		storage += ":" + pwh;
	}

	return storage;
}

//*****************************************************************************
bool HawkeyeUser::SetPassword(const std::string& newPassword, bool isAdUser)
{
	if (!IsValid())
		return false;

	std::vector<unsigned char> message(newPassword.begin(), newPassword.end());
	std::string temphash = SecurityHelpers::CalculateHash(message, pwSalt, REPETITIONS);

	if (isAdUser)
	{
		// AD users need to have just the current password in the list
		pwHash.clear();
		pwHash.push_back(temphash);
		return true;
	}

	// Can't repeat current or prior passwords
	for (auto pwh : pwHash)
	{
		if (temphash == pwh)
		{
			// cannot reuse old passwords
			Logger::L().Log(MODULENAME, severity_level::warning, "SetPassword: <attempt to re-use old password>");			
			return false;
		}
	}

	pwHash.push_front(temphash);
	while (pwHash.size() > MAXPWQUEUESIZE)
		pwHash.pop_back();

	return true;
}

//*****************************************************************************
bool HawkeyeUser::SetUserLevel(UserPermissionLevel newPL)
{
	if (!IsValid())
		return false;

	userLevel = newPL;
	return true;
}

//*****************************************************************************
bool HawkeyeUser::SetActivation(bool active)
{
	if (!IsValid())
		return false;

	isActive = active;
	return true;
}

//*****************************************************************************
std::string HawkeyeUser::HashPermissionLevel(UserPermissionLevel ul, const std::vector<unsigned char>& saltA, const std::vector<unsigned char>& saltB)
{
	std::vector<unsigned char> message;
	switch (ul)
	{
		case eNormal:
			message = { 0x6e, 0x6f, 0x72, 0x6d, 0x61, 0x6c, 0x5f, 0x70, 0x6c }; break;
		case eElevated:
			message = { 0x65, 0x6c, 0x65, 0x76, 0x61, 0x74, 0x65, 0x64, 0x5f, 0x70, 0x6c }; break;
		case eAdministrator:
			message = { 0x61, 0x64, 0x6d, 0x69, 0x6e, 0x69, 0x73, 0x74, 0x72, 0x61, 0x74, 0x6f, 0x72, 0x5f, 0x70, 0x6c }; break;
		case eService:
			message = { 0x73, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x5f, 0x70, 0x6c }; break;
	}

	std::vector<unsigned char> salt;
	salt.insert(salt.end(), saltA.begin(), saltA.end());
	salt.insert(salt.end(), saltB.begin(), saltB.end());
	return SecurityHelpers::CalculateHash(message, salt, REPETITIONS);
}

//*****************************************************************************
std::string HawkeyeUser::HashActivationState(bool act, const std::vector<unsigned char>& saltA, const std::vector<unsigned char>& saltB)
{
	std::vector<unsigned char> message;
	switch (act)
	{
		case true:
			message = { 0x61, 0x63, 0x74, 0x69, 0x76, 0x65, 0x5f, 0x73, 0x74, 0x61, 0x74, 0x65 }; break;
		case false:
			message = { 0x6c, 0x6f, 0x73, 0x65, 0x72, 0x5f, 0x73, 0x74, 0x61, 0x74, 0x65 }; break;
	}

	std::vector<unsigned char> salt;
	salt.insert(salt.end(), saltA.begin(), saltA.end());
	salt.insert(salt.end(), saltB.begin(), saltB.end());
	return SecurityHelpers::CalculateHash(message, salt, REPETITIONS);
}

//*****************************************************************************
bool HawkeyeUser::MatchPermissionLevel(const std::vector<unsigned char>& plHash, const std::vector<unsigned char>& saltA, const std::vector<unsigned char>& saltB, UserPermissionLevel& PL)
{
	std::string strHash = SecurityHelpers::DataToText(plHash);

	std::vector<unsigned char> salt;
	salt.insert(salt.end(), saltA.begin(), saltA.end());
	salt.insert(salt.end(), saltB.begin(), saltB.end());

	std::vector<unsigned char> message;
	{
		PL = eNormal;
		message = { 0x6e, 0x6f, 0x72, 0x6d, 0x61, 0x6c, 0x5f, 0x70, 0x6c };
		std::string hash = SecurityHelpers::CalculateHash(message, salt, REPETITIONS);
		if (hash == strHash)
			return true;
	}
	{
		PL = eElevated;
		message = { 0x65, 0x6c, 0x65, 0x76, 0x61, 0x74, 0x65, 0x64, 0x5f, 0x70, 0x6c };
		std::string hash = SecurityHelpers::CalculateHash(message, salt, REPETITIONS);
		if (hash == strHash)
			return true;
	}
	{
		PL = eAdministrator;
		message = { 0x61, 0x64, 0x6d, 0x69, 0x6e, 0x69, 0x73, 0x74, 0x72, 0x61, 0x74, 0x6f, 0x72, 0x5f, 0x70, 0x6c };
		std::string hash = SecurityHelpers::CalculateHash(message, salt, REPETITIONS);
		if (hash == strHash)
			return true;
	}
	{
		PL = eService;
		message = { 0x73, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x5f, 0x70, 0x6c };
		std::string hash = SecurityHelpers::CalculateHash(message, salt, REPETITIONS);
		if (hash == strHash)
			return true;
	}

	PL = eNormal;
	return false;
}

//*****************************************************************************
bool HawkeyeUser::MatchActivationState(const std::vector<unsigned char>& acHash, const std::vector<unsigned char>& saltA, const std::vector<unsigned char>& saltB, bool& ACT)
{
	std::string strHash = SecurityHelpers::DataToText(acHash);

	std::vector<unsigned char> salt;
	salt.insert(salt.end(), saltA.begin(), saltA.end());
	salt.insert(salt.end(), saltB.begin(), saltB.end());

	std::vector<unsigned char> message;
	{
		ACT = true;
		message = { 0x61, 0x63, 0x74, 0x69, 0x76, 0x65, 0x5f, 0x73, 0x74, 0x61, 0x74, 0x65 };
		std::string hash = SecurityHelpers::CalculateHash(message, salt, REPETITIONS);
		if (hash == strHash)
			return true;
	}
	{
		ACT = false;
		message = { 0x6c, 0x6f, 0x73, 0x65, 0x72, 0x5f, 0x73, 0x74, 0x61, 0x74, 0x65 };
		std::string hash = SecurityHelpers::CalculateHash(message, salt, REPETITIONS);
		if (hash == strHash)
			return true;
	}
	
	ACT = false;
	return false;
}

//*****************************************************************************
void HawkeyeUser::UsernameToLowerCase()
{
	boost::algorithm::to_lower (this->userName_);
}
