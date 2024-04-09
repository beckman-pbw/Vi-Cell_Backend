Public Class ReagentInfo
    Public Index As UInt16
    Public Description As String
    Public Mix As UInt16
    Public PartNumber As String

    Public Sub New()
        Index = 0
        Description = New String("")
        Mix = 0
        PartNumber = New String("")
    End Sub
    Public Sub New(ByVal i As UInt16, ByVal d As String, ByVal p As String, Optional ByVal m As UInt16 = 0)
        Index = i
        Description = d
        PartNumber = p
        Mix = m
    End Sub

    Public Overrides Function ToString() As String
        Return Description & " (" & Index.ToString & ") x" & Mix.ToString
    End Function
End Class

Public Class ReagentList
    Public RList As List(Of ReagentInfo)

    Public Sub AddReagent(ByRef r As ReagentInfo)

        ' Allow no duplicates.
        For Each ri As ReagentInfo In RList
            If ri.Index = r.Index Then
                Return
            End If
        Next

        Dim nr As New ReagentInfo(r.Index, r.Description, r.Mix)
        RList.Add(nr)
    End Sub

    Public Function SetFromFile(ByVal filename As String) As Boolean
        Dim rl As New List(Of ReagentInfo)
        Try
            Using SR As New Microsoft.VisualBasic.FileIO.TextFieldParser(filename)

                SR.TextFieldType = FileIO.FieldType.Delimited
                SR.SetDelimiters("::")

                Dim CurrentLine As String()
                While Not SR.EndOfData
                    Try
                        CurrentLine = SR.ReadFields()
                        If CurrentLine.Length = 4 Then
                            Dim ri As New ReagentInfo
                            ri.Index = Convert.ToInt16(CurrentLine.ElementAt(0))
                            ri.Description = CurrentLine.ElementAt(1)
                            ri.Mix = Convert.ToInt16(CurrentLine.ElementAt(2))
                            ri.PartNumber = CurrentLine.ElementAt(3)

                            rl.Add(ri)
                        End If
                    Catch ex As Exception
                        ' invalid line.
                    End Try
                End While

                SR.Close()
            End Using
        Catch ex As Exception
            MsgBox("Unable to open reagent info file (""" & filename & """)")
        End Try


        RList = rl
        Return True

    End Function
    Public Function WriteToFile(ByVal filename) As Boolean

        Dim SW As System.IO.StreamWriter
        SW = My.Computer.FileSystem.OpenTextFileWriter(filename, False)

        For Each r As ReagentInfo In RList
            Dim s As String = r.Index.ToString & "::" & r.Description & "::" & r.Mix.ToString & "::" & r.PartNumber
            SW.WriteLine(s)
        Next

        SW.Close()
        Return True
    End Function
End Class
