
--  This file contains all cumulative DB schema changes since the initial schema creation in 1.0;
--  Changes should NOT be removed, since a customer may skip a release! (e.g. 1.3->1.5 instead of 1.3->1.4->1.5)
--
--  Where changes have been previously applied (e.g. re-installing 1.4 over 1.4), the psql utility will report
--  that as a non-fatal error, allowing the ViCell applicatiaon installation to properly complete installation
--  with all other relevant changes applied.
--


\connect "ViCellDB_template"

-- Replace "Disinfectant" with "Conditioning Solution" in CellHealthReagents table per LH6531-4784
UPDATE "ViCellInstrument"."CellHealthReagents" SET "Name" = 'Conditioning Solution' WHERE "IdNum" = 3;

-- Add in factory-default low-density cell types per LH6531-6907
SELECT
	EXISTS(SELECT 1 FROM "ViCellInstrument"."CellTypes" WHERE "ViCellInstrument"."CellTypes"."CellTypeName" = 'BCI Default_LowCellDensity') as template_has_lowdensity_celltypes
\gset
\if :template_has_lowdensity_celltypes
	-- No-Op
\else
	INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES ('45b743be-abe3-436e-aab1-99bee2a7d404', 7, 'BCI Default_LowCellDensity', false, 250, 3, 1, 50, 0.100000001, 7, 0, '{}', 2, 0, 60, 60, 1, '{af21144c-b7ed-4445-b709-ecfad10ba38f}', 0, true);
	INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES ('cf3720e3-a6e8-4ff6-a72d-cd433b92c92a', 8, 'Mammalian_LowCellDensity', false, 250, 3, 6, 30, 0.100000001, 7, 0, '{}', 2, 0, 60, 60, 1, '{3f4d3a35-20bd-41ad-999a-beec31095464}', 0, true);
	INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES ('2b397692-eb04-4a21-98a2-8f9623bc011e', 9, 'Insect_LowCellDensity', false, 250, 3, 8, 50, 0.100000001, 7, 0, '{}', 2, 0, 60, 60, 1, '{fba1bfb9-a30b-4a98-9571-0fe2ca5053a7}', 0, true);
	INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES ('96f826c2-890d-4fe3-a5dd-7884f45214d5', 10, 'Yeast_LowCellDensity', false, 250, 3, 3, 20, 0.100000001, 4, 0, '{}', 3, 0, 60, 60, 1, '{f36b4017-0f6c-4a59-afff-f21b912d40ac}', 0, true);
	INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES ('2adff531-ea8c-41c8-8e89-7acd9c23692a', 11, 'BCI Viab Beads_LowCellDensity', false, 250, 3, 5, 25, 0.5, 22, 0, '{}', 2, 0, 60, 60, 1, '{9f0c7de5-489c-4c0f-8747-56846cabb9d8}', 0, true);
	INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES ('c5516ff5-ad29-407c-9d1f-9a2090827374', 12, 'BCI Conc Beads_LowCellDensity', false, 250, 3, 2.5, 12, 0.75, 17, 0, '{}', 3, 0, 60, 60, 1, '{17615cb6-a557-4cda-88fd-0452421daa03}', 0, true);
	INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES ('ef8d7357-9d67-4154-aa6e-5e915ed251a5', 13, 'BCI L10 Beads_LowCellDensity', false, 250, 3, 5, 15, 0.5, 22, 0, '{}', 2, 0, 60, 60, 1, '{4672bd76-a7ec-4a38-9457-72193392e923}', 0, true);
\endif


\connect "ViCellDB"

-- Replace "Disinfectant" with "Conditioning Solution" in CellHealthReagents table per LH6531-4784
UPDATE "ViCellInstrument"."CellHealthReagents" SET "Name" = 'Conditioning Solution' WHERE "IdNum" = 3;

-- Add in factory-default low-density cell types per LH6531-6907
SELECT
	EXISTS(SELECT 1 FROM "ViCellInstrument"."CellTypes" WHERE "ViCellInstrument"."CellTypes"."CellTypeName" = 'BCI Default_LowCellDensity') as has_lowdensity_celltypes
\gset
\if :has_lowdensity_celltypes
	-- No-Op
\else
	INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES ('45b743be-abe3-436e-aab1-99bee2a7d404', 7, 'BCI Default_LowCellDensity', false, 250, 3, 1, 50, 0.100000001, 7, 0, '{}', 2, 0, 60, 60, 1, '{af21144c-b7ed-4445-b709-ecfad10ba38f}', 0, true);
	INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES ('cf3720e3-a6e8-4ff6-a72d-cd433b92c92a', 8, 'Mammalian_LowCellDensity', false, 250, 3, 6, 30, 0.100000001, 7, 0, '{}', 2, 0, 60, 60, 1, '{3f4d3a35-20bd-41ad-999a-beec31095464}', 0, true);
	INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES ('2b397692-eb04-4a21-98a2-8f9623bc011e', 9, 'Insect_LowCellDensity', false, 250, 3, 8, 50, 0.100000001, 7, 0, '{}', 2, 0, 60, 60, 1, '{fba1bfb9-a30b-4a98-9571-0fe2ca5053a7}', 0, true);
	INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES ('96f826c2-890d-4fe3-a5dd-7884f45214d5', 10, 'Yeast_LowCellDensity', false, 250, 3, 3, 20, 0.100000001, 4, 0, '{}', 3, 0, 60, 60, 1, '{f36b4017-0f6c-4a59-afff-f21b912d40ac}', 0, true);
	INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES ('2adff531-ea8c-41c8-8e89-7acd9c23692a', 11, 'BCI Viab Beads_LowCellDensity', false, 250, 3, 5, 25, 0.5, 22, 0, '{}', 2, 0, 60, 60, 1, '{9f0c7de5-489c-4c0f-8747-56846cabb9d8}', 0, true);
	INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES ('c5516ff5-ad29-407c-9d1f-9a2090827374', 12, 'BCI Conc Beads_LowCellDensity', false, 250, 3, 2.5, 12, 0.75, 17, 0, '{}', 3, 0, 60, 60, 1, '{17615cb6-a557-4cda-88fd-0452421daa03}', 0, true);
	INSERT INTO "ViCellInstrument"."CellTypes" ("CellTypeID", "CellTypeIndex", "CellTypeName", "Retired", "MaxImages", "AspirationCycles", "MinDiamMicrons", "MaxDiamMicrons", "MinCircularity", "SharpnessLimit", "NumCellIdentParams", "CellIdentParamIDList", "DeclusterSetting", "RoiExtent", "RoiXPixels", "RoiYPixels", "NumAnalysisSpecializations", "AnalysisSpecializationIDList", "CalculationAdjustmentFactor", "Protected") VALUES ('ef8d7357-9d67-4154-aa6e-5e915ed251a5', 13, 'BCI L10 Beads_LowCellDensity', false, 250, 3, 5, 15, 0.5, 22, 0, '{}', 2, 0, 60, 60, 1, '{4672bd76-a7ec-4a38-9457-72193392e923}', 0, true);
\endif
