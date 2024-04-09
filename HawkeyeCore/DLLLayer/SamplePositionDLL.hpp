#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <boost/format.hpp>

#include "AnalysisDefinitionDLL.hpp"
#include "CellTypesDLL.hpp"
#include "ChronoUtilities.hpp"
#include "ReportsCommon.hpp"
#include "SamplePosition.hpp"


typedef struct SamplePositionDLL
{
private:
	char    row_;
	uint8_t col_;
	SamplePositionHelper helper_;

public:
	SamplePositionDLL()
	{
		row_ = '?';
		col_ = 0;
	}

	SamplePositionDLL(SamplePosition samplePosition)
	{
		row_ = samplePosition.row;
		col_ = samplePosition.col;
	}

	SamplePositionDLL(char row, uint8_t col)
	{
		row_ = row;
		col_ = col;
	}

	bool equals(const SamplePositionDLL& other) const
	{
		return other.col_ == this->col_
			&& other.row_ == this->row_
			&& this->isValid();
	}

	bool setRowColumn(char row, uint8_t col)
	{
		if (isValid(row, col))
		{
			row_ = row;
			col_ = col;
			return true;

		}
		return false;
	}

	char getRow() const
	{
		return row_;
	};

	uint32_t getColumn() const
	{
		return static_cast<uint32_t>(col_);
	}

	void incrementRow()
	{
		uint8_t newRow = row_ + 1;
		if (helper_.isValidRowForPlate(row_))
		{
			row_ = newRow > eStageRows::PlateRowLast ? eStageRows::PlateRowFirst : newRow;
		}
	};

	void incrementColumn()
	{
		uint8_t newCol = col_ + 1;
		if (helper_.isValidRowForCarousel(row_))
		{
			col_ = newCol > MaxCarouselTubes ? 1 : newCol;
		}
		else if (helper_.isValidRowForPlate(row_))
		{
			col_ = newCol > MaxPlateColNum ? 1 : newCol;
		}
	};

	bool isValidForCarousel() const
	{
		return helper_.isValidForCarousel(row_, col_);
	}

	bool isValidForPlate96() const
	{
		return helper_.isValidForPlate96(row_, col_);
	}

	bool isValidForACup() const
	{
		return (row_ == ACupRow && col_ == 1);
	}

	bool isValid(char row, uint8_t col) const
	{
		return helper_.isValid(row, col);
	}

	bool isValid() const
	{
		return isValid(row_, col_);
	}

	std::string getAsStr() const
	{
		return boost::str(boost::format("%c-%d") % row_ % (int)col_);
	}

	uint16_t getHash() const
	{
		return (row_ << 8) | col_;
	}

	eCarrierType getcarrierType() const
	{
		if (isValidForPlate96()) { return eCarrierType::ePlate_96; }
		if (isValidForCarousel()) { return eCarrierType::eCarousel; }
		if (isValidForACup()) { return eCarrierType::eACup; }
		return eCarrierType::eUnknown;
	}

	SamplePosition getSamplePosition() const
	{
		SamplePosition sp;
		sp.setRowColumn(row_, col_);
		return sp;
	}

} SamplePositionDLL;
