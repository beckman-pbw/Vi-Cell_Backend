// UnitTests.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "gtest\gtest.h"
#include "gmock\gmock.h"


GTEST_API_ int main(int argc, char **argv)
{
	printf("Running from main()\n");
	//testing::InitGoogleTest(&argc, argv); //only if you running gtest only
	testing::InitGoogleMock(&argc, argv); //call this if you running  
										  //google mock (google test will be called from there.)
	return RUN_ALL_TESTS();
}