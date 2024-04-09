#pragma once

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include "HawkeyeError.hpp"
#include "SyringePumpPort.hpp"
#include "WorkflowOperation.hpp"

//*****************************************************************************
class SyringeWorkflowOperation : public WorkflowOperation
{
public:
	enum SyringeOperation : uint8_t
	{
		Initialize = 0,
		Move,
		SetValve,
		RotateValve,
		SendValveCommand,
	};

	struct Command
	{
		SyringeOperation operation;

		// Initialize has no data.

		// Move data.
		uint32_t targetVolumeUL;
		uint32_t speed;

		// Set valve data.
		SyringePumpPort port;
		SyringePumpDirection direction;

		// Rotate valve data, also uses SyringePumpDirection.
		uint32_t angle;

		// Valve command string
		string valveCommand;
	};

	SyringeWorkflowOperation (Command cmd)
		: WorkflowOperation (SyringePump)
		, cmd_(cmd)	
	{}

	virtual std::string getTypeAsString() override;
	virtual uint8_t getOperation() override;

	SyringePumpPort getPort();
	uint32_t getTargetVolume();

	static std::unique_ptr<SyringeWorkflowOperation> BuildInitializeCommand()
	{
		SyringeWorkflowOperation::Command syringeWorkflowCmd{};
		syringeWorkflowCmd.operation = SyringeWorkflowOperation::SyringeOperation::Initialize;
		return std::make_unique<SyringeWorkflowOperation>(syringeWorkflowCmd);
	};

	static std::unique_ptr<SyringeWorkflowOperation> BuildSetValveCommand (SyringePumpPort port, SyringePumpDirection direction)
	{
		SyringeWorkflowOperation::Command syringeWorkflowCmd{};
		syringeWorkflowCmd.operation = SyringeWorkflowOperation::SyringeOperation::SetValve;
		syringeWorkflowCmd.port = port;
		syringeWorkflowCmd.direction = direction;
		return std::make_unique<SyringeWorkflowOperation>(syringeWorkflowCmd);
	};

	static std::unique_ptr<SyringeWorkflowOperation> BuildMoveCommand (uint32_t targetVolumeUL, uint32_t speed)
	{
		SyringeWorkflowOperation::Command syringeWorkflowCmd{};
		syringeWorkflowCmd.operation = SyringeWorkflowOperation::SyringeOperation::Move;
		syringeWorkflowCmd.targetVolumeUL = targetVolumeUL;
		syringeWorkflowCmd.speed = speed;
		return std::make_unique<SyringeWorkflowOperation>(syringeWorkflowCmd);
	};

	static std::unique_ptr<SyringeWorkflowOperation> BuildRotateValveCommand (uint32_t angle, SyringePumpDirection direction)
	{
		Command syringeWorkflowCmd{};
		syringeWorkflowCmd.operation = RotateValve;
		syringeWorkflowCmd.angle = angle;
		syringeWorkflowCmd.direction = direction;
		return std::make_unique<SyringeWorkflowOperation>(syringeWorkflowCmd);
	};

	static std::unique_ptr<SyringeWorkflowOperation> BuildSendValveCommand (string valveCommand)
	{
		Command syringeWorkflowCmd{};
		syringeWorkflowCmd.operation = SendValveCommand;
		syringeWorkflowCmd.valveCommand = valveCommand;
		return std::make_unique<SyringeWorkflowOperation>(syringeWorkflowCmd);
	};

protected:
	virtual void executeInternal(Wf_Operation_Callback onCompleteCallback) override;

private:
	struct Command cmd_;
};
