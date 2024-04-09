#pragma once

#include "ReagentControllerBase.hpp"

class DLL_CLASS ReagentControllerSim : public ReagentControllerBase
{
public:

	ReagentControllerSim(std::shared_ptr<CBOService> pCBOService);
	virtual ~ReagentControllerSim() override;

	virtual void Initialize (std::function<void(bool)> callback, std::string cfgFile, bool apply = false) override;
	virtual void Reset(void) override;
	virtual bool ControllerOk(void) override;
	virtual void Stop(void) override;
	virtual bool IsDoorClosed(void) override;
	virtual bool IsPackInstalled(void) override;
	virtual bool IsHome(void) override;
	virtual bool IsUp(void) override;
	virtual bool IsDown(void) override;
	virtual void ClearErrors(std::function<void(bool)> callback) override;
	virtual void GetMotorStatus(MotorStatus & reagentMotorStatus) override;
	virtual void ArmPurge(std::function<void(bool)> callback) override;
	virtual void ArmHome(std::function<void(bool)> callback) override;
	virtual void ArmUp(std::function<void(bool)> callback) override;
	virtual void ArmDown(std::function<void(bool)> callback) override;
	virtual void UnlatchDoor (std::function<void(bool)> callback) override;

protected:
	virtual bool isFluidAvailableInternal(std::shared_ptr<std::vector<RfidTag_t>> rfidTags, const SyringePumpPort::Port& port, uint32_t volume) override;

private:
	int32_t MoveToPosition(int32_t target);	
	bool MoveToLimit(bool downLimit);

	int32_t armHomePos_;
	int32_t armHomeOffset_;
	int32_t armPurgePosition_;
	int32_t armPosition_;
	bool isDoorClosed_;
	bool isPackInstalled_;
};