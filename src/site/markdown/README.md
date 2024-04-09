Application Source
==================

### v2.0.x
* Merge 1.2.17 down into Master branch.
---

### v1.2.17
#### Features
#### Fixes
* Jira PC3527-4152 - Remove msiexec unistall from batch script.  Rely on UI to do that correctly.

### v1.2.16
#### Features
#### Fixes
* Jira PC3527-4155, PC3527-4333 - Autofocus exit during settling and autofocus fail when dirty

### v1.2.15
#### Features
#### Fixes
* Jira PC3527-2969 - Do not fail registration if tube is detected during final homing sequence (BUGFIX)

### v1.2.14
#### Features
#### Fixes
* Jira PC3527-4277 - New LED code needs to be refactored to remove redundancy
* Jira PC3527-4252 - BCI LED Development: Test/Debug BCI LED using prototype BCI LED (updated to fix crash)

### v1.2.13
#### Features
#### Fixes
* Jira PC3527-4099-4153 - Change password API to string
* Jira PC3527-4275 - Integrate abi_log_values from ABI_Instability_Testing branch

### v1.2.12
#### Features
#### Fixes
* Jira PC3527-4307 - ABI failure on Val-1 preventing sample processing
* Jira PC3527-4020 - Full Spin Carousel Runaway

### v1.2.11
#### Features
#### Fixes
* Jira PC3527-4248 - BCI LED Development: Integrate prior work
* Jira PC3527-4250 - BCI LED Development: Test/Debug Simulation mode
* Jira PC3527-4251 - BCI LED Development: Test/Debug Omicron mode
* Jira PC3527-4252 - BCI LED Development: Test/Debug BCI LED using prototype BCI LED

### v1.2.10
#### Features
#### Fixes
* Jira PC3527-3946 - Focus run dumped sample

### v1.2.9
#### Features
#### Fixes
* Jira PC3527-3809 - Manual movement of carousel during operation (second contribution)

### v1.2.8
#### Features
#### Fixes
* Jira PC3527-3809 - Manual movement of carousel during operation (additional work)

### v1.2.8
#### Features
#### Fixes
* Jira PC3527-3809 - Manual movement of carousel during operation

### v1.2.7
#### Features
* Jira PC3527-2969 - Do not fail registration if tube is detected during final homing sequence
#### Fixes
* Jira PC3527-4046 - Cleanup workflow cancellation

### v1.2.6
#### Features
* Jira PC3527-3712 - Audit log formatting improvements
#### Fixes

### v1.2.5
#### Features
#### Fixes
* Jira PC3527-3720 - Report logs newest to oldest
* Jira PC3527-4137 - Change Backend log level to DBG1
* Jira PC3527-3449 - Failure to update status in message hub 
* Jira PC3527-3698 - Autofocus coarse scan range too large

### v1.2.4
#### Features
#### Fixes
* Jira PC3527-4085 - Correct dark-image detection code in Camera (improves reporting, no effect otherwise)

### v1.2.3
#### Features
#### Fixes
* Jira PC3527-3550 - Remaining reagent not updating for Decontaminate and NightlyClean

### v1.2.2
#### Features
#### Fixes
* Jira PC3527-3555 - Cancel decontaminate does not stop tube search or plate movement or fully cancel the workflow
* Jira PC3527-3790 - Omicron LED interlock error not checked

### v1.2.1
#### Features
* Merge of 1.1.9 changes
#### Fixes
* Jira PC3527-3746 - UI shows bogus night clean error on startup
* Jira PC3527-3940 - Remove dependence on C: drive in data import script
* Jira PC3527-3745 - Ensure that all reanalyses run with original concentration factor settings
* Jira PC3527-3555 - Cancel decontaminate does not stop tube search or plate movement or fully cancel the workflow

### v1.2.0
#### Features
* Merge of v1.1.8 release branch
#### Fixes
* Jira PC3527-3699 - Correct auto-focus calculations
* Jira PC3527-3501 - Changed encoding of Sample_Workflow-Normal.txt from UTF-8 BOM to UTF-8.
* Jira PC3527-3548 - Enhance firmware update logging for failures; record correct original firmware version to audit log

---
### v1.1.9
#### Features
* Jira PC3527-82 - Additional installation script changes for whitelisting.
#### Fixes

### v1.1.8
#### Features
* Jira PC3527-82 - Enable application whitelisting in Operating System scripts
#### Fixes


### v1.1.7
#### Features
* Jira PC3527-3779 - Include positioning information in autofocus images retained after an AF run (debug aide)
#### Fixes
* Jira PC3527-3781 - Retake black or dark Auto focus images
* Jira PC3527-3782 - Cancelling motor registration left motor offsets incorrect if not fully re-initialized.
* Jira PC3527-3786 - Pausing, then Stopping queue crashes software (introduced in 1.1.5)
* Jira PC3527-3795 - Prevent disabled user from validating credentials

