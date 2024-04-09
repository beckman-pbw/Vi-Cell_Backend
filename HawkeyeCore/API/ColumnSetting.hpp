#pragma once
#include <cstdint>

enum ColumnOption
{
    ecoSampleStatus = 0,
    ecoSamplePosition,
    ecoSampleName,  // NOTE: This is displayed as Sample ID in the UI
    eTotalConcentration,
    eViableConcentration,
    eTotalViability,
    eTotalCells,
    eAverageDiameter,
    eCellTypeQcName,
    eDilution,
    eWashType,
    eSampleTag
};

struct ColumnSetting
{
    ColumnOption Column;
    uint32_t Width;
    bool IsVisible;
};
