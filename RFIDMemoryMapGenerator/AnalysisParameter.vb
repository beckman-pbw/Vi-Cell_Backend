Public Class AnalysisParameter
    Public Label As String
    Public Key As Integer
    Public Subkey As Integer
    Public SubSubkey As Integer

    Public Sub New()
        Label = ""
        Key = 0
        Subkey = 0
        SubSubkey = 0
    End Sub
    Public Sub New(ByVal l As String, ByVal k As Integer, Optional ByVal sk As Integer = 0, Optional ByVal ssk As Integer = 0)
        Label = l
        Key = k
        Subkey = sk
        SubSubkey = ssk
    End Sub

    Public Overrides Function ToString() As String
        Return Label & " (" & Key.ToString & ")"
    End Function

End Class
