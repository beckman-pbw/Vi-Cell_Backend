Python v3.0 or newer must be installed on system
All the logs must be generated with logging level set to atleast "DBG1"

The python script(s) look for all the files with name "HawkeyeDLL.log*" in input directory only (no subdirectories)
All the files are read in chronological order, most recent file ("HawkeyeDLL.log") is processed last
The script are designed to analyze data between particular block of log data
	Start_of_Block = "opening workflow file: Sample_Workflow-Normal"
	End_of_Block = "Deleting current Workflow"
	
	Note : "Deleting current Workflow" string is logged when starting the new workflow (of any kind), so in case where user has paused the work queue and started it again after sometime or started some other
			workflow in between, the timing analysis for that sample (data between "Start_of_Block" and "End_of_Block") may be incorrect.
			We can discard this data once the data is generated.
			

Each script creates two output files
	FileName.csv = Contains the time taken (in millisec) for certain operation for each sample
	FileName.txt = Contains the lines extracted from original log files for which the timing analysis is done
	
Each script takes "\\Full_Path_To_Directory\\"  as input argument. If none is given then look for files in default location of "C:\\"

**********************************************************************************************************************************
RunExtractInfos.bat
	Edit this batch file to set the processing directorty and run it to execute all the python scripts
	'folder_path=root\path_to_directory\'
**********************************************************************************************************************************
	
##################################################################################################################################
Extract_SetValveTime.py
	
	Creates "SetValveTime.csv" and "SetValveTime.txt"
	Timing is done for syringe value move operation
	Looks for text in original log files
		"setValve: <enter>"
		"setValve: <exit>"
##################################################################################################################################


##################################################################################################################################
Extract_SetPositionTime.py
	
	Creates "SetPositionTime.csv" and "SetPositionTime.txt"
	Timing is done for syringe aspirate/dispense operation
	Looks for text in original log files
		"setPosition: <enter>"
		"setPosition: <exit>"
##################################################################################################################################


##################################################################################################################################
Extract_StateChangeTime.py
	
	Creates "StateChangeTime.csv" and "StateChangeTime.txt"
	Timing is done for how much time each state has taken
	Looks for text in original log files
		"onSampleWorkflowStateChanged:: WorkQueueItem:"
		
	Note : Description of various states
		state_0 : Timing between line containing "Start_of_Block (opening workflow file: Sample_Workflow-Normal)" and line containing "onSampleWorkflowStateChanged:: WorkQueueItem: (*), Status: 0"
		state_1 : Timing between line containing "onSampleWorkflowStateChanged:: WorkQueueItem: (*), Status: 0" and line containing "onSampleWorkflowStateChanged:: WorkQueueItem: (*), Status: 1"
		state_2 : Timing between line containing "onSampleWorkflowStateChanged:: WorkQueueItem: (*), Status: 1" and line containing "onSampleWorkflowStateChanged:: WorkQueueItem: (*), Status: 2"
		state_3 : Timing between line containing "onSampleWorkflowStateChanged:: WorkQueueItem: (*), Status: 2" and line containing "onSampleWorkflowStateChanged:: WorkQueueItem: (*), Status: 3"
		state_4 : Timing between line containing "onSampleWorkflowStateChanged:: WorkQueueItem: (*), Status: 3" and line containing "onSampleWorkflowStateChanged:: WorkQueueItem: (*), Status: 4"
		state_5 : Timing between line containing "onSampleWorkflowStateChanged:: WorkQueueItem: (*), Status: 4" and line containing "onSampleWorkflowStateChanged:: WorkQueueItem: (*), Status: 5"
		end 	: Timing between line containing "onSampleWorkflowStateChanged:: WorkQueueItem: (*), Status: 5" and line containing "End_of_Block (Deleting current Workflow)"
		
	Sum of these states will give total time for sample to sample processing (including stage move)
##################################################################################################################################


##################################################################################################################################
Extract_ProbeDownUpTime.py
	
	Creates "ProbeDownUpTime.csv" and "ProbeDownUpTime.txt"
	Timing is done between probe down to probe up for each sample
	Looks for text in original log files
		"<StageController> Moving Probe motor down"
		"onSampleWorkflowStateChanged:: WorkQueueItem: (*), Status: 5"

	Note : "Status: 5" is where we are finished with sample processing (including move probe up)
##################################################################################################################################