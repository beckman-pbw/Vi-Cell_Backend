Imports System
Imports System.Reflection
Imports System.Runtime.InteropServices

' General Information about an assembly is controlled through the following 
' set of attributes. Change these attribute values to modify the information
' associated with an assembly.

' Review the values of the assembly attributes

<Assembly: AssemblyTitle("RFIDMemoryMapGenerator")>
<Assembly: AssemblyDescription("")>
<assembly: AssemblyCompany("Beckman Coulter Life Sciences")>
<assembly: AssemblyCopyright("Copyright (C) ${current.year} Beckman Coulter Life Sciences. All rights reserved.")>
<Assembly: AssemblyProduct("RFIDMemoryMapGenerator")>
<Assembly: AssemblyTrademark("Beckman Coulter, the stylized logo, and the Beckman Coulter product and service marks mentioned herein are trademarks or registered trademarks of Beckman Coulter, Inc. in the United States and other countries.")>

<Assembly: ComVisible(False)>

'The following GUID is for the ID of the typelib if this project is exposed to COM
<Assembly: Guid("097b58d7-ba8a-41a0-847b-982c448adaf6")>

' Version information for an assembly consists of the following four values:
'
'      Major Version
'      Minor Version 
'      Build Number
'      Revision
'
' You can specify all the values or you can default the Build and Revision Numbers 
' by using the '*' as shown below:
' <Assembly: AssemblyVersion("1.0.*")> 

<Assembly: AssemblyVersion("${parsedVersion.majorVersion}.${parsedVersion.minorVersion}.${parsedVersion.incrementalVersion}.${timestamp}")>
<Assembly: AssemblyFileVersion("${parsedVersion.majorVersion}.${parsedVersion.minorVersion}.${parsedVersion.incrementalVersion}.${timestamp}")>
