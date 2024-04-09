''' <summary>
''' Responsible for the maintenance of the entire RFID data package.
''' 
''' The package includes:
'''     Payload File - list of all supported RFID tag layouts (by GTIN) (as RFIDPayloads.txt)
'''     RFID layout BIN files - collection of BIN files - one per RFID tag layout (as [GTIN].bin)
'''     
''' Should be able to ADD new layouts
''' Should build a release package (.zip?)
''' </summary>

Public Class FirmwareBinaryEntry
    Private GTIN As String
    Private Description As String
    Private BINFilename As String
    Private BINFileHash As String

    Public Sub New()
        GTIN = New String("")
        Description = New String("")
        BINFilename = New String("")
        BINFileHash = New String("")
    End Sub

    ''' <summary>
    ''' To load a Firmware Binary Entry from the payload file string
    ''' Format: [GTIN][DESCRIPTION][BIN FILENAME][BIN HASH]
    ''' </summary>
    ''' <param name="s"></param>
    Public Sub New(ByVal s As String)
        Try
            Dim CurrentLine As String() = s.Split({"["c, "]"c}, StringSplitOptions.RemoveEmptyEntries)
            If CurrentLine.Length = 4 Then
                GTIN = CurrentLine.ElementAt(0)
                Description = CurrentLine.ElementAt(1)
                BINFilename = CurrentLine.ElementAt(2)
                BINFileHash = CurrentLine.ElementAt(3)
            End If
        Catch ex As Exception
            ' invalid line.
        End Try

    End Sub

    ''' <summary>
    ''' Create a firmware binary entry as well as the BIN file that goes with it.
    ''' </summary>
    ''' <param name="G">GTIN value</param>
    ''' <param name="D">Description</param>
    ''' <param name="B">Binary data</param>
    ''' <returns></returns>
    Public Function Create(ByVal G As String, ByVal D As String, ByVal B As Byte(), Optional ByVal OVERWRITE As Boolean = False)
        Dim fn As String = G & ".bin"

        If Not OVERWRITE And My.Computer.FileSystem.FileExists(fn) Then
            Return False
        End If
        Dim s As String = "BeckmanCoulterReagent"
        Try
            Dim salt As New List(Of Byte)
            For Each c As Char In s
                salt.Add(Convert.ToByte(c))
            Next
            Dim sha As New RunningSHADigest(salt.ToArray, 1978)
            sha.UpdateDigest(B)
            Dim h As String = sha.FinalizeDigestToHash()

            FileIO.FileSystem.WriteAllBytes(fn, B.ToArray, False)

            Me.GTIN = G
            Me.Description = D
            Me.BINFilename = fn
            Me.BINFileHash = h

        Catch ex As Exception
            Return False
        End Try


        Return True
    End Function

    Public Overrides Function ToString() As String

        Return "[" & GTIN & "][" & Description & "][" & BINFilename & "][" & BINFileHash & "]"

    End Function

    Public ReadOnly Property GetGTIN As String
        Get
            Return GTIN
        End Get
    End Property
    Public ReadOnly Property GetDesc As String
        Get
            Return Description
        End Get
    End Property
    Public ReadOnly Property GetFilename As String
        Get
            Return BINFilename
        End Get
    End Property
    ReadOnly Property GetHash As String
        Get
            Return BINFileHash
        End Get
    End Property



    ''' <summary>
    ''' Check the data and make sure that
    '''  * filename exists (current folder)
    '''  * file hash matches the record
    ''' </summary>
    ''' <returns></returns>
    Public Function IsValidData() As Boolean
        If GTIN.Length = 0 Or
                Description.Length = 0 Or
                BINFilename.Length = 0 Or
                BINFileHash.Length = 0 Then
            Return False
        End If

        If Not FileExists() Then
            Return False
        End If

        Return FileValidates()
    End Function
    Public Function FileExists() As Boolean
        If Not My.Computer.FileSystem.FileExists(BINFilename) Then
            Return False
        End If

        Return True
    End Function
    Public Function FileValidates() As Boolean
        If Not FileExists() Then
            Return False
        End If
        Try
            Dim b As Byte() = FileIO.FileSystem.ReadAllBytes(BINFilename)

            Dim s As String = "BeckmanCoulterReagent"
            Dim salt As New List(Of Byte)
            For Each c As Char In s
                salt.Add(Convert.ToByte(c))
            Next
            Dim rsd As New RunningSHADigest(salt.ToArray(), 1978)

            rsd.UpdateDigest(b)

            Dim calc_hash As String = rsd.FinalizeDigestToHash


            If (calc_hash <> BINFileHash) Then
                Return False
            End If
        Catch ex As Exception
            Return False
        End Try

        Return True
    End Function
