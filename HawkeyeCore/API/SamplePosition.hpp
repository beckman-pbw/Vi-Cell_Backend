#pragma once

#include <cstdint>

#include "StageDefines.hpp"

typedef struct SamplePositionHelper
{
	static bool isValidRowForCarousel(char row)
	{
		return row == eStageRows::CarouselRow;
	}

	static bool isValidColumnForCarousel(uint8_t col)
	{
		return col >= 1 && col <= MaxCarouselTubes;
	}

	static bool isValidRowForPlate(char row)
	{
		return row >= eStageRows::PlateRowFirst && row <= eStageRows::PlateRowLast;
	}

	static bool isValidColumnForPlate(uint8_t col)
	{
		return col >= 1 && col <= MaxPlateColNum;
	}

	static bool isValidForCarousel(char row, uint8_t col)
	{
		return isValidRowForCarousel(row) && isValidColumnForCarousel(col);
	}

	static bool isValidForPlate96(char row, uint8_t col)
	{
		return isValidRowForPlate(row) && isValidColumnForPlate(col);
	}

	static bool isValidForACup(char row, uint8_t col)
	{
		return (row == eStageRows::ACupRow && col == 1);
	}

	static bool isValid(char row, uint8_t col)
	{
		return isValidForCarousel(row, col) || isValidForPlate96(row, col) || isValidForACup(row, col);
	}

} SamplePositionHelper;


/// Sample Position
///   Used to identify the location of a sample within a Plate or Carousel.
///   Valid Ranges:
///      Plate: A1..H12
///      Carousel: Z1..Z24
typedef struct SamplePosition
{
public:
	SamplePosition()
	{
		row = '?';
		col = 0;
	}

	SamplePosition (char row, uint8_t col)
	{
		this->row = row;
		this->col = col;
	}

	bool setRowColumn(char row, uint32_t col)
	{
		if (SamplePositionHelper().isValid(row, (uint8_t)col))
		{
			this->row = row;
			this->col = static_cast<uint8_t>(col);
			return true;
		}
		return false;
	}

	bool isValid() const
	{
		return SamplePositionHelper().isValid(row, col);
	}

	void FromStr (std::string str)
	{
		row = str[0];
		col = static_cast<uint8_t>(atoi (str.substr(2).c_str()));
	}

	char    row;
	uint8_t col;

} SamplePosition;
