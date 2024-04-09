#include "stdafx.h"
#include <iostream>
#include "boost\filesystem.hpp"
#include "Core\UserList.hpp"
#include "Core\HawkeyeUser.hpp"
#include "Core\SecurityHelpers.hpp"
#include "API\UserLevels.hpp"

#include "gtest\gtest.h"
#include <afx.h>


TEST(UserList, FileCreation)
{
	auto user = std::make_shared<HawkeyeUser>("Dennis", "DennisPassword", eElevated);

	FILE* fptr;
	fopen_s(&fptr, "UserTest.txt", "wt");
	std::string user_output = user->ToStorageString();
	fputs(user_output.c_str(), fptr);
	fclose(fptr);

	EXPECT_TRUE(boost::filesystem::exists(".\\UserTest.txt"));

}

TEST(UserList, OpenCreatedFile)
{
	char user_input[1024];
	FILE* fptr;

	HawkeyeUser user2;

	fopen_s(&fptr, "UserTest.txt", "rt");
	fgets(user_input, 1024, fptr);
	fclose(fptr);
	std::string user_inputs(user_input);

	user2.FromStorageString(user_inputs);
	EXPECT_TRUE(user2.IsValid());

	EXPECT_EQ("Dennis", user2.UserName());

	EXPECT_TRUE(user2.ValidatePassword("DennisPassword"));

	EXPECT_FALSE(user2.ValidatePassword("DennisPassword1"));

	EXPECT_EQ(eElevated, user2.PermisionLevel());

	EXPECT_TRUE(user2.Active());
}
	 
TEST(UserList, NoFile)
{ 
	HawkeyeUser user3("Dennis", "DennisPassword", eElevated);;

	EXPECT_TRUE(user3.IsValid());

	EXPECT_EQ("Dennis", user3.UserName());

	EXPECT_TRUE(user3.ValidatePassword("DennisPassword"));

	EXPECT_FALSE(user3.ValidatePassword("DennisPassword1"));

	EXPECT_EQ(eElevated, user3.PermisionLevel());

	EXPECT_TRUE(user3.Active());
}