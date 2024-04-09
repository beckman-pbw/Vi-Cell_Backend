#pragma once

// NB: Projet GpioControl must already have been built.
#ifdef _DEBUG
#import "..\GpioControl\obj\x64\Debug\RfidExtensions.tlb" raw_interfaces_only
#else
#import "..\GpioControl\obj\x64\Release\RfidExtensions.tlb" raw_interfaces_only
#endif

#include <boost/signals2/signal.hpp>

#define API_CALLING_CONVENTION __cdecl
#define DLL_EXPORT __declspec( dllexport )

using namespace RfidExtensions;

namespace Gpio
{
	class GpioWrapper
	{
	public:
		typedef boost::signals2::signal<void(bool)> Connection_State_Signal;

		DLL_EXPORT GpioWrapper();
		DLL_EXPORT ~GpioWrapper();

		DLL_EXPORT bool IsHardwareConnected();

		DLL_EXPORT bool DisableWriting();

		DLL_EXPORT bool SignalWriteSuccess();

		DLL_EXPORT bool EnableWriting();

		DLL_EXPORT void Add_ConnectionStateChanged_Handler(const Connection_State_Signal::slot_type &slot);

	private:
		IRfidExtensionsPtr _gpio_controller;
		bool _is_connected = false;
		Connection_State_Signal Connection_State_Changed;

		void CheckConnectionState(bool isConnected);
	};
}