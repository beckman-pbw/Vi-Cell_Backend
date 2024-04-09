Application Source
=========

Backend Business Logic layer of the Hawkeye application package

Preferred implementation: robust DLL invoked by the UI layer.

This would allow us to quickly build other UIs on top of the business logic for development, service, etc.

###Here is the set of activities that the UI will need to perform (THIS LIST IS INCOMPLETE) : 

1. Initialization
  * DLL internal state 
  * Hardware start / stop / reset
1. Configuration
  * Retrieve R-Theta type (carousel or 96-well plate)
  * Focus Adjust
  * ?Network config
  * Log in / Log out
  * System calibration data (get/set)
1. Administration
  * User Add
  * User List
  * User delete / disable
  * Change user PW
  * Change user permissions
  * System ID change / view (serial number (RO), friendly name (RW))
  * Retrieve audit logs
  * Get/Set export folder locations
1. Service
  * Retrieve knob and status item lists
  * Get status
  * Set knob
  * Acquire camera image
  * Prime fluidics
  * Flush fluidics
  * Disinfect fluidics
1. Task Queue (my name for all the samples currently enqueued for execution
  * Retrieve task queue
  * Add task (carrier Position, name, identifier, cell type, answer params…)
  * Clear tatsk
  * Queue status (idle, executing, current task info…)
  * Start / pause / halt queue
  * Set configuration for unqueued samples (what happens to random adds to carousel after queue created, executed?)
1. AnswerPack / Reagents
  * Load / Unload
  * Get Status (volume(s) remaining, expiration date(s); abstracted as “number of samples” if possible)
  * AnswerPack Info (maybe return some help / informational about the pack / answers available)
1. Data & Results (READ-ONLY INTERFACE. UI does NOT get to modify results EVER)
  * Retrieve sample run log
  * Retrieve results (by run id, lot id, batch number, various things)
  * Retrieve images (by run ID) (probably returns a list of identifiers, then a separate call to pull an individual image from that list)
  * Reprocess image (with some new criteria; would create a new Result )
1. Cell Types
  * Retrieve List
  * Modify
  * Add
  * Delete
  * ?Copy
  * ?Import/Export from file (maybe make the UI handle this at a higher level so we don’t have to care?)

###Additonally...
...there is a set of abstractions around the hardware interface that we will want to create in order to get the building blocks that the business logic can stand on (this list is in rough order of priority, high to low):

1. Camera
  * Set settings
  * Acquire image (once triggered by controller board)
1. Controller Board
  * Owns the RS-232 interface / queue
  * Knows how to program firmware
1. Motor 
  * Stepper motor; does usual motor things
  * Don’t live alone, but owned by other devices who abstract an interface around them
1. Camera Trigger
  * Set exposure time
  * Trigger image acquisition (one-shot)
1. Sample Stage (R-Theta)
  * 2 motors : radius, angular(theta)
  * Should be able to identify type somehow (either direct by sensor data or indirectly by being told)
    * 12-position carousel
    * 96-well plate
  * Get number of sample positions
  * Move to sample position X
1. Syringe Pump
  * Composite of
    * Syringe (Aspirate / Dispense)
    * Valve selector (8 ports)
  * Commands
    * Set Valve position (NB: valve “meanings” are known (ex: 1: Trypan Blue, 2: waste, etc) to the creamy filling)
    * Aspirate N uL   (NB: Controller board should FAIL THIS REQUEST if the available liquid for a given port (that is: a reagent) is insufficient)(NB: may want to prohibit aspirating from certain valve positions (waste, flow-cell).
    * Dispense N uL (NB: probably want to prohibit dispensing to certain valve positions (should never dispense back into a reagent bottle)
1. Probe Motor
  * Motor (with some complex motions pre-written
  * Pierce (forceful/fast down through possible seal, gentle/slower to bottom of sample to stall, back up a bit from bottom)
  * Retract (back to the top of travel; do NOT do this while anything might be sucking!)
  * Get Final Z (NB: stalls out against bottom of sample well; need to monitor final resting position so we can tell if we’re stalling early)
1. Camera Focus
  * In/Out
  * Store new offset from home
1. Bubble Detector
  * Is Bubble?
  * Is Fluid?
  * Calibrate Air
  * Calibrate Fluid
  * Num bubbles since… / Clear bubble count
  * ?OnBubble (callback)
1. Reagent Pack Motor
  * Motor
  * Load
  * Unload (do NOT do this while anything may be aspirating from the reagents!
1. Configuration
  * Load / Store
1. LED Rack Select Motor
  * Motor
  * Several distinct target locations to move different LEDs / optics into place (HUNTER)
1. LED
  * Brightness / intensity setting
  * On/Off



  
