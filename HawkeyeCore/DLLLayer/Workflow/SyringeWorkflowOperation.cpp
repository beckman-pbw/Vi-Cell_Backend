#include "stdafx.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "Hardware.hpp"
#include "HawkeyeError.hpp"
#include "Logger.hpp"
#include "SyringeWorkflowOperation.hpp"

static const char MODULENAME[] = "SyringeWorkflowOperation";

//*****************************************************************************
void SyringeWorkflowOperation::executeInternal (Wf_Operation_Callback onCompleteCallback)
{
	std::string logStr = getTypeAsString() + " : ";
	const auto& syringePump = Hardware::Instance().getSyringePump();

	switch (cmd_.operation)
	{
		case Initialize:
		{
			// This is disabled so that the SyringePump.cfg file is not read.
			logStr.append("Asynchronous");
			syringePump->initialize ([this, onCompleteCallback](bool success)
				{
					if (!success)
					{
						Logger::L().Log (MODULENAME, severity_level::error, "Failed to initialize syringe pump");
					}
					triggerCallback (onCompleteCallback, success);
				});
			if (Logger::L().IsOfInterest (severity_level::debug2) && !logStr.empty())
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, logStr);
			}
			return;
		}

		case Move:
		{
			logStr.append(boost::str(boost::format("tgtVolume: %d, tgtSpeed: %d") % cmd_.targetVolumeUL % cmd_.speed));

			std::string errStr = logStr;
			
			PhysicalPort_t physicalValve;

			bool success = syringePump->getPhysicalValve (physicalValve);
			if (syringePump->isAspirating(cmd_.targetVolumeUL))
			{
				// Prevent aspirating from Waste.
				success = success && syringePump->isAspirationAllowed(physicalValve);
				if (!success)
				{
					errStr.append("Aspiration from current port not allowed!");
				}
			} 
			else
			{
				success = success && syringePump->isDisPenseAllowed(physicalValve);
				if (!success)
				{
					errStr.append("Dispense to current port not allowed!");
				}
			}

			if (!success)
			{
				Logger::L().Log (MODULENAME, severity_level::error, errStr);
				triggerCallback(onCompleteCallback, HawkeyeError::eHardwareFault);
				return;
			}

			if (cmd_.speed == 0)
			{
				errStr.append("Failed to set syringe position : Invalid syringe pump speed <0> !");
				Logger::L().Log (MODULENAME, severity_level::error, errStr);
				triggerCallback(onCompleteCallback, HawkeyeError::eInvalidArgs);
				return;
			}

			logStr.append("Asynchronous \tSuccess");
			syringePump->setPosition([this, onCompleteCallback, errStr](bool success) -> void
			{
				if (!success)
				{
					auto newErrStr = errStr;
					newErrStr.append("Failed to set syringe position");
					Logger::L().Log (MODULENAME, severity_level::error, newErrStr);
				}

				triggerCallback(onCompleteCallback, success);
			}, cmd_.targetVolumeUL, cmd_.speed);

			if (Logger::L().IsOfInterest(severity_level::debug2) && !logStr.empty())
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, logStr);
			}
			return;
		}

		case SetValve:
		{
			logStr.append(boost::str(boost::format("Setting syringe valve : tgtPort<%s> tgtDirection<%s>") % cmd_.port.ToString() % cmd_.direction.ToString()));

			std::string errStr = logStr;

			if (cmd_.port.Get() == SyringePumpPort::InvalidPort)
			{
				errStr.append("Invalid syringe pump port : " + cmd_.port.ToString());
				Logger::L().Log (MODULENAME, severity_level::error, errStr);
				triggerCallback(onCompleteCallback, HawkeyeError::eInvalidArgs);
				return;
			}

			if (cmd_.direction.Get() == SyringePumpDirection::DirectionError)
			{
				errStr.append("Invalid syringe pump direction : " + cmd_.direction.ToString());
				Logger::L().Log (MODULENAME, severity_level::error, errStr);
				triggerCallback(onCompleteCallback, HawkeyeError::eInvalidArgs);
				return;
			}

			logStr.append("Asynchronous \tSuccess");
			syringePump->setValve([this, onCompleteCallback, errStr](bool success) -> void
			{
				if (!success)
				{
					auto newErrStr = errStr;
					newErrStr.append("Failed to set syringe valve");
					Logger::L().Log (MODULENAME, severity_level::error, newErrStr);
				}

				triggerCallback(onCompleteCallback, success);

			}, cmd_.port, cmd_.direction);
				
			if (Logger::L().IsOfInterest(severity_level::debug2) && !logStr.empty())
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, logStr);
			}
			return;
		}

		case RotateValve:
		{
			logStr.append(boost::str(boost::format("Rotating syringe valve:: angle: %d direction: %s") % cmd_.angle % cmd_.direction.ToString()));

			std::string errStr = logStr;

			if (cmd_.angle > 345)	// Max angle is 345 degrees.
			{
				errStr.append("Invalid syringe valve angle: " + cmd_.angle);
				Logger::L().Log(MODULENAME, severity_level::error, errStr);
				triggerCallback(onCompleteCallback, HawkeyeError::eInvalidArgs);
				return;
			}

			if (cmd_.direction.Get() == SyringePumpDirection::DirectionError)
			{
				errStr.append("Invalid syringe pump direction : " + cmd_.direction.ToString());
				Logger::L().Log(MODULENAME, severity_level::error, errStr);
				triggerCallback(onCompleteCallback, HawkeyeError::eInvalidArgs);
				return;
			}

			logStr.append("Asynchronous \tSuccess");
			syringePump->rotateValve([this, onCompleteCallback, errStr](bool success) -> void
				{
					if (!success)
					{
						auto newErrStr = errStr;
						newErrStr.append("Failed to set syringe valve");
						Logger::L().Log(MODULENAME, severity_level::error, newErrStr);
					}

					triggerCallback(onCompleteCallback, success);

				}, cmd_.angle, cmd_.direction);

			if (Logger::L().IsOfInterest(severity_level::debug2) && !logStr.empty())
			{
				Logger::L().Log(MODULENAME, severity_level::debug2, logStr);
			}

			return;
		}

		case SendValveCommand:
		{
			logStr.append(boost::str(boost::format("Sending syringe valve command:: %s") % cmd_.valveCommand.c_str()));

			std::string errStr = logStr;

			if (cmd_.valveCommand.length() <= 0)	// must define command string
			{
				errStr.append("No syringe valve command specified");
				Logger::L().Log(MODULENAME, severity_level::error, errStr);
				triggerCallback(onCompleteCallback, HawkeyeError::eInvalidArgs);
				return;
			}

			logStr.append("Asynchronous \tSuccess");
			syringePump->sendValveCommand([this, onCompleteCallback, errStr](bool success) -> void
				{
					if (!success)
					{
						auto newErrStr = errStr;
						newErrStr.append("Failed to send syringe valve command");
						Logger::L().Log(MODULENAME, severity_level::error, newErrStr);
					}

					triggerCallback(onCompleteCallback, success);

				}, cmd_.valveCommand);

			if (Logger::L().IsOfInterest(severity_level::debug2) && !logStr.empty())
			{
				Logger::L().Log(MODULENAME, severity_level::debug2, logStr);
			}

			return;
		}

		default:
		{
			logStr.append("Workflow operation is unknown!");
			Logger::L().Log (MODULENAME, severity_level::error, logStr);
			triggerCallback(onCompleteCallback, HawkeyeError::eSoftwareFault);
			return;
		}
	}

	if (Logger::L().IsOfInterest(severity_level::debug2) && !logStr.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, logStr);
	}

	triggerCallback(onCompleteCallback, HawkeyeError::eSuccess);
}

//*****************************************************************************
std::string SyringeWorkflowOperation::getTypeAsString()
{
	std::string operationAsString;
	switch (cmd_.operation)
	{
		case SyringeWorkflowOperation::Initialize:
			operationAsString = "Initialize";
			break;
		case SyringeWorkflowOperation::Move:
			operationAsString = "Move";
			break;
		case SyringeWorkflowOperation::SetValve:
			operationAsString = "SetValve";
			break;
		case SyringeWorkflowOperation::RotateValve:
			operationAsString = "RotateValve";
			break;
		case SyringeWorkflowOperation::SendValveCommand:
			operationAsString = "SendValveCommand";
			break;
		default:
			operationAsString = "Unknown";
			break;
	}

	return WorkflowOperation::getTypeAsString() + ":" + operationAsString;
}

uint8_t SyringeWorkflowOperation::getOperation()
{
	return static_cast<uint8_t>(cmd_.operation);
}

SyringePumpPort SyringeWorkflowOperation::getPort()
{
	return cmd_.port;
}

uint32_t SyringeWorkflowOperation::getTargetVolume()
{
	return cmd_.targetVolumeUL;
}
