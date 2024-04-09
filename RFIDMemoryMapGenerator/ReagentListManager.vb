Public Class ReagentDefinition
    Private partnumber_ As String
    Private label_ As String
    Private index_ As Integer

    Property PartNumber As String
        Get
            Return partnumber_
        End Get
        Set(value As String)
            partnumber_ = value
        End Set
    End Property
    Property Label As String
        Get
            Return label_
        End Get
        Set(value As String)
            label_ = value
        End Set
    End Property
    Property Index As Integer
        Get
            Return index_
        End Get
        Set(value As Integer)
            index_ = value
        End Set
    End Property

    Public Sub Clear()
        Index = -1
        Label = ""
        PartNumber = ""
    End Sub

    Public Overrides Function ToString() As String
        Return index_.ToString & ":" & partnumber_ & ":" & label_
    End Function

    Public Function FromString(ByVal str As String) As Boolean
        Clear()

        Dim ls As String() = Split(str, ":")
        If ls.Count() <> 3 Then
            Return False
        End If

        Index = Convert.ToInt16(ls.ElementAt(0))
        PartNumber = ls.ElementAt(1)
        Label = ls.ElementAt(2)

        Return True
    End Function
End Class

Public Class ReagentListManager

    Private cleaners As List(Of ReagentDefinition)
    Private reagents As List(Of ReagentDefinition)


    Public Sub New()
        cleaners = New List(Of ReagentDefinition)
        reagents = New List(Of ReagentDefinition)
    End Sub
    Public Function FromFile(ByVal FN As String) As Boolean
        cleaners.Clear()
        reagents.Clear()

        If Not My.Computer.FileSystem.FileExists(FN) Then

            Dim c As New ReagentDefinition
            c.FromString("1:C10815:Cleaning Agent")
            cleaners.Add(c)
            c = New ReagentDefinition
            c.FromString("2:C10816:Conditioning Solution")
            cleaners.Add(c)
            c = New ReagentDefinition
            c.FromString("3:C10818:Buffer Solution")
            cleaners.Add(c)

            Dim r As New ReagentDefinition
            r.FromString("4:C10819:Trypan Blue")
            reagents.Add(r)
            r = New ReagentDefinition
            r.FromString("6:C09146:Viability Scarlet")
            reagents.Add(r)


            Return False
        End If


        Dim sr As IO.StreamReader = FileIO.FileSystem.OpenTextFileReader(FN)


        While Not sr.EndOfStream
            Dim s As String = sr.ReadLine

            Dim ls As String() = Split(s, ">")
            If ls.Count <> 2 Then
                Continue While
            End If

            If ls.ElementAt(0) = "CLN" Then
                Dim c As New ReagentDefinition
                c.FromString(ls.ElementAt(1))
                cleaners.Add(c)
            ElseIf ls.ElementAt(0) = "RGT" Then
                Dim r As New ReagentDefinition
                r.FromString(ls.ElementAt(1))
                reagents.Add(r)
            End If
        End While

        sr.Close()
        Return True
    End Function
    Public Function ToFile(ByVal FN As String) As Boolean

        Dim s As String = ""

        For Each c In cleaners
            s += "CLN>" & c.ToString & vbLf
        Next

        For Each r In reagents
            s += "RGT>" & r.ToString & vbLf
        Next

        FileIO.FileSystem.WriteAllText(FN, s, False)
        Return True
    End Function

    ReadOnly Property CleanerList As List(Of ReagentDefinition)
        Get
            Return cleaners
        End Get
    End Property
    ReadOnly Property ReagentList As List(Of ReagentDefinition)
        Get
            Return reagents
        End Get
    End Property
End Class