### v1.1.6
#### Features
#### Fixes
* Jira PC3527-3342 - Ensure that a "tube found" message is displayed during motor registration sequence.

### v1.1.5
#### Features
* Jira PC3527-3349 - Re-enabled the data integrity check.
* Jira PC3527-3680 - Record FTDI version in log files
#### Fixes
* Jira PC3527-3335 - Audit logs: Audit log not logged for firmware update failed.

### v1.1.4
#### Features

#### Fixes
* Jira PC3527-3483 - Fixed issues with adjusting the LED power in Maintenance/Optics screen.
* Jira PC3527-3582 - Ensure that decluster settings are recorded in TEXT form in the audit log
* Jira PC3527-3420 - Remove dependency on C Drive.
* Jira PC3527-3687 - Fix handling of Sample Activity Log entires with zero samples in the queue
* Jira PC3527-3515 - Add config parameter to allow saving binary images.

### v1.1.3
#### Features
* Jira PC3527-3397 - Auto focus : Save best focus image

#### Fixes
* Jira PC3527-3408 - Preserve all system-or-user modified configuration files when performing a software update installation.
* Jira PC3527-3369 - Export Samples Time Increases Per Samples Added. 
* Jira PC3527-3454 - Add hardening around interface to Basler camera driver to gracefully handle a failure seen during manufacturing.

### v1.1.1
#### Features
* Jira PC3527-3358 - New API to return Resource Keys for System Error Code values
   * See /Documentation/SystemErrorCode_ResourceList.md

#### Fixes
* Jira PC3527-3412 - Normalization image metadata is created only for the 1st sample of the work queue

### v1.1.0
#### Features

#### Fixes
* Jira PC3527-3367 - Not All Hardware Faults Are Showing As Errors : Plate/Carousel are not reporting error with casuse of failure to user in case of motor motion fails
* Jira PC3527-3374 - NULL cell counting algorithm pointer passed to renalysis legacy data saving when reanlysis is done without images( Binary data instead).
* Jira PC3527-3339 - POM.xml file change, added the import_data.bat script file to back end maven build output package.
* Jira PC3527-3258 - Application Crashing when Camera disconnects during Sample processing
---
### v1.0.11
#### Features
#### Fixes
* Jira PC3527-3687 - Do not write sample log entry for empty queue; corectly check sample log entry length

### v1.0.10
#### Features

#### Fixes
* Jira PC3527-3552 - Unrestrict access for all svc_GetXXXXXX API calls. Reports correct flow cell depth in System Status report.

### v1.0.9
#### Features

#### Fixes
* Jira PC3527-3542 - Expand password validation window for internal administraive user

### v1.0.8
#### Features
* Jira PC3527-3515 - Store binary images generated during autofocus sequence

#### Fixes

### v1.0.7
#### Features

#### Fixes
* Jira PC3527-3476 - Remove "must be logged in" protection from svc_GetValvePort to reduce "noise" events in Audit Trail when a user's account is disabled.
* Jira PC3527-3490 - Add Quality Control / Bioprocess label to sample record creation audit trail
* Jira PC3527-3475 - Log bad validation attempts through "ValidateMe" API
* Jira PC3527-3454 - Add hardening around interface to Basler camera driver to gracefully handle a failure seen during manufacturing.
* Jira PC3527-3408 - Preserve user or system modified configuration files on software update installation
* Jira PC3527-3369 - Export Samples Time Increases Per Samples Added. 


#### Fixes

### v1.0.5, v1.0.6
#### No changes - version roll only

### v1.0.4
#### Features

#### Fixes
* Jira PC3527-3339 - POM.xml Change, added the import_data.bat script file to back end maven build output package.

### v1.0.3
#### Features

#### Fixes
* Jira PC3527-3349 - Disabled the data integrity check.
* Jira PC3527-3327 - Add UTC time zone to the time stamp string audit log description EX: "MM/DD/YY HH:MM:SS UTC"
* Jira PC3527-3342 - Fix difference in effective expiration in reagent status screen and in audit log
* Jira PC3527-3412 - Normalization image metadata is created only for the 1 sample of the work queue

### v1.0.0
#### Features
* Jira PC3527-3218/3220 - Change logic for decimating exported images sets on already decimated stored images
* Jira PC3527-1553 - Scan for orphaned / missing data files during nightly clean sequence
* Set default inactivity timeout to 60 minutes
* Enforce non-empty short/long signature text fileds
* Add logging to record reanalysis request parameters (from image, from data)

