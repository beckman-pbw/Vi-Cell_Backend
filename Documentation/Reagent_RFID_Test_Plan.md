# Reagent Tracking Feature Stress Test{#reagent_tracking_test}

____

## Definitions

* *Valid Reagent Pack* – An unexpired reagent pack with at least 4 uses left
* *Expired Reagent Pack* – A reagent pack past EXP date or past Service Life after put in service
* *Authentic Reagent Pack* – A reagent pack with all security configurations in place. Un-authentic if not programmed, or programmed with no or different key than the valid master key or failing any other security query
* *Invalid reagent Pack* – Could be any or combination of expired, un-authentic and 0 uses left
* *Flushing Kit* - Special reagent pack with Cleanz filled in all bottles and the RFID tag says so.
* *Unload Reagent Pack* – Lifts the Reagent arm up, unlatches the Door and latches it back after few seconds. It comes with 3 options, None, Purge & Drain. With None being default, doesn’t do anything before lifting the arm up. 
* *Purge* – Reagent arm raised up to bring reagent probes out of the liquid but keeps them in the bottles so that air can be aspirated into the lines and fluid in the tubes pushed to waste.
* *Drain* – With Reagent arm still down, fluids from all bottles aspirated and dispensed to waste, until number of uses in the RFID tag for all becomes 0. Note it is not expected to empty the bottle, but only empty the uses remaining.
* *Firmware Test Application* – A C++ application project using Hawkeye code base to test Controller Board functionalities

____

## Positive Path Test

### Unload & load a valid reagent pack (just priming)
Expected – Application boots up with or without a warning depending on the pack present. Successfully unload an existing reagent pack and load a new one with all fluid lines aspirates new fluid and existing fluid pushed to waste.

### Start application with a valid reagent pack
Precondition - Reagent pack loaded successfully as in the previous test

Expected – Reagent pack recognized and arm brought down on boot up

### Unload a valid reagent pack with purging and load a valid reagent pack
Precondition - Reagent pack loaded successfully

Expected – Reagent arm raised up to top of bottle, all fluid pushed to the waste, new reagent pack or the same reagent pack is successfully loaded priming all 4 fluid lines

### Unload a valid reagent pack with draining and load a valid reagent pack
Precondition - Reagent pack loaded successfully

Expected – Reagent pack recognized and arm brought down on boot up

### Unload a reagent pack and load a Flushing kit (just priming)
Expected – Reagent pack recognized, all fluid lines aspirates cleaner and existing fluid pushed to waste.

____

## Error Condition Test

### Start with no pack
Expected – Reagent arm stays high and user is warned about no Reagent pack.

### Unload and load an Expired pack 
Expected – User is warned about pack expiry and prompt to replace reagent pack again.

### Start with invalid pack
Test Steps – Replace reagent pack with an invalid pack (expired, 0 uses, invalid), close the application at error state (door closed). Restart the application

Expected – User is warned about invalid reagent pack (should say expired if it is expired, empty if 0 uses and invalid if validation failed or authentication failed), Reagent arm position unchanged

### Unload an invalid pack and load a valid pack
Test steps - 
* Start the application 
* Replace the reagent pack with an invalid pack 
* When errors displayed close Replace Reagnet window
* Clear errors in instrument status screen
* Do replace reagent pack again

Expected – Successfully replace the reagent pack

### Unload an expired pack with purge selected
Test steps - Leave a an expired reagent pack in reagent bay, do replace reagent pack, select Purge during replace reagent pack unload

Expected – Reagent pack purge completes successfully, user prompted to load new reagent pack

### Unload an expired pack with drain selected
Test steps - Leave a an expired reagent pack in reagent bay, do replace reagent pack, select Drain during replace reagent pack unload

Expected – Reagent pack drain completes successfully, user prompted to load new reagent pack

### Reagent pack expires in middle of work queue run
Test Steps – 
* Find effective expiration date of a valid Reagent pack in hand (should have sufficient uses left for at least half of the 96 well plate)
* Through PC admin login, set Instrument time to 11:30 PM with date same as effective expiration date
* Start the application, run a 96 well plate with all well selected

