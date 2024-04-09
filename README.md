# ApplicationSource
This is the backend "business logic" layer for the Hawkeye platform. This creates a DLL file and associated set of configuration and accessory files and packages them into an installer.
Application source depends (and packages) an associated version of the 
 * Algorithms
 
 ...project.  These together form an installable release package.
 
 ## Critical Branches
 |*PartNumber*|*Description*|*Branch*|
 |------------|-------------|--------|
 | C90104 | Application Package | Master |
 | C19582 | RFID Programming Station | RELEASES_RFIDProgrammingStation |
 | C19701 | RFID Content Generator | RELEASES_ReagentPICCContentGenerator |
 | C____ | Controller Board Programmer | ManufTool_ControllerProgrammer |
 | PC3527SH-0160 | Service Password Generator | RELEASES_ServicePasswordGenerator |
 | N/A | Firmware Test App | TEST_FirmwareTest |
 | N/A |STAGING_ViCELL-BLU-1.2.X| 1.2.X staging branch |
 
 
 ## To build this project
 1. Install Git for Windows 
 1. Install Visual Studio Professional (or greater) 2015 Update 3
 1. Install Maven, Java Runtime Environment (JRE), NullSoft Scriptable Installer (NSI) and WiX [as described here](https://github.com/BeclsParticle/HawkeyeDeployment/blob/master/src/site/markdown/developer-setup.md)
 1. Clone the project from the Github Server git@github.com:BeclsParticle/HawkeyeApplicationSource.git
    1. This document will assume you have cloned into the folder `c:\DEV\Hawkeye`.  This will create a folder `c:\DEV\Hawkeye\ApplicationSource`.
 1. Open a command prompt and change to the newly-cloned repository
    1. `cd c:\DEV\Hawkeye\ApplicationSource`
 1. Invoke Maven to execute the build process and generate the artifacts
    1. `mvn package`
    1. This will initially generate a significant amount of traffic as the build system retrives the correct dependency artifacts from the Beckman Coulter Amazon Web Services cloud cache.  Repeated builds will used cached copies of these resources
 1. Upon sucessful completion, artifacts will be found in `c:\DEV\Hawkeye\ApplicationSource\target`
 
 ## To recreate artifacts from a particular build
 1. Note the tag associated with the build
    1. Ex: `backend_0.85.3`
 1. Execute the steps from the **To build this project** section of the document through opening a command prompt to the newly-cloned repository
 1. Use Git to checkout the repository to the desired state
    1. `git checkout backend_0.85.3`
 1. Resume the steps from the **To build this project** section 
 
