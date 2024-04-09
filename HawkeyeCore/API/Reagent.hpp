#pragma once

#include <cstdint>

#include "ReagentCommon.hpp"

/*
* Regent definitions will be added as new reagent bottles
* are introduced to the system.  Each bottle will contain
* information describing its contents and capacities.
*/
struct ReagentDefinition
{
	/// BCI-assigned value unique among all reagents
	uint16_t reagent_index;
	char label[30];
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
struct ReagentState
{
	uint16_t reagent_index; // Index to ReagentDefinition::reagent_index
	char* lot_information;
	
	uint16_t events_possible;
	uint16_t events_remaining;

	uint8_t valve_location; // Which valve selection on the syringe pump accesses this reagent
};


/*
 * Reagents are packaged into a Reagent Container.
 * A Reagent Container may contain 1 or More Reagents.
 * The reagent container itself has a manufacturing lot and
 * an expiration (the lowest value among all reagents in the
 * container).
 * Absolute dates given as days since Epoch (1/1/1970)
 */
struct ReagentContainerState
{
	// Identifier unique to this container; used for disambiguation.
	uint8_t identifier[8];

	char* bci_part_number;
	
	uint32_t in_service_date;
	uint32_t exp_date;		// Either shelf-life date OR service-life date if in_service_date != 0

	char* lot_information;

	ReagentContainerStatus status;	
	
	uint16_t events_remaining;	// set to min (reagent_states[n].events_remaining);

	ReagentContainerPosition position;

	uint8_t num_reagents;
	ReagentState* reagent_states;
};

struct ReagentContainerUnloadOption
{
	uint8_t container_id[8];
	ReagentUnloadOption container_action;
};
