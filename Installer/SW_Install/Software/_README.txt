The following DLLs are explicitly placed in the Release directory to avoid Windows getting confused and copying from the wrong directory.
As MSVS 2015 is a 32bit application running in a 64bit environment, when we ask to copy one of these4 DLLs 
from C:\Windows\System32 (where 64 bit code resides on 64 bit OS) Windows tries to be smart and it copies 
the C:\Windows\SysWOW64 version instead.  The instrument application software crashes because Windows has copied the 32 bit
version of these DLLs from C:\Windows\SysWOW64.

msvcp120.dll		645 KB
msvcp140.dll		619 KB
msvcr120.dll		941 KB
vcruntime140.dll	 85 KB
