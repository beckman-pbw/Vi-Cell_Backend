Developer Setup
===============

## Install JRE

### Download JRE:

http://www.oracle.com/technetwork/java/javase/downloads/jre8-downloads-2133155.html
 
### Set JAVA_HOME

Set "JAVA_HOME" environment variable
C:\Program Files\Java\jre1.8.0_161\

### Install Maven

https://archive.apache.org/dist/maven/maven-3/3.3.9/binaries/apache-maven-3.3.9-bin.zip

Unzip to C:\Dev and add the directory "C:/Dev/apache-maven-3.3.9/bin" folder to your PATH environment variable

### Install NullSoft Scriptable Installer (3.03)

https://sourceforge.net/projects/nsis/?source=typ_redirect

### Clone test repository containing new build environment:

http://10.101.20.145/kconnors/ApplicationSource

### Run build

* Open command prompt into new clone (assuming c:\Dev\Hawkeye\ApplicationSource_KC)
    * Start > Run > cmd
    * cd c:\Dev\Hawkeye\ApplicationSource_KC
* Run Maven
    * mvn package
    * [This should now download a whopping boatload of stuff and then begin a compilation of the source code]
