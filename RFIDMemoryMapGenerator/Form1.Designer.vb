<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class Form1
    Inherits System.Windows.Forms.Form

    'Form overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()> _
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        Try
            If disposing AndAlso components IsNot Nothing Then
                components.Dispose()
            End If
        Finally
            MyBase.Dispose(disposing)
        End Try
    End Sub

    'Required by the Windows Form Designer
    Private components As System.ComponentModel.IContainer

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()> _
    Private Sub InitializeComponent()
        Me.Panel1 = New System.Windows.Forms.Panel()
        Me.Label35 = New System.Windows.Forms.Label()
        Me.Lbl_ContentBytesRemaining = New System.Windows.Forms.Label()
        Me.Lbl_NumAnalyses = New System.Windows.Forms.Label()
        Me.Btn_Reset = New System.Windows.Forms.Button()
        Me.Lbl_HasReagent = New System.Windows.Forms.Label()
        Me.Btn_Generate = New System.Windows.Forms.Button()
        Me.Lbl_NumCleaners = New System.Windows.Forms.Label()
        Me.Label1 = New System.Windows.Forms.Label()
        Me.Lbl_ContentBytes = New System.Windows.Forms.Label()
        Me.Label4 = New System.Windows.Forms.Label()
        Me.Label3 = New System.Windows.Forms.Label()
        Me.Label2 = New System.Windows.Forms.Label()
        Me.Panel2 = New System.Windows.Forms.Panel()
        Me.TabControl1 = New System.Windows.Forms.TabControl()
        Me.TP_Header = New System.Windows.Forms.TabPage()
        Me.Panel3 = New System.Windows.Forms.Panel()
        Me.GroupBox4 = New System.Windows.Forms.GroupBox()
        Me.GroupBox8 = New System.Windows.Forms.GroupBox()
        Me.Btn_ClnStepUP_S = New System.Windows.Forms.Button()
        Me.Btn_ClnStepDEL_S = New System.Windows.Forms.Button()
        Me.Btn_ClnStepDN_S = New System.Windows.Forms.Button()
        Me.LB_SecondaryCleaningSteps = New System.Windows.Forms.ListBox()
        Me.CB_ClnPrimary = New System.Windows.Forms.CheckBox()
        Me.Label33 = New System.Windows.Forms.Label()
        Me.CB_RgtCleanerList = New System.Windows.Forms.ComboBox()
        Me.Btn_ClnStepAdd = New System.Windows.Forms.Button()
        Me.GB_Volume = New System.Windows.Forms.GroupBox()
        Me.RB_Cln1200uL = New System.Windows.Forms.RadioButton()
        Me.RB_Cln600uL = New System.Windows.Forms.RadioButton()
        Me.Btn_ClnStepUP_P = New System.Windows.Forms.Button()
        Me.CB_AirFlush = New System.Windows.Forms.CheckBox()
        Me.Btn_ClnStepDEL_P = New System.Windows.Forms.Button()
        Me.Btn_ClnStepDN_P = New System.Windows.Forms.Button()
        Me.GB_Speed = New System.Windows.Forms.GroupBox()
        Me.RB_Cln250uLs = New System.Windows.Forms.RadioButton()
        Me.RB_Cln100uLs = New System.Windows.Forms.RadioButton()
        Me.RB_Cln60uLs = New System.Windows.Forms.RadioButton()
        Me.RB_Cln30uLs = New System.Windows.Forms.RadioButton()
        Me.CB_BackFlush = New System.Windows.Forms.CheckBox()
        Me.LB_PrimaryCleaningSteps = New System.Windows.Forms.ListBox()
        Me.GB_Target = New System.Windows.Forms.GroupBox()
        Me.RB_ClnBoth = New System.Windows.Forms.RadioButton()
        Me.RB_ClnFlowCell = New System.Windows.Forms.RadioButton()
        Me.RB_ClnSampleTube = New System.Windows.Forms.RadioButton()
        Me.LBL_PackReagentDesc = New System.Windows.Forms.Label()
        Me.LBL_PackReagentPN = New System.Windows.Forms.Label()
        Me.CB_PackReagent = New System.Windows.Forms.ComboBox()
        Me.Label26 = New System.Windows.Forms.Label()
        Me.Label25 = New System.Windows.Forms.Label()
        Me.CB_IncludeReagent = New System.Windows.Forms.CheckBox()
        Me.Label7 = New System.Windows.Forms.Label()
        Me.Label8 = New System.Windows.Forms.Label()
        Me.Label9 = New System.Windows.Forms.Label()
        Me.UD_ReagentCycles = New System.Windows.Forms.NumericUpDown()
        Me.UD_ReagentUses = New System.Windows.Forms.NumericUpDown()
        Me.Panel9 = New System.Windows.Forms.Panel()
        Me.GroupBox3 = New System.Windows.Forms.GroupBox()
        Me.Panel11 = New System.Windows.Forms.Panel()
        Me.LB_Cleaners = New System.Windows.Forms.ListBox()
        Me.Btn_CleanerUP = New System.Windows.Forms.Button()
        Me.Btn_CleanerDEL = New System.Windows.Forms.Button()
        Me.Btn_CleanerDN = New System.Windows.Forms.Button()
        Me.Panel12 = New System.Windows.Forms.Panel()
        Me.LBL_CleanerPN = New System.Windows.Forms.Label()
        Me.LBL_CleanerIndex = New System.Windows.Forms.Label()
        Me.LBL_CleanerDescription = New System.Windows.Forms.Label()
        Me.Label32 = New System.Windows.Forms.Label()
        Me.CB_CleanerList = New System.Windows.Forms.ComboBox()
        Me.Label14 = New System.Windows.Forms.Label()
        Me.Label6 = New System.Windows.Forms.Label()
        Me.UD_CleanerUses = New System.Windows.Forms.NumericUpDown()
        Me.Label5 = New System.Windows.Forms.Label()
        Me.Label24 = New System.Windows.Forms.Label()
        Me.Btn_AddCleaner = New System.Windows.Forms.Button()
        Me.Panel10 = New System.Windows.Forms.Panel()
        Me.GroupBox2 = New System.Windows.Forms.GroupBox()
        Me.CB_ContainerType = New System.Windows.Forms.ComboBox()
        Me.Label40 = New System.Windows.Forms.Label()
        Me.TB_GTIN = New System.Windows.Forms.TextBox()
        Me.Label34 = New System.Windows.Forms.Label()
        Me.lblPackLotNo = New System.Windows.Forms.Label()
        Me.lblPackExpirationDate = New System.Windows.Forms.Label()
        Me.udPackLotNumber = New System.Windows.Forms.NumericUpDown()
        Me.dtpPackExpiration = New System.Windows.Forms.DateTimePicker()
        Me.cbFullLayout = New System.Windows.Forms.CheckBox()
        Me.Label10 = New System.Windows.Forms.Label()
        Me.TB_PackPN = New System.Windows.Forms.TextBox()
        Me.Label11 = New System.Windows.Forms.Label()
        Me.UP_ServiceLife = New System.Windows.Forms.NumericUpDown()
        Me.Label12 = New System.Windows.Forms.Label()
        Me.TP_Analyses = New System.Windows.Forms.TabPage()
        Me.Panel8 = New System.Windows.Forms.Panel()
        Me.GroupBox7 = New System.Windows.Forms.GroupBox()
        Me.CB_AnalysisReagent = New System.Windows.Forms.ComboBox()
        Me.Btn_AnalysisReagentDN = New System.Windows.Forms.Button()
        Me.Btn_AnalysisReagentDEL = New System.Windows.Forms.Button()
        Me.Btn_AnalysisReagentUP = New System.Windows.Forms.Button()
        Me.Btn_AnalysisReagentAdd = New System.Windows.Forms.Button()
        Me.LB_AnalysisReagents = New System.Windows.Forms.ListBox()
        Me.Label15 = New System.Windows.Forms.Label()
        Me.GroupBox6 = New System.Windows.Forms.GroupBox()
        Me.Label41 = New System.Windows.Forms.Label()
        Me.UD_AnalysisEmissionWavelength = New System.Windows.Forms.NumericUpDown()
        Me.Label40 = New System.Windows.Forms.Label()
        Me.Btn_AnalysisIlluminatorAdd = New System.Windows.Forms.Button()
        Me.Btn_AnalysisIlluminatorDN = New System.Windows.Forms.Button()
        Me.Btn_AnalysisIlluminatorDEL = New System.Windows.Forms.Button()
        Me.Btn_AnalysisIlluminatorUP = New System.Windows.Forms.Button()
        Me.LB_AnalysisIlluminators = New System.Windows.Forms.ListBox()
        Me.Label20 = New System.Windows.Forms.Label()
        Me.Label18 = New System.Windows.Forms.Label()
        Me.UD_AnalysisIlluminatorMS = New System.Windows.Forms.NumericUpDown()
        Me.UD_AnalysisIlluminatorWL = New System.Windows.Forms.NumericUpDown()
        Me.Label17 = New System.Windows.Forms.Label()
        Me.GroupBox5 = New System.Windows.Forms.GroupBox()
        Me.Label31 = New System.Windows.Forms.Label()
        Me.CB_AnalysisParameter = New System.Windows.Forms.ComboBox()
        Me.LBL_AnalysisParameterKey = New System.Windows.Forms.Label()
        Me.UD_AnalysisParamThreshold = New System.Windows.Forms.NumericUpDown()
        Me.Btn_AnalysisParamDN = New System.Windows.Forms.Button()
        Me.Btn_AnalysisParamDEL = New System.Windows.Forms.Button()
        Me.Btn_AnalysisParamUP = New System.Windows.Forms.Button()
        Me.LB_AnalysisParams = New System.Windows.Forms.ListBox()
        Me.Btn_AddAnalysisParam = New System.Windows.Forms.Button()
        Me.RB_AnalysisParamBelow = New System.Windows.Forms.RadioButton()
        Me.RB_AnalysisParamAtAbove = New System.Windows.Forms.RadioButton()
        Me.UD_AnalysisParamSubkey = New System.Windows.Forms.NumericUpDown()
        Me.Label27 = New System.Windows.Forms.Label()
        Me.Label28 = New System.Windows.Forms.Label()
        Me.Label29 = New System.Windows.Forms.Label()
        Me.GroupBox1 = New System.Windows.Forms.GroupBox()
        Me.Lbl_AnalysisPopulationKey = New System.Windows.Forms.Label()
        Me.Label30 = New System.Windows.Forms.Label()
        Me.CB_AnalysisPopParameter = New System.Windows.Forms.ComboBox()
        Me.UD_AnalysisPopThreshold = New System.Windows.Forms.NumericUpDown()
        Me.Btn_PopulationClear = New System.Windows.Forms.Button()
        Me.RB_PopBelow = New System.Windows.Forms.RadioButton()
        Me.RB_PopAtAbove = New System.Windows.Forms.RadioButton()
        Me.UD_AnalysisPopSubkey = New System.Windows.Forms.NumericUpDown()
        Me.Label23 = New System.Windows.Forms.Label()
        Me.Label22 = New System.Windows.Forms.Label()
        Me.Label21 = New System.Windows.Forms.Label()
        Me.TB_AnalysisDesc = New System.Windows.Forms.TextBox()
        Me.Label16 = New System.Windows.Forms.Label()
        Me.UD_AnalysisIndex = New System.Windows.Forms.NumericUpDown()
        Me.Label19 = New System.Windows.Forms.Label()
        Me.Btn_AddAnalysis = New System.Windows.Forms.Button()
        Me.Panel7 = New System.Windows.Forms.Panel()
        Me.LB_Analyses = New System.Windows.Forms.ListBox()
        Me.Btn_AnalysisDN = New System.Windows.Forms.Button()
        Me.Btn_AnalysisDEL = New System.Windows.Forms.Button()
        Me.Btn_AnalysisUP = New System.Windows.Forms.Button()
        Me.TP_ContentManagement = New System.Windows.Forms.TabPage()
        Me.LBL_PayloadFileStatus = New System.Windows.Forms.Label()
        Me.Label37 = New System.Windows.Forms.Label()
        Me.BTN_PayloadFileVerificationSelect = New System.Windows.Forms.Button()
        Me.LBL_SelectedPayloadFile = New System.Windows.Forms.Label()
        Me.Label36 = New System.Windows.Forms.Label()
        Me.Label39 = New System.Windows.Forms.Label()
        Me.LB_PayloadEntries = New System.Windows.Forms.ListBox()
        Me.Label13 = New System.Windows.Forms.Label()
        Me.LBL_version = New System.Windows.Forms.Label()
        Me.Panel1.SuspendLayout()
        Me.Panel2.SuspendLayout()
        Me.TabControl1.SuspendLayout()
        Me.TP_Header.SuspendLayout()
        Me.Panel3.SuspendLayout()
        Me.GroupBox4.SuspendLayout()
        Me.GroupBox8.SuspendLayout()
        Me.GB_Volume.SuspendLayout()
        Me.GB_Speed.SuspendLayout()
        Me.GB_Target.SuspendLayout()
        CType(Me.UD_ReagentCycles, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.UD_ReagentUses, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.Panel9.SuspendLayout()
        Me.GroupBox3.SuspendLayout()
        Me.Panel11.SuspendLayout()
        Me.Panel12.SuspendLayout()
        CType(Me.UD_CleanerUses, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.Panel10.SuspendLayout()
        Me.GroupBox2.SuspendLayout()
        CType(Me.udPackLotNumber, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.UP_ServiceLife, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.TP_Analyses.SuspendLayout()
        Me.Panel8.SuspendLayout()
        Me.GroupBox7.SuspendLayout()
        Me.GroupBox6.SuspendLayout()
        CType(Me.UD_AnalysisEmissionWavelength, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.UD_AnalysisIlluminatorMS, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.UD_AnalysisIlluminatorWL, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.GroupBox5.SuspendLayout()
        CType(Me.UD_AnalysisParamThreshold, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.UD_AnalysisParamSubkey, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.GroupBox1.SuspendLayout()
        CType(Me.UD_AnalysisPopThreshold, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.UD_AnalysisPopSubkey, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.UD_AnalysisIndex, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.Panel7.SuspendLayout()
        Me.TP_ContentManagement.SuspendLayout()
        Me.SuspendLayout()
        '
        'Panel1
        '
        Me.Panel1.Controls.Add(Me.LBL_version)
        Me.Panel1.Controls.Add(Me.Label35)
        Me.Panel1.Controls.Add(Me.Lbl_ContentBytesRemaining)
        Me.Panel1.Controls.Add(Me.Lbl_NumAnalyses)
        Me.Panel1.Controls.Add(Me.Btn_Reset)
        Me.Panel1.Controls.Add(Me.Lbl_HasReagent)
        Me.Panel1.Controls.Add(Me.Btn_Generate)
        Me.Panel1.Controls.Add(Me.Lbl_NumCleaners)
        Me.Panel1.Controls.Add(Me.Label1)
        Me.Panel1.Controls.Add(Me.Lbl_ContentBytes)
        Me.Panel1.Controls.Add(Me.Label4)
        Me.Panel1.Controls.Add(Me.Label3)
        Me.Panel1.Controls.Add(Me.Label2)
        Me.Panel1.Dock = System.Windows.Forms.DockStyle.Top
        Me.Panel1.Location = New System.Drawing.Point(0, 0)
        Me.Panel1.Name = "Panel1"
        Me.Panel1.Size = New System.Drawing.Size(864, 91)
        Me.Panel1.TabIndex = 0
        '
        'Label35
        '
        Me.Label35.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label35.Location = New System.Drawing.Point(577, 36)
        Me.Label35.Name = "Label35"
        Me.Label35.Size = New System.Drawing.Size(75, 23)
        Me.Label35.TabIndex = 11
        Me.Label35.Text = "Remaining:"
        '
        'Lbl_ContentBytesRemaining
        '
        Me.Lbl_ContentBytesRemaining.AutoSize = True
        Me.Lbl_ContentBytesRemaining.Location = New System.Drawing.Point(648, 36)
        Me.Lbl_ContentBytesRemaining.Name = "Lbl_ContentBytesRemaining"
        Me.Lbl_ContentBytesRemaining.Size = New System.Drawing.Size(50, 13)
        Me.Lbl_ContentBytesRemaining.TabIndex = 10
        Me.Lbl_ContentBytesRemaining.Text = "NBYTES"
        '
        'Lbl_NumAnalyses
        '
        Me.Lbl_NumAnalyses.AutoSize = True
        Me.Lbl_NumAnalyses.Location = New System.Drawing.Point(445, 17)
        Me.Lbl_NumAnalyses.Name = "Lbl_NumAnalyses"
        Me.Lbl_NumAnalyses.Size = New System.Drawing.Size(67, 13)
        Me.Lbl_NumAnalyses.TabIndex = 5
        Me.Lbl_NumAnalyses.Text = "NANALYSIS"
        '
        'Btn_Reset
        '
        Me.Btn_Reset.Location = New System.Drawing.Point(777, 36)
        Me.Btn_Reset.Name = "Btn_Reset"
        Me.Btn_Reset.Size = New System.Drawing.Size(75, 23)
        Me.Btn_Reset.TabIndex = 9
        Me.Btn_Reset.Text = "Reset"
        Me.Btn_Reset.UseVisualStyleBackColor = True
        '
        'Lbl_HasReagent
        '
        Me.Lbl_HasReagent.AutoSize = True
        Me.Lbl_HasReagent.Location = New System.Drawing.Point(267, 17)
        Me.Lbl_HasReagent.Name = "Lbl_HasReagent"
        Me.Lbl_HasReagent.Size = New System.Drawing.Size(39, 13)
        Me.Lbl_HasReagent.TabIndex = 3
        Me.Lbl_HasReagent.Text = "YesNo"
        '
        'Btn_Generate
        '
        Me.Btn_Generate.Location = New System.Drawing.Point(777, 7)
        Me.Btn_Generate.Name = "Btn_Generate"
        Me.Btn_Generate.Size = New System.Drawing.Size(75, 23)
        Me.Btn_Generate.TabIndex = 8
        Me.Btn_Generate.Text = "Generate..."
        Me.Btn_Generate.UseVisualStyleBackColor = True
        '
        'Lbl_NumCleaners
        '
        Me.Lbl_NumCleaners.AutoSize = True
        Me.Lbl_NumCleaners.Location = New System.Drawing.Point(103, 17)
        Me.Lbl_NumCleaners.Name = "Lbl_NumCleaners"
        Me.Lbl_NumCleaners.Size = New System.Drawing.Size(50, 13)
        Me.Lbl_NumCleaners.TabIndex = 1
        Me.Lbl_NumCleaners.Text = "NCLEAN"
        '
        'Label1
        '
        Me.Label1.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label1.Location = New System.Drawing.Point(12, 12)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(85, 23)
        Me.Label1.TabIndex = 0
        Me.Label1.Text = "Cleaners:"
        '
        'Lbl_ContentBytes
        '
        Me.Lbl_ContentBytes.AutoSize = True
        Me.Lbl_ContentBytes.Location = New System.Drawing.Point(648, 17)
        Me.Lbl_ContentBytes.Name = "Lbl_ContentBytes"
        Me.Lbl_ContentBytes.Size = New System.Drawing.Size(50, 13)
        Me.Lbl_ContentBytes.TabIndex = 7
        Me.Lbl_ContentBytes.Text = "NBYTES"
        '
        'Label4
        '
        Me.Label4.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label4.Location = New System.Drawing.Point(587, 12)
        Me.Label4.Name = "Label4"
        Me.Label4.Size = New System.Drawing.Size(65, 23)
        Me.Label4.TabIndex = 6
        Me.Label4.Text = "Bytes:"
        '
        'Label3
        '
        Me.Label3.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label3.Location = New System.Drawing.Point(356, 12)
        Me.Label3.Name = "Label3"
        Me.Label3.Size = New System.Drawing.Size(100, 23)
        Me.Label3.TabIndex = 4
        Me.Label3.Text = "Analyses:"
        '
        'Label2
        '
        Me.Label2.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label2.Location = New System.Drawing.Point(186, 12)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(90, 23)
        Me.Label2.TabIndex = 2
        Me.Label2.Text = "Reagent:"
        '
        'Panel2
        '
        Me.Panel2.Controls.Add(Me.TabControl1)
        Me.Panel2.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Panel2.Location = New System.Drawing.Point(0, 91)
        Me.Panel2.Name = "Panel2"
        Me.Panel2.Size = New System.Drawing.Size(864, 715)
        Me.Panel2.TabIndex = 1
        '
        'TabControl1
        '
        Me.TabControl1.Controls.Add(Me.TP_Header)
        Me.TabControl1.Controls.Add(Me.TP_Analyses)
        Me.TabControl1.Controls.Add(Me.TP_ContentManagement)
        Me.TabControl1.Dock = System.Windows.Forms.DockStyle.Fill
        Me.TabControl1.Location = New System.Drawing.Point(0, 0)
        Me.TabControl1.Name = "TabControl1"
        Me.TabControl1.SelectedIndex = 0
        Me.TabControl1.Size = New System.Drawing.Size(864, 715)
        Me.TabControl1.TabIndex = 0
        '
        'TP_Header
        '
        Me.TP_Header.Controls.Add(Me.Panel3)
        Me.TP_Header.Controls.Add(Me.Panel9)
        Me.TP_Header.Controls.Add(Me.Panel10)
        Me.TP_Header.Location = New System.Drawing.Point(4, 22)
        Me.TP_Header.Name = "TP_Header"
        Me.TP_Header.Padding = New System.Windows.Forms.Padding(3)
        Me.TP_Header.Size = New System.Drawing.Size(856, 689)
        Me.TP_Header.TabIndex = 0
        Me.TP_Header.Text = "Header"
        Me.TP_Header.UseVisualStyleBackColor = True
        '
        'Panel3
        '
        Me.Panel3.Controls.Add(Me.GroupBox4)
        Me.Panel3.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Panel3.Location = New System.Drawing.Point(3, 334)
        Me.Panel3.Name = "Panel3"
        Me.Panel3.Size = New System.Drawing.Size(850, 352)
        Me.Panel3.TabIndex = 8
        '
        'GroupBox4
        '
        Me.GroupBox4.Controls.Add(Me.GroupBox8)
        Me.GroupBox4.Controls.Add(Me.LBL_PackReagentDesc)
        Me.GroupBox4.Controls.Add(Me.LBL_PackReagentPN)
        Me.GroupBox4.Controls.Add(Me.CB_PackReagent)
        Me.GroupBox4.Controls.Add(Me.Label26)
        Me.GroupBox4.Controls.Add(Me.Label25)
        Me.GroupBox4.Controls.Add(Me.CB_IncludeReagent)
        Me.GroupBox4.Controls.Add(Me.Label7)
        Me.GroupBox4.Controls.Add(Me.Label8)
        Me.GroupBox4.Controls.Add(Me.Label9)
        Me.GroupBox4.Controls.Add(Me.UD_ReagentCycles)
        Me.GroupBox4.Controls.Add(Me.UD_ReagentUses)
        Me.GroupBox4.Dock = System.Windows.Forms.DockStyle.Fill
        Me.GroupBox4.Location = New System.Drawing.Point(0, 0)
        Me.GroupBox4.Name = "GroupBox4"
        Me.GroupBox4.Size = New System.Drawing.Size(850, 352)
        Me.GroupBox4.TabIndex = 0
        Me.GroupBox4.TabStop = False
        Me.GroupBox4.Text = "Reagent"
        '
        'GroupBox8
        '
        Me.GroupBox8.Controls.Add(Me.Btn_ClnStepUP_S)
        Me.GroupBox8.Controls.Add(Me.Btn_ClnStepDEL_S)
        Me.GroupBox8.Controls.Add(Me.Btn_ClnStepDN_S)
        Me.GroupBox8.Controls.Add(Me.LB_SecondaryCleaningSteps)
        Me.GroupBox8.Controls.Add(Me.CB_ClnPrimary)
        Me.GroupBox8.Controls.Add(Me.Label33)
        Me.GroupBox8.Controls.Add(Me.CB_RgtCleanerList)
        Me.GroupBox8.Controls.Add(Me.Btn_ClnStepAdd)
        Me.GroupBox8.Controls.Add(Me.GB_Volume)
        Me.GroupBox8.Controls.Add(Me.Btn_ClnStepUP_P)
        Me.GroupBox8.Controls.Add(Me.CB_AirFlush)
        Me.GroupBox8.Controls.Add(Me.Btn_ClnStepDEL_P)
        Me.GroupBox8.Controls.Add(Me.Btn_ClnStepDN_P)
        Me.GroupBox8.Controls.Add(Me.GB_Speed)
        Me.GroupBox8.Controls.Add(Me.CB_BackFlush)
        Me.GroupBox8.Controls.Add(Me.LB_PrimaryCleaningSteps)
        Me.GroupBox8.Controls.Add(Me.GB_Target)
        Me.GroupBox8.Location = New System.Drawing.Point(305, 21)
        Me.GroupBox8.Name = "GroupBox8"
        Me.GroupBox8.Size = New System.Drawing.Size(527, 331)
        Me.GroupBox8.TabIndex = 23
        Me.GroupBox8.TabStop = False
        Me.GroupBox8.Text = "Cleaning Sequence"
        '
        'Btn_ClnStepUP_S
        '
        Me.Btn_ClnStepUP_S.Location = New System.Drawing.Point(446, 241)
        Me.Btn_ClnStepUP_S.Name = "Btn_ClnStepUP_S"
        Me.Btn_ClnStepUP_S.Size = New System.Drawing.Size(75, 23)
        Me.Btn_ClnStepUP_S.TabIndex = 28
        Me.Btn_ClnStepUP_S.Text = "Move Up"
        Me.Btn_ClnStepUP_S.UseVisualStyleBackColor = True
        '
        'Btn_ClnStepDEL_S
        '
        Me.Btn_ClnStepDEL_S.Location = New System.Drawing.Point(446, 270)
        Me.Btn_ClnStepDEL_S.Name = "Btn_ClnStepDEL_S"
        Me.Btn_ClnStepDEL_S.Size = New System.Drawing.Size(75, 23)
        Me.Btn_ClnStepDEL_S.TabIndex = 29
        Me.Btn_ClnStepDEL_S.Text = "Remove"
        Me.Btn_ClnStepDEL_S.UseVisualStyleBackColor = True
        '
        'Btn_ClnStepDN_S
        '
        Me.Btn_ClnStepDN_S.Location = New System.Drawing.Point(446, 299)
        Me.Btn_ClnStepDN_S.Name = "Btn_ClnStepDN_S"
        Me.Btn_ClnStepDN_S.Size = New System.Drawing.Size(75, 23)
        Me.Btn_ClnStepDN_S.TabIndex = 30
        Me.Btn_ClnStepDN_S.Text = "Move Down"
        Me.Btn_ClnStepDN_S.UseVisualStyleBackColor = True
        '
        'LB_SecondaryCleaningSteps
        '
        Me.LB_SecondaryCleaningSteps.FormattingEnabled = True
        Me.LB_SecondaryCleaningSteps.Location = New System.Drawing.Point(6, 236)
        Me.LB_SecondaryCleaningSteps.Name = "LB_SecondaryCleaningSteps"
        Me.LB_SecondaryCleaningSteps.Size = New System.Drawing.Size(434, 95)
        Me.LB_SecondaryCleaningSteps.TabIndex = 27
        '
        'CB_ClnPrimary
        '
        Me.CB_ClnPrimary.AutoSize = True
        Me.CB_ClnPrimary.Checked = True
        Me.CB_ClnPrimary.CheckState = System.Windows.Forms.CheckState.Checked
        Me.CB_ClnPrimary.Location = New System.Drawing.Point(6, 32)
        Me.CB_ClnPrimary.Name = "CB_ClnPrimary"
        Me.CB_ClnPrimary.Size = New System.Drawing.Size(112, 17)
        Me.CB_ClnPrimary.TabIndex = 26
        Me.CB_ClnPrimary.Text = "Primary Sequence"
        Me.CB_ClnPrimary.UseVisualStyleBackColor = True
        '
        'Label33
        '
        Me.Label33.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label33.Location = New System.Drawing.Point(6, 58)
        Me.Label33.Name = "Label33"
        Me.Label33.Size = New System.Drawing.Size(73, 23)
        Me.Label33.TabIndex = 25
        Me.Label33.Text = "Cleaner"
        '
        'CB_RgtCleanerList
        '
        Me.CB_RgtCleanerList.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.CB_RgtCleanerList.FormattingEnabled = True
        Me.CB_RgtCleanerList.Location = New System.Drawing.Point(6, 84)
        Me.CB_RgtCleanerList.Name = "CB_RgtCleanerList"
        Me.CB_RgtCleanerList.Size = New System.Drawing.Size(121, 21)
        Me.CB_RgtCleanerList.TabIndex = 24
        '
        'Btn_ClnStepAdd
        '
        Me.Btn_ClnStepAdd.Location = New System.Drawing.Point(446, 85)
        Me.Btn_ClnStepAdd.Name = "Btn_ClnStepAdd"
        Me.Btn_ClnStepAdd.Size = New System.Drawing.Size(75, 23)
        Me.Btn_ClnStepAdd.TabIndex = 23
        Me.Btn_ClnStepAdd.Text = "Add"
        Me.Btn_ClnStepAdd.UseVisualStyleBackColor = True
        '
        'GB_Volume
        '
        Me.GB_Volume.Controls.Add(Me.RB_Cln1200uL)
        Me.GB_Volume.Controls.Add(Me.RB_Cln600uL)
        Me.GB_Volume.Location = New System.Drawing.Point(138, 19)
        Me.GB_Volume.Name = "GB_Volume"
        Me.GB_Volume.Size = New System.Drawing.Size(77, 116)
        Me.GB_Volume.TabIndex = 18
        Me.GB_Volume.TabStop = False
        Me.GB_Volume.Text = "Volume (uL)"
        '
        'RB_Cln1200uL
        '
        Me.RB_Cln1200uL.AutoSize = True
        Me.RB_Cln1200uL.Location = New System.Drawing.Point(6, 42)
        Me.RB_Cln1200uL.Name = "RB_Cln1200uL"
        Me.RB_Cln1200uL.Size = New System.Drawing.Size(49, 17)
        Me.RB_Cln1200uL.TabIndex = 1
        Me.RB_Cln1200uL.TabStop = True
        Me.RB_Cln1200uL.Text = "1200"
        Me.RB_Cln1200uL.UseVisualStyleBackColor = True
        '
        'RB_Cln600uL
        '
        Me.RB_Cln600uL.AutoSize = True
        Me.RB_Cln600uL.Location = New System.Drawing.Point(6, 19)
        Me.RB_Cln600uL.Name = "RB_Cln600uL"
        Me.RB_Cln600uL.Size = New System.Drawing.Size(43, 17)
        Me.RB_Cln600uL.TabIndex = 0
        Me.RB_Cln600uL.TabStop = True
        Me.RB_Cln600uL.Text = "600"
        Me.RB_Cln600uL.UseVisualStyleBackColor = True
        '
        'Btn_ClnStepUP_P
        '
        Me.Btn_ClnStepUP_P.Location = New System.Drawing.Point(446, 146)
        Me.Btn_ClnStepUP_P.Name = "Btn_ClnStepUP_P"
        Me.Btn_ClnStepUP_P.Size = New System.Drawing.Size(75, 23)
        Me.Btn_ClnStepUP_P.TabIndex = 15
        Me.Btn_ClnStepUP_P.Text = "Move Up"
        Me.Btn_ClnStepUP_P.UseVisualStyleBackColor = True
        '
        'CB_AirFlush
        '
        Me.CB_AirFlush.AutoSize = True
        Me.CB_AirFlush.Location = New System.Drawing.Point(445, 62)
        Me.CB_AirFlush.Name = "CB_AirFlush"
        Me.CB_AirFlush.Size = New System.Drawing.Size(63, 17)
        Me.CB_AirFlush.TabIndex = 22
        Me.CB_AirFlush.Text = "Air flush"
        Me.CB_AirFlush.UseVisualStyleBackColor = True
        '
        'Btn_ClnStepDEL_P
        '
        Me.Btn_ClnStepDEL_P.Location = New System.Drawing.Point(446, 175)
        Me.Btn_ClnStepDEL_P.Name = "Btn_ClnStepDEL_P"
        Me.Btn_ClnStepDEL_P.Size = New System.Drawing.Size(75, 23)
        Me.Btn_ClnStepDEL_P.TabIndex = 16
        Me.Btn_ClnStepDEL_P.Text = "Remove"
        Me.Btn_ClnStepDEL_P.UseVisualStyleBackColor = True
        '
        'Btn_ClnStepDN_P
        '
        Me.Btn_ClnStepDN_P.Location = New System.Drawing.Point(446, 204)
        Me.Btn_ClnStepDN_P.Name = "Btn_ClnStepDN_P"
        Me.Btn_ClnStepDN_P.Size = New System.Drawing.Size(75, 23)
        Me.Btn_ClnStepDN_P.TabIndex = 17
        Me.Btn_ClnStepDN_P.Text = "Move Down"
        Me.Btn_ClnStepDN_P.UseVisualStyleBackColor = True
        '
        'GB_Speed
        '
        Me.GB_Speed.Controls.Add(Me.RB_Cln250uLs)
        Me.GB_Speed.Controls.Add(Me.RB_Cln100uLs)
        Me.GB_Speed.Controls.Add(Me.RB_Cln60uLs)
        Me.GB_Speed.Controls.Add(Me.RB_Cln30uLs)
        Me.GB_Speed.Location = New System.Drawing.Point(221, 19)
        Me.GB_Speed.Name = "GB_Speed"
        Me.GB_Speed.Size = New System.Drawing.Size(84, 116)
        Me.GB_Speed.TabIndex = 19
        Me.GB_Speed.TabStop = False
        Me.GB_Speed.Text = "Speed (uL/s)"
        '
        'RB_Cln250uLs
        '
        Me.RB_Cln250uLs.AutoSize = True
        Me.RB_Cln250uLs.Location = New System.Drawing.Point(6, 88)
        Me.RB_Cln250uLs.Name = "RB_Cln250uLs"
        Me.RB_Cln250uLs.Size = New System.Drawing.Size(43, 17)
        Me.RB_Cln250uLs.TabIndex = 3
        Me.RB_Cln250uLs.TabStop = True
        Me.RB_Cln250uLs.Text = "250"
        Me.RB_Cln250uLs.UseVisualStyleBackColor = True
        '
        'RB_Cln100uLs
        '
        Me.RB_Cln100uLs.AutoSize = True
        Me.RB_Cln100uLs.Location = New System.Drawing.Point(6, 65)
        Me.RB_Cln100uLs.Name = "RB_Cln100uLs"
        Me.RB_Cln100uLs.Size = New System.Drawing.Size(43, 17)
        Me.RB_Cln100uLs.TabIndex = 2
        Me.RB_Cln100uLs.TabStop = True
        Me.RB_Cln100uLs.Text = "100"
        Me.RB_Cln100uLs.UseVisualStyleBackColor = True
        '
        'RB_Cln60uLs
        '
        Me.RB_Cln60uLs.AutoSize = True
        Me.RB_Cln60uLs.Location = New System.Drawing.Point(6, 42)
        Me.RB_Cln60uLs.Name = "RB_Cln60uLs"
        Me.RB_Cln60uLs.Size = New System.Drawing.Size(37, 17)
        Me.RB_Cln60uLs.TabIndex = 1
        Me.RB_Cln60uLs.TabStop = True
        Me.RB_Cln60uLs.Text = "60"
        Me.RB_Cln60uLs.UseVisualStyleBackColor = True
        '
        'RB_Cln30uLs
        '
        Me.RB_Cln30uLs.AutoSize = True
        Me.RB_Cln30uLs.Location = New System.Drawing.Point(6, 19)
        Me.RB_Cln30uLs.Name = "RB_Cln30uLs"
        Me.RB_Cln30uLs.Size = New System.Drawing.Size(37, 17)
        Me.RB_Cln30uLs.TabIndex = 0
        Me.RB_Cln30uLs.TabStop = True
        Me.RB_Cln30uLs.Text = "30"
        Me.RB_Cln30uLs.UseVisualStyleBackColor = True
        '
        'CB_BackFlush
        '
        Me.CB_BackFlush.AutoSize = True
        Me.CB_BackFlush.Location = New System.Drawing.Point(445, 39)
        Me.CB_BackFlush.Name = "CB_BackFlush"
        Me.CB_BackFlush.Size = New System.Drawing.Size(76, 17)
        Me.CB_BackFlush.TabIndex = 21
        Me.CB_BackFlush.Text = "Back flush"
        Me.CB_BackFlush.UseVisualStyleBackColor = True
        '
        'LB_PrimaryCleaningSteps
        '
        Me.LB_PrimaryCleaningSteps.FormattingEnabled = True
        Me.LB_PrimaryCleaningSteps.Location = New System.Drawing.Point(6, 141)
        Me.LB_PrimaryCleaningSteps.Name = "LB_PrimaryCleaningSteps"
        Me.LB_PrimaryCleaningSteps.Size = New System.Drawing.Size(434, 95)
        Me.LB_PrimaryCleaningSteps.TabIndex = 14
        '
        'GB_Target
        '
        Me.GB_Target.Controls.Add(Me.RB_ClnBoth)
        Me.GB_Target.Controls.Add(Me.RB_ClnFlowCell)
        Me.GB_Target.Controls.Add(Me.RB_ClnSampleTube)
        Me.GB_Target.Location = New System.Drawing.Point(311, 19)
        Me.GB_Target.Name = "GB_Target"
        Me.GB_Target.Size = New System.Drawing.Size(128, 116)
        Me.GB_Target.TabIndex = 20
        Me.GB_Target.TabStop = False
        Me.GB_Target.Text = "Target"
        '
        'RB_ClnBoth
        '
        Me.RB_ClnBoth.AutoSize = True
        Me.RB_ClnBoth.Location = New System.Drawing.Point(6, 65)
        Me.RB_ClnBoth.Name = "RB_ClnBoth"
        Me.RB_ClnBoth.Size = New System.Drawing.Size(47, 17)
        Me.RB_ClnBoth.TabIndex = 2
        Me.RB_ClnBoth.TabStop = True
        Me.RB_ClnBoth.Text = "Both"
        Me.RB_ClnBoth.UseVisualStyleBackColor = True
        '
        'RB_ClnFlowCell
        '
        Me.RB_ClnFlowCell.AutoSize = True
        Me.RB_ClnFlowCell.Location = New System.Drawing.Point(6, 42)
        Me.RB_ClnFlowCell.Name = "RB_ClnFlowCell"
        Me.RB_ClnFlowCell.Size = New System.Drawing.Size(67, 17)
        Me.RB_ClnFlowCell.TabIndex = 1
        Me.RB_ClnFlowCell.TabStop = True
        Me.RB_ClnFlowCell.Text = "Flow Cell"
        Me.RB_ClnFlowCell.UseVisualStyleBackColor = True
        '
        'RB_ClnSampleTube
        '
        Me.RB_ClnSampleTube.AutoSize = True
        Me.RB_ClnSampleTube.Location = New System.Drawing.Point(6, 19)
        Me.RB_ClnSampleTube.Name = "RB_ClnSampleTube"
        Me.RB_ClnSampleTube.Size = New System.Drawing.Size(85, 17)
        Me.RB_ClnSampleTube.TabIndex = 0
        Me.RB_ClnSampleTube.TabStop = True
        Me.RB_ClnSampleTube.Text = "SampleTube"
        Me.RB_ClnSampleTube.UseVisualStyleBackColor = True
        '
        'LBL_PackReagentDesc
        '
        Me.LBL_PackReagentDesc.AutoSize = True
        Me.LBL_PackReagentDesc.Location = New System.Drawing.Point(146, 105)
        Me.LBL_PackReagentDesc.Name = "LBL_PackReagentDesc"
        Me.LBL_PackReagentDesc.Size = New System.Drawing.Size(76, 13)
        Me.LBL_PackReagentDesc.TabIndex = 13
        Me.LBL_PackReagentDesc.Text = "Reagent Desc"
        '
        'LBL_PackReagentPN
        '
        Me.LBL_PackReagentPN.AutoSize = True
        Me.LBL_PackReagentPN.Location = New System.Drawing.Point(146, 84)
        Me.LBL_PackReagentPN.Name = "LBL_PackReagentPN"
        Me.LBL_PackReagentPN.Size = New System.Drawing.Size(66, 13)
        Me.LBL_PackReagentPN.TabIndex = 12
        Me.LBL_PackReagentPN.Text = "Reagent PN"
        '
        'CB_PackReagent
        '
        Me.CB_PackReagent.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.CB_PackReagent.FormattingEnabled = True
        Me.CB_PackReagent.Location = New System.Drawing.Point(100, 49)
        Me.CB_PackReagent.Name = "CB_PackReagent"
        Me.CB_PackReagent.Size = New System.Drawing.Size(182, 21)
        Me.CB_PackReagent.TabIndex = 11
        '
        'Label26
        '
        Me.Label26.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label26.Location = New System.Drawing.Point(25, 49)
        Me.Label26.Name = "Label26"
        Me.Label26.Size = New System.Drawing.Size(89, 23)
        Me.Label26.TabIndex = 1
        Me.Label26.Text = "Reagent"
        '
        'Label25
        '
        Me.Label25.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label25.Location = New System.Drawing.Point(25, 146)
        Me.Label25.Name = "Label25"
        Me.Label25.Size = New System.Drawing.Size(124, 23)
        Me.Label25.TabIndex = 9
        Me.Label25.Text = "Total Uses"
        '
        'CB_IncludeReagent
        '
        Me.CB_IncludeReagent.AutoSize = True
        Me.CB_IncludeReagent.Location = New System.Drawing.Point(6, 21)
        Me.CB_IncludeReagent.Name = "CB_IncludeReagent"
        Me.CB_IncludeReagent.Size = New System.Drawing.Size(67, 17)
        Me.CB_IncludeReagent.TabIndex = 0
        Me.CB_IncludeReagent.Text = "Include?"
        Me.CB_IncludeReagent.UseVisualStyleBackColor = True
        '
        'Label7
        '
        Me.Label7.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label7.Location = New System.Drawing.Point(25, 79)
        Me.Label7.Name = "Label7"
        Me.Label7.Size = New System.Drawing.Size(111, 21)
        Me.Label7.TabIndex = 3
        Me.Label7.Text = "Part Number"
        '
        'Label8
        '
        Me.Label8.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label8.Location = New System.Drawing.Point(25, 100)
        Me.Label8.Name = "Label8"
        Me.Label8.Size = New System.Drawing.Size(108, 21)
        Me.Label8.TabIndex = 5
        Me.Label8.Text = "Description"
        '
        'Label9
        '
        Me.Label9.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label9.Location = New System.Drawing.Point(25, 124)
        Me.Label9.Name = "Label9"
        Me.Label9.Size = New System.Drawing.Size(121, 23)
        Me.Label9.TabIndex = 7
        Me.Label9.Text = "Mixing Cycles"
        '
        'UD_ReagentCycles
        '
        Me.UD_ReagentCycles.Location = New System.Drawing.Point(149, 124)
        Me.UD_ReagentCycles.Maximum = New Decimal(New Integer() {255, 0, 0, 0})
        Me.UD_ReagentCycles.Name = "UD_ReagentCycles"
        Me.UD_ReagentCycles.Size = New System.Drawing.Size(73, 20)
        Me.UD_ReagentCycles.TabIndex = 8
        '
        'UD_ReagentUses
        '
        Me.UD_ReagentUses.Location = New System.Drawing.Point(149, 149)
        Me.UD_ReagentUses.Maximum = New Decimal(New Integer() {32767, 0, 0, 0})
        Me.UD_ReagentUses.Name = "UD_ReagentUses"
        Me.UD_ReagentUses.Size = New System.Drawing.Size(73, 20)
        Me.UD_ReagentUses.TabIndex = 10
        '
        'Panel9
        '
        Me.Panel9.Controls.Add(Me.GroupBox3)
        Me.Panel9.Dock = System.Windows.Forms.DockStyle.Top
        Me.Panel9.Location = New System.Drawing.Point(3, 117)
        Me.Panel9.Name = "Panel9"
        Me.Panel9.Size = New System.Drawing.Size(850, 217)
        Me.Panel9.TabIndex = 9
        '
        'GroupBox3
        '
        Me.GroupBox3.Controls.Add(Me.Panel11)
        Me.GroupBox3.Controls.Add(Me.Panel12)
        Me.GroupBox3.Dock = System.Windows.Forms.DockStyle.Fill
        Me.GroupBox3.Location = New System.Drawing.Point(0, 0)
        Me.GroupBox3.Name = "GroupBox3"
        Me.GroupBox3.Size = New System.Drawing.Size(850, 217)
        Me.GroupBox3.TabIndex = 0
        Me.GroupBox3.TabStop = False
        Me.GroupBox3.Text = "Cleaners"
        '
        'Panel11
        '
        Me.Panel11.Controls.Add(Me.LB_Cleaners)
        Me.Panel11.Controls.Add(Me.Btn_CleanerUP)
        Me.Panel11.Controls.Add(Me.Btn_CleanerDEL)
        Me.Panel11.Controls.Add(Me.Btn_CleanerDN)
        Me.Panel11.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Panel11.Location = New System.Drawing.Point(3, 111)
        Me.Panel11.Name = "Panel11"
        Me.Panel11.Size = New System.Drawing.Size(844, 103)
        Me.Panel11.TabIndex = 0
        '
        'LB_Cleaners
        '
        Me.LB_Cleaners.Dock = System.Windows.Forms.DockStyle.Left
        Me.LB_Cleaners.FormattingEnabled = True
        Me.LB_Cleaners.Location = New System.Drawing.Point(0, 0)
        Me.LB_Cleaners.Name = "LB_Cleaners"
        Me.LB_Cleaners.Size = New System.Drawing.Size(591, 103)
        Me.LB_Cleaners.TabIndex = 0
        '
        'Btn_CleanerUP
        '
        Me.Btn_CleanerUP.Location = New System.Drawing.Point(626, 5)
        Me.Btn_CleanerUP.Name = "Btn_CleanerUP"
        Me.Btn_CleanerUP.Size = New System.Drawing.Size(75, 23)
        Me.Btn_CleanerUP.TabIndex = 1
        Me.Btn_CleanerUP.Text = "Move Up"
        Me.Btn_CleanerUP.UseVisualStyleBackColor = True
        '
        'Btn_CleanerDEL
        '
        Me.Btn_CleanerDEL.Location = New System.Drawing.Point(626, 34)
        Me.Btn_CleanerDEL.Name = "Btn_CleanerDEL"
        Me.Btn_CleanerDEL.Size = New System.Drawing.Size(75, 23)
        Me.Btn_CleanerDEL.TabIndex = 2
        Me.Btn_CleanerDEL.Text = "Remove"
        Me.Btn_CleanerDEL.UseVisualStyleBackColor = True
        '
        'Btn_CleanerDN
        '
        Me.Btn_CleanerDN.Location = New System.Drawing.Point(626, 63)
        Me.Btn_CleanerDN.Name = "Btn_CleanerDN"
        Me.Btn_CleanerDN.Size = New System.Drawing.Size(75, 23)
        Me.Btn_CleanerDN.TabIndex = 3
        Me.Btn_CleanerDN.Text = "Move Down"
        Me.Btn_CleanerDN.UseVisualStyleBackColor = True
        '
        'Panel12
        '
        Me.Panel12.Controls.Add(Me.LBL_CleanerPN)
        Me.Panel12.Controls.Add(Me.LBL_CleanerIndex)
        Me.Panel12.Controls.Add(Me.LBL_CleanerDescription)
        Me.Panel12.Controls.Add(Me.Label32)
        Me.Panel12.Controls.Add(Me.CB_CleanerList)
        Me.Panel12.Controls.Add(Me.Label14)
        Me.Panel12.Controls.Add(Me.Label6)
        Me.Panel12.Controls.Add(Me.UD_CleanerUses)
        Me.Panel12.Controls.Add(Me.Label5)
        Me.Panel12.Controls.Add(Me.Label24)
        Me.Panel12.Controls.Add(Me.Btn_AddCleaner)
        Me.Panel12.Dock = System.Windows.Forms.DockStyle.Top
        Me.Panel12.Location = New System.Drawing.Point(3, 16)
        Me.Panel12.Name = "Panel12"
        Me.Panel12.Size = New System.Drawing.Size(844, 95)
        Me.Panel12.TabIndex = 0
        '
        'LBL_CleanerPN
        '
        Me.LBL_CleanerPN.AutoSize = True
        Me.LBL_CleanerPN.Location = New System.Drawing.Point(477, 66)
        Me.LBL_CleanerPN.Name = "LBL_CleanerPN"
        Me.LBL_CleanerPN.Size = New System.Drawing.Size(58, 13)
        Me.LBL_CleanerPN.TabIndex = 13
        Me.LBL_CleanerPN.Text = "CleanerPN"
        '
        'LBL_CleanerIndex
        '
        Me.LBL_CleanerIndex.AutoSize = True
        Me.LBL_CleanerIndex.Location = New System.Drawing.Point(477, 39)
        Me.LBL_CleanerIndex.Name = "LBL_CleanerIndex"
        Me.LBL_CleanerIndex.Size = New System.Drawing.Size(57, 13)
        Me.LBL_CleanerIndex.TabIndex = 12
        Me.LBL_CleanerIndex.Text = "CleanerIdx"
        '
        'LBL_CleanerDescription
        '
        Me.LBL_CleanerDescription.AutoSize = True
        Me.LBL_CleanerDescription.Location = New System.Drawing.Point(477, 14)
        Me.LBL_CleanerDescription.Name = "LBL_CleanerDescription"
        Me.LBL_CleanerDescription.Size = New System.Drawing.Size(68, 13)
        Me.LBL_CleanerDescription.TabIndex = 11
        Me.LBL_CleanerDescription.Text = "CleanerDesc"
        '
        'Label32
        '
        Me.Label32.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label32.Location = New System.Drawing.Point(38, 10)
        Me.Label32.Name = "Label32"
        Me.Label32.Size = New System.Drawing.Size(73, 23)
        Me.Label32.TabIndex = 10
        Me.Label32.Text = "Cleaner"
        '
        'CB_CleanerList
        '
        Me.CB_CleanerList.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.CB_CleanerList.FormattingEnabled = True
        Me.CB_CleanerList.Location = New System.Drawing.Point(117, 12)
        Me.CB_CleanerList.Name = "CB_CleanerList"
        Me.CB_CleanerList.Size = New System.Drawing.Size(162, 21)
        Me.CB_CleanerList.TabIndex = 9
        '
        'Label14
        '
        Me.Label14.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label14.Location = New System.Drawing.Point(346, 37)
        Me.Label14.Name = "Label14"
        Me.Label14.Size = New System.Drawing.Size(120, 23)
        Me.Label14.TabIndex = 0
        Me.Label14.Text = "Cleaner Index"
        '
        'Label6
        '
        Me.Label6.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label6.Location = New System.Drawing.Point(35, 37)
        Me.Label6.Name = "Label6"
        Me.Label6.Size = New System.Drawing.Size(124, 23)
        Me.Label6.TabIndex = 5
        Me.Label6.Text = "Total Uses"
        '
        'UD_CleanerUses
        '
        Me.UD_CleanerUses.Location = New System.Drawing.Point(206, 40)
        Me.UD_CleanerUses.Maximum = New Decimal(New Integer() {32767, 0, 0, 0})
        Me.UD_CleanerUses.Name = "UD_CleanerUses"
        Me.UD_CleanerUses.Size = New System.Drawing.Size(73, 20)
        Me.UD_CleanerUses.TabIndex = 7
        '
        'Label5
        '
        Me.Label5.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label5.Location = New System.Drawing.Point(346, 61)
        Me.Label5.Name = "Label5"
        Me.Label5.Size = New System.Drawing.Size(124, 23)
        Me.Label5.TabIndex = 1
        Me.Label5.Text = "Cleaner PN"
        '
        'Label24
        '
        Me.Label24.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label24.Location = New System.Drawing.Point(346, 9)
        Me.Label24.Name = "Label24"
        Me.Label24.Size = New System.Drawing.Size(125, 23)
        Me.Label24.TabIndex = 4
        Me.Label24.Text = "Cleaner Desc"
        '
        'Btn_AddCleaner
        '
        Me.Btn_AddCleaner.Location = New System.Drawing.Point(626, 37)
        Me.Btn_AddCleaner.Name = "Btn_AddCleaner"
        Me.Btn_AddCleaner.Size = New System.Drawing.Size(75, 23)
        Me.Btn_AddCleaner.TabIndex = 8
        Me.Btn_AddCleaner.Text = "Add"
        Me.Btn_AddCleaner.UseVisualStyleBackColor = True
        '
        'Panel10
        '
        Me.Panel10.Controls.Add(Me.GroupBox2)
        Me.Panel10.Dock = System.Windows.Forms.DockStyle.Top
        Me.Panel10.Location = New System.Drawing.Point(3, 3)
        Me.Panel10.Name = "Panel10"
        Me.Panel10.Size = New System.Drawing.Size(850, 114)
        Me.Panel10.TabIndex = 18
        '
        'GroupBox2
        '
        Me.GroupBox2.Controls.Add(Me.CB_ContainerType)
        Me.GroupBox2.Controls.Add(Me.Label40)
        Me.GroupBox2.Controls.Add(Me.TB_GTIN)
        Me.GroupBox2.Controls.Add(Me.Label34)
        Me.GroupBox2.Controls.Add(Me.lblPackLotNo)
        Me.GroupBox2.Controls.Add(Me.lblPackExpirationDate)
        Me.GroupBox2.Controls.Add(Me.udPackLotNumber)
        Me.GroupBox2.Controls.Add(Me.dtpPackExpiration)
        Me.GroupBox2.Controls.Add(Me.cbFullLayout)
        Me.GroupBox2.Controls.Add(Me.Label10)
        Me.GroupBox2.Controls.Add(Me.TB_PackPN)
        Me.GroupBox2.Controls.Add(Me.Label11)
        Me.GroupBox2.Controls.Add(Me.UP_ServiceLife)
        Me.GroupBox2.Controls.Add(Me.Label12)
        Me.GroupBox2.Dock = System.Windows.Forms.DockStyle.Fill
        Me.GroupBox2.Location = New System.Drawing.Point(0, 0)
        Me.GroupBox2.Name = "GroupBox2"
        Me.GroupBox2.Size = New System.Drawing.Size(850, 114)
        Me.GroupBox2.TabIndex = 0
        Me.GroupBox2.TabStop = False
        Me.GroupBox2.Text = "Header"
        '
        'CB_ContainerType
        '
        Me.CB_ContainerType.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.CB_ContainerType.FormattingEnabled = True
        Me.CB_ContainerType.Location = New System.Drawing.Point(416, 56)
        Me.CB_ContainerType.Name = "CB_ContainerType"
        Me.CB_ContainerType.Size = New System.Drawing.Size(121, 21)
        Me.CB_ContainerType.TabIndex = 25
        '
        'Label40
        '
        Me.Label40.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label40.Location = New System.Drawing.Point(318, 54)
        Me.Label40.Name = "Label40"
        Me.Label40.Size = New System.Drawing.Size(92, 23)
        Me.Label40.TabIndex = 14
        Me.Label40.Text = "Container"
        '
        'TB_GTIN
        '
        Me.TB_GTIN.Location = New System.Drawing.Point(322, 19)
        Me.TB_GTIN.MaxLength = 14
        Me.TB_GTIN.Name = "TB_GTIN"
        Me.TB_GTIN.Size = New System.Drawing.Size(215, 20)
        Me.TB_GTIN.TabIndex = 13
        '
        'Label34
        '
        Me.Label34.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label34.Location = New System.Drawing.Point(41, 16)
        Me.Label34.Name = "Label34"
        Me.Label34.Size = New System.Drawing.Size(289, 23)
        Me.Label34.TabIndex = 12
        Me.Label34.Text = "Global Trade Item Number (GTIN)"
        '
        'lblPackLotNo
        '
        Me.lblPackLotNo.Enabled = False
        Me.lblPackLotNo.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.lblPackLotNo.Location = New System.Drawing.Point(580, 81)
        Me.lblPackLotNo.Name = "lblPackLotNo"
        Me.lblPackLotNo.Size = New System.Drawing.Size(127, 23)
        Me.lblPackLotNo.TabIndex = 10
        Me.lblPackLotNo.Text = "Pack Lot No."
        '
        'lblPackExpirationDate
        '
        Me.lblPackExpirationDate.Enabled = False
        Me.lblPackExpirationDate.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.lblPackExpirationDate.Location = New System.Drawing.Point(580, 58)
        Me.lblPackExpirationDate.Name = "lblPackExpirationDate"
        Me.lblPackExpirationDate.Size = New System.Drawing.Size(143, 23)
        Me.lblPackExpirationDate.TabIndex = 9
        Me.lblPackExpirationDate.Text = "Expiration Date"
        '
        'udPackLotNumber
        '
        Me.udPackLotNumber.Enabled = False
        Me.udPackLotNumber.Location = New System.Drawing.Point(729, 84)
        Me.udPackLotNumber.Maximum = New Decimal(New Integer() {0, 1, 0, 0})
        Me.udPackLotNumber.Name = "udPackLotNumber"
        Me.udPackLotNumber.Size = New System.Drawing.Size(120, 20)
        Me.udPackLotNumber.TabIndex = 8
        '
        'dtpPackExpiration
        '
        Me.dtpPackExpiration.Enabled = False
        Me.dtpPackExpiration.Format = System.Windows.Forms.DateTimePickerFormat.[Short]
        Me.dtpPackExpiration.Location = New System.Drawing.Point(729, 61)
        Me.dtpPackExpiration.MinDate = New Date(2017, 1, 1, 0, 0, 0, 0)
        Me.dtpPackExpiration.Name = "dtpPackExpiration"
        Me.dtpPackExpiration.Size = New System.Drawing.Size(120, 20)
        Me.dtpPackExpiration.TabIndex = 6
        '
        'cbFullLayout
        '
        Me.cbFullLayout.AutoSize = True
        Me.cbFullLayout.Location = New System.Drawing.Point(583, 38)
        Me.cbFullLayout.Name = "cbFullLayout"
        Me.cbFullLayout.Size = New System.Drawing.Size(140, 17)
        Me.cbFullLayout.TabIndex = 5
        Me.cbFullLayout.Text = "Set Complete Contents?"
        Me.cbFullLayout.UseVisualStyleBackColor = True
        '
        'Label10
        '
        Me.Label10.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label10.Location = New System.Drawing.Point(39, 54)
        Me.Label10.Name = "Label10"
        Me.Label10.Size = New System.Drawing.Size(127, 23)
        Me.Label10.TabIndex = 0
        Me.Label10.Text = "Pack Part No."
        '
        'TB_PackPN
        '
        Me.TB_PackPN.CharacterCasing = System.Windows.Forms.CharacterCasing.Upper
        Me.TB_PackPN.Location = New System.Drawing.Point(182, 56)
        Me.TB_PackPN.MaxLength = 6
        Me.TB_PackPN.Name = "TB_PackPN"
        Me.TB_PackPN.Size = New System.Drawing.Size(100, 20)
        Me.TB_PackPN.TabIndex = 1
        '
        'Label11
        '
        Me.Label11.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label11.Location = New System.Drawing.Point(39, 77)
        Me.Label11.Name = "Label11"
        Me.Label11.Size = New System.Drawing.Size(116, 23)
        Me.Label11.TabIndex = 2
        Me.Label11.Text = "Service Life"
        '
        'UP_ServiceLife
        '
        Me.UP_ServiceLife.Location = New System.Drawing.Point(183, 80)
        Me.UP_ServiceLife.Maximum = New Decimal(New Integer() {32767, 0, 0, 0})
        Me.UP_ServiceLife.Name = "UP_ServiceLife"
        Me.UP_ServiceLife.Size = New System.Drawing.Size(67, 20)
        Me.UP_ServiceLife.TabIndex = 3
        Me.UP_ServiceLife.Value = New Decimal(New Integer() {1, 0, 0, 0})
        '
        'Label12
        '
        Me.Label12.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label12.Location = New System.Drawing.Point(256, 80)
        Me.Label12.Name = "Label12"
        Me.Label12.Size = New System.Drawing.Size(43, 23)
        Me.Label12.TabIndex = 4
        Me.Label12.Text = "Days"
        '
        'TP_Analyses
        '
        Me.TP_Analyses.Controls.Add(Me.Panel8)
        Me.TP_Analyses.Controls.Add(Me.Panel7)
        Me.TP_Analyses.Location = New System.Drawing.Point(4, 22)
        Me.TP_Analyses.Name = "TP_Analyses"
        Me.TP_Analyses.Padding = New System.Windows.Forms.Padding(3)
        Me.TP_Analyses.Size = New System.Drawing.Size(856, 689)
        Me.TP_Analyses.TabIndex = 3
        Me.TP_Analyses.Text = "Analyses"
        Me.TP_Analyses.UseVisualStyleBackColor = True
        '
        'Panel8
        '
        Me.Panel8.Controls.Add(Me.GroupBox7)
        Me.Panel8.Controls.Add(Me.GroupBox6)
        Me.Panel8.Controls.Add(Me.GroupBox5)
        Me.Panel8.Controls.Add(Me.GroupBox1)
        Me.Panel8.Controls.Add(Me.TB_AnalysisDesc)
        Me.Panel8.Controls.Add(Me.Label16)
        Me.Panel8.Controls.Add(Me.UD_AnalysisIndex)
        Me.Panel8.Controls.Add(Me.Label19)
        Me.Panel8.Controls.Add(Me.Btn_AddAnalysis)
        Me.Panel8.Dock = System.Windows.Forms.DockStyle.Fill
        Me.Panel8.Location = New System.Drawing.Point(3, 3)
        Me.Panel8.Name = "Panel8"
        Me.Panel8.Size = New System.Drawing.Size(850, 540)
        Me.Panel8.TabIndex = 1
        '
        'GroupBox7
        '
        Me.GroupBox7.Controls.Add(Me.CB_AnalysisReagent)
        Me.GroupBox7.Controls.Add(Me.Btn_AnalysisReagentDN)
        Me.GroupBox7.Controls.Add(Me.Btn_AnalysisReagentDEL)
        Me.GroupBox7.Controls.Add(Me.Btn_AnalysisReagentUP)
        Me.GroupBox7.Controls.Add(Me.Btn_AnalysisReagentAdd)
        Me.GroupBox7.Controls.Add(Me.LB_AnalysisReagents)
        Me.GroupBox7.Controls.Add(Me.Label15)
        Me.GroupBox7.Location = New System.Drawing.Point(18, 75)
        Me.GroupBox7.Name = "GroupBox7"
        Me.GroupBox7.Size = New System.Drawing.Size(290, 123)
        Me.GroupBox7.TabIndex = 4
        Me.GroupBox7.TabStop = False
        Me.GroupBox7.Text = "Reagents"
        '
        'CB_AnalysisReagent
        '
        Me.CB_AnalysisReagent.FormattingEnabled = True
        Me.CB_AnalysisReagent.Location = New System.Drawing.Point(81, 21)
        Me.CB_AnalysisReagent.Name = "CB_AnalysisReagent"
        Me.CB_AnalysisReagent.Size = New System.Drawing.Size(131, 21)
        Me.CB_AnalysisReagent.TabIndex = 7
        '
        'Btn_AnalysisReagentDN
        '
        Me.Btn_AnalysisReagentDN.Location = New System.Drawing.Point(223, 78)
        Me.Btn_AnalysisReagentDN.Name = "Btn_AnalysisReagentDN"
        Me.Btn_AnalysisReagentDN.Size = New System.Drawing.Size(28, 23)
        Me.Btn_AnalysisReagentDN.TabIndex = 5
        Me.Btn_AnalysisReagentDN.Text = "D"
        Me.Btn_AnalysisReagentDN.UseVisualStyleBackColor = True
        '
        'Btn_AnalysisReagentDEL
        '
        Me.Btn_AnalysisReagentDEL.Location = New System.Drawing.Point(257, 63)
        Me.Btn_AnalysisReagentDEL.Name = "Btn_AnalysisReagentDEL"
        Me.Btn_AnalysisReagentDEL.Size = New System.Drawing.Size(28, 23)
        Me.Btn_AnalysisReagentDEL.TabIndex = 4
        Me.Btn_AnalysisReagentDEL.Text = "X"
        Me.Btn_AnalysisReagentDEL.UseVisualStyleBackColor = True
        '
        'Btn_AnalysisReagentUP
        '
        Me.Btn_AnalysisReagentUP.Location = New System.Drawing.Point(223, 49)
        Me.Btn_AnalysisReagentUP.Name = "Btn_AnalysisReagentUP"
        Me.Btn_AnalysisReagentUP.Size = New System.Drawing.Size(28, 23)
        Me.Btn_AnalysisReagentUP.TabIndex = 3
        Me.Btn_AnalysisReagentUP.Text = "U"
        Me.Btn_AnalysisReagentUP.UseVisualStyleBackColor = True
        '
        'Btn_AnalysisReagentAdd
        '
        Me.Btn_AnalysisReagentAdd.Location = New System.Drawing.Point(223, 21)
        Me.Btn_AnalysisReagentAdd.Name = "Btn_AnalysisReagentAdd"
        Me.Btn_AnalysisReagentAdd.Size = New System.Drawing.Size(62, 23)
        Me.Btn_AnalysisReagentAdd.TabIndex = 2
        Me.Btn_AnalysisReagentAdd.Text = "Add"
        Me.Btn_AnalysisReagentAdd.UseVisualStyleBackColor = True
        '
        'LB_AnalysisReagents
        '
        Me.LB_AnalysisReagents.FormattingEnabled = True
        Me.LB_AnalysisReagents.Location = New System.Drawing.Point(7, 49)
        Me.LB_AnalysisReagents.Name = "LB_AnalysisReagents"
        Me.LB_AnalysisReagents.Size = New System.Drawing.Size(205, 56)
        Me.LB_AnalysisReagents.TabIndex = 6
        '
        'Label15
        '
        Me.Label15.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label15.Location = New System.Drawing.Point(3, 19)
        Me.Label15.Name = "Label15"
        Me.Label15.Size = New System.Drawing.Size(114, 23)
        Me.Label15.TabIndex = 0
        Me.Label15.Text = "Reagent"
        '
        'GroupBox6
        '
        Me.GroupBox6.Controls.Add(Me.Label41)
        Me.GroupBox6.Controls.Add(Me.UD_AnalysisEmissionWavelength)
        Me.GroupBox6.Controls.Add(Me.Label40)
        Me.GroupBox6.Controls.Add(Me.Btn_AnalysisIlluminatorAdd)
        Me.GroupBox6.Controls.Add(Me.Btn_AnalysisIlluminatorDN)
        Me.GroupBox6.Controls.Add(Me.Btn_AnalysisIlluminatorDEL)
        Me.GroupBox6.Controls.Add(Me.Btn_AnalysisIlluminatorUP)
        Me.GroupBox6.Controls.Add(Me.LB_AnalysisIlluminators)
        Me.GroupBox6.Controls.Add(Me.Label20)
        Me.GroupBox6.Controls.Add(Me.Label18)
        Me.GroupBox6.Controls.Add(Me.UD_AnalysisIlluminatorMS)
        Me.GroupBox6.Controls.Add(Me.UD_AnalysisIlluminatorWL)
        Me.GroupBox6.Controls.Add(Me.Label17)
        Me.GroupBox6.Location = New System.Drawing.Point(321, 48)
        Me.GroupBox6.Name = "GroupBox6"
        Me.GroupBox6.Size = New System.Drawing.Size(424, 150)
        Me.GroupBox6.TabIndex = 5
        Me.GroupBox6.TabStop = False
        Me.GroupBox6.Text = "FL Illuminators"
        '
        'Label41
        '
        Me.Label41.AutoSize = True
        Me.Label41.Location = New System.Drawing.Point(196, 55)
        Me.Label41.Name = "Label41"
        Me.Label41.Size = New System.Drawing.Size(21, 13)
        Me.Label41.TabIndex = 12
        Me.Label41.Text = "nm"
        '
        'UD_AnalysisEmissionWavelength
        '
        Me.UD_AnalysisEmissionWavelength.Location = New System.Drawing.Point(116, 53)
        Me.UD_AnalysisEmissionWavelength.Maximum = New Decimal(New Integer() {32767, 0, 0, 0})
        Me.UD_AnalysisEmissionWavelength.Name = "UD_AnalysisEmissionWavelength"
        Me.UD_AnalysisEmissionWavelength.Size = New System.Drawing.Size(73, 20)
        Me.UD_AnalysisEmissionWavelength.TabIndex = 11
        '
        'Label40
        '
        Me.Label40.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label40.Location = New System.Drawing.Point(21, 50)
        Me.Label40.Name = "Label40"
        Me.Label40.Size = New System.Drawing.Size(100, 23)
        Me.Label40.TabIndex = 10
        Me.Label40.Text = "Emission:"
        '
        'Btn_AnalysisIlluminatorAdd
        '
        Me.Btn_AnalysisIlluminatorAdd.Location = New System.Drawing.Point(339, 29)
        Me.Btn_AnalysisIlluminatorAdd.Name = "Btn_AnalysisIlluminatorAdd"
        Me.Btn_AnalysisIlluminatorAdd.Size = New System.Drawing.Size(63, 23)
        Me.Btn_AnalysisIlluminatorAdd.TabIndex = 5
        Me.Btn_AnalysisIlluminatorAdd.Text = "Add"
        Me.Btn_AnalysisIlluminatorAdd.UseVisualStyleBackColor = True
        '
        'Btn_AnalysisIlluminatorDN
        '
        Me.Btn_AnalysisIlluminatorDN.Location = New System.Drawing.Point(338, 106)
        Me.Btn_AnalysisIlluminatorDN.Name = "Btn_AnalysisIlluminatorDN"
        Me.Btn_AnalysisIlluminatorDN.Size = New System.Drawing.Size(28, 23)
        Me.Btn_AnalysisIlluminatorDN.TabIndex = 8
        Me.Btn_AnalysisIlluminatorDN.Text = "D"
        Me.Btn_AnalysisIlluminatorDN.UseVisualStyleBackColor = True
        '
        'Btn_AnalysisIlluminatorDEL
        '
        Me.Btn_AnalysisIlluminatorDEL.Location = New System.Drawing.Point(373, 91)
        Me.Btn_AnalysisIlluminatorDEL.Name = "Btn_AnalysisIlluminatorDEL"
        Me.Btn_AnalysisIlluminatorDEL.Size = New System.Drawing.Size(28, 23)
        Me.Btn_AnalysisIlluminatorDEL.TabIndex = 7
        Me.Btn_AnalysisIlluminatorDEL.Text = "X"
        Me.Btn_AnalysisIlluminatorDEL.UseVisualStyleBackColor = True
        '
        'Btn_AnalysisIlluminatorUP
        '
        Me.Btn_AnalysisIlluminatorUP.Location = New System.Drawing.Point(339, 77)
        Me.Btn_AnalysisIlluminatorUP.Name = "Btn_AnalysisIlluminatorUP"
        Me.Btn_AnalysisIlluminatorUP.Size = New System.Drawing.Size(28, 23)
        Me.Btn_AnalysisIlluminatorUP.TabIndex = 6
        Me.Btn_AnalysisIlluminatorUP.Text = "U"
        Me.Btn_AnalysisIlluminatorUP.UseVisualStyleBackColor = True
        '
        'LB_AnalysisIlluminators
        '
        Me.LB_AnalysisIlluminators.FormattingEnabled = True
        Me.LB_AnalysisIlluminators.Location = New System.Drawing.Point(25, 76)
        Me.LB_AnalysisIlluminators.Name = "LB_AnalysisIlluminators"
        Me.LB_AnalysisIlluminators.Size = New System.Drawing.Size(308, 56)
        Me.LB_AnalysisIlluminators.TabIndex = 9
        '
        'Label20
        '
        Me.Label20.AutoSize = True
        Me.Label20.Location = New System.Drawing.Point(313, 29)
        Me.Label20.Name = "Label20"
        Me.Label20.Size = New System.Drawing.Size(20, 13)
        Me.Label20.TabIndex = 4
        Me.Label20.Text = "ms"
        '
        'Label18
        '
        Me.Label18.AutoSize = True
        Me.Label18.Location = New System.Drawing.Point(196, 29)
        Me.Label18.Name = "Label18"
        Me.Label18.Size = New System.Drawing.Size(21, 13)
        Me.Label18.TabIndex = 2
        Me.Label18.Text = "nm"
        '
        'UD_AnalysisIlluminatorMS
        '
        Me.UD_AnalysisIlluminatorMS.Location = New System.Drawing.Point(234, 27)
        Me.UD_AnalysisIlluminatorMS.Maximum = New Decimal(New Integer() {32767, 0, 0, 0})
        Me.UD_AnalysisIlluminatorMS.Name = "UD_AnalysisIlluminatorMS"
        Me.UD_AnalysisIlluminatorMS.Size = New System.Drawing.Size(73, 20)
        Me.UD_AnalysisIlluminatorMS.TabIndex = 3
        '
        'UD_AnalysisIlluminatorWL
        '
        Me.UD_AnalysisIlluminatorWL.Location = New System.Drawing.Point(117, 27)
        Me.UD_AnalysisIlluminatorWL.Maximum = New Decimal(New Integer() {32767, 0, 0, 0})
        Me.UD_AnalysisIlluminatorWL.Name = "UD_AnalysisIlluminatorWL"
        Me.UD_AnalysisIlluminatorWL.Size = New System.Drawing.Size(73, 20)
        Me.UD_AnalysisIlluminatorWL.TabIndex = 1
        '
        'Label17
        '
        Me.Label17.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label17.Location = New System.Drawing.Point(22, 24)
        Me.Label17.Name = "Label17"
        Me.Label17.Size = New System.Drawing.Size(100, 23)
        Me.Label17.TabIndex = 0
        Me.Label17.Text = "Exitation:"
        '
        'GroupBox5
        '
        Me.GroupBox5.Controls.Add(Me.Label31)
        Me.GroupBox5.Controls.Add(Me.CB_AnalysisParameter)
        Me.GroupBox5.Controls.Add(Me.LBL_AnalysisParameterKey)
        Me.GroupBox5.Controls.Add(Me.UD_AnalysisParamThreshold)
        Me.GroupBox5.Controls.Add(Me.Btn_AnalysisParamDN)
        Me.GroupBox5.Controls.Add(Me.Btn_AnalysisParamDEL)
        Me.GroupBox5.Controls.Add(Me.Btn_AnalysisParamUP)
        Me.GroupBox5.Controls.Add(Me.LB_AnalysisParams)
        Me.GroupBox5.Controls.Add(Me.Btn_AddAnalysisParam)
        Me.GroupBox5.Controls.Add(Me.RB_AnalysisParamBelow)
        Me.GroupBox5.Controls.Add(Me.RB_AnalysisParamAtAbove)
        Me.GroupBox5.Controls.Add(Me.UD_AnalysisParamSubkey)
        Me.GroupBox5.Controls.Add(Me.Label27)
        Me.GroupBox5.Controls.Add(Me.Label28)
        Me.GroupBox5.Controls.Add(Me.Label29)
        Me.GroupBox5.Location = New System.Drawing.Point(16, 293)
        Me.GroupBox5.Name = "GroupBox5"
        Me.GroupBox5.Size = New System.Drawing.Size(729, 241)
        Me.GroupBox5.TabIndex = 7
        Me.GroupBox5.TabStop = False
        Me.GroupBox5.Text = "Analysis Parameters"
        '
        'Label31
        '
        Me.Label31.AutoSize = True
        Me.Label31.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.75!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label31.Location = New System.Drawing.Point(6, 24)
        Me.Label31.Name = "Label31"
        Me.Label31.Size = New System.Drawing.Size(80, 16)
        Me.Label31.TabIndex = 12
        Me.Label31.Text = "Parameter"
        '
        'CB_AnalysisParameter
        '
        Me.CB_AnalysisParameter.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.CB_AnalysisParameter.FormattingEnabled = True
        Me.CB_AnalysisParameter.Location = New System.Drawing.Point(92, 22)
        Me.CB_AnalysisParameter.Name = "CB_AnalysisParameter"
        Me.CB_AnalysisParameter.Size = New System.Drawing.Size(191, 21)
        Me.CB_AnalysisParameter.TabIndex = 12
        '
        'LBL_AnalysisParameterKey
        '
        Me.LBL_AnalysisParameterKey.AutoSize = True
        Me.LBL_AnalysisParameterKey.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.75!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.LBL_AnalysisParameterKey.Location = New System.Drawing.Point(80, 51)
        Me.LBL_AnalysisParameterKey.Name = "LBL_AnalysisParameterKey"
        Me.LBL_AnalysisParameterKey.Size = New System.Drawing.Size(34, 16)
        Me.LBL_AnalysisParameterKey.TabIndex = 13
        Me.LBL_AnalysisParameterKey.Text = "Key"
        '
        'UD_AnalysisParamThreshold
        '
        Me.UD_AnalysisParamThreshold.DecimalPlaces = 4
        Me.UD_AnalysisParamThreshold.Location = New System.Drawing.Point(375, 27)
        Me.UD_AnalysisParamThreshold.Maximum = New Decimal(New Integer() {10000, 0, 0, 0})
        Me.UD_AnalysisParamThreshold.Minimum = New Decimal(New Integer() {10000, 0, 0, -2147483648})
        Me.UD_AnalysisParamThreshold.Name = "UD_AnalysisParamThreshold"
        Me.UD_AnalysisParamThreshold.Size = New System.Drawing.Size(86, 20)
        Me.UD_AnalysisParamThreshold.TabIndex = 5
        '
        'Btn_AnalysisParamDN
        '
        Me.Btn_AnalysisParamDN.Location = New System.Drawing.Point(568, 177)
        Me.Btn_AnalysisParamDN.Name = "Btn_AnalysisParamDN"
        Me.Btn_AnalysisParamDN.Size = New System.Drawing.Size(75, 23)
        Me.Btn_AnalysisParamDN.TabIndex = 11
        Me.Btn_AnalysisParamDN.Text = "Down"
        Me.Btn_AnalysisParamDN.UseVisualStyleBackColor = True
        '
        'Btn_AnalysisParamDEL
        '
        Me.Btn_AnalysisParamDEL.Location = New System.Drawing.Point(568, 145)
        Me.Btn_AnalysisParamDEL.Name = "Btn_AnalysisParamDEL"
        Me.Btn_AnalysisParamDEL.Size = New System.Drawing.Size(75, 23)
        Me.Btn_AnalysisParamDEL.TabIndex = 10
        Me.Btn_AnalysisParamDEL.Text = "Remove"
        Me.Btn_AnalysisParamDEL.UseVisualStyleBackColor = True
        '
        'Btn_AnalysisParamUP
        '
        Me.Btn_AnalysisParamUP.Location = New System.Drawing.Point(568, 116)
        Me.Btn_AnalysisParamUP.Name = "Btn_AnalysisParamUP"
        Me.Btn_AnalysisParamUP.Size = New System.Drawing.Size(75, 23)
        Me.Btn_AnalysisParamUP.TabIndex = 9
        Me.Btn_AnalysisParamUP.Text = "Up"
        Me.Btn_AnalysisParamUP.UseVisualStyleBackColor = True
        '
        'LB_AnalysisParams
        '
        Me.LB_AnalysisParams.FormattingEnabled = True
        Me.LB_AnalysisParams.Location = New System.Drawing.Point(58, 89)
        Me.LB_AnalysisParams.Name = "LB_AnalysisParams"
        Me.LB_AnalysisParams.Size = New System.Drawing.Size(484, 134)
        Me.LB_AnalysisParams.TabIndex = 12
        '
        'Btn_AddAnalysisParam
        '
        Me.Btn_AddAnalysisParam.Location = New System.Drawing.Point(467, 24)
        Me.Btn_AddAnalysisParam.Name = "Btn_AddAnalysisParam"
        Me.Btn_AddAnalysisParam.Size = New System.Drawing.Size(75, 23)
        Me.Btn_AddAnalysisParam.TabIndex = 8
        Me.Btn_AddAnalysisParam.Text = "Add"
        Me.Btn_AddAnalysisParam.UseVisualStyleBackColor = True
        '
        'RB_AnalysisParamBelow
        '
        Me.RB_AnalysisParamBelow.AutoSize = True
        Me.RB_AnalysisParamBelow.Location = New System.Drawing.Point(375, 50)
        Me.RB_AnalysisParamBelow.Name = "RB_AnalysisParamBelow"
        Me.RB_AnalysisParamBelow.Size = New System.Drawing.Size(54, 17)
        Me.RB_AnalysisParamBelow.TabIndex = 7
        Me.RB_AnalysisParamBelow.Text = "Below"
        Me.RB_AnalysisParamBelow.UseVisualStyleBackColor = True
        '
        'RB_AnalysisParamAtAbove
        '
        Me.RB_AnalysisParamAtAbove.AutoSize = True
        Me.RB_AnalysisParamAtAbove.Checked = True
        Me.RB_AnalysisParamAtAbove.Location = New System.Drawing.Point(298, 50)
        Me.RB_AnalysisParamAtAbove.Name = "RB_AnalysisParamAtAbove"
        Me.RB_AnalysisParamAtAbove.Size = New System.Drawing.Size(71, 17)
        Me.RB_AnalysisParamAtAbove.TabIndex = 6
        Me.RB_AnalysisParamAtAbove.TabStop = True
        Me.RB_AnalysisParamAtAbove.Text = "At/Above"
        Me.RB_AnalysisParamAtAbove.UseVisualStyleBackColor = True
        '
        'UD_AnalysisParamSubkey
        '
        Me.UD_AnalysisParamSubkey.Location = New System.Drawing.Point(196, 51)
        Me.UD_AnalysisParamSubkey.Maximum = New Decimal(New Integer() {32767, 0, 0, 0})
        Me.UD_AnalysisParamSubkey.Name = "UD_AnalysisParamSubkey"
        Me.UD_AnalysisParamSubkey.Size = New System.Drawing.Size(73, 20)
        Me.UD_AnalysisParamSubkey.TabIndex = 3
        '
        'Label27
        '
        Me.Label27.AutoSize = True
        Me.Label27.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.75!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label27.Location = New System.Drawing.Point(138, 51)
        Me.Label27.Name = "Label27"
        Me.Label27.Size = New System.Drawing.Size(60, 16)
        Me.Label27.TabIndex = 2
        Me.Label27.Text = "Subkey"
        '
        'Label28
        '
        Me.Label28.AutoSize = True
        Me.Label28.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.75!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label28.Location = New System.Drawing.Point(291, 27)
        Me.Label28.Name = "Label28"
        Me.Label28.Size = New System.Drawing.Size(78, 16)
        Me.Label28.TabIndex = 4
        Me.Label28.Text = "Threshold"
        '
        'Label29
        '
        Me.Label29.AutoSize = True
        Me.Label29.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.75!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label29.Location = New System.Drawing.Point(40, 51)
        Me.Label29.Name = "Label29"
        Me.Label29.Size = New System.Drawing.Size(34, 16)
        Me.Label29.TabIndex = 0
        Me.Label29.Text = "Key"
        '
        'GroupBox1
        '
        Me.GroupBox1.Controls.Add(Me.Lbl_AnalysisPopulationKey)
        Me.GroupBox1.Controls.Add(Me.Label30)
        Me.GroupBox1.Controls.Add(Me.CB_AnalysisPopParameter)
        Me.GroupBox1.Controls.Add(Me.UD_AnalysisPopThreshold)
        Me.GroupBox1.Controls.Add(Me.Btn_PopulationClear)
        Me.GroupBox1.Controls.Add(Me.RB_PopBelow)
        Me.GroupBox1.Controls.Add(Me.RB_PopAtAbove)
        Me.GroupBox1.Controls.Add(Me.UD_AnalysisPopSubkey)
        Me.GroupBox1.Controls.Add(Me.Label23)
        Me.GroupBox1.Controls.Add(Me.Label22)
        Me.GroupBox1.Controls.Add(Me.Label21)
        Me.GroupBox1.Location = New System.Drawing.Point(16, 204)
        Me.GroupBox1.Name = "GroupBox1"
        Me.GroupBox1.Size = New System.Drawing.Size(729, 83)
        Me.GroupBox1.TabIndex = 6
        Me.GroupBox1.TabStop = False
        Me.GroupBox1.Text = "Extra Population Parameter"
        '
        'Lbl_AnalysisPopulationKey
        '
        Me.Lbl_AnalysisPopulationKey.AutoSize = True
        Me.Lbl_AnalysisPopulationKey.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.75!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Lbl_AnalysisPopulationKey.Location = New System.Drawing.Point(89, 51)
        Me.Lbl_AnalysisPopulationKey.Name = "Lbl_AnalysisPopulationKey"
        Me.Lbl_AnalysisPopulationKey.Size = New System.Drawing.Size(0, 16)
        Me.Lbl_AnalysisPopulationKey.TabIndex = 11
        '
        'Label30
        '
        Me.Label30.AutoSize = True
        Me.Label30.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.75!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label30.Location = New System.Drawing.Point(6, 22)
        Me.Label30.Name = "Label30"
        Me.Label30.Size = New System.Drawing.Size(80, 16)
        Me.Label30.TabIndex = 10
        Me.Label30.Text = "Parameter"
        '
        'CB_AnalysisPopParameter
        '
        Me.CB_AnalysisPopParameter.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.CB_AnalysisPopParameter.FormattingEnabled = True
        Me.CB_AnalysisPopParameter.Location = New System.Drawing.Point(92, 21)
        Me.CB_AnalysisPopParameter.Name = "CB_AnalysisPopParameter"
        Me.CB_AnalysisPopParameter.Size = New System.Drawing.Size(191, 21)
        Me.CB_AnalysisPopParameter.TabIndex = 9
        '
        'UD_AnalysisPopThreshold
        '
        Me.UD_AnalysisPopThreshold.DecimalPlaces = 4
        Me.UD_AnalysisPopThreshold.Location = New System.Drawing.Point(399, 21)
        Me.UD_AnalysisPopThreshold.Maximum = New Decimal(New Integer() {10000, 0, 0, 0})
        Me.UD_AnalysisPopThreshold.Minimum = New Decimal(New Integer() {10000, 0, 0, -2147483648})
        Me.UD_AnalysisPopThreshold.Name = "UD_AnalysisPopThreshold"
        Me.UD_AnalysisPopThreshold.Size = New System.Drawing.Size(100, 20)
        Me.UD_AnalysisPopThreshold.TabIndex = 5
        '
        'Btn_PopulationClear
        '
        Me.Btn_PopulationClear.Location = New System.Drawing.Point(598, 19)
        Me.Btn_PopulationClear.Name = "Btn_PopulationClear"
        Me.Btn_PopulationClear.Size = New System.Drawing.Size(75, 23)
        Me.Btn_PopulationClear.TabIndex = 8
        Me.Btn_PopulationClear.Text = "Clear"
        Me.Btn_PopulationClear.UseVisualStyleBackColor = True
        '
        'RB_PopBelow
        '
        Me.RB_PopBelow.AutoSize = True
        Me.RB_PopBelow.Location = New System.Drawing.Point(399, 51)
        Me.RB_PopBelow.Name = "RB_PopBelow"
        Me.RB_PopBelow.Size = New System.Drawing.Size(54, 17)
        Me.RB_PopBelow.TabIndex = 7
        Me.RB_PopBelow.Text = "Below"
        Me.RB_PopBelow.UseVisualStyleBackColor = True
        '
        'RB_PopAtAbove
        '
        Me.RB_PopAtAbove.AutoSize = True
        Me.RB_PopAtAbove.Checked = True
        Me.RB_PopAtAbove.Location = New System.Drawing.Point(322, 51)
        Me.RB_PopAtAbove.Name = "RB_PopAtAbove"
        Me.RB_PopAtAbove.Size = New System.Drawing.Size(71, 17)
        Me.RB_PopAtAbove.TabIndex = 6
        Me.RB_PopAtAbove.TabStop = True
        Me.RB_PopAtAbove.Text = "At/Above"
        Me.RB_PopAtAbove.UseVisualStyleBackColor = True
        '
        'UD_AnalysisPopSubkey
        '
        Me.UD_AnalysisPopSubkey.Location = New System.Drawing.Point(212, 51)
        Me.UD_AnalysisPopSubkey.Maximum = New Decimal(New Integer() {32767, 0, 0, 0})
        Me.UD_AnalysisPopSubkey.Name = "UD_AnalysisPopSubkey"
        Me.UD_AnalysisPopSubkey.Size = New System.Drawing.Size(73, 20)
        Me.UD_AnalysisPopSubkey.TabIndex = 3
        '
        'Label23
        '
        Me.Label23.AutoSize = True
        Me.Label23.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.75!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label23.Location = New System.Drawing.Point(154, 51)
        Me.Label23.Name = "Label23"
        Me.Label23.Size = New System.Drawing.Size(60, 16)
        Me.Label23.TabIndex = 2
        Me.Label23.Text = "Subkey"
        '
        'Label22
        '
        Me.Label22.AutoSize = True
        Me.Label22.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.75!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label22.Location = New System.Drawing.Point(315, 22)
        Me.Label22.Name = "Label22"
        Me.Label22.Size = New System.Drawing.Size(78, 16)
        Me.Label22.TabIndex = 4
        Me.Label22.Text = "Threshold"
        '
        'Label21
        '
        Me.Label21.AutoSize = True
        Me.Label21.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.75!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label21.Location = New System.Drawing.Point(40, 51)
        Me.Label21.Name = "Label21"
        Me.Label21.Size = New System.Drawing.Size(34, 16)
        Me.Label21.TabIndex = 0
        Me.Label21.Text = "Key"
        '
        'TB_AnalysisDesc
        '
        Me.TB_AnalysisDesc.Location = New System.Drawing.Point(415, 10)
        Me.TB_AnalysisDesc.MaxLength = 19
        Me.TB_AnalysisDesc.Name = "TB_AnalysisDesc"
        Me.TB_AnalysisDesc.Size = New System.Drawing.Size(186, 20)
        Me.TB_AnalysisDesc.TabIndex = 3
        '
        'Label16
        '
        Me.Label16.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label16.Location = New System.Drawing.Point(253, 11)
        Me.Label16.Name = "Label16"
        Me.Label16.Size = New System.Drawing.Size(156, 23)
        Me.Label16.TabIndex = 2
        Me.Label16.Text = "Analysis Desc:"
        '
        'UD_AnalysisIndex
        '
        Me.UD_AnalysisIndex.Location = New System.Drawing.Point(153, 11)
        Me.UD_AnalysisIndex.Maximum = New Decimal(New Integer() {32767, 0, 0, 0})
        Me.UD_AnalysisIndex.Name = "UD_AnalysisIndex"
        Me.UD_AnalysisIndex.Size = New System.Drawing.Size(73, 20)
        Me.UD_AnalysisIndex.TabIndex = 1
        '
        'Label19
        '
        Me.Label19.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label19.Location = New System.Drawing.Point(5, 11)
        Me.Label19.Name = "Label19"
        Me.Label19.Size = New System.Drawing.Size(126, 23)
        Me.Label19.TabIndex = 0
        Me.Label19.Text = "Analysis Idx:"
        '
        'Btn_AddAnalysis
        '
        Me.Btn_AddAnalysis.Location = New System.Drawing.Point(760, 75)
        Me.Btn_AddAnalysis.Name = "Btn_AddAnalysis"
        Me.Btn_AddAnalysis.Size = New System.Drawing.Size(75, 459)
        Me.Btn_AddAnalysis.TabIndex = 8
        Me.Btn_AddAnalysis.Text = "Add"
        Me.Btn_AddAnalysis.UseVisualStyleBackColor = True
        '
        'Panel7
        '
        Me.Panel7.Controls.Add(Me.LB_Analyses)
        Me.Panel7.Controls.Add(Me.Btn_AnalysisDN)
        Me.Panel7.Controls.Add(Me.Btn_AnalysisDEL)
        Me.Panel7.Controls.Add(Me.Btn_AnalysisUP)
        Me.Panel7.Dock = System.Windows.Forms.DockStyle.Bottom
        Me.Panel7.Location = New System.Drawing.Point(3, 543)
        Me.Panel7.Name = "Panel7"
        Me.Panel7.Size = New System.Drawing.Size(850, 143)
        Me.Panel7.TabIndex = 0
        '
        'LB_Analyses
        '
        Me.LB_Analyses.Dock = System.Windows.Forms.DockStyle.Left
        Me.LB_Analyses.FormattingEnabled = True
        Me.LB_Analyses.Location = New System.Drawing.Point(0, 0)
        Me.LB_Analyses.MultiColumn = True
        Me.LB_Analyses.Name = "LB_Analyses"
        Me.LB_Analyses.Size = New System.Drawing.Size(620, 143)
        Me.LB_Analyses.TabIndex = 0
        '
        'Btn_AnalysisDN
        '
        Me.Btn_AnalysisDN.Location = New System.Drawing.Point(648, 90)
        Me.Btn_AnalysisDN.Name = "Btn_AnalysisDN"
        Me.Btn_AnalysisDN.Size = New System.Drawing.Size(75, 23)
        Me.Btn_AnalysisDN.TabIndex = 3
        Me.Btn_AnalysisDN.Text = "Down"
        Me.Btn_AnalysisDN.UseVisualStyleBackColor = True
        '
        'Btn_AnalysisDEL
        '
        Me.Btn_AnalysisDEL.Location = New System.Drawing.Point(648, 61)
        Me.Btn_AnalysisDEL.Name = "Btn_AnalysisDEL"
        Me.Btn_AnalysisDEL.Size = New System.Drawing.Size(75, 23)
        Me.Btn_AnalysisDEL.TabIndex = 2
        Me.Btn_AnalysisDEL.Text = "Remove"
        Me.Btn_AnalysisDEL.UseVisualStyleBackColor = True
        '
        'Btn_AnalysisUP
        '
        Me.Btn_AnalysisUP.Location = New System.Drawing.Point(648, 32)
        Me.Btn_AnalysisUP.Name = "Btn_AnalysisUP"
        Me.Btn_AnalysisUP.Size = New System.Drawing.Size(75, 23)
        Me.Btn_AnalysisUP.TabIndex = 1
        Me.Btn_AnalysisUP.Text = "Up"
        Me.Btn_AnalysisUP.UseVisualStyleBackColor = True
        '
        'TP_ContentManagement
        '
        Me.TP_ContentManagement.Controls.Add(Me.LBL_PayloadFileStatus)
        Me.TP_ContentManagement.Controls.Add(Me.Label37)
        Me.TP_ContentManagement.Controls.Add(Me.BTN_PayloadFileVerificationSelect)
        Me.TP_ContentManagement.Controls.Add(Me.LBL_SelectedPayloadFile)
        Me.TP_ContentManagement.Controls.Add(Me.Label36)
        Me.TP_ContentManagement.Controls.Add(Me.Label39)
        Me.TP_ContentManagement.Controls.Add(Me.LB_PayloadEntries)
        Me.TP_ContentManagement.Location = New System.Drawing.Point(4, 22)
        Me.TP_ContentManagement.Name = "TP_ContentManagement"
        Me.TP_ContentManagement.Padding = New System.Windows.Forms.Padding(3)
        Me.TP_ContentManagement.Size = New System.Drawing.Size(856, 689)
        Me.TP_ContentManagement.TabIndex = 4
        Me.TP_ContentManagement.Text = "Content Management"
        Me.TP_ContentManagement.UseVisualStyleBackColor = True
        '
        'LBL_PayloadFileStatus
        '
        Me.LBL_PayloadFileStatus.AutoSize = True
        Me.LBL_PayloadFileStatus.Location = New System.Drawing.Point(158, 227)
        Me.LBL_PayloadFileStatus.Name = "LBL_PayloadFileStatus"
        Me.LBL_PayloadFileStatus.Size = New System.Drawing.Size(38, 13)
        Me.LBL_PayloadFileStatus.TabIndex = 8
        Me.LBL_PayloadFileStatus.Text = "NONE"
        '
        'Label37
        '
        Me.Label37.AutoSize = True
        Me.Label37.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label37.Location = New System.Drawing.Point(72, 227)
        Me.Label37.Name = "Label37"
        Me.Label37.Size = New System.Drawing.Size(71, 13)
        Me.Label37.TabIndex = 7
        Me.Label37.Text = "File Status:"
        '
        'BTN_PayloadFileVerificationSelect
        '
        Me.BTN_PayloadFileVerificationSelect.Location = New System.Drawing.Point(74, 157)
        Me.BTN_PayloadFileVerificationSelect.Name = "BTN_PayloadFileVerificationSelect"
        Me.BTN_PayloadFileVerificationSelect.Size = New System.Drawing.Size(184, 23)
        Me.BTN_PayloadFileVerificationSelect.TabIndex = 6
        Me.BTN_PayloadFileVerificationSelect.Text = "Select Payload File..."
        Me.BTN_PayloadFileVerificationSelect.UseVisualStyleBackColor = True
        '
        'LBL_SelectedPayloadFile
        '
        Me.LBL_SelectedPayloadFile.AutoSize = True
        Me.LBL_SelectedPayloadFile.Location = New System.Drawing.Point(158, 209)
        Me.LBL_SelectedPayloadFile.Name = "LBL_SelectedPayloadFile"
        Me.LBL_SelectedPayloadFile.Size = New System.Drawing.Size(38, 13)
        Me.LBL_SelectedPayloadFile.TabIndex = 5
        Me.LBL_SelectedPayloadFile.Text = "NONE"
        '
        'Label36
        '
        Me.Label36.AutoSize = True
        Me.Label36.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label36.Location = New System.Drawing.Point(72, 209)
        Me.Label36.Name = "Label36"
        Me.Label36.Size = New System.Drawing.Size(80, 13)
        Me.Label36.TabIndex = 4
        Me.Label36.Text = "Payload File:"
        '
        'Label39
        '
        Me.Label39.AutoSize = True
        Me.Label39.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label39.Location = New System.Drawing.Point(72, 245)
        Me.Label39.Name = "Label39"
        Me.Label39.Size = New System.Drawing.Size(210, 13)
        Me.Label39.TabIndex = 2
        Me.Label39.Text = "Entries In the Selected Payload File"
        '
        'LB_PayloadEntries
        '
        Me.LB_PayloadEntries.FormattingEnabled = True
        Me.LB_PayloadEntries.Location = New System.Drawing.Point(74, 261)
        Me.LB_PayloadEntries.Name = "LB_PayloadEntries"
        Me.LB_PayloadEntries.Size = New System.Drawing.Size(644, 212)
        Me.LB_PayloadEntries.TabIndex = 1
        '
        'Label13
        '
        Me.Label13.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label13.Location = New System.Drawing.Point(6, 97)
        Me.Label13.Name = "Label13"
        Me.Label13.Size = New System.Drawing.Size(156, 23)
        Me.Label13.TabIndex = 28
        Me.Label13.Text = "Reagent Description:"
        '
        'LBL_version
        '
        Me.LBL_version.AutoSize = True
        Me.LBL_version.Location = New System.Drawing.Point(13, 75)
        Me.LBL_version.Name = "LBL_version"
        Me.LBL_version.Size = New System.Drawing.Size(113, 13)
        Me.LBL_version.TabIndex = 12
        Me.LBL_version.Text = "Version: {0}.{1}.{2}.{3}"
        '
        'Form1
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(864, 806)
        Me.Controls.Add(Me.Panel2)
        Me.Controls.Add(Me.Panel1)
        Me.Name = "Form1"
        Me.Text = "Reagent Kit RFID Layout Generator"
        Me.Panel1.ResumeLayout(False)
        Me.Panel1.PerformLayout()
        Me.Panel2.ResumeLayout(False)
        Me.TabControl1.ResumeLayout(False)
        Me.TP_Header.ResumeLayout(False)
        Me.Panel3.ResumeLayout(False)
        Me.GroupBox4.ResumeLayout(False)
        Me.GroupBox4.PerformLayout()
        Me.GroupBox8.ResumeLayout(False)
        Me.GroupBox8.PerformLayout()
        Me.GB_Volume.ResumeLayout(False)
        Me.GB_Volume.PerformLayout()
        Me.GB_Speed.ResumeLayout(False)
        Me.GB_Speed.PerformLayout()
        Me.GB_Target.ResumeLayout(False)
        Me.GB_Target.PerformLayout()
        CType(Me.UD_ReagentCycles, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.UD_ReagentUses, System.ComponentModel.ISupportInitialize).EndInit()
        Me.Panel9.ResumeLayout(False)
        Me.GroupBox3.ResumeLayout(False)
        Me.Panel11.ResumeLayout(False)
        Me.Panel12.ResumeLayout(False)
        Me.Panel12.PerformLayout()
        CType(Me.UD_CleanerUses, System.ComponentModel.ISupportInitialize).EndInit()
        Me.Panel10.ResumeLayout(False)
        Me.GroupBox2.ResumeLayout(False)
        Me.GroupBox2.PerformLayout()
        CType(Me.udPackLotNumber, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.UP_ServiceLife, System.ComponentModel.ISupportInitialize).EndInit()
        Me.TP_Analyses.ResumeLayout(False)
        Me.Panel8.ResumeLayout(False)
        Me.Panel8.PerformLayout()
        Me.GroupBox7.ResumeLayout(False)
        Me.GroupBox6.ResumeLayout(False)
        Me.GroupBox6.PerformLayout()
        CType(Me.UD_AnalysisEmissionWavelength, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.UD_AnalysisIlluminatorMS, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.UD_AnalysisIlluminatorWL, System.ComponentModel.ISupportInitialize).EndInit()
        Me.GroupBox5.ResumeLayout(False)
        Me.GroupBox5.PerformLayout()
        CType(Me.UD_AnalysisParamThreshold, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.UD_AnalysisParamSubkey, System.ComponentModel.ISupportInitialize).EndInit()
        Me.GroupBox1.ResumeLayout(False)
        Me.GroupBox1.PerformLayout()
        CType(Me.UD_AnalysisPopThreshold, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.UD_AnalysisPopSubkey, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.UD_AnalysisIndex, System.ComponentModel.ISupportInitialize).EndInit()
        Me.Panel7.ResumeLayout(False)
        Me.TP_ContentManagement.ResumeLayout(False)
        Me.TP_ContentManagement.PerformLayout()
        Me.ResumeLayout(False)

    End Sub

    Friend WithEvents Panel1 As Panel
    Friend WithEvents Btn_Generate As Button
    Friend WithEvents Btn_Reset As Button
    Friend WithEvents Lbl_ContentBytes As Label
    Friend WithEvents Label4 As Label
    Friend WithEvents Lbl_NumAnalyses As Label
    Friend WithEvents Lbl_HasReagent As Label
    Friend WithEvents Lbl_NumCleaners As Label
    Friend WithEvents Label3 As Label
    Friend WithEvents Label2 As Label
    Friend WithEvents Label1 As Label
    Friend WithEvents Panel2 As Panel
    Friend WithEvents TabControl1 As TabControl
    Friend WithEvents TP_Analyses As TabPage
    Friend WithEvents Panel8 As Panel
    Friend WithEvents GroupBox1 As GroupBox
    Friend WithEvents Btn_PopulationClear As Button
    Friend WithEvents RB_PopBelow As RadioButton
    Friend WithEvents RB_PopAtAbove As RadioButton
    Friend WithEvents UD_AnalysisPopSubkey As NumericUpDown
    Friend WithEvents Label23 As Label
    Friend WithEvents Label22 As Label
    Friend WithEvents Label21 As Label
    Friend WithEvents Btn_AnalysisIlluminatorAdd As Button
    Friend WithEvents Btn_AnalysisIlluminatorDN As Button
    Friend WithEvents Btn_AnalysisIlluminatorDEL As Button
    Friend WithEvents Btn_AnalysisIlluminatorUP As Button
    Friend WithEvents LB_AnalysisIlluminators As ListBox
    Friend WithEvents Btn_AnalysisReagentDN As Button
    Friend WithEvents Btn_AnalysisReagentDEL As Button
    Friend WithEvents Btn_AnalysisReagentUP As Button
    Friend WithEvents Btn_AnalysisReagentAdd As Button
    Friend WithEvents LB_AnalysisReagents As ListBox
    Friend WithEvents Label20 As Label
    Friend WithEvents Label18 As Label
    Friend WithEvents UD_AnalysisIlluminatorMS As NumericUpDown
    Friend WithEvents UD_AnalysisIlluminatorWL As NumericUpDown
    Friend WithEvents Label17 As Label
    Friend WithEvents Label15 As Label
    Friend WithEvents TB_AnalysisDesc As TextBox
    Friend WithEvents Label16 As Label
    Friend WithEvents UD_AnalysisIndex As NumericUpDown
    Friend WithEvents Label19 As Label
    Friend WithEvents Btn_AddAnalysis As Button
    Friend WithEvents Panel7 As Panel
    Friend WithEvents Btn_AnalysisDN As Button
    Friend WithEvents Btn_AnalysisDEL As Button
    Friend WithEvents Btn_AnalysisUP As Button
    Friend WithEvents LB_Analyses As ListBox
    Friend WithEvents TP_Header As TabPage
    Friend WithEvents Panel10 As Panel
    Friend WithEvents Panel9 As Panel
    Friend WithEvents Panel3 As Panel
    Friend WithEvents GroupBox2 As GroupBox
    Friend WithEvents UP_ServiceLife As NumericUpDown
    Friend WithEvents TB_PackPN As TextBox
    Friend WithEvents Label6 As Label
    Friend WithEvents Label5 As Label
    Friend WithEvents GroupBox3 As GroupBox
    Friend WithEvents Panel12 As Panel
    Friend WithEvents Label8 As Label
    Friend WithEvents Label9 As Label
    Friend WithEvents UD_CleanerUses As NumericUpDown
    Friend WithEvents Label7 As Label
    Friend WithEvents Label24 As Label
    Friend WithEvents Btn_AddCleaner As Button
    Friend WithEvents Panel11 As Panel
    Friend WithEvents LB_Cleaners As ListBox
    Friend WithEvents Btn_CleanerUP As Button
    Friend WithEvents Btn_CleanerDEL As Button
    Friend WithEvents Btn_CleanerDN As Button
    Friend WithEvents GroupBox4 As GroupBox
    Friend WithEvents CB_IncludeReagent As CheckBox
    Friend WithEvents Label12 As Label
    Friend WithEvents Label11 As Label
    Friend WithEvents Label10 As Label
    Friend WithEvents UD_ReagentUses As NumericUpDown
    Friend WithEvents Label13 As Label
    Friend WithEvents Label14 As Label
    Friend WithEvents UD_ReagentCycles As NumericUpDown
    Friend WithEvents Label26 As Label
    Friend WithEvents Label25 As Label
    Friend WithEvents GroupBox5 As GroupBox
    Friend WithEvents RB_AnalysisParamBelow As RadioButton
    Friend WithEvents RB_AnalysisParamAtAbove As RadioButton
    Friend WithEvents UD_AnalysisParamSubkey As NumericUpDown
    Friend WithEvents Label27 As Label
    Friend WithEvents Label28 As Label
    Friend WithEvents Label29 As Label
    Friend WithEvents Btn_AnalysisParamDN As Button
    Friend WithEvents Btn_AnalysisParamDEL As Button
    Friend WithEvents Btn_AnalysisParamUP As Button
    Friend WithEvents LB_AnalysisParams As ListBox
    Friend WithEvents Btn_AddAnalysisParam As Button
    Friend WithEvents GroupBox7 As GroupBox
    Friend WithEvents GroupBox6 As GroupBox
    Friend WithEvents UD_AnalysisParamThreshold As NumericUpDown
    Friend WithEvents UD_AnalysisPopThreshold As NumericUpDown
    Friend WithEvents lblPackLotNo As Label
    Friend WithEvents lblPackExpirationDate As Label
    Friend WithEvents udPackLotNumber As NumericUpDown
    Friend WithEvents dtpPackExpiration As DateTimePicker
    Friend WithEvents cbFullLayout As CheckBox
    Friend WithEvents Label30 As Label
    Friend WithEvents CB_AnalysisPopParameter As ComboBox
    Friend WithEvents Lbl_AnalysisPopulationKey As Label
    Friend WithEvents Label31 As Label
    Friend WithEvents CB_AnalysisParameter As ComboBox
    Friend WithEvents LBL_AnalysisParameterKey As Label
    Friend WithEvents CB_CleanerList As ComboBox
    Friend WithEvents Label32 As Label
    Friend WithEvents LBL_CleanerPN As Label
    Friend WithEvents LBL_CleanerIndex As Label
    Friend WithEvents LBL_CleanerDescription As Label
    Friend WithEvents CB_AnalysisReagent As ComboBox
    Friend WithEvents TB_GTIN As TextBox
    Friend WithEvents Label34 As Label
    Friend WithEvents CB_PackReagent As ComboBox
    Friend WithEvents LBL_PackReagentDesc As Label
    Friend WithEvents LBL_PackReagentPN As Label
    Friend WithEvents Label35 As Label
    Friend WithEvents Lbl_ContentBytesRemaining As Label
    Friend WithEvents GroupBox8 As GroupBox
    Friend WithEvents Label33 As Label
    Friend WithEvents CB_RgtCleanerList As ComboBox
    Friend WithEvents Btn_ClnStepAdd As Button
    Friend WithEvents GB_Volume As GroupBox
    Friend WithEvents RB_Cln1200uL As RadioButton
    Friend WithEvents RB_Cln600uL As RadioButton
    Friend WithEvents Btn_ClnStepUP_P As Button
    Friend WithEvents CB_AirFlush As CheckBox
    Friend WithEvents Btn_ClnStepDEL_P As Button
    Friend WithEvents Btn_ClnStepDN_P As Button
    Friend WithEvents GB_Speed As GroupBox
    Friend WithEvents RB_Cln250uLs As RadioButton
    Friend WithEvents RB_Cln100uLs As RadioButton
    Friend WithEvents RB_Cln60uLs As RadioButton
    Friend WithEvents RB_Cln30uLs As RadioButton
    Friend WithEvents CB_BackFlush As CheckBox
    Friend WithEvents LB_PrimaryCleaningSteps As ListBox
    Friend WithEvents GB_Target As GroupBox
    Friend WithEvents RB_ClnBoth As RadioButton
    Friend WithEvents RB_ClnFlowCell As RadioButton
    Friend WithEvents RB_ClnSampleTube As RadioButton
    Friend WithEvents CB_ClnPrimary As CheckBox
    Friend WithEvents Btn_ClnStepUP_S As Button
    Friend WithEvents Btn_ClnStepDEL_S As Button
    Friend WithEvents Btn_ClnStepDN_S As Button
    Friend WithEvents LB_SecondaryCleaningSteps As ListBox
    Friend WithEvents TP_ContentManagement As TabPage
    Friend WithEvents Label39 As Label
    Friend WithEvents LB_PayloadEntries As ListBox
    Friend WithEvents CB_ContainerType As ComboBox
    Friend WithEvents Label40 As Label
    Friend WithEvents LBL_PayloadFileStatus As Label
    Friend WithEvents Label37 As Label
    Friend WithEvents BTN_PayloadFileVerificationSelect As Button
    Friend WithEvents LBL_SelectedPayloadFile As Label
    Friend WithEvents Label36 As Label
    Friend WithEvents UD_AnalysisEmissionWavelength As NumericUpDown
    Friend WithEvents Label41 As Label
    Friend WithEvents LBL_version As Label
End Class
