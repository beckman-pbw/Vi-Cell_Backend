//************************************************************************************************************************************************
// There is a known issue with Boost 1.62 and before.  It is unknown whether this issue is fixed in a later version of Boost.
// Compiling C++ CLI wrapper code that includes *boost::filesystem.hpp* results in the following error:
//
// C:\DEV\Boost\boost_1_62_0\boost / filesystem / path.hpp(458) : error C2059 : syntax error : 'generic'
// C:\DEV\Boost\boost_1_62_0\boost / filesystem / path.hpp(459) : error C2334 : unexpected token(s) preceding '{'; skipping apparent function body
// 
// There is a work-around documented at https://svn.boost.org/trac/boost/ticket/11855 and implemented below
//
#ifdef __cplusplus_cli
#define generic __identifier(generic)
#endif
#include <boost/filesystem.hpp>
#ifdef __cplusplus_cli
#undef generic
#endif
//************************************************************************************************************************************************


#include "HawkeyeLogicImpl.hpp"
#include "HawkeyeLogicInterfaceCLI.hpp"

using namespace std;

//*****************************************************************************
HawkeyeLogicInterfaceCLI::HawkeyeLogicInterfaceCLI()
	: impl_(new HawkeyeLogicImpl())
{

}

//*****************************************************************************
HawkeyeLogicInterfaceCLI::~HawkeyeLogicInterfaceCLI() {

}

//*****************************************************************************
void HawkeyeLogicInterfaceCLI::Initialize() {

	impl_->Initialize();
}
