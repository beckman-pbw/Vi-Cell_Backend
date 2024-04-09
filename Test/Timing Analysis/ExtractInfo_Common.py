from datetime import datetime
import glob
import os
import sys

class ExtractInfoCommon(object):

    def __init__(self, dirpath, block_start_tag, block_end_tag, filename):
        
        self.__block_start_tag = block_start_tag
        self.__block_end_tag = block_end_tag
        self.private_initialize(dirpath, filename)

    def private_initialize(self, dirpath, filename):
        
        self.__logfile_folder = ''
        if len(dirpath) > 0 and os.path.exists(os.path.dirname(dirpath)) is True:
            self.__logfile_folder = dirpath
            
        print("folder path : " + self.__logfile_folder)
        self.__logfile_list = sorted(glob.glob(self.__logfile_folder + 'HawkeyeDLL.log*'), reverse = True)

        self.__logextract_data = self.__logfile_folder + filename + '.csv'
        self.__logextract_lines = self.__logfile_folder + filename + '.txt'
        
        if os.path.isfile(self.__logextract_data):
            os.remove(self.__logextract_data)
        if os.path.isfile(self.__logextract_lines):
            os.remove(self.__logextract_lines)

    def blockExist(self, searchlines):
        
        startTagFound = False

        for i, line in enumerate(searchlines):
            if self.__block_start_tag in line:
                startTagFound = True
            if startTagFound is True:
                if self.__block_end_tag in line:
                    return True
        return False

    def getBlockLines(self, searchlines, matchTag, previous_incomplete_lines, allBlocks):
        lines = []
        lengths = []
        temp_lines = previous_incomplete_lines.copy()

        start_found = False
        for i, line in enumerate(searchlines):
            if start_found is False and (self.__block_start_tag in line or len(previous_incomplete_lines) > 0):
                start_found = True
                previous_incomplete_lines.clear()
                #print(line)
            if start_found is True and (len(matchTag) <= 0 or matchTag in line):
                temp_lines.append(line)
            if start_found is True and self.__block_end_tag in line:
                #print(line)
                lengths.append(len(temp_lines))
                lines = lines + temp_lines
                temp_lines.clear()
                start_found = False
                if allBlocks is False:
                    break
        return lines, lengths, temp_lines

    def getAllBlockLines(self, searchlines, previous_incomplete_lines, allBlocks):
        return self.getBlockLines(searchlines, "", previous_incomplete_lines, allBlocks)

    def getLogfileList(self):
        return self.__logfile_list

    def getDataFilename(self):
        return self.__logextract_data

    def getLinesFilename(self):
        return self.__logextract_lines

    # get time in millisec
    def getTimeFromString(self, inStr):
        
        l,s,r = inStr.partition(']')
        if s:
            datetime_object = datetime.strptime((l + s), '[%Y-%b-%d %H:%M:%S.%f]')
            return datetime_object

    # get time differnce in millisec
    def getTimeDifference(self, start, end):
        
        stTime = self.getTimeFromString(start)
        edTime = self.getTimeFromString(end)
        elapsedTime = edTime - stTime

        return str(int(elapsedTime.total_seconds() * 1000))
