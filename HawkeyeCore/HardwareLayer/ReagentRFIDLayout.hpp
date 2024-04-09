#pragma once

#include <map>

#include "AnalysisDefinitionCommon.hpp"

/*
Layout proposal for RFID tag.

RFID tag has 256 byets of user memory.  Last 4 bytes are reserved for a counter.
We plan to encrypt the tag data which requires a 16-byte boundary, so our practical limit is 240 bytes (15*16)
 but the encryption task is handled in firmware and is transparent to us.

Design goals:
 * Allow RFID tag to deliver new reagent definitions to the instrument
 * Allow RFID tag to deliver new analysis definitions to the instrument

Reagents and Cleaners both reference a UINT16 "index" value.  This index refers to the reagent/cleaner's unique ID
 within the Beckman Coulter Reagent World.  The Reagent team is responsible for keeping a list of the known fluids.
 "1", for instance, should always map to "Trypan Blue".  Analyses will refer to particular reagents that are necessary
 for that analysis using the reagent's index value.

 The Reagent section below describes the "full" reagent definition:
  * Index
  * Description
  * Mixing cycles
  New reagent definitions should be added to a locally-stored list.
  XXXThe Cleaning fluids below use the "reagent definition", but (for space reasons) only have the Index
  XXX value on the RFID tag.  THAT MEANS: the instrument configuration will have to already contain the known cleaning
  XXX agents ("Buffer", "Conditioning Solution", "Cleaning Agent", "Bleach"...) so that the control software can use the index
  XXX from the RFID tag to look up the remainder of the definition.
  Cleaning fluids contain everything from the complete reagent set except the mixing cycle count which does not apply
   to a cleaning fluid.

 Additionally, a reagent MAY be followed by a series of steps that describe one or two post-sampling cleaning sequences
  specific to that reagent.

Certain sections of the tag contents may REPEAT or may be OMITTED ENTIRELY depending on a count value preceeding them.
 * NUM_CLEANERS - repeats the "CLEAN_" section 0..N times.
 * HAS_REAGENT - if 0, the "REAGENT_" section is omitted.
 * NUM_CLEANING_STEPS - if 0, the CLEAN_INDEX and CLEAN_STEP_DESCRIPTION repetitions are omitted)
 * NUM_ANALYSES - repeats the "AD_" section 0..N times
 * AD_REAGENT_COUNT - repeats "AD_REAGENT_INDEX_N" 0..N times.
 * AD_ILLUMINATION_COUNT - repeats "AD_ILLUMINATION_FREQ/EXPOSURE" 0..N times
 * AD_NUM_PARAMETERS - repeats "AD_PARAM_" section 0..N times.

 Analyses:
  Similar to reagents/cleaners, each analysis lives within an address space controlled by BCI's Reagent team.
  Indices 0x00 to 0x7FFF are reserved for analyses defined by Beckman Coulter.  Indices 0x8000 to 0xFFFF are
  available for use by customer-defined analyses (each address space allows for ~32,000 analyses, so that should
  be enough to go around).
  All analyses rely on a brightfield image, so that is taken as a given.  This covers the Trypan Blue use-case.
  Certain analyses may include fluorescent data as well, so they will specify the set of fluorescent sources
  to enable (and for how long).
  If no FL illumination sources are given, then the instrument should flow the cells non-stop during the analysis.
  If FL illumination is required, then the system must stop the sample flow and image the cells through brightfield
   and all requested fluorescence channels.
  The reagent indices within an analysis specify which reagents are required for this analysis.  Some analyses may
  not require ANY reagents (ex: GFP+ or RFP+ cells naturally produce fluorescent proteins).

 AD_PARAM_KEY needs to refer to a parameter ID that is negotiated/known between the Reagent team and the Image Analysis Algorithm team.
  Parameters are defined as having a value at/above or below a certain threshold value.


  Offset    Length    FieldName                             Format                   R/W    Description
  0         7         PACK_PN                               Null-terminated string    RO    BCI part number for the reagent pack (6 chars + /0)
  7         4         PACK_EXP                              UINT32                    RO    Expiration date for pack (as days since 1/1/1970)
  11        2         SERVICE_LIFE                          UINT16                    RO    Maximum service life (days)
  13        4         IN_SERVICE_DATE                       UINT32                    RW (ONCE)    Date first placed in service (as days since 1/1/1970)
  17        4         PACK_LOT_NUM                          UINT32                    RO    Manufacturing lot number for the reagent pack
  21        1         NUM_CLEANERS    0..N                  UINT8                     RO    Number of cleaners in pack (descriptions follow)
  22        2             CLEAN_INDEX                       UINT16                    RO    BCI Consumable Definition Index- describes the fluid
  24        1    (..N)    CLEAN_DESC                        Null-terminated string    RO    Description of Cleaner
  24        2             CLEAN_TOTAL_USES                  UINT16                    RO    Allowed uses for this consumable
  26        2             CLEAN_REMAINING_USES              UINT16                    RW    Remaining uses for this consumable (decrement-only)
  28        7             CLEAN_PN                          Null-terminated string    RO    BCI Consumable Part Number - describes the fluid
  35        1         HAS_REAGENT    0 | 1                  UINT8                     RO    0: No reagent
  36        2             REAGENT_INDEX                     UINT16                    RO    BCI Consumable Definition Index- describes the fluid
  38        1    (..N)    REAGENT_DESC                      Null-terminated string    RO    Description of Reagent
  39        1             REAGENT_CYCLES                    UINT8                     RO    Number of mixing cycles required for this reagent
  40        2             REAGENT_TOTAL_USES                UINT16                    RO    Allowed uses for this consumable
  42        2             REAGENT_REMAINING_USES            UINT16                    RW    Remaining uses for this consumable (decrement-only)
  44        7             REAGENT_PN                        Null-terminated string    RO    BCI Reagent part number - describes the fluid
  51        1             NUM_CLEANING_STEPS                UINT8                     RO    Number of cleaning steps to follow
  52        2                 CLEAN_INDEX                   UINT16                    RO    BCI Consumable Definition Index - Describes the cleaner used in this step
  54        1                 CLEAN_STEP_DESCRIPTION        UINT8 bitfield            RO    Bitfield describing the cleaning step
  55        1         NUM_ANALYSES                          UINT8                     RO    Number of Analysis Definitions to follow
  56        2             AD_INDEX                          UINT16                    RO    BCI Analysis Definition Index
  58        1    (..N)    AD_DESC                           Null-terminated string    RO    Analysis description
  59        1             AD_REAGENT_COUNT    0..N          UINT8                     RO    Number of reagents required for analysis
  60        2                 AD_REAGENT_INDEX_N            UINT16                    RO    Ref: Consumable Definition Index
  62        1             AD_ILLUMINATION_COUNT    0..N     UINT8                     RO    Number of FL illuminators (BF is a given) defined
  63        2                 AD_ILLUMINATION_FREQ          UINT16                    RO    Illuminator frequency, nanometers
  65        2                 AD_EMISSION_FREQ              UINT16                    RO    Emission filter frequency, nanometers
  67        2                 AD_ILLUMINATION_EXPOSURE      UINT16                    RO    Illuminator exposure time, milliseconds
  68        2             AD_POPULATION_PARAM_KEY           UINT16                    RO    Key value for parameter to augment the brightfield map for population (0xFFFF - BF-ONLY)
  71        2                 AD_POPULATION_PARAM_SUBKEY    UINT16                    RO    Sub-key for parameter
  73        4                 AD_POPULATION_PARAM_VALUE_TH  32b Floating point        RO    Parameter threshold value
  77        1                 AD_POPULATION_PARAM_ABOVE     UINT8                     RO    0: intersted in population below VALUE_TH; Non-zero: interested in value at/above VALUE_TH
  78        1             AD_NUM_PARAMETERS    1..N         UINT16                    RO    Number of parameters in analysis
  79        2                 AD_PARAM_KEY                  UINT16                    RO    Key value for parameter (ties to image analysis algoritm)
  81        2                 AD_PARAM_SUBKEY               UINT16                    RO    Sub-key value for parameter.
  83        4                 AD_PARAM_VALUE_TH             32b Floating point        RO    Parameter threshold value
  87        1                 AD_PARAM_ABOVE                UINT8                     RO    0: intersted in population below VALUE_TH; Non-zero: interested in value at/above VALUE_TH


*/

