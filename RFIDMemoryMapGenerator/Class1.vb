
Public Class AddressMap
    Shared Function ToAddressMap(ByRef RegisterNumber As Integer, ByRef ByteNumber As Integer, ByVal Bytes As List(Of Byte), ByVal isASCII As Boolean) As String
        Dim s As String = ""

        Dim isFirst As Boolean = True


        For Each b In Bytes
            Dim sb As String = ""

            ' Register
            If isFirst Then
                sb = RegisterNumber.ToString
            End If
            sb += ","

            ' Offset DEC
            sb += ByteNumber.ToString + ","

            ' Offset HEX
            sb += ByteNumber.ToString("X2") + ","

            ' Contents
            sb += "0x" + b.ToString("X2")
            If (isASCII) Then
                If b = 0 Then
                    sb += " - ASCII NUL"
                Else
                    sb += " - ASCII """ + Chr(b) + """"
                End If

            End If
            sb += ","

            'Size
            If isFirst Then
                sb += Bytes.Count.ToString + " Bytes"
                isFirst = False
            End If
            sb += vbCrLf

            s += sb

            ' Next byte
            ByteNumber += 1
        Next

        'Next Register
        RegisterNumber += 1

        Return s
    End Function

    Shared Function ToModuleMap(ByRef RegisterNumber As Integer, ByVal Name As String, ByVal Value As String, Optional ByVal M As String = "") As String
        Dim s As String = ""

        ' Module
        s += M + ","

        ' Register number + Name
        s += "(" + RegisterNumber.ToString + ") " + Name + ","

        ' Value
        s += Value + vbCrLf

        RegisterNumber += 1
        Return s
    End Function

End Class



Public Class LayoutHeader

    Private pack_pn As String
    Private pack_exp As UInt32
    Private service_life As UInt16
    Private in_service_date As UInt32
    Private lot_number As UInt32

    Public Sub New()
        pack_pn = ""
        pack_exp = 0
        service_life = 0
        in_service_date = 0
        lot_number = 0
    End Sub

    Public Sub Clear()
        pack_pn = ""
        pack_exp = 0
        service_life = 0
        in_service_date = 0
        lot_number = 0
    End Sub

    Property PackPN As String
        Get
            If pack_pn.Length > 6 Then
                Return pack_pn.Substring(0, 6)
            Else
                Return pack_pn
            End If
        End Get
        Set(value As String)
            If value.Length > 6 Then
                pack_pn = value.Substring(0, 6)
            Else
                pack_pn = value
            End If

        End Set
    End Property

    Property Expiration As UInt32
        Get
            Return pack_exp
        End Get
        Set(value As UInt32)
            pack_exp = value
        End Set
    End Property
    Public Sub SetExpiration(ByVal d As Date)
        Dim epoch As Date
        epoch = #1/1/1970#

        pack_exp = DateDiff(DateInterval.Day, epoch, d)
    End Sub

    Property ServiceLife As UInt16
        Get
            Return service_life
        End Get
        Set(value As UInt16)
            service_life = value
        End Set
    End Property

    Property InServiceDate As Date
        Get
            Dim epoch As Date
            epoch = #1/1/1970#

            Return epoch.AddDays(in_service_date)
        End Get
        Set(value As Date)
            Dim epoch As Date
            epoch = #1/1/1970#

            in_service_date = DateDiff(DateInterval.Day, epoch, value)
        End Set
    End Property
    Property LotNumber As UInt32
        Get
            Return lot_number
        End Get
        Set(value As UInt32)
            lot_number = value
        End Set
    End Property

    ReadOnly Property Size As Integer
        Get
            Return (7 + 4 + 2 + 4 + 4)
        End Get
    End Property

    Public Function ToByteList() As List(Of Byte)
        Dim l As New List(Of Byte)

        Dim i As Integer = 0

        ' PN: 6+null
        For i = 0 To 6 Step 1
            If i >= PackPN.Length Then
                l.Add(0)
            Else
                l.Add(Convert.ToByte(PackPN.ElementAt(i)))
            End If
        Next


        ' Expiration: 4
        Dim bytes_E As Byte() = BitConverter.GetBytes(Expiration)
        l.AddRange(bytes_E)

        ' Service life: 2
        Dim bytes_SL As Byte() = BitConverter.GetBytes(ServiceLife)
        l.AddRange(bytes_SL)

        ' in service date: 4
        Dim bytes_SD As Byte() = BitConverter.GetBytes(in_service_date)
        l.AddRange(bytes_SD)

        ' Lot Number: 4
        Dim bytes_L As Byte() = BitConverter.GetBytes(LotNumber)
        l.AddRange(bytes_L)

        Return l
    End Function

    Public Function AddToLayoutString(ByRef RegisterNumber As Integer, ByRef ByteOffset As Integer, ByRef Output As String) As Boolean

        Dim Bytes As New List(Of Byte)
        Bytes = ToByteList()

        ' Pack PN
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(0, 7), True)
        ' Pack EXP
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(7, 4), False)
        ' Service Life
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(11, 2), False)
        ' In-service date
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(13, 4), False)
        ' Lot number
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(17, 4), False)

        Return True
    End Function
    Public Function AddToRegisterMapString(ByRef RegisterNumber As Integer, ByRef Output As String) As Boolean

        Dim v As String = "ASCII """ + pack_pn + """"
        Output += AddressMap.ToModuleMap(RegisterNumber, "Pack Part Number", v)

        v = "UINT32 - Expiration date of the pack as days since 1970-01-01 per MFG data"
        Output += AddressMap.ToModuleMap(RegisterNumber, "Pack Expiration", v)

        v = "UINT16 - " + service_life.ToString
        Output += AddressMap.ToModuleMap(RegisterNumber, "Service Life", v)

        v = "UINT32 - " + in_service_date.ToString
        Output += AddressMap.ToModuleMap(RegisterNumber, "In Service Date", v)

        v = "UINT32 - Lot Number per MFG data"
        Output += AddressMap.ToModuleMap(RegisterNumber, "Pack Lot Number", v)

        Return True
    End Function
End Class

Public Class ReagentCleaningStep
    ' Bit Layout:
    '  0: Primary (0) / Secondary (1)
    '  1: Volume
    '  2-3: Speed
    '  4: Back flush?
    '  5-6: Cleaning Target
    '  7: Air flush?
    Public Enum CleaningVolume
        v_600 = &H0
        v_1200 = &H2
    End Enum
    Public Enum CleaningSpeed
        s_30 = &H0
        s_60 = &H4
        s_100 = &H8
        s_250 = &HC
    End Enum
    Public Enum CleaningBackFlush
        bf_off = &H0
        bf_on = &H10
    End Enum
    Public Enum CleaningTarget
        t_Tube = &H20
        t_Flow = &H40
        t_Both = &H60
    End Enum
    Public Enum CleaningAirFlush
        af_off = &H0
        af_on = &H80
    End Enum

    Private reagent_idx As UInt16
    Private reagent_label As String
    Private vol As CleaningVolume
    Private spd As CleaningSpeed
    Private back_flush As CleaningBackFlush
    Private tgt As CleaningTarget
    Private air_flush As CleaningAirFlush

    Private primary_ As Boolean

    Public Sub New()
        reagent_idx = 0
        vol = CleaningVolume.v_600
        spd = CleaningSpeed.s_30
        back_flush = CleaningBackFlush.bf_off
        Target = CleaningTarget.t_Tube
        air_flush = CleaningAirFlush.af_off

        primary_ = True
    End Sub

    Property Reagent As UInt16
        Get
            Return reagent_idx
        End Get
        Set(value As UInt16)
            reagent_idx = value
        End Set
    End Property
    Property ReagentLabel As String
        Get
            Return reagent_label
        End Get
        Set(value As String)
            reagent_label = value
        End Set
    End Property
    Property Volume As CleaningVolume
        Get
            Return vol
        End Get
        Set(value As CleaningVolume)
            vol = value
        End Set
    End Property
    Property Speed As CleaningSpeed
        Get
            Return spd
        End Get
        Set(value As CleaningSpeed)
            spd = value
        End Set
    End Property
    Property Target As CleaningTarget
        Get
            Return tgt
        End Get
        Set(value As CleaningTarget)
            tgt = value
        End Set
    End Property
    Property BackFlush As Boolean
        Get
            Return (back_flush = CleaningBackFlush.bf_on)
        End Get
        Set(value As Boolean)
            If value Then
                back_flush = CleaningBackFlush.bf_on
            Else
                back_flush = CleaningBackFlush.bf_off
            End If
        End Set
    End Property
    Property AirFlush As Boolean
        Get
            Return air_flush = CleaningAirFlush.af_on
        End Get
        Set(value As Boolean)
            If value Then
                air_flush = CleaningAirFlush.af_on
            Else
                air_flush = CleaningAirFlush.af_off
            End If
        End Set
    End Property
    Property Primary As Boolean
        Get
            Return primary_
        End Get
        Set(value As Boolean)
            primary_ = value
        End Set
    End Property

    ReadOnly Property Instruction As Byte
        Get
            Dim b As Byte = 0

            If Not primary_ Then
                b = 1
            End If

            b = b Or vol
            b = b Or spd
            b = b Or back_flush
            b = b Or tgt
            b = b Or air_flush

            Return b
        End Get
    End Property

    Public Overrides Function ToString() As String
        Dim s As String = ""
        If Not Primary Then
            s = "(S) "
        End If

        s += "Reagent: " + reagent_label + " - "

        Select Case vol
            Case CleaningVolume.v_600
                s += "600uL - "
            Case CleaningVolume.v_1200
                s += "1200uL - "
        End Select

        Select Case spd
            Case CleaningSpeed.s_30
                s += "30 uL/s - "
            Case CleaningSpeed.s_60
                s += "60 uL/s - "
            Case CleaningSpeed.s_100
                s += "100 uL/s - "
            Case CleaningSpeed.s_250
                s += "250 uL/s - "
        End Select

        Select Case tgt
            Case CleaningTarget.t_Tube
                s += "Tube - "
            Case CleaningTarget.t_Flow
                s += "FlowCell - "
            Case CleaningTarget.t_Both
                s += "Both - "
        End Select

        If AirFlush Then
            s += "Air - "
        End If
        If BackFlush Then
            s += "Back - "
        End If

        Return s
    End Function

    Public Function ToByteList() As List(Of Byte)
        Dim l As New List(Of Byte)


        ' Reagent Index: 2
        Dim bytes_RI As Byte() = BitConverter.GetBytes(reagent_idx)
        For Each b In bytes_RI
            l.Add(b)
        Next

        ' Instructions: 1
        l.Add(Instruction)

        Return l
    End Function

    Public Function AddToLayoutString(ByRef RegisterNumber As Integer, ByRef ByteOffset As Integer, ByRef Output As String) As Boolean

        Dim Bytes As New List(Of Byte)
        Bytes = ToByteList()

        Dim b As Integer = 0

        ' REAGENT INDEX
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(b, 2), False)
        b += 2

        ' INSTRUCTION
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(b, 1), False)

        Return True
    End Function
    Public Function AddToRegisterMapString(ByRef RegisterNumber As Integer, ByRef Output As String, ByVal asCleaner As Boolean) As Boolean

        Dim v As String = "UINT16 - " + reagent_idx.ToString
        Output += AddressMap.ToModuleMap(RegisterNumber, "Cleaning Reagent Index", v)

        v = "UINT8 - " + Instruction.ToString
        Output += AddressMap.ToModuleMap(RegisterNumber, "Instruction Byte", v)
        Return True
    End Function
End Class

Public Class LayoutReagent
    Private idx As UInt16
    Private desc As String
    Private mix_cycles As Byte
    Private total_uses As UInt16
    Private remaining_uses As UInt16
    Private part_number As String

    Private primary_cleaning_instructions As List(Of ReagentCleaningStep)
    Private secondary_cleaning_instructions As List(Of ReagentCleaningStep)

    Public Sub New()
        idx = 0
        desc = ""
        mix_cycles = 0
        total_uses = 0
        remaining_uses = 0
        part_number = ""

        primary_cleaning_instructions = New List(Of ReagentCleaningStep)
        secondary_cleaning_instructions = New List(Of ReagentCleaningStep)
    End Sub
    Property Index As UInt16
        Get
            Return idx
        End Get
        Set(value As UInt16)
            idx = value
        End Set
    End Property
    Property Description As String
        Get
            Return desc

        End Get
        Set(value As String)
            desc = value
        End Set
    End Property
    Property MixingCycles As Byte
        Get
            Return mix_cycles
        End Get
        Set(value As Byte)
            mix_cycles = value
        End Set
    End Property
    Property TotalUses As UInt16
        Get
            Return total_uses
        End Get
        Set(value As UInt16)
            total_uses = value
            remaining_uses = total_uses
        End Set
    End Property
    Property RemainingUses As UInt16
        Get
            Return remaining_uses
        End Get
        Set(value As UInt16)
            remaining_uses = value
        End Set
    End Property
    Property PartNumber As String
        Get
            If part_number.Length > 6 Then
                Return part_number.Substring(0, 6)
            Else
                Return part_number
            End If

        End Get
        Set(value As String)
            If value.Length > 6 Then
                part_number = value.Substring(0, 6)
            Else
                part_number = value
            End If

        End Set
    End Property

    Property PrimaryCleaningSteps As List(Of ReagentCleaningStep)
        Get
            Return primary_cleaning_instructions
        End Get
        Set(value As List(Of ReagentCleaningStep))
            primary_cleaning_instructions = value
        End Set
    End Property

    Property SecondaryCleaningSteps As List(Of ReagentCleaningStep)
        Get
            Return secondary_cleaning_instructions
        End Get
        Set(value As List(Of ReagentCleaningStep))
            secondary_cleaning_instructions = value
        End Set
    End Property


    '''
    '''    Index (2)
    '''    Description (strlen + 1)
    ''' R  Mixing Cycles (1)
    '''    Total uses (2)
    '''    Remaining uses (2)
    '''    PN (7)
    '''    Num Cleaning Steps (1)
    '''      Cleaning steps (3 x Num Cleaning Steps)
    '''
    Public Function Size(ByVal AsCleaner As Boolean) As Integer
        Dim sz As Integer = 2 + 1 + 2 + 2 + 7 ' index, null-terminator, total uses, remaning uses, pn
        sz += Description.Length

        If Not AsCleaner Then
            sz += 1 + 1 ' mixing cycles, num cleaning steps
            sz += 3 * PrimaryCleaningSteps.Count ' cleaning steps
            sz += 3 * SecondaryCleaningSteps.Count
        End If
        Return sz

    End Function

    Public Sub Clear()
        idx = 0
        desc = ""
        mix_cycles = 0
        total_uses = 0
        part_number = ""
        primary_cleaning_instructions.Clear()
        secondary_cleaning_instructions.Clear()
    End Sub
    Public Function ToByteList(ByVal AsCleaner As Boolean) As List(Of Byte)
        Dim l As New List(Of Byte)

        Dim i As Integer = 0

        ' Index 2
        Dim bytes_I As Byte() = BitConverter.GetBytes(Index)
        For Each b In bytes_I
            l.Add(b)
        Next


        'Desc +null
        For Each c As Char In desc
            l.Add(Convert.ToByte(c))
        Next
        l.Add(0)

        If Not AsCleaner Then
            'R: Cycles: 1
            l.Add(MixingCycles)
        End If

        ' Total Uses: 2
        Dim bytes_TU As Byte() = BitConverter.GetBytes(TotalUses)
        For Each b In bytes_TU
            l.Add(b)
        Next

        ' Remaining Uses: 2
        Dim bytes_RU As Byte() = BitConverter.GetBytes(TotalUses)
        For Each b In bytes_RU
            l.Add(b)
        Next


        ' Part Num: 6 + NULL
        For i = 0 To 6 Step 1
            If i >= PartNumber.Length Then
                l.Add(0)
            Else
                l.Add(Convert.ToByte(PartNumber.ElementAt(i)))
            End If
        Next

        ' R: Cleaning instructions
        If Not AsCleaner Then
            Dim bcln As Byte = primary_cleaning_instructions.Count + secondary_cleaning_instructions.Count
            l.Add(bcln)
            For Each cln As ReagentCleaningStep In primary_cleaning_instructions
                cln.Primary = True
                l.AddRange(cln.ToByteList)
            Next
            For Each cln As ReagentCleaningStep In secondary_cleaning_instructions
                cln.Primary = False
                l.AddRange(cln.ToByteList)
            Next
        End If

        Return l
    End Function

    Public Function AddToLayoutString(ByRef RegisterNumber As Integer, ByRef ByteOffset As Integer, ByRef Output As String, ByVal asCleaner As Boolean) As Boolean

        Dim Bytes As New List(Of Byte)
        Bytes = ToByteList(asCleaner)

        Dim b As Integer = 0

        ' INDEX
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(b, 2), False)
        b += 2

        ' Description
        Dim dl As Integer = desc.Length + 1
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(b, dl), True)
        b += dl

        If Not asCleaner Then
            ' !Cleaner: Mix Cyles
            Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(b, 1), False)
            b += 1
        End If

        ' Total Uses
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(b, 2), False)
        b += 2
        ' Remaining Uses
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(b, 2), False)
        b += 2
        ' Part Number
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(b, 7), True)
        b += 7

        If Not asCleaner Then
            ' !Cleaner: Cleaning instructions
            Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(b, 1), False)
            b += 1

            For Each inst As ReagentCleaningStep In primary_cleaning_instructions
                inst.AddToLayoutString(RegisterNumber, ByteOffset, Output)
            Next
            For Each inst As ReagentCleaningStep In secondary_cleaning_instructions
                inst.AddToLayoutString(RegisterNumber, ByteOffset, Output)
            Next
        End If
        Return True
    End Function
    Public Function AddToRegisterMapString(ByRef RegisterNumber As Integer, ByRef Output As String, ByVal asCleaner As Boolean) As Boolean

        Dim v As String = "UINT16 - " + idx.ToString
        Output += AddressMap.ToModuleMap(RegisterNumber, "Fluid Index", v)

        v = "ASCII """ + desc + """"
        Output += AddressMap.ToModuleMap(RegisterNumber, "Fluid Description", v)

        If Not asCleaner Then
            v = "UINT8 - " + mix_cycles.ToString
            Output += AddressMap.ToModuleMap(RegisterNumber, "Mixing Cycles", v)
        End If

        v = "UINT16 - " + TotalUses.ToString
        Output += AddressMap.ToModuleMap(RegisterNumber, "Total Uses", v)

        v = "UINT16 - " + RemainingUses.ToString
        Output += AddressMap.ToModuleMap(RegisterNumber, "Remaining Uses", v)

        v = "ASCII """ + part_number + """"
        Output += AddressMap.ToModuleMap(RegisterNumber, "Fluid Part Number", v)

        If Not asCleaner Then
            Dim cnt As Integer = primary_cleaning_instructions.Count + secondary_cleaning_instructions.Count
            v = "UINT8 - " + cnt.ToString
            Output += AddressMap.ToModuleMap(RegisterNumber, "Cleaning Instructions Count", v)

            For Each cln As ReagentCleaningStep In primary_cleaning_instructions
                v = "UINT16 - " + cln.Reagent.ToString
                Output += AddressMap.ToModuleMap(RegisterNumber, "Cleaning Reagent Index for step (P)", v)

                v = "UINT8 - " + cln.Instruction.ToString
                Output += AddressMap.ToModuleMap(RegisterNumber, "Cleaning Instruction for step (P)", v)
            Next
            For Each cln As ReagentCleaningStep In secondary_cleaning_instructions
                v = "UINT16 - " + cln.Reagent.ToString
                Output += AddressMap.ToModuleMap(RegisterNumber, "Cleaning Reagent Index for step (S)", v)

                v = "UINT8 - " + cln.Instruction.ToString
                Output += AddressMap.ToModuleMap(RegisterNumber, "Cleaning Instruction for step (S)", v)
            Next
        End If

        Return True
    End Function

    Public Sub RemovePCleanStep(ByVal idx As Integer)
        If idx >= primary_cleaning_instructions.Count() Then
            Return
        End If

        primary_cleaning_instructions.RemoveAt(idx)
    End Sub
    Public Sub MovePCleanStepUp(ByVal idx As Integer)
        If (idx >= primary_cleaning_instructions.Count Or idx = 0) Then
            Return
        End If

        Dim atminus1 As ReagentCleaningStep = primary_cleaning_instructions(idx - 1)
        primary_cleaning_instructions(idx - 1) = primary_cleaning_instructions(idx)
        primary_cleaning_instructions(idx) = atminus1
    End Sub
    Public Sub MovePCleanStepDown(ByVal idx As Integer)
        If (idx >= (primary_cleaning_instructions.Count - 1)) Then
            Return
        End If

        Dim atplus1 As ReagentCleaningStep = primary_cleaning_instructions(idx + 1)
        primary_cleaning_instructions(idx + 1) = primary_cleaning_instructions(idx)
        primary_cleaning_instructions(idx) = atplus1
    End Sub

    Public Sub RemoveSCleanStep(ByVal idx As Integer)
        If idx >= secondary_cleaning_instructions.Count() Then
            Return
        End If

        secondary_cleaning_instructions.RemoveAt(idx)
    End Sub
    Public Sub MoveSCleanStepUp(ByVal idx As Integer)
        If (idx >= secondary_cleaning_instructions.Count Or idx = 0) Then
            Return
        End If

        Dim atminus1 As ReagentCleaningStep = secondary_cleaning_instructions(idx - 1)
        secondary_cleaning_instructions(idx - 1) = secondary_cleaning_instructions(idx)
        secondary_cleaning_instructions(idx) = atminus1
    End Sub
    Public Sub MoveSCleanStepDown(ByVal idx As Integer)
        If (idx >= (secondary_cleaning_instructions.Count - 1)) Then
            Return
        End If

        Dim atplus1 As ReagentCleaningStep = secondary_cleaning_instructions(idx + 1)
        secondary_cleaning_instructions(idx + 1) = secondary_cleaning_instructions(idx)
        secondary_cleaning_instructions(idx) = atplus1
    End Sub
End Class


Public Class LayoutAnalysisParameter
    Private key_ As UInt16
    Private sub_key As UInt16
    Private threshold_ As Single
    Private polarity_above As Boolean
    Public Sub New()
        key_ = &HFFFF
        sub_key = 0
        threshold_ = 0.0
        polarity_above = False
    End Sub
    Property Key As UInt16
        Get
            Return key_
        End Get
        Set(value As UInt16)
            key_ = value
        End Set
    End Property
    Property SubKey As UInt16
        Get
            Return sub_key
        End Get
        Set(value As UInt16)
            sub_key = value
        End Set
    End Property
    Property Threshold As Single
        Get
            Return threshold_
        End Get
        Set(value As Single)
            threshold_ = value
        End Set
    End Property
    Property AtAbove As Boolean
        Get
            Return polarity_above
        End Get
        Set(value As Boolean)
            polarity_above = value
        End Set
    End Property
    ReadOnly Property Size As Integer
        Get
            Dim sz As Integer = 2

            If key_ <> &HFFFF And key_ <> &H0 Then
                sz += 2 + 4 + 1
            End If
            Return sz
        End Get
    End Property
    Public Sub Clear()
        key_ = &HFFFF
        sub_key = 0
        threshold_ = 0.0
        polarity_above = False
    End Sub

    Public Function ToByteList()
        Dim l As New List(Of Byte)

        ' Key 2
        Dim bytes_K As Byte() = BitConverter.GetBytes(Key)
        For Each b In bytes_K
            l.Add(b)
        Next

        If key_ <> &HFFFF And key_ <> &H0 Then
            ' SKey 2
            Dim bytes_SK As Byte() = BitConverter.GetBytes(SubKey)
            For Each b In bytes_SK
                l.Add(b)
            Next

            ' Param 4
            Dim bytes_T As Byte() = BitConverter.GetBytes(Threshold)
            For Each b In bytes_T
                l.Add(b)
            Next
            ' Above 1
            If AtAbove Then
                l.Add(1)
            Else
                l.Add(0)
            End If
        End If


        Return l
    End Function

    Public Function AddToLayoutString(ByRef RegisterNumber As Integer, ByRef ByteOffset As Integer, ByRef Output As String) As Boolean

        Dim Bytes As New List(Of Byte)
        Bytes = ToByteList()

        ' Key
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(0, 2), False)
        If key_ <> &HFFFF And key_ <> &H0 Then
            ' Subkey
            Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(2, 2), False)
            ' Value
            Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(4, 4), False)
            ' Above/Below
            Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(8, 1), False)
        End If

        Return True
    End Function
    Public Function AddToRegisterMapString(ByRef RegisterNumber As Integer, ByRef Output As String) As Boolean

        Dim v As String = "UINT16 - " + key_.ToString()
        Output += AddressMap.ToModuleMap(RegisterNumber, "Parameter Key", v)
        If key_ <> &HFFFF And key_ <> &H0 Then
            v = "UINT16 - " + sub_key.ToString()
            Output += AddressMap.ToModuleMap(RegisterNumber, "Parameter Subkey", v)

            v = "FLOAT32 - " + threshold_.ToString()
            Output += AddressMap.ToModuleMap(RegisterNumber, "Parameter Threshold", v)

            v = "UINT8 - " + polarity_above.ToString()
            Output += AddressMap.ToModuleMap(RegisterNumber, "Parameter Polarity", v)
        End If
        Return True
    End Function

End Class

Public Class FLIlluminator
    Private illum_wl As UInt16
    Private emission_wl As UInt16
    Private exposure As UInt16
    Public Sub New()
        illum_wl = 0
        emission_wl = 0
        exposure = 0
    End Sub
    Property Illumination As UInt16
        Get
            Return illum_wl
        End Get
        Set(value As UInt16)
            illum_wl = value
        End Set
    End Property

    Property Emission As UInt16
        Get
            Return emission_wl
        End Get
        Set(value As UInt16)
            emission_wl = value
        End Set
    End Property
    Property ExposureTime As UInt16
        Get
            Return exposure
        End Get
        Set(value As UInt16)
            exposure = value
        End Set
    End Property
    ReadOnly Property Size As Integer
        Get
            Return 2 + 2 + 2
        End Get
    End Property

    Public Function ToByteList() As List(Of Byte)
        Dim l As New List(Of Byte)

        Dim bytes_I As Byte() = BitConverter.GetBytes(Illumination)
        For Each b In bytes_I
            l.Add(b)
        Next

        Dim bytes_E As Byte() = BitConverter.GetBytes(Emission)
        For Each b In bytes_E
            l.Add(b)
        Next

        Dim bytes_X As Byte() = BitConverter.GetBytes(ExposureTime)
        For Each b In bytes_X
            l.Add(b)
        Next

        Return l
    End Function

    Public Function AddToLayoutString(ByRef RegisterNumber As Integer, ByRef ByteOffset As Integer, ByRef Output As String) As Boolean

        Dim Bytes As New List(Of Byte)
        Bytes = ToByteList()

        ' Illuminator WL
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(0, 2), False)

        ' Emission WL
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(2, 2), False)

        ' Exposure
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(4, 2), False)

        Return True
    End Function
    Public Function AddToRegisterMapString(ByRef RegisterNumber As Integer, ByRef Output As String) As Boolean

        Dim v As String = "UINT16 - " + illum_wl.ToString()
        Output += AddressMap.ToModuleMap(RegisterNumber, "Illuminator WL", v)

        v = "UINT16 - " + emission_wl.ToString()
        Output += AddressMap.ToModuleMap(RegisterNumber, "Emission WL", v)

        v = "UINT16 - " + exposure.ToString()
        Output += AddressMap.ToModuleMap(RegisterNumber, "Illuminator Exp", v)

        Return True
    End Function
End Class


Public Class LayoutAnalysis
    Private idx As UInt16
    Private desc As String
    Private reagents As List(Of UInt16)
    Private fl_illum As List(Of FLIlluminator)
    Private pop_key As LayoutAnalysisParameter
    Private params As List(Of LayoutAnalysisParameter)

    Sub New()
        idx = 0
        desc = ""
        reagents = New List(Of UShort)
        fl_illum = New List(Of FLIlluminator)
        pop_key = New LayoutAnalysisParameter
        params = New List(Of LayoutAnalysisParameter)
    End Sub

    Property Index As UInt16
        Get
            Return idx
        End Get
        Set(value As UInt16)
            idx = value
        End Set
    End Property
    Property Description As String
        Get
            Return desc
        End Get
        Set(value As String)
            desc = value
        End Set
    End Property
    Property Reagent As List(Of UInt16)
        Get
            Return reagents
        End Get
        Set(value As List(Of UInt16))
            reagents = value
        End Set
    End Property
    Property Illuminator As List(Of FLIlluminator)
        Get
            Return fl_illum
        End Get
        Set(value As List(Of FLIlluminator))
            fl_illum = value
        End Set
    End Property
    Property PopulationKey As LayoutAnalysisParameter
        Get
            Return pop_key
        End Get
        Set(value As LayoutAnalysisParameter)
            pop_key = value
        End Set
    End Property
    Property Parameters As List(Of LayoutAnalysisParameter)
        Get
            Return params
        End Get
        Set(value As List(Of LayoutAnalysisParameter))
            params = value
        End Set
    End Property

    ReadOnly Property Size As Integer
        Get
            Dim SZ As Integer
            Dim i As Integer

            SZ = 2 ' Index
            SZ += desc.Length + 1 ' Description + Null

            SZ += 1
            For i = 1 To reagents.Count Step 1
                SZ += 2
            Next

            SZ += 1
            For Each FL In fl_illum
                SZ += FL.Size
            Next

            SZ += pop_key.Size

            SZ += 1
            For Each AP In params
                SZ += AP.Size
            Next

            Return SZ
        End Get
    End Property

    Public Sub AddParam(ByVal p As LayoutAnalysisParameter)
        Parameters.Add(p)
    End Sub
    Public Sub AddIlluminator(ByVal FL As FLIlluminator)
        Illuminator.Add(FL)
    End Sub
    Public Sub AddReagent(ByVal r As UInt16)
        Reagent.Add(r)
    End Sub

    Public Function NumReagents() As Integer
        Return reagents.Count()
    End Function
    Public Sub RemoveReagent(ByVal idx As Integer)
        If idx >= reagents.Count() Then
            Return
        End If

        reagents.RemoveAt(idx)
    End Sub
    Public Sub MoveReagentUp(ByVal idx As Integer)
        If (idx >= reagents.Count Or idx = 0) Then
            Return
        End If

        Dim atminus1 As UInt16 = reagents(idx - 1)
        reagents(idx - 1) = reagents(idx)
        reagents(idx) = atminus1
    End Sub
    Public Sub MoveReagentDown(ByVal idx As Integer)
        If (idx >= (reagents.Count - 1)) Then
            Return
        End If

        Dim atplus1 As UInt16 = reagents(idx + 1)
        reagents(idx + 1) = reagents(idx)
        reagents(idx) = atplus1
    End Sub

    Public Function NumIlluminators() As Integer
        Return fl_illum.Count()
    End Function
    Public Sub RemoveIlluminator(ByVal idx As Integer)
        If idx >= fl_illum.Count() Then
            Return
        End If

        fl_illum.RemoveAt(idx)
    End Sub
    Public Sub MoveIlluminatorUp(ByVal idx As Integer)
        If (idx >= fl_illum.Count Or idx = 0) Then
            Return
        End If

        Dim atminus1 As FLIlluminator = fl_illum(idx - 1)
        fl_illum(idx - 1) = fl_illum(idx)
        fl_illum(idx) = atminus1
    End Sub
    Public Sub MoveIlluminatorDown(ByVal idx As Integer)
        If (idx >= (fl_illum.Count - 1)) Then
            Return
        End If

        Dim atplus1 As FLIlluminator = fl_illum(idx + 1)
        fl_illum(idx + 1) = fl_illum(idx)
        fl_illum(idx) = atplus1
    End Sub

    Public Function NumParameters() As Integer
        Return params.Count()
    End Function
    Public Sub RemoveParameter(ByVal idx As Integer)
        If idx >= params.Count() Then
            Return
        End If

        params.RemoveAt(idx)
    End Sub
    Public Sub MoveParameterUp(ByVal idx As Integer)
        If (idx >= params.Count Or idx = 0) Then
            Return
        End If

        Dim atminus1 As LayoutAnalysisParameter = params(idx - 1)
        params(idx - 1) = params(idx)
        params(idx) = atminus1
    End Sub
    Public Sub MoveParameterDown(ByVal idx As Integer)
        If (idx >= (params.Count - 1)) Then
            Return
        End If

        Dim atplus1 As LayoutAnalysisParameter = params(idx + 1)
        params(idx + 1) = params(idx)
        params(idx) = atplus1
    End Sub

    Public Function ToByteList() As List(Of Byte)
        Dim l As New List(Of Byte)

        ' index
        Dim bytes_I As Byte() = BitConverter.GetBytes(Index)
        For Each b In bytes_I
            l.Add(b)
        Next

        'description + null
        For Each c As Char In desc
            l.Add(Convert.ToByte(c))
        Next
        l.Add(0)

        'reagentcount 1
        Dim rc As Byte = Reagent.Count
        l.Add(rc)

        '  reagent indices
        For Each x In Reagent
            Dim bytes_r As Byte() = BitConverter.GetBytes(x)
            For Each b In bytes_r
                l.Add(b)
            Next
        Next

        'illuminationcount 1
        Dim ic As Byte = Illuminator.Count
        l.Add(ic)

        '  illuminators
        For Each i In Illuminator
            l.AddRange(i.ToByteList())
        Next

        'Population parameter
        l.AddRange(PopulationKey.ToByteList())

        'numparameters 1
        Dim pc As Byte = Parameters.Count
        l.Add(pc)

        '   parameters
        For Each p In Parameters
            l.AddRange(p.ToByteList())
        Next

        Return l
    End Function

    Public Function AddToLayoutString(ByRef RegisterNumber As Integer, ByRef ByteOffset As Integer, ByRef Output As String) As Boolean

        Dim Bytes As New List(Of Byte)
        Bytes = ToByteList()

        Dim b As Integer = 0

        ' Index
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(b, 2), False)
        b += 2

        ' Description
        Dim dl As Integer = desc.Length + 1
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(b, dl), True)
        b += dl

        ' Reagent Count
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(b, 1), False)
        b += 1

        ' * Reagents
        For Each r As UInt16 In reagents
            Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(b, 2), False)
            b += 2
        Next

        ' Illumination Count
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(b, 1), False)
        b += 1

        ' * Illuminator
        For Each i As FLIlluminator In Illuminator
            Dim ibytes As List(Of Byte)
            ibytes = i.ToByteList

            i.AddToLayoutString(RegisterNumber, ByteOffset, Output)


            b += ibytes.Count
        Next

        ' Population Key
        Dim pkbytes As List(Of Byte)
        pkbytes = pop_key.ToByteList

        pop_key.AddToLayoutString(RegisterNumber, ByteOffset, Output)
        b += pkbytes.Count

        ' Num Params
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(b, 1), False)
        b += 1

        ' * param
        For Each p In params
            pkbytes = p.ToByteList

            p.AddToLayoutString(RegisterNumber, ByteOffset, Output)
            b += pkbytes.Count
        Next

        Return True
    End Function
    Public Function AddToRegisterMapString(ByRef RegisterNumber As Integer, ByRef Output As String) As Boolean

        Dim v As String = "UINT16 - " + idx.ToString()
        Output += AddressMap.ToModuleMap(RegisterNumber, "Analysis Index", v)

        v = "ASCII """ + desc + """"
        Output += AddressMap.ToModuleMap(RegisterNumber, "Analysis Description", v)

        v = "UINT8 - " + reagents.Count.ToString
        Output += AddressMap.ToModuleMap(RegisterNumber, "Number of Analysis Reagents", v)

        For Each r In reagents
            v = "UINT16 - " + r.ToString
            Output += AddressMap.ToModuleMap(RegisterNumber, "Analysis Reagent Index", v)
        Next

        v = "UINT8 - " + fl_illum.Count.ToString
        Output += AddressMap.ToModuleMap(RegisterNumber, "Number of Analysis Illuminators", v)

        For Each i In fl_illum
            i.AddToRegisterMapString(RegisterNumber, Output)
        Next

        pop_key.AddToRegisterMapString(RegisterNumber, Output)

        v = "UINT8 - " + params.Count.ToString
        Output += AddressMap.ToModuleMap(RegisterNumber, "Number of Analysis Parameters", v)

        For Each p In params
            p.AddToRegisterMapString(RegisterNumber, Output)
        Next

        Return True
    End Function
End Class


Public Enum ContainerTypes As UInt16 ' 4 bits.
    Multi = &H0&
    Single_Round = &H1&
    Single_Square = &H2&
End Enum

Public Class Layout
    Private header_ As LayoutHeader
    Private container_ As ContainerTypes 'Really a 4bit that overwrites the top section of "cleaners"
    Private cleaner As List(Of LayoutReagent)
    Private reagent As List(Of LayoutReagent)
    Private analysis As List(Of LayoutAnalysis)

    Sub New()
        header_ = New LayoutHeader
        container_ = New ContainerTypes
        cleaner = New List(Of LayoutReagent)
        reagent = New List(Of LayoutReagent)
        analysis = New List(Of LayoutAnalysis)

        container_ = ContainerTypes.Multi
    End Sub

    Public Function ToByteList() As List(Of Byte)
        Dim l As New List(Of Byte)

        ' Header (include cleaner count?)
        l.AddRange(Header.ToByteList())

        ' Container type / Cleaners
        Dim cc As Byte = (Cleaners.Count And &HF&) Or ((container_ And &HF&) << 4)
        l.Add(cc)
        For Each c In Cleaners
            l.AddRange(c.ToByteList(True))
        Next

        ' Reagent? 1
        '   Reagents
        If Reagents.Count = 0 Then
            l.Add(0)
        Else
            l.Add(1)
            For Each r In Reagents
                l.AddRange(r.ToByteList(False))
            Next
        End If

        'Num Analyses
        Dim ac As Byte = Analyses.Count
        l.Add(ac)

        ' Analysis..
        For Each a In Analyses
            l.AddRange(a.ToByteList())
        Next

        Dim sz As Integer = l.Count


        Return l
    End Function
    Property Header As LayoutHeader
        Get
            Return header_
        End Get
        Set(value As LayoutHeader)
            header_ = value
        End Set
    End Property
    Property Cleaners As List(Of LayoutReagent)
        Get
            Return cleaner
        End Get
        Set(value As List(Of LayoutReagent))
            cleaner = value
        End Set
    End Property
    Property Reagents As List(Of LayoutReagent)
        Get
            Return reagent
        End Get
        Set(value As List(Of LayoutReagent))
            reagent = value
        End Set
    End Property
    Property Analyses As List(Of LayoutAnalysis)
        Get
            Return analysis
        End Get
        Set(value As List(Of LayoutAnalysis))
            analysis = value
        End Set
    End Property

    ReadOnly Property Size As Integer
        Get
            Dim SZ As Integer

            SZ = Header.Size

            SZ += 1
            For Each CL In cleaner
                SZ += CL.Size(True)
            Next

            SZ += 1
            For Each RG In reagent
                SZ += RG.Size(False)
            Next

            SZ += 1
            For Each AN In analysis
                SZ += AN.Size
            Next

            Return SZ
        End Get
    End Property
    Public Function NumAnalyses() As Integer
        Return analysis.Count()
    End Function

    Property ContainerType As ContainerTypes
        Get
            Return container_
        End Get
        Set(value As ContainerTypes)
            container_ = value
        End Set
    End Property

    Public Sub RemoveAnalysis(ByVal idx As Integer)
        If idx >= analysis.Count() Then
            Return
        End If

        analysis.RemoveAt(idx)
    End Sub
    Public Sub MoveAnalysisUp(ByVal idx As Integer)
        If (idx >= analysis.Count Or idx = 0) Then
            Return
        End If

        Dim atminus1 As LayoutAnalysis = analysis(idx - 1)
        analysis(idx - 1) = analysis(idx)
        analysis(idx) = atminus1
    End Sub
    Public Sub MoveAnalysisDown(ByVal idx As Integer)
        If (idx >= (analysis.Count - 1)) Then
            Return
        End If

        Dim atplus1 As LayoutAnalysis = analysis(idx + 1)
        analysis(idx + 1) = Analyses(idx)
        analysis(idx) = atplus1
    End Sub


    Public Function NumReagents() As Integer
        Return reagent.Count()
    End Function
    Public Sub RemoveReagent(ByVal idx As Integer)
        If idx >= reagent.Count() Then
            Return
        End If

        analyses.RemoveAt(idx)
    End Sub
    Public Sub MoveReagentUp(ByVal idx As Integer)
        If (idx >= reagent.Count Or idx = 0) Then
            Return
        End If

        Dim atminus1 As LayoutReagent = reagent(idx - 1)
        reagent(idx - 1) = reagent(idx)
        reagent(idx) = atminus1
    End Sub
    Public Sub MoveReagentDown(ByVal idx As Integer)
        If (idx >= (reagent.Count - 1)) Then
            Return
        End If

        Dim atplus1 As LayoutReagent = reagent(idx + 1)
        reagent(idx + 1) = reagent(idx)
        reagent(idx) = atplus1
    End Sub

    Public Function NumCleaners() As Integer
        Return cleaner.Count()
    End Function
    Public Sub AddCleaner(ByVal clean As LayoutReagent)
        Cleaners.Add(clean)
    End Sub
    Public Sub AddAnalysis(ByVal an As LayoutAnalysis)
        Analyses.Add(an)
    End Sub
    Public Sub AddReagent(ByVal reag As LayoutReagent)
        reagent.Clear()
        reagent.Add(reag)
    End Sub
    Public Sub RemoveCleaner(ByVal idx As Integer)
        If idx >= cleaner.Count() Then
            Return
        End If

        cleaner.RemoveAt(idx)
    End Sub
    Public Sub MoveCleanerUp(ByVal idx As Integer)
        If (idx >= cleaner.Count Or idx = 0) Then
            Return
        End If

        Dim atminus1 As LayoutReagent = cleaner(idx - 1)
        cleaner(idx - 1) = cleaner(idx)
        cleaner(idx) = atminus1
    End Sub
    Public Sub MoveCleanerDown(ByVal idx As Integer)
        If (idx >= (cleaner.Count - 1)) Then
            Return
        End If

        Dim atplus1 As LayoutReagent = cleaner(idx + 1)
        cleaner(idx + 1) = cleaner(idx)
        cleaner(idx) = atplus1
    End Sub
    Public Function AddToLayoutString(ByRef RegisterNumber As Integer, ByRef ByteOffset As Integer, ByRef Output As String) As Boolean

        Dim Bytes As New List(Of Byte)
        Bytes = ToByteList()

        Dim b As Integer = 0
        ' header
        Header.AddToLayoutString(RegisterNumber, ByteOffset, Output)
        b += Header.ToByteList.Count


        ' container type / ncleaners
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(b, 1), False)
        b += 1

        ' *cleaners
        For Each c In cleaner
            Dim cbytes As List(Of Byte)
            cbytes = c.ToByteList(True)
            c.AddToLayoutString(RegisterNumber, ByteOffset, Output, True)
            b += cbytes.Count
        Next

        ' has reagent
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(b, 1), False)
        b += 1

        ' * reagent
        For Each r In reagent
            Dim rbytes As List(Of Byte)
            rbytes = r.ToByteList(False)
            r.AddToLayoutString(RegisterNumber, ByteOffset, Output, False)
            b += rbytes.Count
        Next

        ' n analyses
        Output += AddressMap.ToAddressMap(RegisterNumber, ByteOffset, Bytes.GetRange(b, 1), False)
        b += 1

        ' * analyses
        For Each a In analysis
            Dim abytes As List(Of Byte)
            abytes = a.ToByteList()
            a.AddToLayoutString(RegisterNumber, ByteOffset, Output)
            b += abytes.Count
        Next

        Return True
    End Function
    Public Function AddToRegisterMapString(ByRef RegisterNumber As Integer, ByRef Output As String) As Boolean
        ' header
        Header.AddToRegisterMapString(RegisterNumber, Output)

        ' ncleaners
        Dim cc As Integer = (Cleaners.Count And &HF&) Or ((container_ And &HF&) << 4)
        Dim v As String = "UINT8 - " + cc.ToString
        Output += AddressMap.ToModuleMap(RegisterNumber, "Container Type / Number of Cleaners", v)

        ' *cleaners
        For Each c In cleaner
            c.AddToRegisterMapString(RegisterNumber, Output, True)
        Next

        ' has reagent
        v = "UINT8 - " + reagent.Count.ToString
        Output += AddressMap.ToModuleMap(RegisterNumber, "Has Reagent", v)
        ' * reagent
        For Each r In reagent
            r.AddToRegisterMapString(RegisterNumber, Output, False)
        Next

        ' n analyses
        v = "UINT8 - " + analysis.Count.ToString
        Output += AddressMap.ToModuleMap(RegisterNumber, "Number of Analyses", v)

        ' * analyses
        For Each a In analysis
            a.AddToRegisterMapString(RegisterNumber, Output)
        Next

        Return True
    End Function
End Class
