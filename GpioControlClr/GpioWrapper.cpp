#include "stdafx.h"
#include "GpioWrapper.h"
//#include <iostream>

using namespace Gpio;


GpioWrapper::GpioWrapper()
{
	// Initialize COM.
	HRESULT hr = CoInitialize(NULL);
	// Create the unmanaged interface pointer to the managed GpioController type.
	IRfidExtensionsPtr gpioCtrlPtr(__uuidof(GpioControl));
	_gpio_controller = gpioCtrlPtr;
}

GpioWrapper::~GpioWrapper()
{
	_gpio_controller->Dispose();
	_gpio_controller->Release();

	// Uninitialize COM.
	CoUninitialize();
}

/* Gets a value indicating that the GPIO hardware was found and a connection to it exists. */
bool GpioWrapper::IsHardwareConnected()
{
	LONG result = 0;
	HRESULT hr = _gpio_controller->GetIsHardwareConnected(&result);
	CheckConnectionState(result == 0);
	return _is_connected;
}

bool GpioWrapper::DisableWriting()
{
	LONG result = 0;
	HRESULT hr = _gpio_controller->SignalWriteFailure(&result);
	if (result != 0)
	{
		IsHardwareConnected();
	}
	return result == 0;
}

bool GpioWrapper::SignalWriteSuccess()
{
	LONG result = 0;
	HRESULT hr = _gpio_controller->SignalWriteSuccess(&result);
	if (result != 0)
	{
		IsHardwareConnected();
	}
	return result == 0;
}

bool GpioWrapper::EnableWriting()
{
	LONG result = 0;
	HRESULT hr = _gpio_controller->SignalOperationNominal(&result);
	if (result != 0)
	{
		IsHardwareConnected();
	}
	return result == 0;
}

void GpioWrapper::CheckConnectionState(bool isConnected)
{
	if (_is_connected != isConnected)
	{
		Connection_State_Changed(isConnected);
	}
	_is_connected = isConnected;
}

void GpioWrapper::Add_ConnectionStateChanged_Handler(const Connection_State_Signal::slot_type &slot)
{
	Connection_State_Changed.connect(slot);
}