Expected – 
* Application boots up with not errors / warnings
* Work queue execution starts on play
* Work queue paused after several sample due to expired reagent pack
* User is informed of error, Reagent icon goes red
* Replace reagent option enabled for user to replace reagent pack
* Work queue can be resumed after replacing the reagent pack

### System time lagging UTC and reagent pack expires on same day
Test Steps – 
* Find effective expiration date of a valid Reagent pack in hand
* Through PC admin login, set Instrument time to Hawaiian Time Zone (UTC - 10) with date same as the effective expiration date
* Start the application at 4:30 AM MST, run a sample after 5PM MST (i.e., past 12:00 am UTC ) 
* Unload and load the same reagent pack

Expected – 
* Application start
* Sample run completes successfully
* Reagent pack reload successful

### System time leading UTC and reagent pack expires on same day
Test Steps – 
* Find effective expiration date of a valid Reagent pack in hand
* Through PC admin login, set Instrument time to Indochina Time Zone (UTC + 7) with date same as the effective expiration date
* Start the application at 9:30 AM MST, run a sample after 10 AM MST (i.e., past 12:00 am Indochina Time)
* Replace reagent pack with a valid one

Expected – 
* Application start
* Sample run fails due to expired reagent pack
* Reagent pack replace successful

### Reagent expires on a leap day
Test Steps – 
* Using RFAPPS, make a special RFID tag which expires on a leap day (Feb 29, 2020)
* Through PC admin login, set Instrument date to Feb 29 2020, time to 11:30 PM
* Start the application,  unload and load the Reagent pack with leap day expiration
* Run a sample after 30 minutes from login

Expected – 
* Application starts (warns about expired pack, if pack present or warns about no pack present )
* Reagent unload & load completes successfully
* Sample run fails due to expired reagent pack

### Reagent expires on day of day light saving begins
Test Steps – 
* Using RFAPPS, make a special RFID tag which expires on the day daylight savings begins (Oct 06, 2019)
* Begin the test at 7:00 AM MST
* Through PC admin login, set Instrument date to Oct 05 2019, with time zone set to  Australian Central Standard Time (Adelaide, AUS)
* Start the application, unload and load the Reagent pack with RFID tag programmed in the first step
* Run a sample 30 minutes from login

Expected – 
* Application starts (warns about expired pack, if pack present or warns about no pack present )
* Reagent replaced successfully
* Sample run fails due to expired reagent pack

### Reagent expires on day of day light saving ends
Test Steps – 
* Using RFAPPS, make a special RFID tag which expires on the day daylight savings ends (Apr 07, 2019)
* Begin the test at 7:00 AM MST
* Through PC admin login, set Instrument date to Apr 06 2019, with time zone set to  Australian Central Standard Time (Adelaide, AUS)
* Start the application, unload and load the Reagent pack with RFID tag programmed in the first step
* Run a sample 30 minutes from login

Expected – 
* Application starts (warns about expired pack, if pack present or warns about no pack present )
* Reagent replaced successfully
* Sample run fails due to expired reagent pack

### Reagent expires in middle of loading (priming process)
Test Steps – 
* Find effective expiration date of a valid Reagent pack in hand
* Through PC admin login, set Instrument date to time to same as the effective expiration date and time to 11:30 PM
* Start the application
* Begin replacing reagent pack exactly at 11:59

Expected – 
* Application starts
* Reagent pack replace fails in middle of priming

### Insufficient reagent uses for the sample
Test steps – 
Run a sample with empty reagent pack

Expected – 
Fails to run sample

### Insufficient reagent uses for the work queue
Test steps – 
Run a 96 well plate with all wells selected for the work queue with < 5 reagent uses left in the pack

Expected – 
Fails to run samples after 5

### Insufficient reagent uses for priming (< 4 uses)
Test steps – 
* Replace reagent pack with the one which has only 1 use left

Expected – 
* Fails to complete reagent pack load and prompts replace reagent pack again

### Reagent pack expired before nightly clean
Test steps – 
* Load an expired reagent pack
* Through admin login set the time to 1:30 AM 

Expected – 
* Fails to complete nightly clean at 2:00 AM due to expired reagent pack

### No uses left for nightly clean
Test steps – 
* Load a reagent pack with no uses left
* Through admin login set the time to 1:30 AM 

Expected – 
* Fails to complete nightly clean at 2:00 AM due to no reagent uses left

