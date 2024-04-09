// Copyright(C)2017 by L&T Technology Services Inc. All rights reserved.

// This software contains proprietary and confidential information of 
// L&T Technology Services Inc., and its suppliers. Except as may be set forth 
// in the license agreement under which this software is supplied, use, 
// disclosure, or reproduction is prohibited without the prior express 
// written consent of L&T Technology Services, Inc.

#ifndef _CHARACTERISTICS_H_
#define _CHARACTERISTICS_H_

#include <ErrorCodes.h>

#include <InputConfigurationParams.h>

#include <map>
#include <tuple>
#include <utility>
#include <vector>



/**
@enum E_POLARITY
@brief This enumerator describes polarity for parameter definition
*/
enum E_POLARITY
{
	eBELOW = 0, /*!< Below threshold value*/
	eATABOVE = 1, /*!< At Above threshold value*/
	eInvalidPolarity /*!< Invalid polarity*/
};

/*! \var typedef std::tuple<uint16_t, uint16_t, uint16_t> Characteristic_t
    \brief A type definition for blob characteristic key
*/
typedef std::tuple<uint16_t, uint16_t, uint16_t> Characteristic_t;

/*! \var std::vector<std::tuple<Characteristic_t, float, E_POLARITY>> v_IdentificationParams_t
\brief A type definition for setting threshold for cell identification parameters
*/
typedef std::vector<std::tuple<Characteristic_t, float, E_POLARITY>> v_IdentificationParams_t;

/*! \var std::vector<std::pair<Characteristic_t, E_ERRORCODE>> v_IdentifiParamsErrorCode
\brief A type definition for identification parameters error codes
*/
typedef std::vector<std::pair<Characteristic_t, E_ERRORCODE>> v_IdentifiParamsErrorCode;

/*! \var typedef std::map<Characteristic_t, std::string> CharacteristicLabelList_t
	\brief A type definition for blob characteristics label
*/
typedef std::map<Characteristic_t, std::string> CharacteristicLabelList_t; 

/*! \var typedef std::map<ConfigParameters::E_CONFIG_PARAMETERS, double> ConfigParamList_t;
	\brief A type definition for setting configuration parameters
*/
typedef std::map<ConfigParameters::E_CONFIG_PARAMETERS, double>	ConfigParamList_t;

/*! \var typedef std::map<ConfigParameters::E_CONFIG_PARAMETERS, E_ERRORCODE> AcknConfigParamList_t; 
	\brief A type definition for configuration error type
*/
typedef std::map<ConfigParameters::E_CONFIG_PARAMETERS, E_ERRORCODE> AcknConfigParamList_t; 


#endif   /*_CHARACTERISTICS_H_*/
