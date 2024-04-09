from ExtractInfo_Common import ExtractInfoCommon
import sys

dirpath = ''
filename = 'ProbeDownUpTime'
block_start_tag = "opening workflow file: Sample_Workflow-Normal"
block_end_tag = "Deleting current Workflow"
headerWritten = False
roll_over_lines = []
wf_counter = 0

if len(sys.argv) > 1:
    dirpath = sys.argv[1]
common_func = ExtractInfoCommon(dirpath, block_start_tag, block_end_tag, filename)

if len(common_func.getLogfileList()) == 0:
    print("Folder doesn't contains ncecessary files")

for logfile in common_func.getLogfileList():
    with open(logfile, "r") as fin, open(common_func.getDataFilename(),"a") as fdata, open(common_func.getLinesFilename(),"a") as flines:

        flines.write("\n=============================================================" + logfile + "\n")
        
        searchlines = fin.readlines()

        if headerWritten is False:
            if common_func.blockExist(searchlines) is False:
                continue
            fdata.write(',')
            headerWritten = True
            fdata.write('Probe Down to Up Time' + ',')

        print("==============================")
        fdata.write('\n')
        lines, lengths, roll_over_lines = common_func.getAllBlockLines(searchlines, roll_over_lines, True)
        if len(lines) <= 0:
            continue
        
        startTimeStr = ""
        endTimeStr = ""
        probeUpDone = False

        line_break_count = 0
        if len(lengths) > 0:
            line_break_count = lengths[0]
        for i, line in enumerate(lines):
            if i == 0:
                fdata.write("WF_" + str(wf_counter) + ',')
                flines.write("Work flow is WF_" + str(wf_counter) + "\n")
                #print("WF_" + str(wf_counter))
                wf_counter += 1
            if line_break_count == 0:
                 fdata.write('\n')
                 fdata.write("WF_" + str(wf_counter) + ',')
                 flines.write("Work flow is WF_" + str(wf_counter) + "\n")
                 #print("WF_" + str(wf_counter))
                 wf_counter += 1
                 lengths = lengths[1:]
                 if len(lengths) > 0:
                     line_break_count = lengths[0]
            line_break_count -= 1

            if "<StageController> Moving Probe motor down" in line:
                startTimeStr = line
                flines.write(line)

            if "<StageController> doProbeUpAsync : Probe_Sequence_Complete" in line:
                probeUpDone = True
                flines.write(line)

            if probeUpDone is True and "onSampleWorkflowStateChanged:: WorkQueueItem:" in line:
                tempstring = line
                left,sep,right = tempstring.partition('Status:')
                if sep and right.strip().replace('\n', '') is "5":
                    endTimeStr = line
                    fdata.write(common_func.getTimeDifference(startTimeStr, endTimeStr) + ',')
                    flines.write(line)

                    startTimeStr = ""
                    endTimeStr = ""

    fin.close()
    fdata.close()
    flines.close()
