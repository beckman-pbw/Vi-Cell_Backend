#pragma once

#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)
#include <boost/thread.hpp>

#include "HawkeyeLogic.hpp"

class Scout
{
public:
	Scout (boost::asio::io_context& io_s);
	virtual ~Scout();

	bool init (bool withHardware);
	void loginUser (std::string username, std::string password);
	void displayWorkQueueItem (WorkQueueItem& wqi);

private:
	void prompt();
	void showHelp();
	bool localInit();
	void print_reagents_definitions(ReagentDefinition*& reagents, size_t num_reagents);
	void print_reagents_containerstates(ReagentContainerState*& reagents);
	void handleInput (const boost::system::error_code& error);

	void runAsync_ (boost::function<void(bool)> cb_Handler);
	void doWork_ (boost::function<void(bool)> cb_Handler);

	void waitForInit (const boost::system::error_code& ec);
	void waitForInitTimeout (const boost::system::error_code& ec);
	void getWorkQueueStatus(const boost::system::error_code& ec);
//	boost::asio::io_context& io_service_;
//	std::shared_ptr<std::thread> pio_thread_;
//	boost::asio::strand strand_;
//	boost::function<void(bool)> cb_Handler_;


	void SystemStatusTest (boost::system::error_code ec);
	void StopSystemStatusTest();
	bool buildCarouselWq(WorkQueue*& wq);
	bool buildPlateWq(WorkQueue *& wq);
	bool buildNewCellType (CellType*& ct);
	bool buildModifiedCellType (CellType*& ct);
	bool buildAnalysisDefinition (AnalysisDefinition*& ad);

	uint32_t timeout_msecs_;
	std::shared_ptr<boost::asio::deadline_timer> initTimer_;
	std::shared_ptr<boost::asio::deadline_timer> initTimeoutTimer_;
	std::shared_ptr<boost::asio::deadline_timer> getWqStatusTimer_;
	std::string currentUser_;

	static Scout* pScout;

	static void cb_ReagentDrainStatus(eDrainReagentPackState rls);
	static void cb_ReagentPackLoadStatus (ReagentLoadSequence rls);
	static void cb_ReagentPackLoadComplete (ReagentLoadSequence rls);
	static void cb_ReagentPackUnloadStatus (ReagentUnloadSequence rls);
	static void cb_ReagentPackUnloadComplete (ReagentUnloadSequence rls);
	static void cb_ReagentFlowCellFlush(eFlushFlowCellState rls);
	static void cb_ReagentDecontaminateFlowCell(eDecontaminateFlowCellState rls);

	static void cb_WorkQueueItemStatus (WorkQueueItem*, uuid__t uuid);
	static void cb_WorkQueueItemComplete (WorkQueueItem*, uuid__t uuid);
	static void cb_WorkQueueComplete (uuid__t workQueueId /*work queue record indentifier*/);
	static void cb_WorkQueueItemImageProcessed (
		WorkQueueItem* wqi, 
		uint16_t imageSeqNum,					/* image sequence number */
		ImageSetWrapper_t* image,				/* image */
		BasicResultAnswers cumulativeResults,	/* cumulative */
		BasicResultAnswers imageResults);		/* this_image */
};