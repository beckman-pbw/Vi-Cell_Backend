#pragma once

#include <cstdint>

#include "uuid__t.hpp"

/* 
 A Characteristic is a particular property measured by the image analysis system.
 Characteristics are identified by a <Key, Subkey, Sub-Subkey> triplet.
    The base Key identifies the root measurement
	The Subkey identifies a particular fluorescent channel
	The Sub-Subkey identifies a sub-measurement within the channel

 All Characteristics have a non-zero value for Key
 Some characteristics may accept a sub-key (otherwise a value of '0' is used)
 A characteristic which uses a sub-key may also accept a sub-sub-key (otherwise a value of '0' is used)

 A caller may request an English text description of a particular characteristic.
 */

namespace Hawkeye
{
	struct Characteristic_t
	{
		uint16_t key;
		uint16_t s_key;
		uint16_t s_s_key;

		bool operator==(const Characteristic_t& rhs) const
		{
			return ((key == rhs.key) &&
			        (s_key == rhs.s_key) &&
			        (s_s_key == rhs.s_s_key));
		}
	} ;
};

/*
 * Instructions on which fluorescent illumination sources should be used
 * and how long of an exposure time is needed.
 */
struct FL_IlluminationSettings
{
	uint16_t illuminator_wavelength_nm;
	uint16_t emission_wavelength_nm;
	uint16_t exposure_time_ms;

	bool operator==(const FL_IlluminationSettings& rhs) const
	{
		return ((illuminator_wavelength_nm == rhs.illuminator_wavelength_nm) &&
		        (emission_wavelength_nm == rhs.emission_wavelength_nm) &&
		        (exposure_time_ms == rhs.exposure_time_ms));
	}
};
