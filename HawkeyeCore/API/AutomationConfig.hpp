#pragma once

#include <cstdint>

struct AutomationConfig
{
    AutomationConfig() {
        port = 0;
        automationIsEnabled = false;
    }

	uint32_t port;
	uint32_t padding;   // This is required for the structure to be xferred correctly from the UI to the Backend.
	bool automationIsEnabled;
	bool acupIsEnabled;
};