#### Fixes
* Jira PC3527-3318 - Change FTDI USB Latency from 1 to 2 ms
* Fix crash on saving "legacy" data when reanalyzed from previous data
* Fix behavior around "extra" audit log entries for "inactivity timeout" and "password expirate" changes
* Jira PC3527-3528 - Fix camera-related crash during sampling if frame grabbing fails
* Jira PC3527-3238 - Fix application crash when reagent pack reaches 0 uses mid-queue

### v0.64.8 / v0.64.9
#### Features
* Jira PC3527-3216 - Additional API to allow an adminstrator to unlock a (logged-in and locked) user without the administrator formally logging in
* Jira PC3527-3184 - Report "Syringe pump over-pressure" as a specific System Error

#### Fixes
* Jira PC3527-3225 - Application crashes on import of incorrect .cfg file
* Jira PC3527-3239 - No cell types displayed for "Silent Admin"
* Trigger completion callback to UI when a sample is skipped because it was not found.

#### Additional
* Installer script file changes
   * Remove obsoleted batch script
   * Script rename to avoid overwrite conflicts
   * Allow install scripts to work with non-C: paths
   * Record a change to an OS image file that is tracked in Git
   * Fix error in a non-Scout script
   * Update installer scripts for:  non-C: paths; add parameters to preserve config files; add "help" option

### v0.64.3 through v0.64.7
### Features

### Fixes
* Fix to save normalization image functions
* Fixes for service user cell type list
* Fix password for "factory_admin" user
* Fixes to audit/error/sample log recording and readback functions
* "Retrieve\*Log\*" APIs no longer return "eNoneFound" on empty lists
* Fix to service LED state
* Fixed installation issue where overwriting workflow files.

### v0.64.1 / v0.64.2
#### Features
* Jira PC3527-3093 - Implement Audit/Sample/Error log archiving
* Jira PC3527-3158 - "Calibrate" removed from all user strings
* Jira PC3527-2973 - Added encryption for the workflow scripts
* Jira PC3527-3055 - Archive the export data without making a temp copy

#### Fixes 
* Jira PC3527-3135 - Fix the date calculation for stored date in the reagent info records within a sample
* Jira PC3527-2972 - Retain most recent size/concentration factor setting when history is cleared
* Jira PC3527-3159 - Fix crash when disposing of log entries (data size mismatch -> integer wrap)
* Jira PC3527-3169 - Reanalysing a decimated ("Save every 10th image") sample doesn't work
* BugFix in save normalization image when no "Dust subtract" image is available

### v0.64.0
* New Simulator Feature: Import Your Own Images
   * Jira PC3527-2892
   * Add a set of UNENCRYPTED .png images to /Instrument/ExternalImages and boot the simulator.
   * Instrument will reference those images when creating new samples.
   * A note in HawkeyeDLL.log will indicate that "CameraSim is using XXX images from ../ExternalImages"
* Bug Fixes
   * Jira PC3527-3111 - correct the loading of "average viable circularity" from stored data.
   * PC3527-3126-Create tool to construct default user list
     * User: factory_admin, Default Password: Vi-CELL#0

### v0.63.1
> **WARNING** Due to fixes in place for the nightly clean sequence, initial startup of the system will attempt to
> begin a cleaning sequence. This may FAIL if 
> * Reagent pack is not present (or expired or empty)
> * Motor registration is not done
> These are nearly GUARANTEED failures at first installation of the system!

> **WARNING** Hashing of Audit/Sample/Error logs (Jira PC3527-3049) requires FORCE INSTALL 
> of the software as existing logs will not have the correct hash value present.

