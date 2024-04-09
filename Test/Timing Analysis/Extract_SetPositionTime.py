from ExtractInfo_Common import ExtractInfoCommon
import sys

dirpath = ''
filename = 'SetPositionTime'
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
            lines, lengths, temp_lines = common_func.getBlockLines(searchlines, "SyringePump::setPosition:: moving to", [], False)
            for i, line in enumerate(lines):
                tempstring = line
                left,sep,right = tempstring.partition('SyringePump::setPosition:: moving to')
                if sep:
                    tempstring = right
                    left,sep,right = tempstring.partition(', current volume')
                    if sep:
                        fdata.write(left.replace('ul', '').replace("ul/sec", '').replace(' ', '') + ',')            

        print("==============================")
        startTimeStr = ""
        endTimeStr = ""
        fdata.write('\n')
        #filename, fileext = os.path.splitext(os.path.basename(logfile))
        #fdata.write(filename + ',')
        lines, lengths, roll_over_lines = common_func.getBlockLines(searchlines, "<SyringePump>", roll_over_lines, True)
        print(lengths)

        line_break_count = 0
        if len(lengths) > 0:
            line_break_count = lengths[0]
        for i, line in enumerate(lines):
            if i == 0:
                fdata.write("WF_" + str(wf_counter) + ',')
                flines.write("Work flow is WF_" + str(wf_counter) + "\n")
                wf_counter += 1
            if line_break_count == 0:
                 fdata.write('\n')
                 fdata.write("WF_" + str(wf_counter) + ',')
                 flines.write("Work flow is WF_" + str(wf_counter) + "\n")
                 wf_counter += 1
                 lengths = lengths[1:]
                 if len(lengths) > 0:
                     line_break_count = lengths[0]
            line_break_count -= 1
            
            if "setPosition: <enter>" in line:
                startTimeStr = line
                flines.write(line)

            if "setPosition: <exit>" in line:
                endTimeStr = line
                flines.write(line)
                fdata.write(common_func.getTimeDifference(startTimeStr, endTimeStr) + ',')
                startTimeStr = endTimeStr
                endTimeStr = ""

    fin.close()
    fdata.close()
    flines.close()
