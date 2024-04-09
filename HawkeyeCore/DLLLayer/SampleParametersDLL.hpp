#pragma once

#include <cstdint>
#include <string>

#include <boost/algorithm/string.hpp>

#include "AnalysisDefinitionsDLL.hpp"
#include "DataConversion.hpp"
#include "SampleParameters.hpp"

bool getCellTypeByIndex (uint32_t index, CellTypeDLL& cellType);
bool getQualityControlByName (const std::string qcName, QualityControlDLL& qc);

struct SampleParametersDLL
{
	//*****************************************************************************
	SampleParametersDLL& operator= (const SampleParameters& sp)
	{
		DataConversion::convertToStandardString (label, sp.label);
		DataConversion::convertToStandardString(tag, sp.tag);
		DataConversion::convertToStandardString(bp_qc_name, sp.bp_qc_name);

		boost::algorithm::trim (bp_qc_name);

		if (!bp_qc_name.empty())
		{
			QualityControlDLL qc = {};
			getQualityControlByName (bp_qc_name, qc);
			qc_uuid = qc.uuid;
		}
		AnalysisDefinitionsDLL::GetAnalysisByIndex (sp.analysisIndex, analysis);
		CellTypesDLL::getCellTypeByIndex (sp.celltypeIndex, celltype);
		dilutionFactor = sp.dilutionFactor;
		postWash = sp.postWash;
		saveEveryNthImage = sp.saveEveryNthImage;

		return *this;
	}

	//*****************************************************************************
	SampleParameters ToCStyle()
	{
		SampleParameters sp = {};

		DataConversion::convertToCharPointer (sp.label, label);
		DataConversion::convertToCharPointer (sp.tag, tag);
		DataConversion::convertToCharPointer (sp.bp_qc_name, bp_qc_name);
		sp.analysisIndex = analysis.analysis_index;
		sp.celltypeIndex = celltype.celltype_index;
		sp.dilutionFactor = dilutionFactor;
		sp.postWash = postWash;
		sp.saveEveryNthImage = saveEveryNthImage;

		return sp;
	}

	//*****************************************************************************
	void FromDbStyle (const DBApi::DB_SampleItemRecord& dbsi)
	{
		label = dbsi.SampleItemNameStr;
		tag = dbsi.ItemLabel;
		if (!dbsi.BioProcessNameStr.empty())
		{
			bp_qc_name = dbsi.BioProcessNameStr;
		}
		else
		{
			if (!dbsi.QcProcessNameStr.empty())
			{
				bp_qc_name = dbsi.QcProcessNameStr;
			}
		}

//TODO: get the analysis and celltype data from the loaded collections???
		analysis.analysis_index = dbsi.AnalysisDefIndex;
		celltype.celltype_index = dbsi.CellTypeIndex;
		dilutionFactor = dbsi.Dilution;
		qc_uuid = dbsi.QcProcessId;
		//bp_qc_name = dbsi.QcProcessNameStr;  //TODO: remove "QcProcessNameStr", it is not in the QcProcesses table.
		postWash = static_cast<eSamplePostWash>(dbsi.WashTypeIndex);
		saveEveryNthImage = dbsi.SaveNthImage;
	}

	std::string label;
	std::string tag;
	std::string bp_qc_name;
	uuid__t qc_uuid = {};
	AnalysisDefinitionDLL analysis;
	CellTypeDLL celltype;
	uint32_t    dilutionFactor;
	eSamplePostWash postWash;
	uint32_t    saveEveryNthImage;

	//*****************************************************************************
	static void Log (std::stringstream& ss, const SampleParametersDLL& sp)
	{
		ss << boost::str(boost::format("\t\tLabel: %s\n") % sp.label);
		ss << boost::str(boost::format("\t\tTag: %s\n") % sp.tag);
		ss << boost::str (boost::format ("\t\tQC Name: %s\n") % sp.bp_qc_name);

		ss << boost::str(boost::format("\t\tCelltype Index: 0x%08X (%d)\n\t\tSaveEveryNthImage: %d\n\t\tDilution Factor: %d\n\t\tPostwash: %d\n")
			% sp.celltype.celltype_index
			% sp.celltype.celltype_index
			% sp.saveEveryNthImage
			% sp.dilutionFactor
			% sp.postWash
		);

		ss << boost::str(boost::format("\t\tAnalysis Index: 0x%04X (%d)\n")
			% (int)sp.analysis.analysis_index
			% (int)sp.analysis.analysis_index
		);
	}

};