### Full syringe volume aspirations
Test steps – 
* Login as service
* In maintenance -> low level controls, set the valve to cleaner 1 
* Move syringe to 1000 uL (Aspirate) 
* Move valve to Waste
* Move syringe to 0 uL (Dispense)
* Repeat the same for 2 more times
* Repeat the same for all other reagents

Expected – 
* Log in as service, note down the number of uses remaining for each reagents in pack at start of test
* Reagents remaining uses should be down by 5 at the end of the test for all reagents

____

## Abnormal Condition Test

### Un-Authentic Reagent Pack
Test Steps 
* Load reagent pack with un-authentic RFID tag

Expected
* Invalid reagent pack error displayed and replace reagent option available

### No RFID Tag
Test Steps 
* Load reagent pack with NO RFID tag

Expected
* RFID read error displayed and replace reagent option available

### Reagent pack with RFID tag data corrupted
Test Steps 
* Load reagent pack with RFID tag data corrupted

Expected
* Invalid reagent pack error displayed and replace reagent option available

### Unplug power adaptor in middle of priming and sample processing
Test Steps 
* Start the application
* Load a valid reagent pack
* Start running a sample
* Unplug power cord when the UI shows capturing images
* Connect the Power cord back and start the application
* Run a sample

Expected
* Application starts and loads the reagent pack successfully
* Sample processing begins
* Instrument shutdown abruptly 
* Application boots up without any issues
* Sample runs successfully

### Close application in middle of priming and sample processing
Test Steps 
* Start the application
* Load a valid reagent pack
* Start running a sample
* Close the application when the UI shows capturing images
* Restart the application
* Run a sample

Expected
* Application starts and loads the reagent pack successfully
* Sample processing begins
* Application closes abruptly, controller board continues to run
* Application boots up without any issues
* Sample runs successfully

____

## Stress Test

### Reload An Invalid Pack For Several Times In a Row
Test Steps 
* Load an invalid reagent pack
* Keep loading the invalid pack for 30 times in a row
* Load a valid reagent pack

Expected
* Fails to load the reagent pack as many times tried 
* Eventually loads the reagent pack successfully

### Reload A Valid Pack For Several Times In a Row
Test Steps 
* Unload and Load a valid reagent pack
* Keep loading the invalid pack for 30 times in a row

Expected
* Loads the reagent pack successfully every time with 4 reagent uses reduced 

## Stress Test Controller Board Firmware

### Scan RFID Tag
Test setup
* Scout instrument with a valid reagent pack 
* Firmware Test Application to test RFID scan, read & write operations of Controller Board firmware

Firmware test steps coded in test application
* Send RFID scan command to Reagent SM, wait for command completion
* Read the Reagent SM error code and Reagent register content
* Repeat last to steps for 1000 times with no delays between the commands

Expected
* RFID scan command completes successfully
* Reagent data read from RFID tag is same for all 1000 iterations

### Update RFID Tag
Test setup
* Scout instrument with a valid reagent pack, pierced or put in service before
* Firmware Test Application to test RFID scan, read & write operations of Controller Board firmware

Firmware test steps coded in test application
* Home Sample Probe motor
* Park Sample probe motor at -3mm above home
* Home Radius motor
* Send current instrument time to Reagent SM
* Send scan command to Reagent SM and read back results
* Read RFID data contents and note remaining uses for Cleaner1, Cleaner 2, Cleaner 3 & Reagent 1.
* Send Syringe valve port mapping to Reagent SM
* Repeat the following steps for aspirating & dispensing the reagent fluids for 100 times in a row
* Set Syringe Valve to Cleaner 1 
* Aspirate 600 uL of Cleaner 1 at 300 uL/second rate
* Move radius motor to 80 mm position
* Aspirate 300 uL of Cleaner 1 at 150 uL/second rate
* Set Syringe valve to waste 
* Read and verify the Reagent state machine error code to make sure the tag usage update is successful
* Dispense 900 uL of Cleaner 1 at 900 uL/second rate
* Set Syringe Valve to Cleaner 2 
* Aspirate 600 uL of Cleaner 2 at 300 uL/second rate
* Move radius motor to 0 mm position
* Aspirate 300 uL of Cleaner 2 at 150 uL/second rate
* Set Syringe valve to waste 
* Read and verify the Reagent state machine error code to make sure the tag usage update is successful
* Dispense 900 uL of Cleaner 2 at 900 uL/second rate
* Set Syringe Valve to Cleaner 3 
* Aspirate 600 uL of Cleaner 3 at 300 uL/second rate
* Move radius motor to 80 mm position
* Aspirate 300 uL of Cleaner 3 at 150 uL/second rate
* Set Syringe valve to waste 
* Read and verify the Reagent state machine error code to make sure the tag usage update is successful
* Dispense 900 uL of Cleaner 3 at 900 uL/second rate
* Set Syringe Valve to Reagent 1
* Aspirate 150 uL of Reagent 1 at 50 uL/second rate
* Move radius motor to 0 mm position
* Aspirate 150 uL of Reagent 1 at 150 uL/second rate
* Set Syringe valve to waste 
* Read and verify the Reagent state machine error code to make sure the tag usage update is successful
* Dispense 300 uL of Reagent 1 at 300 uL/second rate
* After the repeated aspiration & dispense cycles, read RFID data content and check remaining uses

