

Public Class Form1
    Private my_layout As New Layout
    Private temp_cleaner As New LayoutReagent
    Private temp_reagent As New LayoutReagent
    Private temp_analysis As New LayoutAnalysis
    Private temp_parameter As New LayoutAnalysisParameter

    Private reagent_list As New ReagentListManager
    Private rfid_payload As New RFIDDataOutput

    Public Function CreateAnalysisParameterList() As List(Of AnalysisParameter)
        Dim L As New List(Of AnalysisParameter)

        ' Current as of L&T Algorithm Revision 3.1
        L.Add(New AnalysisParameter("NONE", 0))
        L.Add(New AnalysisParameter("IsCell", 1))
        L.Add(New AnalysisParameter("IsPOI", 2))
        L.Add(New AnalysisParameter("IsBubble", 3))
        L.Add(New AnalysisParameter("IsDeclustered", 4))
        L.Add(New AnalysisParameter("IsIrregular", 5))
        L.Add(New AnalysisParameter("Area", 6))
        L.Add(New AnalysisParameter("DiameterInPixels", 7))
        L.Add(New AnalysisParameter("DiameterInMicrons", 8))
        L.Add(New AnalysisParameter("Circularity", 9))
        L.Add(New AnalysisParameter("Sharpness", 10))
        L.Add(New AnalysisParameter("Perimeter", 11))
        L.Add(New AnalysisParameter("Eccentricity", 12))
        L.Add(New AnalysisParameter("AspectRatio", 13))
        L.Add(New AnalysisParameter("MinorAxis", 14))
        L.Add(New AnalysisParameter("MajorAxis", 15))
        L.Add(New AnalysisParameter("Volume", 16))
        L.Add(New AnalysisParameter("Roundness", 17))
        L.Add(New AnalysisParameter("MinEnclosedArea", 18))
        L.Add(New AnalysisParameter("Elimination", 19))
        L.Add(New AnalysisParameter("CellSpotArea", 20))
        L.Add(New AnalysisParameter("AvgSpotBrightness", 21))
        L.Add(New AnalysisParameter("FLAvgIntensity", 25))
        L.Add(New AnalysisParameter("FLCellArea", 26))
        L.Add(New AnalysisParameter("FLSubPeakCount", 27))
        L.Add(New AnalysisParameter("FLPeakAvgIntensity", 28))
        L.Add(New AnalysisParameter("FLPeakArea", 29))
        L.Add(New AnalysisParameter("FLPeakLoc_X", 30))
        L.Add(New AnalysisParameter("FLPeakLoc_Y", 31))
        L.Add(New AnalysisParameter("FLSubPeakAvgIntensity", 32))
        L.Add(New AnalysisParameter("FLSubPeakPixelCount", 33))
        L.Add(New AnalysisParameter("FLSubPeakArea", 34))
        L.Add(New AnalysisParameter("FLSubPeakLoc_X", 35))
        L.Add(New AnalysisParameter("FLSubPeakLoc_Y", 36))
        L.Add(New AnalysisParameter("CFG_SpotAreaPercentage", 100))
        L.Add(New AnalysisParameter("CFG_FLPeakPercentage", 101))
        L.Add(New AnalysisParameter("CFG_FLScalableROI", 102))

        Return L
    End Function
    Public Function GenerateMemoryMapText(ByVal Filename As String) As Boolean
        ' Generates a CSV file suitable for use in building a Beckman Coulter Memory Map document.
        ' Key concepts:
        '   REGISTER - logical unit in the file, typically one "value".
        '   BYTE - Offset in DEC and HEX, Value in HEX (, if STRING: ASCII character representation

        ' First we build a table of REGISTERS
        '     REGNUM,BYTEDEC,BYTEHEX,VALUE,REGSIZE
        '     <blank>,BYTEDEC,BYTEHEX,VALUE,<blank>
        '       "      "        "       "     "
        '     REGNUM,BYTEDEC,BYTEHEX,VALUE,REGSIZE
        '     ...
        ' Then we build the Register Description Table
        '         "Module Description","Reg. Name", "Value"
        '         Part Name,"("+RegNum+") "+RegName,Value (description of literal: "Floating Point 123.45", "ASCII 'LLAMAS'", "ASCII PER ___ BAR CODE AT ASSY"

        If My.Computer.FileSystem.FileExists(Filename) Then
            If MessageBox.Show("File " + Filename + " already exists.  Overwrite it?", "File already exists", MessageBoxButtons.YesNo) <> DialogResult.Yes Then
                Return False
            End If
        End If

        Dim output As IO.StreamWriter = My.Computer.FileSystem.OpenTextFileWriter(Filename, False)

        Dim str As String = ""

        ' Register Layout Table
        str = "Reg,Dec,Hex,Contents,Size"
        output.WriteLine(str)

        Return True
    End Function

    Public Sub RecalculateCounts()

        Lbl_NumCleaners.Text = my_layout.NumCleaners.ToString
        If my_layout.NumReagents = 0 Then
            Lbl_HasReagent.Text = "No"
        Else
            Lbl_HasReagent.Text = "Yes"
        End If

        Lbl_NumAnalyses.Text = my_layout.NumAnalyses.ToString

        Dim MAXSZ As Integer = 240

        Dim sz As Integer = my_layout.Size
        Lbl_ContentBytes.Text = sz.ToString
        If sz > MAXSZ Then
            Lbl_ContentBytes.ForeColor = Color.Red
        Else
            Lbl_ContentBytes.ForeColor = Color.Black
        End If

        Dim remain As Integer = MAXSZ - sz
        Lbl_ContentBytesRemaining.Text = remain.ToString
        If remain < 0 Then
            Lbl_ContentBytesRemaining.ForeColor = Color.Red
        Else
            Lbl_ContentBytesRemaining.ForeColor = Color.Black
        End If


    End Sub
    Public Sub ResetCleanerCount()
        Lbl_NumCleaners.Text = my_layout.NumCleaners.ToString
    End Sub
    Public Sub ResetCleanerUI()
        CB_CleanerList.SelectedIndex = 0
        UD_CleanerUses.Value = 0
    End Sub
    Public Sub DisplayCleanerList()
        LB_Cleaners.Items.Clear()

        For Each cl In my_layout.Cleaners
            Dim txt As String
            txt = cl.Description + " (Index: " + cl.Index.ToString + "), PN: " + cl.PartNumber.ToString + ", " + cl.TotalUses.ToString + " uses"
            LB_Cleaners.Items.Add(txt)
        Next
    End Sub

    Public Sub DisplayCleanStepsList()
        LB_PrimaryCleaningSteps.Items.Clear()
        For Each stp In temp_reagent.PrimaryCleaningSteps
            LB_PrimaryCleaningSteps.Items.Add(stp.ToString)
        Next

        LB_SecondaryCleaningSteps.Items.Clear()
        For Each stp In temp_reagent.SecondaryCleaningSteps
            LB_SecondaryCleaningSteps.Items.Add(stp.ToString)
        Next
    End Sub

    Public Sub ResetReagentCount()
        If my_layout.NumReagents > 0 Then
            Lbl_HasReagent.Text = "Yes"
        Else
            Lbl_HasReagent.Text = "No"
        End If
    End Sub
    Public Sub ResetAnalysisCount()
        Lbl_NumAnalyses.Text = my_layout.NumAnalyses.ToString
    End Sub

    Public Sub DisplayAnalysisReagentList()
        LB_AnalysisReagents.Items.Clear()

        For Each r In temp_analysis.Reagent
            For Each ri As ReagentDefinition In Me.reagent_list.ReagentList()
                If r = ri.Index Then
                    LB_AnalysisReagents.Items.Add(ri.Label)
                    Exit For
                End If
            Next
        Next
    End Sub

    Public Sub DisplayAnalysisIlluminatorList()
        LB_AnalysisIlluminators.Items.Clear()

        For Each il In temp_analysis.Illuminator
            Dim txt As String
            txt = "Exitation: " + il.Illumination.ToString + "nm" + vbTab + "Emission: " + il.Emission.ToString + "nm" + vbTab + "Exposure: " + il.ExposureTime.ToString + "ms"

            LB_AnalysisIlluminators.Items.Add(txt)
        Next
    End Sub
    Public Sub DisplayAnalysisParameterList()
        LB_AnalysisParams.Items.Clear()

        For Each p In temp_analysis.Parameters
            Dim txt As String
            txt = "Key: " + p.Key.ToString + " Subkey: " + p.SubKey.ToString + " Threshold: " + p.Threshold.ToString
            If p.AtAbove Then
                txt += " (At/Above)"
            Else
                txt += " (Below)"
            End If

            LB_AnalysisParams.Items.Add(txt)
        Next
    End Sub

    Public Sub DisplayAnalysisList()
        LB_Analyses.Items.Clear()

        For Each A In my_layout.Analyses
            Dim txt As String
            txt = "(IDX " + A.Index.ToString + ") " + A.Description + ", " + A.Reagent.Count.ToString + " reagents, " + A.Illuminator.Count.ToString + " illum, " + A.Parameters.Count.ToString + " params"
            LB_Analyses.Items.Add(txt)
        Next
    End Sub

    Private Sub TB_PackPN_TextChanged(sender As Object, e As EventArgs) Handles TB_PackPN.TextChanged
        my_layout.Header.PackPN = TB_PackPN.Text.Trim
    End Sub

    Private Sub UP_ServiceLife_ValueChanged(sender As Object, e As EventArgs) Handles UP_ServiceLife.ValueChanged
        my_layout.Header.ServiceLife = UP_ServiceLife.Value
    End Sub

    Private Sub CB_CleanerList_SelectedIndexChanged(sender As Object, e As EventArgs) Handles CB_CleanerList.SelectedIndexChanged
        Dim ri As ReagentDefinition = CB_CleanerList.SelectedItem

        LBL_CleanerIndex.Text = ri.Index.ToString
        LBL_CleanerDescription.Text = ri.Label
        LBL_CleanerPN.Text = ri.PartNumber

        temp_cleaner.Index = ri.Index
        temp_cleaner.Description = ri.Label
        temp_cleaner.PartNumber = ri.PartNumber
        temp_cleaner.MixingCycles = 0
    End Sub

    Private Sub UD_CleanerUses_ValueChanged(sender As Object, e As EventArgs) Handles UD_CleanerUses.ValueChanged
        temp_cleaner.TotalUses = UD_CleanerUses.Value
    End Sub

    Private Sub Btn_AddCleaner_Click(sender As Object, e As EventArgs) Handles Btn_AddCleaner.Click
        my_layout.AddCleaner(temp_cleaner)
        temp_cleaner = New LayoutReagent
        ResetCleanerUI()
        DisplayCleanerList()
        RecalculateCounts()
    End Sub

    Private Sub Btn_CleanerUP_Click(sender As Object, e As EventArgs) Handles Btn_CleanerUP.Click
        If LB_Cleaners.SelectedIndex <> -1 Then
            my_layout.MoveCleanerUp(LB_Cleaners.SelectedIndex)
            DisplayCleanerList()
        End If
    End Sub

    Private Sub Btn_CleanerDN_Click(sender As Object, e As EventArgs) Handles Btn_CleanerDN.Click
        If LB_Cleaners.SelectedIndex <> -1 Then
            my_layout.MoveCleanerDown(LB_Cleaners.SelectedIndex)
            DisplayCleanerList()
        End If
    End Sub

    Private Sub Btn_CleanerDEL_Click(sender As Object, e As EventArgs) Handles Btn_CleanerDEL.Click
        If LB_Cleaners.SelectedIndex <> -1 Then
            my_layout.RemoveCleaner(LB_Cleaners.SelectedIndex)
            DisplayCleanerList()
            RecalculateCounts()
        End If
    End Sub

    Private Sub CB_IncludeReagent_CheckedChanged(sender As Object, e As EventArgs) Handles CB_IncludeReagent.CheckedChanged
        my_layout.Reagents.Clear()

        If CB_IncludeReagent.Checked Then
            Dim ri As ReagentDefinition = CB_PackReagent.SelectedItem

            If Not ri Is Nothing Then
                'Dim temp_reagent As New LayoutReagent
                'temp_reagent.Index = ri.Index
                'temp_reagent.Description = ri.Label
                'temp_reagent.PartNumber = ri.PartNumber
                'temp_reagent.MixingCycles = UD_ReagentCycles.Value
                'temp_reagent.TotalUses = UD_ReagentUses.Value

                my_layout.AddReagent(temp_reagent)
            End If
        End If

        RecalculateCounts()
    End Sub

    Private Sub UD_AnalysisIndex_ValueChanged(sender As Object, e As EventArgs) Handles UD_AnalysisIndex.ValueChanged
        temp_analysis.Index = UD_AnalysisIndex.Value
    End Sub

    Private Sub TB_AnalysisDesc_TextChanged(sender As Object, e As EventArgs) Handles TB_AnalysisDesc.TextChanged
        temp_analysis.Description = TB_AnalysisDesc.Text.Trim
    End Sub

    Private Sub Btn_AnalysisReagentAdd_Click(sender As Object, e As EventArgs) Handles Btn_AnalysisReagentAdd.Click
        Dim ri As ReagentDefinition = CB_AnalysisReagent.SelectedItem

        temp_analysis.AddReagent(ri.Index)

        CB_AnalysisReagent.SelectedIndex = 0
        DisplayAnalysisReagentList()
    End Sub

    Public Sub ResetAnalysisIlluminators()
        UD_AnalysisIlluminatorWL.Value = 0
        UD_AnalysisIlluminatorMS.Value = 0
        LB_AnalysisIlluminators.Items.Clear()
    End Sub

    Private Sub Btn_AnalysisReagentUP_Click(sender As Object, e As EventArgs) Handles Btn_AnalysisReagentUP.Click
        If LB_AnalysisReagents.SelectedIndex <> -1 Then
            temp_analysis.MoveReagentUp(LB_AnalysisReagents.SelectedIndex)
            DisplayAnalysisReagentList()
        End If
    End Sub

    Private Sub Btn_AnalysisReagentDN_Click(sender As Object, e As EventArgs) Handles Btn_AnalysisReagentDN.Click
        If LB_AnalysisReagents.SelectedIndex <> -1 Then
            temp_analysis.MoveReagentDown(LB_AnalysisReagents.SelectedIndex)
            DisplayAnalysisReagentList()
        End If
    End Sub

    Private Sub Btn_AnalysisReagentDEL_Click(sender As Object, e As EventArgs) Handles Btn_AnalysisReagentDEL.Click
        If LB_AnalysisReagents.SelectedIndex <> -1 Then
            temp_analysis.RemoveReagent(LB_AnalysisReagents.SelectedIndex)
            DisplayAnalysisReagentList()
        End If
    End Sub

    Private Sub Btn_AnalysisIlluminatorAdd_Click(sender As Object, e As EventArgs) Handles Btn_AnalysisIlluminatorAdd.Click
        Dim FL As New FLIlluminator
        FL.ExposureTime = UD_AnalysisIlluminatorMS.Value
        FL.Emission = UD_AnalysisEmissionWavelength.Value
        FL.Illumination = UD_AnalysisIlluminatorWL.Value

        temp_analysis.AddIlluminator(FL)

        ResetAnalysisIlluminators()
        DisplayAnalysisIlluminatorList()
    End Sub

    Private Sub Btn_AnalysisIlluminatorUP_Click(sender As Object, e As EventArgs) Handles Btn_AnalysisIlluminatorUP.Click
        If LB_AnalysisIlluminators.SelectedIndex <> -1 Then
            temp_analysis.MoveIlluminatorUp(LB_AnalysisIlluminators.SelectedIndex)
            DisplayAnalysisIlluminatorList()
        End If
    End Sub

    Private Sub Btn_AnalysisIlluminatorDN_Click(sender As Object, e As EventArgs) Handles Btn_AnalysisIlluminatorDN.Click
        If LB_AnalysisIlluminators.SelectedIndex <> -1 Then
            temp_analysis.MoveIlluminatorDown(LB_AnalysisIlluminators.SelectedIndex)
            DisplayAnalysisIlluminatorList()
        End If
    End Sub

    Private Sub Btn_AnalysisIlluminatorDEL_Click(sender As Object, e As EventArgs) Handles Btn_AnalysisIlluminatorDEL.Click
        If LB_AnalysisIlluminators.SelectedIndex <> -1 Then
            temp_analysis.RemoveIlluminator(LB_AnalysisIlluminators.SelectedIndex)
            DisplayAnalysisIlluminatorList()
        End If
    End Sub

    Private Sub NumericUpDown2_ValueChanged(sender As Object, e As EventArgs) Handles UD_AnalysisPopSubkey.ValueChanged
        temp_analysis.PopulationKey.SubKey = UD_AnalysisPopSubkey.Value
    End Sub

    Private Sub UD_PopThreshold_ValueChanged(sender As Object, e As EventArgs) Handles UD_AnalysisPopThreshold.ValueChanged
        temp_analysis.PopulationKey.Threshold = UD_AnalysisPopThreshold.Value
    End Sub

    Private Sub RB_PopAtAbove_CheckedChanged(sender As Object, e As EventArgs) Handles RB_PopAtAbove.CheckedChanged
        temp_analysis.PopulationKey.AtAbove = RB_PopAtAbove.Checked
    End Sub

    Private Sub RB_PopBelow_CheckedChanged(sender As Object, e As EventArgs) Handles RB_PopBelow.CheckedChanged
        temp_analysis.PopulationKey.AtAbove = Not RB_PopBelow.Checked
    End Sub

    Private Sub Btn_PopulationClear_Click(sender As Object, e As EventArgs) Handles Btn_PopulationClear.Click
        temp_analysis.PopulationKey.Clear()
        ResetAnalysisPopulationKey()
    End Sub
    Public Sub ResetAnalysisPopulationKey()
        CB_AnalysisPopParameter.SelectedIndex = 0
        UD_AnalysisPopThreshold.Value = 0
    End Sub

    Private Sub Btn_AddAnalysisParam_Click(sender As Object, e As EventArgs) Handles Btn_AddAnalysisParam.Click
        Dim p As New LayoutAnalysisParameter

        If CB_AnalysisParameter.SelectedIndex = 0 Then
            Return
        End If
        Dim AP As AnalysisParameter = CB_AnalysisParameter.SelectedItem

        p.Key = AP.Key
        p.SubKey = UD_AnalysisParamSubkey.Value
        p.Threshold = UD_AnalysisParamThreshold.Value
        p.AtAbove = RB_AnalysisParamAtAbove.Checked

        temp_analysis.AddParam(p)

        ResetAnalysisParam()

        DisplayAnalysisParameterList()
    End Sub
    Public Sub ResetAnalysisParam()
        CB_AnalysisParameter.SelectedIndex = 0
        UD_AnalysisParamSubkey.Value = 0
        UD_AnalysisParamThreshold.Value = 0
        RB_AnalysisParamAtAbove.Checked = True
    End Sub
    Public Sub ResetAnalysisParamGB()
        ResetAnalysisParam()
        LB_AnalysisParams.Items.Clear()
    End Sub

    Private Sub Btn_AnalysisParamUP_Click(sender As Object, e As EventArgs) Handles Btn_AnalysisParamUP.Click
        If LB_AnalysisParams.SelectedIndex <> -1 Then
            temp_analysis.MoveParameterUp(LB_AnalysisParams.SelectedIndex)
            DisplayAnalysisParameterList()
        End If
    End Sub

    Private Sub Btn_AnalysisParamDN_Click(sender As Object, e As EventArgs) Handles Btn_AnalysisParamDN.Click
        If LB_AnalysisParams.SelectedIndex <> -1 Then
            temp_analysis.MoveParameterDown(LB_AnalysisParams.SelectedIndex)
            DisplayAnalysisParameterList()
        End If
    End Sub

    Private Sub Btn_AnalysisParamDEL_Click(sender As Object, e As EventArgs) Handles Btn_AnalysisParamDEL.Click
        If LB_AnalysisParams.SelectedIndex <> -1 Then
            temp_analysis.RemoveParameter(LB_AnalysisParams.SelectedIndex)
            DisplayAnalysisParameterList()
        End If
    End Sub

    Public Sub ResetPackHeader()
        TB_PackPN.Clear()
        UP_ServiceLife.Value = 0
        cbFullLayout.Checked = False
        dtpPackExpiration.ResetText()
        udPackLotNumber.Value = 0

        my_layout.Header.Clear()
    End Sub

    Public Sub ResetAnalysisHeader()
        UD_AnalysisIndex.Value = 0
        TB_AnalysisDesc.Text = ""
    End Sub
    Private Sub Btn_AddAnalysis_Click(sender As Object, e As EventArgs) Handles Btn_AddAnalysis.Click
        my_layout.AddAnalysis(temp_analysis)
        temp_analysis = New LayoutAnalysis

        ResetAnalysisHeader()
        ResetAnalysisIlluminators()
        ResetAnalysisReagent()
        ResetAnalysisParamGB()
        ResetAnalysisPopulationKey()

        DisplayAnalysisList()
        RecalculateCounts()
    End Sub

    Private Sub ResetAnalysisReagent()
        LB_AnalysisReagents.Items.Clear()
        CB_AnalysisReagent.SelectedIndex = 0
    End Sub

    Private Sub Btn_AnalysisUP_Click(sender As Object, e As EventArgs) Handles Btn_AnalysisUP.Click
        If LB_Analyses.SelectedIndex <> -1 Then
            my_layout.MoveAnalysisUp(LB_Analyses.SelectedIndex)
            DisplayAnalysisList()
        End If
    End Sub

    Private Sub Btn_AnalysisDN_Click(sender As Object, e As EventArgs) Handles Btn_AnalysisDN.Click
        If LB_Analyses.SelectedIndex <> -1 Then
            my_layout.MoveAnalysisDown(LB_Analyses.SelectedIndex)
            DisplayAnalysisList()
        End If
    End Sub

    Private Sub Btn_AnalysisDEL_Click(sender As Object, e As EventArgs) Handles Btn_AnalysisDEL.Click
        If LB_Analyses.SelectedIndex <> -1 Then
            my_layout.RemoveAnalysis(LB_Analyses.SelectedIndex)
            DisplayAnalysisList()
        End If

        RecalculateCounts()
    End Sub

    Private Sub Btn_Reset_Click(sender As Object, e As EventArgs) Handles Btn_Reset.Click
        my_layout = New Layout

        ResetPackHeader()
        ResetAnalysisHeader()
        ResetAnalysisIlluminators()
        ResetAnalysisReagent()
        ResetAnalysisPopulationKey()
        ResetAnalysisParamGB()


        DisplayAnalysisIlluminatorList()
        DisplayAnalysisList()
        DisplayAnalysisParameterList()
        DisplayAnalysisReagentList()
        DisplayCleanerList()



    End Sub

    Private Sub Btn_Generate_Click(sender As Object, e As EventArgs) Handles Btn_Generate.Click

        ' Housekeeping - make sure that any selected reagent is now added to the layout
        If CB_IncludeReagent.Checked Then
            Dim ri As ReagentDefinition = CB_PackReagent.SelectedItem

            If ri Is Nothing Then
                MsgBox("Reagent is included, but no reagent is selected.")
                Return
            End If
        Else
            my_layout.Reagents.Clear()
        End If


        Dim fd As SaveFileDialog = New SaveFileDialog
        Dim filename As String

        fd.InitialDirectory = My.Computer.FileSystem.CurrentDirectory
        fd.Title = "Select Output File Location/Name"


        If fd.ShowDialog <> DialogResult.OK Then
            Return
        End If


        filename = fd.FileName

        Dim ppath As String = System.IO.Path.GetDirectoryName(filename)

        Dim pfile As String = System.IO.Path.GetFileName(filename)


        ' Will create 4 files: <file>.AddressMap.csv, <file>.RegisterMap.csv, <GTIN>.bin and <GTIN>.payload

        Dim addressmap As String = ""
        Dim register_num As Integer = 0
        Dim byte_offset As Integer = 0
        my_layout.AddToLayoutString(register_num, byte_offset, addressmap)

        Dim registermap As String = ""
        register_num = 0
        my_layout.AddToRegisterMapString(register_num, registermap)

        Dim bytes As New List(Of Byte)
        bytes = my_layout.ToByteList()

        ' Ensure that byte map is fully padded out to 252 bytes (256 - 4)
        Dim b_sz As Integer = bytes.Count
        For i As Integer = b_sz To 251
            bytes.Add(0)
        Next

        ' TODO: write.
        Dim adrmap As IO.StreamWriter = FileIO.FileSystem.OpenTextFileWriter(filename + ".addressmap.csv", False)
        adrmap.Write(addressmap)
        adrmap.Close()

        Dim regmap As IO.StreamWriter = FileIO.FileSystem.OpenTextFileWriter(filename + ".registermap.csv", False)
        regmap.Write(registermap)
        regmap.Close()

        FileIO.FileSystem.WriteAllBytes(ppath + "\" + TB_GTIN.Text + ".bin", bytes.ToArray, False)

        Dim payload As New RFIDDataOutput
        If payload.AddBinaryEntry(TB_GTIN.Text, pfile, bytes.ToArray, True) Then
            payload.WritePayloadFile(ppath + "\" + TB_GTIN.Text + ".payload")
        End If


    End Sub

    Private Sub cbFullLayout_CheckedChanged(sender As Object, e As EventArgs) Handles cbFullLayout.CheckedChanged
        udPackLotNumber.Enabled = cbFullLayout.Checked
        dtpPackExpiration.Enabled = cbFullLayout.Checked
        lblPackExpirationDate.Enabled = cbFullLayout.Checked
        lblPackLotNo.Enabled = cbFullLayout.Checked

        If cbFullLayout.Checked Then
            my_layout.Header.SetExpiration(dtpPackExpiration.Value)
            my_layout.Header.LotNumber = udPackLotNumber.Value
        Else
            my_layout.Header.Expiration = 0
            my_layout.Header.LotNumber = 0
        End If
    End Sub

    Private Sub dtpPackExpiration_ValueChanged(sender As Object, e As EventArgs) Handles dtpPackExpiration.ValueChanged
        my_layout.Header.SetExpiration(dtpPackExpiration.Value)
    End Sub

    Private Sub udPackLotNumber_ValueChanged(sender As Object, e As EventArgs) Handles udPackLotNumber.ValueChanged
        my_layout.Header.LotNumber = udPackLotNumber.Value
    End Sub

    Private Sub ComboBox1_SelectedIndexChanged(sender As Object, e As EventArgs) Handles CB_AnalysisPopParameter.SelectedIndexChanged
        ' Populate the Key and Subkey box.
        ' if first index is selected, then CLEAR both.
        Dim AP As AnalysisParameter = CB_AnalysisPopParameter.SelectedItem

        temp_analysis.PopulationKey.Key = AP.Key
        Lbl_AnalysisPopulationKey.Text = AP.Key.ToString
        UD_AnalysisPopSubkey.Value = AP.Subkey
    End Sub

    Private Sub Form1_Load(sender As Object, e As EventArgs) Handles Me.Load
        'set version info
        Dim v As New Version
        ' v = System.Deployment.Application.ApplicationDeployment.CurrentDeployment.CurrentVersion
        v = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version
        Me.LBL_version.Text = String.Format(Me.LBL_version.Text, v.Major, v.Minor, v.Build, v.Revision)


        Me.reagent_list.FromFile("ReagentInfo.txt")

        'Me.rfid_payload.LoadPayloadFile("HawkeyePlatformReagentPayload.txt")
        'LB_PayloadEntries.Items.Clear()
        'For Each pl As FirmwareBinaryEntry In Me.rfid_payload.Entries
        ' LB_PayloadEntries.Items.Add(pl)
        'Next
        TP_ContentManagement.Hide()

        ' Load up the analysis drop-downs.
        CB_AnalysisPopParameter.Items.AddRange(Me.CreateAnalysisParameterList().ToArray)
        CB_AnalysisPopParameter.SelectedIndex = 0

        CB_AnalysisParameter.Items.AddRange(Me.CreateAnalysisParameterList().ToArray)
        CB_AnalysisParameter.SelectedIndex = 0


        CB_CleanerList.Items.AddRange(Me.reagent_list.CleanerList().ToArray)
        CB_RgtCleanerList.Items.AddRange(Me.reagent_list.CleanerList().ToArray)

        CB_AnalysisReagent.Items.AddRange(Me.reagent_list.ReagentList().ToArray)

        ' For purposes of creating a "cleaning pack", let us specify a non-reagent in this slot.
        CB_PackReagent.Items.AddRange(Me.reagent_list.ReagentList().ToArray)
        CB_PackReagent.Items.AddRange(Me.reagent_list.CleanerList().ToArray)

        CB_ContainerType.Items.Clear()
        CB_ContainerType.Items.Add(ContainerTypes.Multi.ToString)
        CB_ContainerType.Items.Add(ContainerTypes.Single_Round.ToString)
        CB_ContainerType.Items.Add(ContainerTypes.Single_Square.ToString)
    End Sub

    Private Sub CB_AnalysisParameter_SelectedIndexChanged(sender As Object, e As EventArgs) Handles CB_AnalysisParameter.SelectedIndexChanged
        Dim AP As AnalysisParameter = CB_AnalysisParameter.SelectedItem
        LBL_AnalysisParameterKey.Text = AP.Key.ToString
        UD_AnalysisParamSubkey.Value = AP.Subkey
    End Sub

    Private Sub CB_ContainerType_SelectedIndexChanged(sender As Object, e As EventArgs) Handles CB_ContainerType.SelectedIndexChanged
        Dim v As New String("")
        v = CB_ContainerType.SelectedItem
        Dim ct As New ContainerTypes
        ct = ContainerTypes.Multi

        If (v = ContainerTypes.Multi.ToString) Then
            ct = ContainerTypes.Multi
        ElseIf v = ContainerTypes.Single_Round.ToString Then
            ct = ContainerTypes.Single_Round
        ElseIf v = ContainerTypes.Single_Square.ToString Then
            ct = ContainerTypes.Single_Square
        End If

        my_layout.ContainerType = ct
    End Sub

    Private Sub CB_PackReagent_SelectedIndexChanged(sender As Object, e As EventArgs) Handles CB_PackReagent.SelectedIndexChanged
        Dim ri As ReagentDefinition = CB_PackReagent.SelectedItem

        If Not ri Is Nothing Then
            temp_reagent.Index = ri.Index
            temp_reagent.PartNumber = ri.PartNumber
            temp_reagent.Description = ri.Label
            temp_reagent.PrimaryCleaningSteps.Clear()
            temp_reagent.SecondaryCleaningSteps.Clear()

            LBL_PackReagentDesc.Text = ri.Label
            LBL_PackReagentPN.Text = ri.PartNumber

            UD_ReagentCycles.Value = 1
            UD_ReagentUses.Value = 1
        End If

        DisplayCleanStepsList()
    End Sub

    Private Sub UD_ReagentCycles_ValueChanged(sender As Object, e As EventArgs) Handles UD_ReagentCycles.ValueChanged
        temp_reagent.MixingCycles = UD_ReagentCycles.Value
    End Sub

    Private Sub UD_ReagentUses_ValueChanged(sender As Object, e As EventArgs) Handles UD_ReagentUses.ValueChanged
        temp_reagent.TotalUses = UD_ReagentUses.Value
    End Sub

    Private Sub Btn_ClnStepUP_Click(sender As Object, e As EventArgs) Handles Btn_ClnStepUP_P.Click
        If LB_PrimaryCleaningSteps.SelectedIndex <> -1 Then
            temp_reagent.MovePCleanStepUp(LB_PrimaryCleaningSteps.SelectedIndex)
            DisplayCleanStepsList()
        End If
    End Sub

    Private Sub Btn_ClnStepDN_Click(sender As Object, e As EventArgs) Handles Btn_ClnStepDN_P.Click
        If LB_PrimaryCleaningSteps.SelectedIndex <> -1 Then
            temp_reagent.MovePCleanStepDown(LB_PrimaryCleaningSteps.SelectedIndex)
            DisplayCleanStepsList()
        End If
    End Sub

    Private Sub Btn_ClnStepDEL_Click(sender As Object, e As EventArgs) Handles Btn_ClnStepDEL_P.Click
        If LB_PrimaryCleaningSteps.SelectedIndex <> -1 Then
            temp_reagent.RemovePCleanStep(LB_PrimaryCleaningSteps.SelectedIndex)
            DisplayCleanStepsList()
        End If

        RecalculateCounts()
    End Sub

    Private Sub Btn_ClnStepAdd_Click(sender As Object, e As EventArgs) Handles Btn_ClnStepAdd.Click
        If CB_PackReagent.SelectedIndex = -1 Then
            Return
        End If


        Dim stp As New ReagentCleaningStep
        Dim rgt As ReagentDefinition = CB_RgtCleanerList.SelectedItem

        stp.Reagent = rgt.Index
        stp.ReagentLabel = rgt.Label

        If RB_Cln600uL.Checked Then
            stp.Volume = ReagentCleaningStep.CleaningVolume.v_600
        Else
            stp.Volume = ReagentCleaningStep.CleaningVolume.v_1200
        End If

        If RB_Cln30uLs.Checked Then
            stp.Speed = ReagentCleaningStep.CleaningSpeed.s_30
        ElseIf RB_Cln60uLs.Checked Then
            stp.Speed = ReagentCleaningStep.CleaningSpeed.s_60
        ElseIf RB_Cln100uLs.Checked Then
            stp.Speed = ReagentCleaningStep.CleaningSpeed.s_100
        ElseIf RB_Cln250uLs.Checked Then
            stp.Speed = ReagentCleaningStep.CleaningSpeed.s_250
        End If

        If RB_ClnSampleTube.Checked Then
            stp.Target = ReagentCleaningStep.CleaningTarget.t_Tube
        ElseIf RB_ClnFlowCell.Checked Then
            stp.Target = ReagentCleaningStep.CleaningTarget.t_Flow
        ElseIf RB_ClnBoth.Checked Then
            stp.Target = ReagentCleaningStep.CleaningTarget.t_Both
        End If

        stp.AirFlush = CB_AirFlush.Checked
        stp.BackFlush = CB_BackFlush.Checked

        If CB_ClnPrimary.Checked Then
            temp_reagent.PrimaryCleaningSteps.Add(stp)
        Else
            temp_reagent.SecondaryCleaningSteps.Add(stp)
        End If


        DisplayCleanStepsList()
        RecalculateCounts()
    End Sub


    Private Sub Btn_ClnStepUP_S_Click(sender As Object, e As EventArgs) Handles Btn_ClnStepUP_S.Click
        If LB_SecondaryCleaningSteps.SelectedIndex <> -1 Then
            temp_reagent.MoveSCleanStepUp(LB_SecondaryCleaningSteps.SelectedIndex)
            DisplayCleanStepsList()
        End If
    End Sub

    Private Sub Btn_ClnStepDN_S_Click(sender As Object, e As EventArgs) Handles Btn_ClnStepDN_S.Click
        If LB_SecondaryCleaningSteps.SelectedIndex <> -1 Then
            temp_reagent.MoveSCleanStepDown(LB_SecondaryCleaningSteps.SelectedIndex)
            DisplayCleanStepsList()
        End If
    End Sub

    Private Sub Btn_ClnStepDEL_S_Click(sender As Object, e As EventArgs) Handles Btn_ClnStepDEL_S.Click
        If LB_SecondaryCleaningSteps.SelectedIndex <> -1 Then
            temp_reagent.RemoveSCleanStep(LB_SecondaryCleaningSteps.SelectedIndex)
            DisplayCleanStepsList()
        End If

        RecalculateCounts()
    End Sub

    Private Sub BTN_PayloadFileVerificationSelect_Click(sender As Object, e As EventArgs) Handles BTN_PayloadFileVerificationSelect.Click
        Dim fd As OpenFileDialog = New OpenFileDialog



        fd.InitialDirectory = My.Computer.FileSystem.CurrentDirectory
        fd.DefaultExt = "payload"
        fd.Title = "Select PayloadFile To Verify"


        If fd.ShowDialog <> DialogResult.OK Then
            LBL_SelectedPayloadFile.Text = "NONE"
            LBL_PayloadFileStatus.Text = "NONE"
            LB_PayloadEntries.Items.Clear()
            Return
        End If

        LB_PayloadEntries.Items.Clear()

        Dim filename As String = fd.FileName

        LBL_SelectedPayloadFile.Text = filename

        Dim currentpath As String = System.IO.Directory.GetCurrentDirectory


        Dim ppath As String = System.IO.Path.GetDirectoryName(filename)
        Dim pfile As String = System.IO.Path.GetFileName(filename)

        Dim payload As New RFIDDataOutput

        System.IO.Directory.SetCurrentDirectory(ppath)

        If Not payload.LoadPayloadFile(pfile) Then
            LBL_PayloadFileStatus.Text = "INVALID"
        Else
            LBL_PayloadFileStatus.Text = "VALID"
            For Each x In payload.Entries
                ' add x to the listbox.
                LB_PayloadEntries.Items.Add(x.GetFilename + "  (" + ppath + ")")
            Next
        End If

        System.IO.Directory.SetCurrentDirectory(currentpath)

    End Sub

End Class
