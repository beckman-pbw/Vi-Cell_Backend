#pragma once

#include <cstdint>
#include <string>

#include "ReagentCommon.hpp"
#include "SyringePumpPort.hpp"

/*
* Regent definitions will be added as new reagent containers
* are introduced to the system.  Each container will contain
* information describing its contents and capacities.
*/
struct ReagentDefinitionDLL
{
	/// BCI-assigned value unique among all reagents
	uint16_t reagent_index;
	std::string label;
	uint8_t mixing_cycles;
};

/*
 * A reagent describes a system consumable product.
 *  Each reagent has a type (reagent_index), manufacturing lot, an expiration
 *  date, the total number of possible events and the number of events remaining.
 *
 * Absolute dates given as days since Epoch (1/1/1970)
 *
 * The Reagent Expiration Date should NOT be exposed to non-service users.
 * Report the reagent container's expiration instead.
 */
struct ReagentStateDLL
{
	uint16_t reagent_index; // Index to ReagentDefinition::reagent_index
	std::string lot_information;
	
	uint16_t events_possible;
	uint16_t events_remaining;

	SyringePumpPort::Port valve_location; // Which valve selection on the syringe pump accesses this reagent
};


/*
 * Reagents are packaged into a Reagent Container.
 * A Reagent Container may contain 1 or More Reagents.
 * The reagent container itself has a manufacturing lot and
 * an expiration (the lowest value among all reagents in the
 * container).  
 * Absolute dates given as days since Epoch (1/1/1970)
 */
struct ReagentContainerStateDLL
{
	// Identifier unique to this container; used for disambiguation.
	uint8_t identifier[8];

	std::string bci_part_number;

	uint32_t in_service_date;
	uint32_t exp_date;

	std::string lot_information;

	ReagentContainerStatus status;

	uint16_t events_remaining;	// set to min (reagent_states[n].events_remaining);

	ReagentContainerPosition position;

	std::vector<ReagentStateDLL> reagent_states;
};

typedef struct ReagentContainerUnloadOptionDLL
{
	ReagentContainerUnloadOptionDLL()
	{ 
		this->location_id = 0;
		this->container_action = ReagentUnloadOption::eULNone;
	}

	uint8_t container_id[8];
	uint8_t location_id;
	ReagentUnloadOption container_action;
} ReagentContainerUnloadOptionDLL;
