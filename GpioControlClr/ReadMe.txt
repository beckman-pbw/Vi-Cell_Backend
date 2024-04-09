========================================================================
    DYNAMIC LINK LIBRARY : GpioControlClr Project Overview
========================================================================

This is a CLR enabled project that wraps the C# GPIO control lib (RfidExtension.dll).

Steps to build:
1) Compile the GpioControl project.

2) Open a developer command prompt for Visual Studio as the system Administrator.

3) Change directory to the location of the compiled RfidExtensions.dll file (or specify the full path to it in the next step)

4) Register the GpioControl.dll assembly with Windows using the 64-bit version of regasm.exe like so:
    
	c:\Windows\Microsoft.NET\Framework64\v4.0.30319\regasm.exe RfidExtensions.dll /tlb:RfidExtensions.tlb /codebase

	To unregister the assembly, use the command:
	c:\Windows\Microsoft.NET\Framework64\v4.0.30319\regasm.exe /unregister RfidExtensions.dll

5) Compile this project.  It may need to be compiled twice to achieve success.  
   The first compilation should generate an RfidExtensions.tlh file for consumption by this project.

6) Build entire solution.

7) If you make changes to GpioControl source code, then you may need to reregister the DLL as well as regenerate the
   auto-generated header file.  
   
   BEWARE that "Clean" may not delete the .TLB file or the .TLH file.  You may need to delete the both the:
     rfidextensions.tlb file from the output folder of the GpioControl project as well as the
     rfidextensions.tlh file from the output folders for this project (and the overall solution) before you recompile this project.