* Instrument data import and export (API Change!)(Jira PC3527-2835, 3051, 3052, 3053, 3054, 
* Data retrival speed improvements (API Change!) (Jira PC3527-2967)
* Instrument configuration import and export (Jira PC3527-2838, 2839
* Nightly cleaning cycle improvements/fixes (Jira PC3527-3065, 3071)
* Autofocus and Dust Subtration sequence cancellation fixes (Jira PC3527-3023, 2975, 2942)
* Correct pre-flight check on workqueue start conditions (don't run when no reagent pack loaded) (Jira PC3527-2951)
* Reanalysis fixes (Jira PC3527-2982)
* Changes to user-facing strings (Jira PC3527-3007)
* Added new audit trail entry for creation of a sample result (Jira PC3527-2947)
* Fix for priming w/ Flushing Kit (Jira PC3527-3039)
* Protect back-end against unhelpful calls during shutdown sequence (Jira PC3527-3078)
* Audit, Sample and Error logs are now encrypted (Jira PC3527-3049)

### v0.62.1
> This version requires that existing data is DELETED from the system before running.  
> Significant changes to the metatdata storage occurred here that invalidate earlier packaging.
* Jira PC3527-2975 - Add cancellation states to Autofocus and Background Dust Subtract state enumerations
* Jira PC3527-2951 - Improve pre-flight checks around failed reagents prior to starting sample processing
* Jira PC3527-2976 - Adjust internal result data storage and access APIs/structs to streamline data access for result information.
   * All information the UI needs to generate the results lists and virtually all reports can now be done without having to decrypt the binary records on disk
   * Backend speed improvements are extreme - 1900 records are now returned to the front end in ~300ms.
   * UI speed improvements (with a patched UI) are extreme - <10 seconds to retrieve and display 1900 records (down from 10+ minutes)
### v0.61.5
* Jira PC3527-3009 - Added new System Error level "Informational" which does NOT alter system health status.
   * UI will need to adapt to this change to avoid showing "red".
* Jira PC3527-3011 - Ensure that all requests for an SResult / ResultRecord pass through the cached record
   * Fixes issue where Detailed Measurements were not being correctly returned for reanalyzed data.

### v0.61.1
* Reagent pack expiration logic change: 
  * Firmware doesn't block aspiration from reagent bottles on an expired pack but checks make sure there is atleast one usage left.
  * Firmware blocks dispense to FLow Cell from an expired pack.
* Added encryption for configuration files, Images, Result binaries and metadata xml file.
* Following are the examples of how the encrypted files look like :

   |**Original**|**Encrypted**|
   |---|---|
   |\*.info|\*.einfo|
   |\*.bin|\*.ebin|
   |\*.xml|\*.exml|
   |\*.png|\*.epng|
   |\*.*|\*.e*|
   
* ABI change from 8 to 10 images (allows better chance for background to "converge"
* Support for (x,y) coordinates in the detailed blob measurements (overlooked originally)
* Firmware powers down much of the system after 60 seconds of host inactivity (fan and 24v power)
* Expiration date fixes
   * Reagent Packs, Quality Controls all expire at midnight LOCAL time
* Autofocus sequence now coarse-searches through entire range of travel

### v0.61.0
* Add X/Y coordinate information to detailed blob measurements
* Roll algorithm to 4.3.3
   * Addresses issue with large clusters - these should /not/ be broken down since we cannot get accurate counts from them.

### v0.59.1
* Restructure memory-freeing API calls, calls that dynamically allocate memory
   * Addresses a flaw in the "generic" memory-freeing implementation.
* Significant rework of internal threading models at UI and Workflow boundaries.
* Start of rework to move hardware-representing classes to an asynchronous "operations" model.

### v0.58.25 (through 0.58.16)
* No changes - throwaway versions from release process.

### v0.58.15 (through 0.58.10)
* Code cleanup from static analysis utility reports
* Hardware initialization is now asynchronous
* API changes for 
   * Backlash settings
   * Workqueue labels
* Reads autofocus coarse/fine step size from configuration file
* Fixes for
   * -544 (Handle missing tube)
   * -565 (Fix probe go-to-top issues)
   * -560 (Camera frame frate issues)
   * -554 (Reagent expiration date wrong in UI)
   * -553 (Bugfix saving image to disk)
   
### v0.58.6
* Add EEPROM storage class to handle persistent storage
* Add Get/Set routines for serial number
   * Store serial number in EEPROM as well as config file
   * Cross-check both copies of serial number (prefer whichver isn't blank)
   * Report issue if both are non-blank
### v0.58.1
* Controller board voltage monitor error converted to Warning w/ added logging
* Fixes for
   * PC3527-551
   * -595 (Images don't load in review screen
   * -683 (Empty syringe pump before starting workflows
   * -672 (Initialization failure), -669 (nightly clean broken)\
   * -41 (Error handling in WQ)
* Fix bugs in temporary analysis handling.
* Improvements to RFDI error handling
* Improvements in motor registration behavior, motor motion in general

### v0.53.3
* Added command line encryption tool

### v0.53.2
* TBD: Perry, Phil?

### v0.53.1
* Initial release intended to be merged to head of master repository
* At present state, 7 projects are not building and have been disabled
  in the Hawkeye.sln
* Added necessary components for release

### v0.0.5
* Updates from team as of 3/21/2018 1700 PDT
* Working build of RFIDProgrammingStation installer using NSIS

### v0.0.4
* Updates from team as of 3/20/2018 1700 PDT

### v0.0.3
* Version / assembly detail updated to match legal guideance

### v0.0.2
* Initial release from build chain
* Assembly info not complete

### v0.0.1
* Initial version