#pragma pack (push, 1)

typedef struct
{
	uint16_t index;
	char*    desc;
	uint16_t totalUses;
	uint16_t remainingUses;
	char     partNum[7];
} Cleaner_t;


/*
  Cleaning instruction bitfield:

  BIT      DESC
   0        Primary/Secondary sequence toggle - 0: Primary, 1: Secondary
   1        Volume - 0: 600 uL, 1: 1200 uL
   2..3     Speed: 0x00: 30uL/s, 0x01: 60uL/s, 0x10: 100uL/s, 0x11: 250uL/s
   4        Backflush? - 0: No, 1: Yes
   5..6     Cleaning Target: 0x00: N/A, 0x01: Sample Tube, 0x10: Flowcell, 0x11: Sample Tube AND Flowcell
   7        Air flush? - 0: No, 1: Yes
*/

typedef struct
{
	uint16_t cleaner_index;
	uint8_t cleaning_instruction;
} CleaningSequenceStep_t;

typedef struct
{
	uint16_t index;
	char*    desc;
	uint8_t  cycles;
	uint16_t totalUses;
	uint16_t remainingUses;
	char     partNum[7];
	uint8_t  numCleaningInstructions; // Includes both Primary and Secondary sequences.
	CleaningSequenceStep_t* cleaning_instructions; // Primarysequence steps should precede secondary steps; regardless, they should execute in order of appearance within the sequence.
} Reagent_t;


