#pragma once

#include <stdint.h>


enum class eCarrierType : uint16_t
{
	eUnknown = 0,
	eCarousel,
	ePlate_96,
	eACup,
};

enum eStageRows
{
	PlateRowA = 'A',
	PlateRowFirst = PlateRowA,
	PlateRowB = 'B',
	PlateRowC = 'C',
	PlateRowD = 'D',
	PlateRowE = 'E',
	PlateRowF = 'F',
	PlateRowG = 'G',
	PlateRowH = 'H',
	PlateRowLast = PlateRowH,

	CarouselRow = 'Z',

	ACupRow = 'Y',
};

const uint32_t  MaxCarouselTubes = 24;
const uint32_t  MaxPlateRowNum = 8;                     // 96 well plates have 8 rows, A-H
const uint32_t  MaxPlateColNum = 12;                    // 96 well plates have 12 columns
