#pragma once

#include "ControllerBoardOperation.hpp"
#include "HawkeyeServices.hpp"

class CBOService : public HawkeyeServices
{
public:
	CBOService (std::shared_ptr<boost::asio::io_context> pMainIos)
		: HawkeyeServices(pMainIos, "CBOService_Thread")
	{
	}

	void SetCBO(std::shared_ptr<ControllerBoardOperation> cbo)
	{
		pCBO_ = cbo;
	}

	void Initialize (std::function<void (bool)> OnComplete_cb)
	{
		HAWKEYE_ASSERT ("CBOService", pCBO_);
		pCBO_->Initialize (OnComplete_cb);
	}

	std::shared_ptr<ControllerBoardOperation> CBO() const
	{
		return pCBO_;
	}

private:
	std::shared_ptr<ControllerBoardOperation> pCBO_;
};
