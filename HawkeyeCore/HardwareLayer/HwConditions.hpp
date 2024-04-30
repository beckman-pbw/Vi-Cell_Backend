#pragma once

// this file is intended to contain compile conditions applicable to the DLL hardware-related modules for
// those applications using the entire DLL or using any of the individual DLL hardware-layer component files


//#define REV1_HW					// enable to use the V1/EP1 hardware configuration

#ifdef REV1_HW

#define REV1_FW					// uses software simulation of some functionality (compatible with both V1 and V2 hardware)

#else

//#define REV2_HW					// USE THIS DEFINE TO ENABLE THE V2/EP1 hardware configuration

#ifdef REV2_HW					// EP1 HARDWARE / V2 CONTROLLER BOARD COMBO

#define REV2_FW					// enable to use the fully-implemented V2 specific firmware
								// if not enabled, uses software simulation of some functionality (compatible with both V1 and V2 hardware)

#else	// REV2_HW				// ALL OTHER NEWER, NON-REV1/NON-REV2 CONDITIONS ARE CAUGHT HERE

#define REV3_FW					// enable to use the fully-implemented V3 specific firmware
								// uses GPIO pin initialization to toggle K70 out of download mode

#endif	// REV2_HW

#endif	// #ifdef REV1_HW

#define CNTLR_PORT_A_SN_STR		"BCI_001A"
#define CNTLR_PORT_B_SN_STR		"BCI_001B"

#define CNTLR_SN_A_STR			"A"
#define CNTLR_SN_B_STR			"B"