End Class


Public Class RFIDDataOutput
    Private payload_file As String
    Private binary_entries As List(Of FirmwareBinaryEntry)

    Public Sub New()
        payload_file = New String("")
        binary_entries = New List(Of FirmwareBinaryEntry)
    End Sub

    ReadOnly Property Entries As List(Of FirmwareBinaryEntry)
        Get
            Return binary_entries
        End Get
    End Property
    ReadOnly Property PayloadFile As String
        Get
            Return payload_file
        End Get
    End Property

    Public Sub Clear()
        binary_entries.Clear()
    End Sub

    Public Function AddBinaryEntry(ByVal GTIN As String, ByVal DESC As String, ByVal RFID As Byte(), Optional ByVal OVERWRITE As Boolean = False) As Boolean
        ' Rule 1: No duplicate GTIN
        For Each be As FirmwareBinaryEntry In binary_entries
            If be.GetGTIN = GTIN Then
                If Not OVERWRITE Then
                    MsgBox("Unable to add entry - GTIN """ & GTIN & """ is already in use.")
                    Return False
                End If
                Exit For
            End If
        Next

        Dim FBE As New FirmwareBinaryEntry
        If Not FBE.Create(GTIN, DESC, RFID, OVERWRITE) Then
            MsgBox("Unable to add entry (firmware binary entry failed)")
            Return False
        End If

        binary_entries.Add(FBE)
        Return True
    End Function

    Public Function WritePayloadFile(ByVal filename As String) As Boolean

        Dim tempfilename As String = filename & ".temp"
        Dim bakfilename As String = filename & ".bak"

        Try
            Dim s As String = "BeckmanCoulterReagent"
            Dim salt As New List(Of Byte)
            For Each c As Char In s
                salt.Add(Convert.ToByte(c))
            Next
            Dim sha As New RunningSHADigest(salt.ToArray, 1978)

            For Each FBE As FirmwareBinaryEntry In binary_entries
                sha.UpdateDigest(FBE.ToString)
            Next
            Dim hash As String = sha.FinalizeDigestToHash()

            Dim output As IO.StreamWriter = My.Computer.FileSystem.OpenTextFileWriter(tempfilename, False, System.Text.Encoding.ASCII)
            output.WriteLine(hash)
            For Each FBE As FirmwareBinaryEntry In binary_entries
                output.WriteLine(FBE.ToString)
            Next
            output.Close()

            ' swap the temp and the real file
            If My.Computer.FileSystem.FileExists(filename) Then
                If (My.Computer.FileSystem.FileExists(bakfilename)) Then
                    My.Computer.FileSystem.DeleteFile(bakfilename)
                End If
                My.Computer.FileSystem.MoveFile(filename, bakfilename)
            End If
            My.Computer.FileSystem.MoveFile(tempfilename, filename)
            Return True

        Catch ex As Exception
            Return False
        End Try

    End Function

    Public Function LoadPayloadFile(ByVal filename As String) As Boolean
        ' does not exist: OK.  Maybe it's the first time.

        Dim tempFBE As New List(Of FirmwareBinaryEntry)

        If Not My.Computer.FileSystem.FileExists(filename) Then
            MsgBox("Payload file """ & filename & """ does not exist.  Starting with empty list.")
            Me.binary_entries = tempFBE
            Return True
        End If

        Try
            Dim input As IO.StreamReader = My.Computer.FileSystem.OpenTextFileReader(filename)

            ' first line is the hash of everything to follow.
            Dim hash As String = input.ReadLine

            Dim fbes As New List(Of String)
            While Not input.EndOfStream
                fbes.Add(input.ReadLine)
            End While
            input.Close()

            Dim s As String = "BeckmanCoulterReagent"
            Dim salt As New List(Of Byte)
            For Each c As Char In s
                salt.Add(Convert.ToByte(c))
            Next
            Dim sha As New RunningSHADigest(salt.ToArray, 1978)

            For Each f As String In fbes
                sha.UpdateDigest(f)
            Next
            Dim calc_hash As String = sha.FinalizeDigestToHash

            If (calc_hash <> hash) Then
                MsgBox("Unable to validate hash signature of payload file")
                Return False
            End If

            For Each f As String In fbes
                Dim FBE As New FirmwareBinaryEntry(f)
                If Not FBE.IsValidData Then
                    MsgBox("Validation of line """ & f & """ failed")
                Else
                    tempFBE.Add(FBE)
                End If
            Next

        Catch ex As Exception
            Return False
        End Try
        Me.binary_entries = tempFBE
        Return True
    End Function
End Class
