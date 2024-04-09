# Design Guide for Instrument Data and Configuration Export Requirements{#data_config_export}

## Overview / Design Goals
* Intended solely for internal use (not for consumption by customers or 3rd parties
* Verifiable archive of instrument state
* Data and Configuration integrity is of the highest concern
* Capable of having output format updated / adjusted on future releases

## Configuration Export
### Specific Consideration
* Tamper-resistant.
   * Encrypted / signed
* File size expected to be small (<1MB) so compression not required

### Naming of Export File
In order to be of most use to the end user, the output filename should contain
* Product type ("ViCell-BLU")
* Indication of contents ("SystemConfig")
* Serial Number of originating instrument
* Timestamp in a universal format ("YYYYMMDD_HHmmss_UCT")

### Contents
There are three main types of isntrument configuration information to include
* Instrument-specific information - data that applies uniquely to this instrument and is unlikely to be usable by anything but the originating instrument
   * Serial number
   * Motor offsets / registrations
   * Focal position
   * Flow-cell depth
   * Lifetime sample counts
   * Size adjustment settings
   * Concentration adjustment settings
* Instrument "general" settings - data that reflects the specific configuration of this instrument but is applicable to other instruments (that is: configuration you'd want to "clone" to other instruments to harmonize a laboratory)
   * Cell Types, Analysis Types
   * User List
   * Bioprocess list
   * Quality Controls (active only)
   * Signature definitions
   * Inactivity timeouts
   * Security settings
* Static "As-shipped" settings - historical record of the non-volatile instrument settings.  These should be identical to other instruments of the same build and software level
   * Motion parameters
   * Image analysis algorithm settings
   
### Layout
1. Export File Type ("CONFIG")
1. Originating Product Type ("ViCELL-BLU")
1. File Format version ("1.0")
1. Originating Instrument Information
   1. Serial Number
   1. Timestamp
1. Configuration Data
   1. Instrument-Specific Section
      1. KEY : VALUE
      1. KEY : VALUE
      1. ... : ...
   1. Instrument-General Section
      1. KEY : VALUE
      1. KEY : VALUE
      1. ... : ...
   1. Static Settings Section
      1. KEY : VALUE
      1. KEY : VALUE
      1. ... : ...

## Data Export
### Specific Consideration
* Tamper-resistant.
   * Encrypted / signed
* File size expected to be significant (multi-MB to multi-GB) so compression / multi-part output REQUIRED
* Must support archive of partial or complete dataset
* Data must maintain tracability
   * Archive must include all accessory data required to display, analyze and verify the data
      * Cell type
      * Analysis settings
      * Signature definitions
      * User names
      * Images (All/some/none)
      * Metadata for linkages

### Naming of Export File
In order to be of most use to the end user, the output filename should contain
* Product type ("ViCell-BLU")
* Indication of contents ("SystemData")
* Serial Number of originating instrument
* Timestamp in a universal format ("YYYYMMDD_HHmmss_UCT")
