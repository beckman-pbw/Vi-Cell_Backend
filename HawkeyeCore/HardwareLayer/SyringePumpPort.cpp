#include "HawkeyeConfig.hpp"
#include "Logger.hpp"
#include "SyringePumpPort.hpp"

static const char MODULENAME[] = "SyringePumpPort";

//*****************************************************************************
SyringePumpPort::SyringePumpPort()
{
	_port = InvalidPort;
}

//*****************************************************************************
SyringePumpPort::SyringePumpPort(Port port)
{
	_port = port;
}

//*****************************************************************************
SyringePumpPort::Port SyringePumpPort::Get()
{
	return _port;
}

//*****************************************************************************
void SyringePumpPort::Set (Port port)
{
	_port = port;
}

//*****************************************************************************
std::string SyringePumpPort::ToString() {

	switch (_port)
	{
	case Waste:
		return "Waste";
	case Sample:
		return "Sample";
	case CHM_ACup:
		return "ACup";
	case FlowCell:
		return "FlowCell";
	case Reagent_1:
		return "Reagent_1";
	case Cleaner_1:
		return "Cleaner_1";
	case Cleaner_2:
		return "Cleaner_2";
	case Cleaner_3:
		return "Cleaner_3";
	case Diluent:
		return "Diluent";
	case ViCell_ACup:
		return "ACup";
	default:
		return "InvalidPort";
	}
}

//*****************************************************************************
SyringePumpPort SyringePumpPort::ParamToPort(uint32_t param)
{

	switch (param) {
		case 1:
			return SyringePumpPort(SyringePumpPort::Waste);
		case 2:
			return SyringePumpPort(SyringePumpPort::Sample);
		case 2 + CHM_PORT_OFFSET:	// CHM A-Cup
			return SyringePumpPort(SyringePumpPort::CHM_ACup);
		case 3:
			return SyringePumpPort(SyringePumpPort::FlowCell);
		case 4:
			return SyringePumpPort(SyringePumpPort::Reagent_1);
		case 5:
			return SyringePumpPort(SyringePumpPort::Cleaner_1);
		case 6:
			return SyringePumpPort(SyringePumpPort::Cleaner_2);
		case 7:
			return SyringePumpPort(SyringePumpPort::Cleaner_3);
		case 8:
			return SyringePumpPort(SyringePumpPort::ViCell_ACup);
		case 8 + CHM_PORT_OFFSET:	// CHM DI Water
			return SyringePumpPort(SyringePumpPort::Diluent);
		default:
			Logger::L().Log(MODULENAME, severity_level::critical, "paramToPort: invalid port: " + std::to_string(param));
			return SyringePumpPort(SyringePumpPort::InvalidPort);
	}
}




