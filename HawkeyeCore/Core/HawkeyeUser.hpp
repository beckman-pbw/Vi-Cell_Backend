#pragma once

#include <list>
#include <string>
#include <vector>

#include "UserLevels.hpp"

class HawkeyeUser
{
public:
	static std::string HashPermissionLevel(UserPermissionLevel ul, const std::vector<unsigned char>& saltA, const std::vector<unsigned char>& saltB);
	static std::string HashActivationState(bool act, const std::vector<unsigned char>& saltA, const std::vector<unsigned char>&saltB);
	static bool MatchPermissionLevel(const std::vector<unsigned char>& plHash, const std::vector<unsigned char>& saltA, const std::vector<unsigned char>& saltB, UserPermissionLevel& PL);
	static bool MatchActivationState(const std::vector<unsigned char>& acHash, const std::vector<unsigned char>& saltA, const std::vector<unsigned char>& saltB, bool& ACT);

	/// Calculate the hash of a given message and salt; returns the result as a std::string
//	static std::string CalculateHash(const std::vector<unsigned char>& plainText, const std::vector<unsigned char> saltValue, std::size_t repetitions);
	
	
	// Empty user - CAN ONLY BE USED WITH "FromStorageString(...)".
	// If you want to create a user by hand, you MUST use the parameterized constructor.
	HawkeyeUser();

	// Create a new user from scratch.
	HawkeyeUser(const std::string& un, const std::string& pw, UserPermissionLevel ul, bool ia=true);

	virtual ~HawkeyeUser();

	bool IsValid() const { return validated; } // If constructed from a text line, a bad actor may have altered the line to try to elevate a privilege level.  That should be detectable.
	void InvalidateUser() { validated = false; } // Externally invalidate this user.  It is not to be trusted but we need to keep it in a list.
	bool ValidatePassword(std::string plainText);

	// Create from a hashed line from a storage file
	// Format:
	//   [username]:[salt]:[PW Hash]:[UL Hash]:[ActivationHash]
	//
	//NOTE: if FromStorageString fails, the ONLY information that will be maintained from the line is the UserName.  The rest will be discarded as unreliable.
	//      The UserName is preserved only to allow the caller to easily identify the line which has been compromised.
	bool FromStorageString(const std::string& fromTextLine);
	std::string ToStorageString();


	std::string UserName() const { return userName_; }
	UserPermissionLevel PermisionLevel() const { return userLevel; }
	bool IsActive() const { return isActive; }

	/*
	 * Modifications: You CANNOT alter a HawkeyeUser that has failed validation. That instance is DEAD
	 * Cannot re-use current or previous 10 passwords for normal password change.
	 * Password Reset clears the list and resets to a known default password (the default for factory_admin)
	 */
	bool SetPassword(const std::string& newPassword, bool isPwdReset = false, bool isOverride = false);
	bool SetUserLevel(UserPermissionLevel newPL);
	bool SetActivation(bool active);

	/*
	 * Allow the user to sign / verify external text.  An implementer can use this to append
	 * extra user-related information to a user data storage file with a way to verify that it is unchanged
	 * without having to explicitly modify the base HawkeyeUser class.
	 * Note:
	 *   1 - Signature will change with password changes (re-sign all text after changing password)
	 *   2 - Cannot verify external text from older signature.
	 *   3 - An unverified user will always return an empty string / "false" verification
	 */
	std::string SignText(const std::string& freeText);
	bool ValidateText(const std::string& freeText, const std::string& signature);
	void UsernameToLowerCase();

private:
	bool validated;
	std::string userName_;
	UserPermissionLevel userLevel;
	bool isActive;
	std::vector<unsigned char> pwSalt;
	std::list<std::string> pwHash; // Current password will be in FRONT, position [0].
};