Expected
* Time is set in Controller Board
* Valve map is set in Controller Board without any errors
* Syringe valve move, aspiration & dispense completes without any errors
* Reagent usage for Cleaner 1 is decremented by 150, for Cleaner 2 is decremented by 150, for Cleaner 3 is decremented by 150 and for Reagent 1 is decremented by 200, from beginning count.

____

## Test Results Iteration 1 - 21-Dec-2018, SW ver 0.12.17, Firmware ver 1.1.0.29 

### Positive Path Test

#### Unload & load a valid reagent pack (just priming)
Results – Passed

#### Start application with a valid reagent pack
Results – Passed

#### Unload a valid reagent pack with purging and load a valid reagent pack
Results – Passed

#### Unload a valid reagent pack with draining and load a valid reagent pack
Results – Passed

#### Unload a reagent pack and load a Flushing kit (just priming)
Results – Flushing kit not available for test
____

### Error Condition Test

#### Start with no pack
Results – Passed

#### Unload and load an Expired pack 
Results – Passed

#### Start with invalid pack
Results – Passed

#### Unload an invalid pack and load a valid pack
Results – Passed

#### Unload an expired pack with purge selected
Results – Passed

#### Unload an expired pack with drain selected
Results – Passed

#### Reagent pack expires in middle of work queue run
Results – Passed

#### System time lagging UTC and reagent pack expires on same day
Results – Passed

#### System time leading UTC and reagent pack expires on same day
Results – Passed

#### Reagent expires on a leap day
Results – Need new tag to program pack expiry on a leapday

#### Reagent expires on day of day light saving begins
Results – Need new tag to program pack expiry on daylight savings beginning day

#### Reagent expires on day of day light saving ends
Results – Need new tag to program pack expiry on daylight savings ending day

#### Reagent expires in middle of loading (priming process)
Results – Passed

#### Insufficient reagent uses for the sample
Results – To be Tested

#### Insufficient reagent uses for the work queue
Results – To be Tested

#### Insufficient reagent uses for priming (< 4 uses)
Results – To be Tested

#### Reagent pack expired before nightly clean
Results – Passed

#### No uses left for nightly clean
Results – Passed

#### Full syringe volume aspirations
Results – Passed. 
Observation - UI display of remaining uses is inconsistent but backend logs shown correct results. UI displayed all fluids are down by 5 though only one of the fluid is aspirated.
____

### Abnormal Condition Test

#### Un-Authentic Reagent Pack
Results – Passed

#### No RFID Tag
Results – Passed

#### Reagent pack with RFID tag data corrupted
Results – Passed

#### Unplug power adaptor in middle of priming and sample processing
Results – Passed

#### Close application in middle of priming and sample processing
Results – Closing application in middle of priming or sample processing is not possible through EXIT menu. Application can be minimized in middle of sample processing and closed from task bar.
____

### Stress Test

#### Reload An Invalid Pack For Several Times In a Row
Results – Passed

#### Reload A Valid Pack For Several Times In a Row
Results – Passed

### Stress Test Controller Board Firmware

#### Scan RFID Tag
Results – Passed

#### Update RFID Tag
Results – Passed