typedef struct
{
	uint16_t key;
	uint16_t subKey;
	float    thresholdValue;
	uint8_t  aboveThreshold;
} Parameter_t;

typedef struct
{
	uint16_t       index;
	char*          desc;
	uint8_t        numReagents;
	uint16_t*      reagentIndices;
	uint8_t        numFL_Illuminators;
	FL_IlluminationSettings* illuminators;
	Parameter_t    populationParameterKey;
	uint8_t        numParameters;
	Parameter_t*   parameters;
} Analysis_t;

typedef struct
{
	uint8_t     tagSN[5];
	char        packPn[7];
	uint32_t    packExp;              // Days since 1/1/1970
	uint16_t    serviceLife;          // Days after first placed in service
	uint32_t    inServiceDate;        // Days since 1/1/1970
	uint32_t    packLotNum;
	uint8_t     numCleaners;
	Cleaner_t*  cleaners;
	uint8_t     hasReagent;
	Reagent_t   reagent;
	uint8_t     numAnalyses;
	Analysis_t* analyses;
	uint16_t    tagIndex;
	std::vector<uint16_t> remainingReagentOffsets; // Cleaners will appear first followed by reagents
} RfidTag_t;


typedef union
{
	uint32_t params;
	struct
	{
		unsigned parameter_length:8;
		unsigned parameter_index:8;
		unsigned tag_index:8;
		unsigned valve_number:8;
	}parameter;
} ReagentValveMap_t;


typedef struct
{
	uint16_t cleaning_index;
	union
	{
		uint8_t bitvalue;
		struct
		{
			unsigned grouping   : 1;   // Primary (0)  / Secondary (1)
			unsigned volume     : 1;   // v_600   (0)  / v_1200    (1)
			unsigned speed      : 2;   // s_30    (00) / s_60      (01) / s_100  (10) / s_250  (11)
			unsigned back_flush : 1;   // bf_off  (0)  / bf_on     (1)
			unsigned target     : 2;   //                t_Tube    (01) / t_Flow (10) / t_Both (11)
			unsigned air_flush  : 1;   // af_off  (0)  / af_on     (1)
		} clean;
	};

} ReagentCleaningInstruct_t;

/* Layout for Map valve to Tag */

#pragma pack ()
