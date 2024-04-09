# Documenting the Resource Keys for the System Error Code expansion{#system_error_codes}

## To Generate:
1. Search `HawkeyeCore/Core/SystemErrors.cpp` for all instances of ` = "`
1. Copy that line list out.
1. Strip out code reference and leading whitespace from each line.
1. Duplicate all instances of quoted strings with a tab between them `"string"` -> `"string"\t"string"`
1. Strip all whitespace from initial instance of quoted string and remove quotes: `"str ing"\t"str ing"` -> `string\t"str ing"`
1. Replace all instances of `subsystem = (.+)\t` with `LID_API_SystemErrorCode_Subsystem_$1\t`
1. Replace all instances of `system = (.+)\t` with `LID_API_SystemErrorCode_System_$1\t`
1. Replace all instances of `instance = (.+)\t` with `LID_API_SystemErrorCode_Instance_$1\t`
1. Replace all instances of `failure = (.+)\t` with `LID_API_SystemErrorCode_Failure_$1\t`
1. Replace all instances of `level = (.+)\t` with `LID_API_SystemErrorCode_Severity_$1\t`
1. Sort
1. Remove duplicate strings

|Resource Key|English String|
|---|---|
|LID_API_SystemErrorCode_Failure_""|""|
|LID_API_SystemErrorCode_Failure_Ambienttemp|"Ambient temp"|
|LID_API_SystemErrorCode_Failure_Backgroundintensityadjustmentfailure|"Background intensity adjustment failure"|
|LID_API_SystemErrorCode_Failure_Backgroundintensityadjustmentneartopofrange|"Background intensity adjustment near top of range"|
|LID_API_SystemErrorCode_Failure_Backuprestorefailed|"Backup restore failed"|
|LID_API_SystemErrorCode_Failure_Bubblesdetected|"Bubbles detected"|
|LID_API_SystemErrorCode_Failure_CDRHerror|"CDRH error"|
|LID_API_SystemErrorCode_Failure_Communicationerror|"Communication error"|
|LID_API_SystemErrorCode_Failure_Configurationinvalid|"Configuration invalid"|
|LID_API_SystemErrorCode_Failure_Connectionerror|"Connection error"|
|LID_API_SystemErrorCode_Failure_Deletionerror|"Deletion error"|
|LID_API_SystemErrorCode_Failure_Diodecurrent|"Diode current"|
|LID_API_SystemErrorCode_Failure_Diodepower|"Diode power"|
|LID_API_SystemErrorCode_Failure_Diodetemp|"Diode temp"|
|LID_API_SystemErrorCode_Failure_EEPROMeraseerror|"EEPROM erase error"|
|LID_API_SystemErrorCode_Failure_EEPROMreaderror|"EEPROM read error"|
|LID_API_SystemErrorCode_Failure_EEPROMwriteerror|"EEPROM write error"|
|LID_API_SystemErrorCode_Failure_Ejectfailure|"Eject failure"|
|LID_API_SystemErrorCode_Failure_Empty|"Empty"|
|LID_API_SystemErrorCode_Failure_Expired|"Expired"|
|LID_API_SystemErrorCode_Failure_Externalinterlock|"External interlock"|
|LID_API_SystemErrorCode_Failure_Failedvalidation|"Failed validation"|
|LID_API_SystemErrorCode_Failure_Filenotfound|"File not found"|
|LID_API_SystemErrorCode_Failure_Firmwarebootuperror|"Firmware bootup error"|
|LID_API_SystemErrorCode_Failure_Firmwareinterfaceerror|"Firmware interface error"|
|LID_API_SystemErrorCode_Failure_Firmwarestatemachineerror|"Firmware state machine error"|
|LID_API_SystemErrorCode_Failure_Firmwareupdateerror|"Firmware update error"|
|LID_API_SystemErrorCode_Failure_GeneralPopulationinvalid|"General Population invalid"|
|LID_API_SystemErrorCode_Failure_HardwareFault|"Hardware Fault"|
|LID_API_SystemErrorCode_Failure_Hardwareerror|"Hardware error"|
|LID_API_SystemErrorCode_Failure_Hardwarehealth|"Hardware health"|
|LID_API_SystemErrorCode_Failure_Holdingcurrentfailure|"Holding current failure"|
|LID_API_SystemErrorCode_Failure_Homingfailure|"Homing failure"|
|LID_API_SystemErrorCode_Failure_HostCommunicationerror|"Host Communication error"|
|LID_API_SystemErrorCode_Failure_Imagequality|"Image quality"|
|LID_API_SystemErrorCode_Failure_Imagesdiscardedfromanalysis|"Images discarded from analysis"|
|LID_API_SystemErrorCode_Failure_Initializationerror|"Initialization error"|
|LID_API_SystemErrorCode_Failure_Initializationfailure|"Initialization failure"|
|LID_API_SystemErrorCode_Failure_Interlock|"Interlock"|
|LID_API_SystemErrorCode_Failure_Internalcommerror|"Internal comm error"|
|LID_API_SystemErrorCode_Failure_Internalerror|"Internal error"|
|LID_API_SystemErrorCode_Failure_Invalid|"Invalid"|
|LID_API_SystemErrorCode_Failure_Invalidfirmwareversion|"Invalid firmware version"|
|LID_API_SystemErrorCode_Failure_Largeclustersdetected|"Large clusters detected"|
|LID_API_SystemErrorCode_Failure_Latcherror|"Latch error"|
|LID_API_SystemErrorCode_Failure_Loadfailed|"Load failed"|
|LID_API_SystemErrorCode_Failure_LogicError|"Logic Error"|
|LID_API_SystemErrorCode_Failure_Logicerror|"Logic error"|
|LID_API_SystemErrorCode_Failure_Motordrivererror|"Motor driver error"|
|LID_API_SystemErrorCode_Failure_Nightlycleancycleskipped|"Nightly clean cycle skipped"|
|LID_API_SystemErrorCode_Failure_NoConnection|"No Connection"|
|LID_API_SystemErrorCode_Failure_Noimagecaptured|"No image captured"|
|LID_API_SystemErrorCode_Failure_Nopackfound|"No pack found"|
|LID_API_SystemErrorCode_Failure_Notmet|"Not met"|
|LID_API_SystemErrorCode_Failure_Notpresent|"Not present"|
|LID_API_SystemErrorCode_Failure_Notregistered|"Not registered"|
|LID_API_SystemErrorCode_Failure_Operationnotallowed|"Operation not allowed"|
|LID_API_SystemErrorCode_Failure_Overpressureerror|"Over pressure error"|
|LID_API_SystemErrorCode_Failure_PopulationOfInterestinvalid|"Population Of Interest invalid"|
|LID_API_SystemErrorCode_Failure_Positioningfailure|"Positioning failure"|
|LID_API_SystemErrorCode_Failure_Powerthreshold|"Power threshold"|
|LID_API_SystemErrorCode_Failure_Primingfailure|"Priming failure"|
|LID_API_SystemErrorCode_Failure_Processingerror|"Processing error"|
|LID_API_SystemErrorCode_Failure_RFIDerror|"RFID error"|
|LID_API_SystemErrorCode_Failure_Readerror|"Read error"|
|LID_API_SystemErrorCode_Failure_Registrationfailure|"Registration failure"|
|LID_API_SystemErrorCode_Failure_Reset|"Reset"|
|LID_API_SystemErrorCode_Failure_Responsetooshort|"Response too short"|
|LID_API_SystemErrorCode_Failure_Sensorerror|"Sensor error"|
|LID_API_SystemErrorCode_Failure_SoftwareFault|"Software Fault"|
|LID_API_SystemErrorCode_Failure_Storagenearcapacity|"Storage near capacity"|
|LID_API_SystemErrorCode_Failure_Thermal|"Thermal"|
|LID_API_SystemErrorCode_Failure_Timeout|"Timeout"|
|LID_API_SystemErrorCode_Failure_Tubedetected|"Tube detected"|
|LID_API_SystemErrorCode_Failure_Under/overvoltage|"Under/over voltage"|
|LID_API_SystemErrorCode_Failure_Unknowntype|"Unknown type"|
|LID_API_SystemErrorCode_Failure_Verificationonread|"Verification on read"|
|LID_API_SystemErrorCode_Failure_Writeerror|"Write error"|
|LID_API_SystemErrorCode_Failure_Writefailure|"Write failure"|
|LID_API_SystemErrorCode_Instance_""|""|
|LID_API_SystemErrorCode_Instance_7-Ziputilityinstalled|"7-Zip utility installed "|
|LID_API_SystemErrorCode_Instance_Analysis|"Analysis"|
|LID_API_SystemErrorCode_Instance_AnalysisDefinition|"Analysis Definition"|
|LID_API_SystemErrorCode_Instance_Audit|"Audit"|
|LID_API_SystemErrorCode_Instance_AuditLog|"Audit Log"|
|LID_API_SystemErrorCode_Instance_BOTTOM_1|"BOTTOM_1"|
|LID_API_SystemErrorCode_Instance_BRIGHTFIELD|"BRIGHTFIELD"|
|LID_API_SystemErrorCode_Instance_Bioprocess|"Bioprocess"|
|LID_API_SystemErrorCode_Instance_CameraLog|"Camera Log"|
|LID_API_SystemErrorCode_Instance_Carousel|"Carousel"|
|LID_API_SystemErrorCode_Instance_Carouselpresent|"Carousel present"|
|LID_API_SystemErrorCode_Instance_Carouselregistration|"Carousel registration"|
|LID_API_SystemErrorCode_Instance_CellType|"CellType"|
|LID_API_SystemErrorCode_Instance_CelltypeDefinition|"Celltype Definition"|
|LID_API_SystemErrorCode_Instance_ConcentrationandSizing|"Concentration and Sizing"|
|LID_API_SystemErrorCode_Instance_Concentrationconfiguration|"Concentration configuration"|
|LID_API_SystemErrorCode_Instance_Configuration(Dynamic)|"Configuration (Dynamic)"|
|LID_API_SystemErrorCode_Instance_Configuration(Static)|"Configuration (Static)"|
|LID_API_SystemErrorCode_Instance_DebugImage|"Debug Image"|
|LID_API_SystemErrorCode_Instance_Directory|"Directory"|
|LID_API_SystemErrorCode_Instance_DoorLeft|"Door Left"|
|LID_API_SystemErrorCode_Instance_DoorRight|"Door Right"|
|LID_API_SystemErrorCode_Instance_DustImage|"Dust Image"|
|LID_API_SystemErrorCode_Instance_Error|"Error"|
|LID_API_SystemErrorCode_Instance_ErrorLog|"Error Log"|
|LID_API_SystemErrorCode_Instance_FLRack1|"FL Rack 1"|
|LID_API_SystemErrorCode_Instance_FLRack2|"FL Rack 2"|
|LID_API_SystemErrorCode_Instance_Focus|"Focus"|
|LID_API_SystemErrorCode_Instance_Focusconfig|"Focus config"|
|LID_API_SystemErrorCode_Instance_General|"General"|
|LID_API_SystemErrorCode_Instance_HawkeyeLog|"Hawkeye Log"|
|LID_API_SystemErrorCode_Instance_Illuminator|"Illuminator"|
|LID_API_SystemErrorCode_Instance_InstrumentConfiguration|"Instrument Configuration"|
|LID_API_SystemErrorCode_Instance_Mainbay|"Main bay"|
|LID_API_SystemErrorCode_Instance_MotorControllerConfig|"Motor Controller Config"|
|LID_API_SystemErrorCode_Instance_MotorInfo|"Motor Info"|
|LID_API_SystemErrorCode_Instance_Plate|"Plate"|
|LID_API_SystemErrorCode_Instance_Platepresent|"Plate present"|
|LID_API_SystemErrorCode_Instance_Plateregistration|"Plate registration"|
|LID_API_SystemErrorCode_Instance_Pumpcontroller|"Pump controller"|
|LID_API_SystemErrorCode_Instance_QualityControl|"QualityControl"|
|LID_API_SystemErrorCode_Instance_Radius|"Radius"|
|LID_API_SystemErrorCode_Instance_Reagent|"Reagent"|
|LID_API_SystemErrorCode_Instance_ReagentPack|"ReagentPack"|
|LID_API_SystemErrorCode_Instance_Reagentdoor|"Reagent door"|
|LID_API_SystemErrorCode_Instance_Reagentprobe|"Reagent probe"|
|LID_API_SystemErrorCode_Instance_Result|"Result"|
|LID_API_SystemErrorCode_Instance_Sample|"Sample"|
|LID_API_SystemErrorCode_Instance_SampleItem|"Sample Item"|
|LID_API_SystemErrorCode_Instance_SampleLog|"Sample Log"|
|LID_API_SystemErrorCode_Instance_SampleSet|"Sample Set"|
|LID_API_SystemErrorCode_Instance_Sampleprobe|"Sample probe"|
|LID_API_SystemErrorCode_Instance_SignatureDefinitions|"Signature Definitions"|
|LID_API_SystemErrorCode_Instance_Signatures|"Signatures"|
|LID_API_SystemErrorCode_Instance_Sizingconfiguration|"Sizing configuration"|
|LID_API_SystemErrorCode_Instance_StorageLog|"Storage Log"|
|LID_API_SystemErrorCode_Instance_SyringeConfig|"Syringe Config"|
|LID_API_SystemErrorCode_Instance_TOP_1|"TOP_1"|
|LID_API_SystemErrorCode_Instance_Theta|"Theta"|
|LID_API_SystemErrorCode_Instance_UserList|"User List"|
|LID_API_SystemErrorCode_Instance_UserList|"UserList"|
|LID_API_SystemErrorCode_Instance_Valvecontroller|"Valve controller"|
|LID_API_SystemErrorCode_Instance_Wastetubetraycapacity|"Waste tube tray capacity"|
|LID_API_SystemErrorCode_Instance_Workflowdesign|"Workflow design"|
|LID_API_SystemErrorCode_Instance_Workflowscript|"Workflow script"|
|LID_API_SystemErrorCode_Instance_instrumentserialnumber|"instrument serial number"|
|LID_API_SystemErrorCode_Severity_""|""|
|LID_API_SystemErrorCode_Severity_???|"???"|
|LID_API_SystemErrorCode_Severity_Error|"Error"|
|LID_API_SystemErrorCode_Severity_Notification|"Notification"|
|LID_API_SystemErrorCode_Severity_Warning|"Warning"|
|LID_API_SystemErrorCode_Subsystem_""|""|
|LID_API_SystemErrorCode_Subsystem_Analysis|"Analysis"|
|LID_API_SystemErrorCode_Subsystem_Camera|"Camera"|
|LID_API_SystemErrorCode_Subsystem_CellCounting|"CellCounting"|
|LID_API_SystemErrorCode_Subsystem_CellType|"CellType"|
|LID_API_SystemErrorCode_Subsystem_Configuration|"Configuration"|
|LID_API_SystemErrorCode_Subsystem_Fluorescentrack|"Fluorescent rack"|
|LID_API_SystemErrorCode_Subsystem_General|"General"|
|LID_API_SystemErrorCode_Subsystem_Integrity|"Integrity"|
|LID_API_SystemErrorCode_Subsystem_LED|"LED"|
|LID_API_SystemErrorCode_Subsystem_OmicronLED|"Omicron LED"|
|LID_API_SystemErrorCode_Subsystem_Motor|"Motor"|
|LID_API_SystemErrorCode_Subsystem_Photodiode|"Photodiode"|
|LID_API_SystemErrorCode_Subsystem_Precondition|"Precondition"|
|LID_API_SystemErrorCode_Subsystem_RFIDhardware|"RFID hardware"|
|LID_API_SystemErrorCode_Subsystem_Reagentbayhardware|"Reagent bay hardware"|
|LID_API_SystemErrorCode_Subsystem_Reagentpack|"Reagent pack"|
|LID_API_SystemErrorCode_Subsystem_Sampledeck|"Sample deck"|
|LID_API_SystemErrorCode_Subsystem_Storage|"Storage"|
|LID_API_SystemErrorCode_Subsystem_Syringepump|"Syringe pump"|
|LID_API_SystemErrorCode_Subsystem_Trigger|"Trigger"|
|LID_API_SystemErrorCode_System_""|""|
|LID_API_SystemErrorCode_System_ControllerBoard|"Controller Board"|
|LID_API_SystemErrorCode_System_Fluidics|"Fluidics"|
|LID_API_SystemErrorCode_System_Imaging|"Imaging"|
|LID_API_SystemErrorCode_System_Instrument|"Instrument"|
|LID_API_SystemErrorCode_System_Motion|"Motion"|
|LID_API_SystemErrorCode_System_Reagents|"Reagents"|
|LID_API_SystemErrorCode_System_Sample|"Sample"|
