Imports System.Security.Cryptography


Public Class RunningSHADigest

    Private myDigest As System.Security.Cryptography.SHA256
    Private mySalt As Byte()
    Private myReps As UInteger

    Public Sub New(ByRef s As Byte(), ByVal r As UInteger)
        myDigest = SHA256Managed.Create()
        mySalt = New Byte(s.Length - 1) {}
        s.CopyTo(mySalt, 0)
        myReps = r
        myDigest.Initialize()
    End Sub

    Public Sub Reset()
        myDigest.Initialize()
    End Sub

    Public Sub UpdateDigest(ByRef b As Byte())
        myDigest.TransformBlock(b, 0, b.Length, b, 0)
    End Sub
    Public Sub UpdateDigest(ByRef s As String)
        Dim b As New List(Of Byte)
        For Each c As Char In s
            b.Add(Convert.ToByte(c))
        Next
        Dim ba As Byte() = b.ToArray()
        myDigest.TransformBlock(ba, 0, ba.Length, ba, 0)
    End Sub

    Public Function FinalizeDigestToHash() As String

        myDigest.TransformFinalBlock(mySalt, 0, mySalt.Length)
        Dim hash1 As Byte() = myDigest.Hash

        Dim hash2 As Byte()
        myDigest.Initialize()

        For i As Integer = 1 To myReps
            hash2 = myDigest.ComputeHash(hash1, 0, hash1.Length)
            hash1 = hash2
        Next

        Dim s As New String("")
        For Each b In hash1
            s &= b.ToString("X2")
        Next
        myDigest.Initialize()
        Return s
    End Function


End Class
