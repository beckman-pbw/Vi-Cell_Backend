#pragma once

//namespace HawkeyeLogicInterface
//{
//	namespace Cpp
//	{
		class HawkeyeLogicInterface;

//		namespace CLI
//		{

class HawkeyeLogicInterfaceCLI
{
public:
	HawkeyeLogicInterfaceCLI();
	~HawkeyeLogicInterfaceCLI();

	void Initialize();

private:
	HawkeyeLogicImpl* impl_;
};

//		}

//	}

//}

