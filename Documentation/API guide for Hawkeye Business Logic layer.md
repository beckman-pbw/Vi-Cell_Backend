# API Guide for Hawkeye Business Logic Layer{#api_guide_business_logic}
As of 25 October 2018

---
# THIS DOCUMENT IS CONFIDENTIAL AND PROPRIETARY TO BECKMAN COULTER. IT IS NOT FOR DISTRIBUTION TO ANY OUTSIDE PARTY WITHOUT A SIGNED NDA.
----

# Project Configuration

## Library

“HawkeyeCore.lib”, “HawkeyeCore.dll” specific for 64-bit debug or release builds

Unless noted otherwise there is simulation and instrument support for all APIs.

## Includes

HawkeyeLogic.hpp - a single “convenience” include file for your code
which includes the following additional header files that define values
and structures related to different functionality in the system:

  - AnalysisDefinition.hpp
  - AnalysisDefinitionCommon.hpp
  - Bioprocess.hpp
  - CellTypes.hpp
  - CellTypesCommon.hpp
  - HawkeyeErrors.hpp
  - ImageWrapper.hpp
  - QualityControl.hpp
  - Reagent.hpp
  - ReagentCommon.hpp
  - ResultDefinition.hpp
  - Signatures.hpp
  - SystemStatus.hpp
  - UserLevels.hpp
  - WorkQueue.hpp
  - WorkQueueCommon.hpp

# Global Guidelines

### Guideline Naming Conventions

Unless the complete parameter list is necessary for the text, function
parameter lists will be omitted and replaced with “(…)”. This is not
intended to imply a variadic function signature.

### HawkeyeError Return Status

Many of the API functions return a status from the HawkeyeError enumerated values list.
Each of these status values has a general "intended meaning" which indicates a particular
class of failure to the caller.  In some instances, a particular function in the API may 
use one or more status values in a way that is not in line with the description of the 
general intent - those instances will be called out in the documentation and clarification
provided for that specific use.

| HawkeyeError              | Meaning                                                                                                                      |
|---------------------------|------------------------------------------------------------------------------------------------------------------------------|
| `eSuccess`                | Action/Request succeeded                                                                                                     |
| `eInvalidArgs`            | Provided argument(s) are invalid                                                                                             |
| `eNotPermittedByUser`     | Current user lacks correct permissions / no user logged in                                                                   |
| `eNotPermittedAtThisTime` | Call is correct, but system is not currently able to honor the request (try again later)                                     |
| `eAlreadyExists`          | Request would have created a duplicate entry                                                                                 |
| `eIdle`                   | System is not presently executing an activity                                                                                |
| `eBusy`                   | Request cannot be honored because system is currently executing a related task / already running that sequence               |
| `eTimedout`               | Unable to execute request in the allowed time                                                                                |
| `eHardwareFault`          | Request cannot be honored because the system has a hardware fault / experienced a hardware fault while executing the request |
| `eSoftwareFault`          | Request cannot be honored because the system has a software fault / experienced a software fault while executing the request |
| `eValidationFailed`       | An item/object/resource required for the request failed validity checks                                                      |
| `eEntryNotFound`          | A specific resource referenced by the request was not found (failure: resource was expected to exist)                        |
| `eNotSupported`           | Feature not yet implemented / Feature unsupported on this hardware                                                           |
| `eNoneFound`              | The requested search returned no matching items (NOT a failure condition)                                                    |
| `eEntryInvalid`           | Resource / item failed an integrity check (similar to `eValidationFailed`                                                    |
| `eHardwareNotInitialized` | Request cannot be honored because hardware has not yet been initialized                                                      |
| `eStorageFault`           | Software experienced a disk fault.                                                                                           |
| `eStageNotRegistered`     | Request cannot be honored because plate,carousel is not registered, demands motor registration.                              |
| `ePlateNotFound`          | Request cannot be honored because plate not found.                                                                           |
| `eCarouselNotFound`       | Request cannot be honored because carousel not found.                                                                        |
| `eLowDiskSpace`           | Request cannot be honored because system running out of memory.                                                              |
| `eReagentError`           | Request cannot be honored because reagent pack is expired/invalid/empty.                                                     |
| `eSpentTubeTray`          | Request cannot be honored because spent tube tray is full.                                                                   |

### Memory Management

Both the API and the Host program may allocate memory during execution.
Ownership of memory is NEVER transferred across the API boundary. That
means:

  - The module which allocates the memory MUST dispose of it.
  - All pointers and lists allocated by the API and returned to the Host
    MUST be released using the appropriate functions within the API as
    soon as possible. The Host should make a copy of any
    dynamically-allocated memory within its own address space if the
    data in that memory needs to be held for any significant time.
      - If a specific deletion routine is provided by the API for a
        structure, then that deletion routine MUST be used to release
        memory used by that structure. The “general purpose” release
        function may be used for any structures or lists that do not
        have a specific deletion routine provided.
  - Memory which has been allocated by the Host and passed into the API
    MUST remain valid until the API call returns. The Host is
    responsible for releasing that memory.

### Callback Functions

Certain API functions require one or more “callback” function pointers
which will be invoked asynchronously at some point in the future.

All callback functions:

  - Must remain valid through the life of the procedure being executed
    (ex: the status and completion callbacks provided to
    `StartWorkQueue(…)` must remain valid until the queue reports full
    completion or (after calling `StopQueue()`) or full
    cancellation/stoppedness.
  - Must be thread-safe. No guarantees are made on what thread will be
    used to invoke a callback function.
  - Must return promptly. A blocking or long-running function may
    adversely affect the performance of the API.

### Polling

With the exception of callback routines, the API is entirely driven by
requests from the Host. This includes the process of acquiring the
current system status. It is recommended that the Host periodically poll
the API for information which will need to be updated and displayed on a
regular basis. Examples of API commands which should be polled regularly
are:

  - `GetWorkQueueStatus(…)`
  - `GetSystemStatus(…)`
  - `IsInitializationComplete(…)`
  - `IsShutdownComplete()`

# Startup and Shutdown

## `void Initialize(bool with_hardware = true)`

Begins the initialization sequence for the instrument including loading
of the configuration from disk. Must be performed before the instrument
can be used. The host should call IsInitializationComplete()periodically
until that function returns true. The various status functions may be
called during this period as well, but it should be expected that any
information returned may be incomplete.

## `InitializationState IsInitializationComplete()`

Returns the current state of the initialization, refer to the
`InitializationState.hpp` file for the
definition.

## `void Shutdown()`

Begins the shutdown sequence for the system. All existing operations
will be stopped, data will be committed to disk and hardware will be
moved to a safe and un-powered state. The Host should call
`IsShutdownComplete()` periodically until that function returns true,
after which the Host application may safely exit or power the instrument
off. No other API functions except the various status functions may be
called after a shutdown has been requested.

## `void IsShutdownComplete()`

Returns “true” when the shutdown process has completed successfully.

# System Status

These functions may be called at any time.

## `void GetSystemStatus(SystemStatus*& status)`

Returns the current state of the system. The API will allocate memory
for “status” which must be disposed of by calling `FreeSystemStatus(…)`.

## `void FreeSystemStatus(SystemStatus* status)`

Release memory allocated by `GetSystemStatus(…)`. This function MUST be
used to free the SystemStatus structure.

## `char* SystemErrorCodeToString(uint32_t system_error_code)`

Takes a reported error code and returns an English-language string
description (this is a concatenation of the individual strings provided
by `SystemErrorCodeToExpandedStrings(…)`).

Unknown / invalid error codes will return a NULL pointer.

## `void SystemErrorCodeToExpandedStrings(uint32_t system_error_code, char*& severity, char*& system, char*& subsystem, char*& instance, char*& failure_mode)`

Takes a reported error code and returns a set of English-language
strings describing the error by Severity, System, Sub-system, Instance
of a sub-system (ex: a particular motor, where “motor” is a sub-system)
and Failure Mode.

Unknown / invalid error codes will return NULL pointers.

## `void SystemErrorCodeToExpandedResourceStrings(uint32_t system_error_code, char*& severity, char*& system, char*& subsystem, char*& instance, char*& failure_mode, char*& cellHealthErrorCode)`

Takes a reported error code and returns a set of character
strings used to look up translation resources that describe the error by Severity, System, Sub-system, Instance
of a sub-system (ex: a particular motor, where “motor” is a sub-system), 
the Failure Mode and the CellHealth error code.
Strings begin `LID_API_SystemError_`

Unknown / invalid error codes will return NULL pointers.

## `HawkeyeError ClearSystemErrorCode(uint32_t active_error)`

Request that a particular set error code be cleared (intended for use
after an error condition has been addressed or acknowledged). An error
may immediately be re-raised if the error condition is not resolved.

Returns

  - eSuccess
  - eNotPermittedByUser – a user must be logged in in order to clear an error.

## `void GetVersionInformation(SystemVersion& version)`

Returns version information for various components in the system as
simple strings.

## `const char* GetErrorAsStr (HawkeyeError he)`

Returns a string representation of a HawkeyeError value.

## `const char* GetPermissionLevelAsStr (UserPermissionLevel permissions)`

Returns a string representation of a UserPermissionLevel value.

## `const char* GetReagentDrainStatusAsStr (eDrainReagentPackState status)`

Returns a string representation of a eDrainReagentPackState value.

## `const char* GetReagentPackLoadStatusAsStr (ReagentLoadSequence status)`

Returns a string representation of a ReagentLoadSequence value.

## `const char* GetReagentPackUnloadStatusAsStr (ReagentUnloadSequence status)`

Returns a string representation of a ReagentUnloadSequence value.

## `const char* GetReagentFlushFlowCellsStatusAsStr (eFlushFlowCellState status)`

Returns a string representation of a eFlushFlowCellState value.

## `const char* GetReagentDecontaminateFlowCellStatusAsStr (eDecontaminateFlowCellState status)`

Returns a string representation of a eDecontaminateFlowCellState value.

## `const char* GetWorkQueueStatusAsStr (eWorkQueueStatus status)`

Returns a string representation of a eWorkQueueStatus value.

## `HawkeyeError SampleTubeDiscardTrayEmptied()`

Sends a signal to the Host that the user has emptied the sample discard
tray and that the tray remaining capacity counter (returned in the
SystemStatus structure) should be reset.

*Do not call this function* without having the user actually empty the
discard tray – the system is at risk of a mechanical fault if the tray
capacity is exceeded.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions 
  

## `HawkeyeError GetSystemSerialNumber(char*& serialNumber)`

Returns

  - eSuccess
  - eNoneFound - no serial number currently recorded (serialNumber will be NULL)
  - eEntryInvalid - serial number data is invalid; serialNumber will have a value, but this return code
                    indicates that Service must reset the instrument serial number as soon as possible.

## `bool GetSystemSecurityFeaturesState()`

Returns the current status of the security features on/off state.

# User Information, Login / Logout

In order to display a helpful login screen showing a list of possible
users, the list of User IDs and User Display Names may be obtained using
the following functions:

## `HawkeyeError GetUserList(bool only_enabled, char**& userList, uint32_t& numUsers)`

Allocates a list of null-terminated strings containing the available set
of User IDs. userList must be disposed of through
`FreeListOfCharBuffer(…)`. For getting the list of only users who are
enabled/active, set `only_enabled` to true.

Returns

  - eSuccess

## `HawkeyeError GetUserDisplayName(const char* name, char*& displayname)`

Returns the Display Name associated with a particular UserID (specified in name). `displayname` must be disposed of through `FreeCharBuffer(…)`.

Returns

  - eSuccess
  - eInvalidArgs – specified user does not exist
  - eValidationFailed - information for the specified account failed validation

## `HawkeyeError GetCurrentUser(char*& name, UserPermissionLevel& permissions)`

Return the user ID and permission level of the currently-logged-in user.
name must be disposed of through `FreeCharBuffer(…)`.

Returns

  - eSuccess
  - eNotPermittedAtThisTime - No user is currently logged in

## `HawkeyeError LoginUser(const char* name, const char* password)`

Attempts to log in the user ID specified in name using the supplied
password.

Returns

  - eSuccess
  - eInvalidArgs – user does not exist or password does not match; user is not enabled.
  - eNotPermittedAtThisTime – a user is currently logged in; current user must log out first.

## `void LogoutUser()`

Logs out the current user

## `HawkeyeError SwapUser(const char* newusername, const char* password)`

Attempt to "swap" active users. If no user is presently logged in, this
API behaves identically to LoginUser(...). If a user is presently logged
in, the system will attempt to validate the new user's credentials. If
validation succeeds, then the existing user is logged out and the new
user is logged in. If validation fails, then an invalid access attempt
is recorded and the existing user remains logged in.

Returns

  - eSuccess
  - eInvalidArgs – user does not exist or password does not match; user is not enabled.
  - eNotPermittedAtThisTime - Failed to log out the current user.
  
## Providing Secured Access for the Host 

At times, the Host needs to perform privileged activities without a user
being actively logged in. An example is in disabling a user account due
to suspicious activity. The ability to perform such actions needs some
level of protection, so the API provides for a rolling “Host” password.
A password is generated using the function `GenerateHostPassword(…)` but
the Host must provide a shared security key used to generate the
password and then supply that password promptly to the function that
requires it.

The security key at this version of the API is `"Phil, Dennis and Perry are pretty neat guys"` and INCLUDES the interior set of quotation marks(as plain quotation marks, not any “smart quotes” that may be rendered
by this or other documents). **THIS INFORMATION IS CONFIDENTIAL AND PROPRIETARY**.

The Host account is protected by a rolling password similar to the
Service user.

The Host also must log in in place of a “regular” user when the System
Security settings are disabled. In that condition, normal user accounts
are unused and a non-published, privileged account should be used by the
Host automatically. The user name for this account is “ViCELL” and will
have a Display Name of “ViCELL”. Access to this account is protected by
the Host password as described above.

THIS ACCOUNT IS TO BE USED ONLY WHEN SYSTEM SECURITY IS DISABLED.

## `uint32_t GenerateHostPassword(const char* securitykey)`

Generates a programmatically-determined password based on the provided
security key.

## `HawkeyeError AdministrativeUserDisable(const char* name, const char* hostpassword)`

Allows the Host program to disable a user without an administrative
account being logged in. This is intended for use when an account must
be disabled due to suspicious activity.

Returns

  - eSuccess
  - eInvalidArgs – user doesn’t exist, password is incorrect, or the Host attempted to disable a built-in system account
  - eNotPermittedAtThisTime – specified user is already logged in and cannot be disabled at this time.
  - eStorageFault - Failed to update the configuration
  
## `HawkeyeError AdministrativeUserEnable(const char* administrator_account, const char* administrator_password, const char* user_account)`

Allows the Host program to enable a user without an administrative
account being logged in. This is intended for use when an user 
who has locked the application (but not logged out of it) but who has since entered the wrong password too many times.

Returns

  - eSuccess - Administrator account verified, user account found and enabled
  - eInvalidArgs - Failed to locate user account or user does not exist
  - eValidationFailed - Failed to validate administrator_account or administrator_account lacks correct privileges
  - eStorageFault - Failed to update the configuration

# User Management

The User Management functions are divided into those which the current
logged in user can request of their own account and those which an
Administrator user can request of any account on the system.

### Current User

## `HawkeyeError ChangeMyPassword(const char* oldpassword, const char* newpassword)`

Change the current user’s password. The user must verify their current
password in order to change the password.

Returns

  - eSuccess
  - eNotPermittedByUser -  Logged in user does not have required permission
  - eNotPermittedAtThisTime – Failed to update user configuration
  - eValidationFailed – Invalid password/ password does not meet criteria
  - eInvalidArgs – Can't change password for built-in accounts
  - eStorageFault - Failed to update the configuration

## `HawkeyeError ValidateMe(const char* password)`

Verify the current user by providing the user’s password. The user
should be prompted for their password and their input should be
validated through a call to ValidateMe(…) prior to allowing any change
to system-critical parameters or before allowing the user to
sign/validate any data.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user does not have required permission
  - eValidationFailed – Invalid password

## `HawkeyeError GetMyCellTypeIndices (uint32_t& nCells, uint32_t*& celltype_index)`

Returns the list of cell types (referenced as a set of indices) to which
the current user has been given access.
  - eSuccess
  - eNoneFound - No entry found
  - eNotPermittedByUser

## `HawkeyeError GetMyAnalysisIndices (uint32_t& nAnalyses, uint32_t*& analysis_index)`

Returns the list of analyses (referenced as a set of indices) to which
the current user has been given access.

  - eSuccess
  - eNotPermittedByUser
  - eNoneFound – Property/value not set

## `HawkeyeError GetMyFolder(char*& folder)`

Returns the current user’s data folder. Implementation TBD. May not
accurately reflect a storage location on the instrument.
  - eSuccess
  - eNoneFound - No entry found
  - eNotPermittedByUser

## `HawkeyeError GetMyDisplayName(char*& displayname)`

Return the current user’s Display Name. `displayname` must be released by
calling `FreeCharBuffer(…)`.
  - eSuccess
  - eNoneFound - No entry found
  - eNotPermittedByUser

## `HawkeyeError GetMyProperty(char* propertyname, uint32_t& nmembers, char**& propertymembers)`

The Host may store additional information for each user as named
Properties. Each Property has a name (`propertyname`) and a list of
null-terminated strings in which the value(s) for the Property are
stored. The content and implementation of these Properties is entirely
up to the Host. A user’s Properties will be stored and are persistent
data.
Returns the data stored under the property identified by `propertyname`.
  - eSuccess
  - eNoneFound - No entry found
  - eNotPermittedByUser

## `HawkeyeError GetMyProperties(UserProperty*& properties, uint32_t& num_properties)`

Returns the complete collection of UserProperties data stored for the
current user. properties must be freed through a call to
FreeUserProperties(…).
  - eSuccess
  - eNoneFound - No entry found
  - eNotPermittedByUser

### Administrator

## `HawkeyeError AddUser(const char* name, const char* displayname, const char* password, UserPermissionLevel` permissions)

Create a new user. Note that it is not possible to create a user with
eService permissions. If the user name or display name is already in
use, the add will fail.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eAlreadyExists - Specified name or display name already exists
  - eInvalidArgs - name or password does not meet criteria; permissions
    has an invalid value

## `HawkeyeError RemoveUser(const char* name)`

Delete the specified user. eService users cannot be removed. The
currently-logged-in account cannot be removed.

Returns

  - eSuccess
  - eNotPermittedByUser
  - eInvalidArgs - specified user does not exist; cannot modify eService user
  - eNotPermittedAtThisTime - tried to remove the currently logged-in user
  - eStorageFault - Failed to update configuration file or backup

## `HawkeyeError EnableUser(const char* name, bool enabled)`

Enable or Disable the specified user. eService users cannot be disabled.
The currently-logged-in account cannot be disabled.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eInvalidArgs - Specified user does not exist/Built in account
  - eNotPermittedAtThisTime - Tried to disable the currently logged-in user
  - eValidationFailed - User validation failed; Failed to enable/disable the specified user
  - eStorageFault - Failed to update the configuration

## `HawkeyeError ChangeUserDisplayName(const char* name, char* displayname)`

Sets the Display Name associated with a particular UserID (specified in
name).

Returns
  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eInvalidArgs - username does not exist/Built in account
  - eValidationFailed - User validation failed
  - eStorageFault - Failed to update the configuration file.
  - eAlreadyExists - A user with the given display name exists.
  
## `HawkeyeError ChangeUserPassword(const char* name, const char* password)`

Set / change the password for the specified user. Cannot modify eService
users. The currently-logged-in account cannot be modified through this
API call (use ChangeMyPassword(…)).

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eNotPermittedAtThisTime - Cannot change PW of current user through this API
  - eInvalidArgs - Invalid password/username does not exist/ Built in account
  - eValidationFailed - Password does not meet criteria/User validation failed
  - eStorageFault -  Failed to update the configuration file.

## `HawkeyeError ChangeUserPermissions(const char* name, UserPermissionLevel permissions)`

Change the permission level of the specified user. Cannot modify
eService users. Cannot modify the currently-logged-in account. Cannot
assign eService permission to an account.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eNotPermittedAtThisTime - Cannot change permissions of the current user
  - eInvalidArgs - user does not exist/Built in account
  - eValidationFailed - Invalid permission/User validation failed
  - eStorageFault - Failed to update the configuration file.
  
## `HawkeyeError SetUserProperty(const char* uname, const char* propertyname, uint32_t nmembers, char** propertymembers)`

Sets the value of a Host-defined property. Value is a set of strings.
Host is responsible for transformation of any non-string data that it
wishes to store.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eInvalidArgs - user does not exist; invalid value
  - eValidationFailed - user name validation failed
  - eNotPermittedAtThisTime - configuration not provided
  - eStorageFault - Failed to update the configuration file.
  
## `HawkeyeError GetUserProperty(const char* uname, const char* propertyname, uint32_t& nmembers, char**& propertymembers)`

Get the value of a Host-defined property. If the property is not defined
for the user, a zero-length list will be returned. Host is responsible
for the transformation of any non-string data that it wishes to store.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eInvalidArgs - user does not exist
  - eValidationFailed - user name does not meet criteria
  - eNoneFound - entry not found.

## `HawkeyeError GetUserProperties(const char* uname, UserProperty*& properties, uint32_t& num_properties)`

Return the set of ALL properties defined for the specified user.

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eInvalidArgs - user does not exist
  - eValidationFailed - user name does not meet criteria

## `void FreeUserProperties(UserProperty* properties, uint32_t num_properties)`

Release memory held in a list of UserProperty objects that was allocated
by the API.

## `HawkeyeError SetUserCellTypeIndices (const char* name, uint32_t nCells, uint32_t* celltype_indices)`

Sets the list of Cell Types which the specified user is allowed to
access. Values in `celltype_indices` are references to
`CellType::celltype_index`. If nCells is zero, then the celltype indices
are deleted.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eInvalidArgs - service user property can not be changed
  - eValidationFailed - user name does not meet criteria
  - eStorageFault - Failed to update the configuration

## `HawkeyeError GetUserCellTypeIndices (const char* name, uint32_t& nCells, uint32_t*& celltype_indices)`

Get the list of cell types which the specified user is allowed to
access. Values in `celltype_indices` are references to
`CellType::celltype_index`. `Celltype_indices` must be disposed of through
`FreeListOfTaggedBuffers(…)`.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eValidationFailed - user name does not meet criteria
  - eNoneFound - no celltype indices found
  - eSoftwareFault - failed to update user configuration file.

## `HawkeyeError SetUserAnalysisIndices (const char* name, uint32_t n_ad, uint32_t* analysis_indices)`

Sets the list of analyses which the specified user is allowed to access.
Values in `analysis_indices` are references to
`AnalysisDefinition::analysis_index`. If `n_ad` is zero, then the analysis
indices are deleted.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eInvalidArgs - Service user property can not be changed; user does not exist
  - eValidationFailed - User data cannot be validated
 - eStorageFault - Failed to update the configuration
 
## `HawkeyeError GetUserAnalysisIndices (const char* name, uint32_t& n_ad, uint32_t*& analysis_indices)`

Gets the list of analyses which the specified user is allowed to access.
Values in `analysis_indices` are references to
`AnalysisDefinition::analysis_index`. `analysis_indices` must be disposed
of through `FreeListOfTaggedBuffers(…)`.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eValidationFailed - user name does not meet criteria
  - eNoneFound - user does not have any analyses.

## `HawkeyeError GetUserAnalyses (uint32_t& num_ad, AnalysisDefinition*& analyses)`

Gets the list of analyses which the specified user is allowed to access.
Values in `analysis_indices` are references to
`AnalysisDefinition::analysis_index`. `analysis_indices` must be disposed
of through `FreeSimpleArrayTypeMemory(…)`.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eNoneFound - analysis does not exist.
  
## `HawkeyeError GetUserFolder(const char* name, char*& folder)`

Get the folder in which the specified user’s data is exported to.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eInvalidArgs - user does not exist
  - eValidationFailed - user data cannot be validated

## `HawkeyeError SetUserFolder(const char* name, const char* folder)`

Sets the folder in which the specified user’s data is exported to.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eValidationFailed - user name does not meet criteria
  - eInvalidArgs - user does not exist
  - eStorageFault - Failed to write the user configuration file.

# User Custom Properties

The API includes some basic properties and functionality into the User
list, but future development of the Host / instrument may require
additional information to be stored for each user. The API functions
Get/SetUserProperty(…) allow for forward flexibility in defining and
storing this information. All properties are stored to disk and
protected against offline alteration.

# Cell Type Management

A Cell Type is the set of parameters that describes the physical
characteristics of a particular cell line that the instrument needs to
locate within a bright-field image. Diameter, sharpness, and circularity
are all parts of a Cell Type definition.

Cell Types are identified within the API primarily by a 32-bit unsigned
integer value. Beckman Coulter may provide a set of “well known” cell
type definitions as part of the API/Instrument. These Cell Types
(“factory” cell types) are identified as having indices within the
range 0x00000000 to 0x7FFFFFFF. Factory cell types may not be deleted or
modified by the Host / end user.

The Host / end user may define additional cell types with indices in the
range `0x80000000` to `0xFFFFFFFF` which may be modified of deleted at will.

Each user must be assigned a set of allowed Cell Types by an instrument
administrator. These are the only cell types which that user will be
able to access on the instrument.

## Administrator Interface

## `HawkeyeError GetFactoryCellTypes(uint32_t& num_ct, CellType*& celltypes)`

Gets the list of all Factory (vendor-defined) cell types on the system.
The list of cell types must be released through a call to
`FreeCellType(…)`.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eNoneFound - cell type does not exist;

## `HawkeyeError GetUserCellTypes(uint32_t& num_ct, CellType*& celltypes)`

Gets the list of all Host- / end-user-defined cell types on the system.
The list of cell types must be released through a call to
`FreeCellType(…)`.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eNoneFound - cell type does not exist;

## `HawkeyeError AddCellType(CellType celltype, uint32_t& ct_index)`

Adds a new Host-/end-user-defined cell type to the system.
`CellType::Label` must be unique and cannot be the same as any existing
cell type. `CellType::celltype_index` should be left as “0”; the new Cell
Type’s index will be assigned by the API and returned through the
`ct_index` parameter.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eSoftwareFault - Failed to update the info/config file
  - eAlreadyExists - cell type index / name already exists (cannot duplicate)
  - eInvalidArgs - Unable to update user list for new cell type 

## `HawkeyeError ModifyCellType(CellType celltype)`

Alters an existing Host/user-defined Cell Type. The cell type to modify
will be identified by the `CellType::celltype_index` field within the
supplied celltype parameter. Any changes to the cell type label cannot
conflict with an existing cell type.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eEntryNotFound - cell type does not exist;
  - eAlreadyExists - cell type index / name already exists (cannot duplicate)
  - eInvalidArgs - cannot modify / delete a factory-defined cell type

## `HawkeyeError RemoveCellType(uint32_t ct_index)`

Deletes a Host/user-defined Cell Type identified by the `ct_index`
parameter.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eNoneFound - cell type does not exist;
  - eInvalidArgs - cannot delete a factory-defined cell type
  - eValidationFailed - Failed to update the info file.

## Normal User Interface

## `HawkeyeError GetAllCellTypes(uint32_t& num_celltypes, CellType*& celltypes)`

Get the full set of cell types available on the instrument. Returns the
complete definition. The list of cell types must be released through a
call to FreeCellType(…).

Returns

  - eSuccess
  - eNotPermittedByUser – no user is presently logged in
  - eNoneFound - no celltypes defined in configuration

## `HawkeyeError GetAllAnalysisDefinitions (uint32_t& num_analyses, AnalysisDefinition*& analyses)`

Get a list of all analyses known to the instrument.

Returns

  - eSuccess
  - eNotPermittedByUser – no user is presently logged in
  - eNoneFound - no analyses defined in configuration.

## `HawkeyeError GetSupportedAnalysisParameterNames (uint32_t& nparameters, char**& parameters)`

Get list of all analysis parameter names known to the instrument

Returns

  - eSuccess
  - eNotPermittedByUser – no user is presently logged in
  
## `HawkeyeError GetSupportedAnalysisCharacteristics(uint32_t& ncharacteristics, characteristic_t*& characteristics)`

Get list of all analysis parameter characteristic keys known to the
system (`characteristic_t` is the “native” method of referring to a
particular property of a cell.

Returns

  - eSuccess
  - eNotPermittedByUser – no user is presently logged in
  
## `char* GetNameForCharacteristic(characteristic_t c)`

Return the display name for a particular characteristic. The returned
name will reflect the specificity of the characteristic (that is:
whether a particular subkey or sub-subkey was specified as part of the
characteristic). A NULL value will be returned for an unknown
characteristic value. If a subkey or sub-subkey is provided for a
characteristic that does not support such specifiers, then the specifier
will be IGNORED and the “base” name of the characteristic will be
reutned

# Analysis Management

Every Analysis begins as a “base analysis” – that is, one whose measured
characteristics are complete and correct but whose values are general,
and not optimized for any given Cell Type. And end user will need to
“specialize” an Analysis for a particular Cell Type to capture the
actual properties exhibited by those cells. The specialization is stored
as a property of the Cell Type for which it is intended and shares the
Index value of its parent analysis.

A certain set of analyses (indexes 0x0000 through 0x7FFF) are provided
by Beckman Coulter and may not be modified or removed (though they may
be specialized). Analyses created by the end user will have indices in
the range 0x8000 to 0xFFFF.

### Administrator Interface

## `HawkeyeError AddAnalysisDefinition (AnalysisDefinition ad, uint32_t& ad_index)`

Create a new user-defined analysis. The analysis’ index will be returned
in `ad_index` if the analysis was successfully created.

Returns

  - eSuccess
  - eNotPermittedByUser – Logged in user lacks sufficient permissions
  - eAlreadyExists - An analysis definition with the same name already exists
  - eStorageFault - Failed to update the configuration file.
  - eInvalidArgs - Unable to update the user list for the new analysis

## `HawkeyeError RemoveAnalysisDefinition (uint32_t ad_index)`

Delete the specified analysis.

Returns

  - eSuccess
  - eNotPermittedByUser
  - eEntryNotFound - analysis does not exist.
  - eStorageFault - Failed to write to Analysis.info file.
  - eInvalidArgs - Some parameter is invalid.

## `HawkeyeError ModifyBaseAnalysisDefinition (AnalysisDefinition ad, bool clear_specializations)`

Modify a base (user-defined) analysis definition and optionally remove
any existing specializations from the system.

Returns

  - eSuccess
  - eNotPermittedByUser – no user is presently logged in
  - eEntryNotFound - None found

## `HawkeyeError SpecializeAnalysisForCellType (AnalysisDefinition ad, uint32_t ct_index)`

Create a specialized version of an analysis definition for the given cell-type.
Returns
  - eSuccess
  - eNotPermittedByUser
  - eEntryNotFound - analysis does not exist;

Returns

  - eSuccess
  - eNotPermittedByUser – no user is presently logged in
  - eEntryNotFound - None found
  
## `HawkeyeError RemoveAnalysisSpecializationForCellType (uint32_t ad_index, uint32_t ct_index)`

Explicitly remove the specialized version of an analysis from the given
cell type.

Returns

  - eSuccess
  - eNotPermittedByUser – no user is presently logged in
  - eEntryNotFound - None found

### Normal User Interface

## `HawkeyeError GetFactoryAnalysisDefinitions (uint32_t& num_ad, AnalysisDefinition*& analyses)`

Returns the set of factory-supplied analyses. Use this list only as a
reference for the analysis names or for when you explicitly need the
base parameter values.
  - eSuccess
  - eNotPermittedByUser - Logged-in user lacks sufficient permissions / no user logged in
  - eEntryNotFound/eNoneFound - analysis does not exist

## `HawkeyeError GetUserAnalysisDefinitions (uint32_t& num_ad, AnalysisDefinition*& analyses)`

Returns the set of user-created analyses. Use this list only as a
reference for the analysis names or for when you explicitly need the
base parameter values.
  - eSuccess
  - eNotPermittedByUser 
  - eNoneFound - analysis does not exist.

## `HawkeyeError GetAllAnalysisDefinitions (uint32_t& num_ad, AnalysisDefinition*& analyses)`

Returns the set of all analyses. Use this list only as a reference for
the analysis names or for when you explicitly need the base parameter
values.

## `HawkeyeError IsAnalysisSpecializedForCellType (uint32_t ad_index, uint32_t ct_index, bool& is_specialized)`

Inquires whether a specialization of a given Analysis (`ad_index`) exists
for a given cell type (`ct_index`).

Returns

  - eSuccess
  - eNotPermittedByUser – no user is presently logged in
  - eEntryNotFound - None found
  
## `HawkeyeError GetAnalysisForCellType (uint32_t ad_index, uint32_t ct_index, AnalysisDefinition* ad)`

Returns the correct analysis definition for use with a given cell type
(that is: the specialized definition (if one exists) or the base
definition (if a specialization does not exist).
  - eSuccess
  - eNotPermittedByUser - Logged-in user lacks sufficient permissions / no user logged in
  - eEntryNotFound - analysis does not exist

# Reagent Status / Manipulation

The instrument consumes a steady stream of Reagents during operation.
Reagents are various types of fluids used to mark/stain cells (ex:
“Trypan Blue”) or to clean and sanitize the instrument (ex: alcohol,
“buffer”, “Clenz”). Reagents are introduced to the instrument in
Containers. Each container may hold one or more different Reagents and
may also contain a “waste” receptacle.

Each Container has an RFID tag embedded in it that is used to identify
the Container and its contents to the Instrument as well as to track the
usage of the various fluids within the container. The RFID tag also
provides a pseudo-unique identifier that the Instrument uses to
communicate with/about a particular container to the Host and to the
control hardware which interfaces with the RFID system.

Each fluid within a Container has a State that includes information
about the contents, quantities and expiration dates of the fluid. Each
State references a specific Reagent Definition for the fluid in use. The
Definition describes the qualities of a specific reagent – the name, a
globally-unique identifier for that particular Reagent Definition and
how many times that reagent must be mixed with a sample to allow the
correct reactions to take place.

The RFID tag on each container will provide the Instrument with the
Reagent Definitions for the fluids within it. For the Host, that means
that the list of known Reagent Definitions is dynamic and may change as
new Reagent Containers are introduced to the instrument.

## Reagent Definitions

## `HawkeyeError GetReagentDefinitions(uint32_t& num_reagents, ReagentDefinition* reagents)`

Returns the list of known Reagent Definitions. This list should be
cached periodically and used as a lookup source whenever the display
string for a given Reagent is required (Reagents are referred to within
the API by its `ReagentDefinition::reagent_index` value). The reagents
list must be released through a call to FreeReagentDefinition(…).

Returns

  - eSuccess
  - eHardwareFault - failed to read the rfid tag data.
  - eNoneFound - Pack is not installed

## `void FreeReagentDefinition(ReagentDefinition* list, uint32_t num_reagents)`

Releases memory held within a ReagentDefinition pointer allocated by the API.

## Reagent Status

## `HawkeyeError GetReagentContainerStatusAll(uint16_t& num_containers, ReagentContainerState*& status)`

Gets the status of all Reagent Containers presently known to the
Instrument. If the Instrument is in a state where no containers are
known (loading/unloading/unloaded…) then a single ReagentContainerState
structure is returned containing the overall status of the Reagent
system. The memory allocated by the status object must be released
through a call to FreeReagentContainerState(…).

Returns

  - eSuccess
  - eHardwareFault - failed to read the rfid tag data.
  - eNoneFound - Pack is not installed

## `HawkeyeError GetReagentContainerStatus(uint16_t container_num, ReagentContainerState*& status)`

Gets the status of a particular Reagent Container known to the
Instrument.

Returns

  - eSuccess
  - eHardwareFault - failed to read the rfid tag data.
  - eInvalidArgs - invalid container status requested
  - eNoneFound - Pack is not installed

## Reagent Maintenance

## `HawkeyeError UnloadReagentPack(ReagentContainerUnloadOption* UnloadActions, uint16_t nContainers, reagent_unload_status_callback onUnloadStatusChange, reagent_unload_complete_callback onUnloadComplete)`

Begin the process of unloading the Reagent Containers from the
Instrument. Each container has an additional option for handling excess
fluid (UnloadActions) that should be specified. If no action is
specified then ReagentUnloadOption::eULNone is assumed.

The status of the unload process will be provided to the host
periodically through the onUnloadStatusChange callback function pointer.
The process is completed when the returned status is set to one of the
“termination states” as described in the ReagentStatus.hpp header
file. When completed onUnloadCompleteChange is called.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged-in user lacks sufficient permissions / no user logged in.
  - eNotPermittedAtThisTime – Reagent pack is busy

## `HawkeyeError LoadReagentPack(reagent_load_status_callback onLoadStatusChange, reagent_load_complete_callback onLoadComplete)`

Begins the process of loading new Reagent Containers into the
Instrument. The status of the load process will be provided to the host
periodically through the onLoadStatusChange callback function pointer.
The process is completed when the returned status is set to one of the
“termination states” as described in the ReagentStatus.hpp header
file.

Due to the Instrument design, if one or more single-reagent containers
are detected in during the load process, the API requires the Host to
intervene and specify the physical locations of these containers. The
need for intervention is signaled if the load status changes to
ReagentLoadSequence::eLWaitingOnContainerLocation. At this point the
Reagent Container information will be available to the Host through the
GetReagentContainerStatus(…) API. The Host will specify the physical
locations of the container(s) by a call to
SetReagentContainerLocation(…) for each container after which the
load process will continue.

Returns

  - eSuccess
  - eNotPermittedAtThisTime – Reagent pack is busy
  - eNotPermittedByUser - Logged-in user lacks sufficient permissions / no user logged in.

## `HawkeyeError SetReagentContainerLocation (ReagentContainerLocation* location)`

Sets the physical location of any single-fluid Reagent Containers. Due
to design limitations the Instrument is unable to determine whether a
given container is in the Left or Right pocket in the ViCell FL reagent
door. Multi-fluid containers are always located in the main reagent bay.

This function must only be called when the Reagent Load process state
has transitioned to ReagentLoadSequence::eLWaitingOnContainerLocation.

Note that the Reagent load process will proceed AUTOMATICALLY as soon as
all containers are assigned valid locations (that is: as soon as NO
container has position eUNKNOWN or eNONE).

Returns

  - eSuccess - locations accepted and load procedure continuing.
  - eNotPermittedByUser - Logged-in user lacks sufficient permissions / no user logged in.
  - eInvalidArgs - attempted to set a location other than "main bay" for
    a multi-fluid reagent container; attempted to set a location other
    than "door left" or "door right" for a single-fluid reagent
    container.
  - eEntryNotFound -  No matching reagent container ID found

# Instrument Settings Status / Management

## Normal User Interface

## `void GetUserInactivityTimeout (uint16_t& minutes)`

Returns the active inactivity timeout value. ‘0’ indicates no inactivity
timeout is in use. While a setting may be configured for the inactivity
timeout, if the security features are disabled the inactivity timeout is
not enforced.

Returns:

  - *none*

## `void GetUserInactivityTimeout_Setting(uint16_t& minutes)`

Returns the inactivity timeout setting (the value that the administrator
wishes to use when the security features are enabled).

Returns:

  - *none*

## `void GetUserPasswordExpiration(uint16_t& days)`

Returns the active password expiration period. ‘0’ indicates that there
is no active password expiration period. This feature is disabled when
the system security features are disabled.

Returns:

  - *none*

## `void GetUserPasswordExpiration_Setting(uint16_t& days)`

Returns the password expiration period setting (the value that the
administrator wishes to use when the security features are enabled).

Returns:

  - *none*

## `HawkeyeError LogoutUser_Inactivity()`

Allows the User Interface to inform the API that the current user should
be logged out for exceeding the configured inactivity period. This is
necessary as the API has no visibility to the user’s activities if they
are not generating API calls.

Returns:

  - eSuccess
  - eNotPermittedByUser - User lacks sufficent permission
  - eInvalidArgs – no inactivity period is currently configured / security features are disabled

## `HawkeyeError GetConcentrationCalibrationStatus(double& slope, double& intercept, uint32_t& cal_image_count, uint64_t& last_calibration_time, uuid__t& queue_id)`

Get the current concentration calibration information including date of
last calibration and the UUID of the queue used to run the calibration
samples.
  - eSuccess
  - eEntryNotFound - No calibration entry found

## `HawkeyeError GetSizeCalibrationStatus(double& slope, double& intercept, uint64_t& last_calibration_time, uuid__t& queue_id)`

Get the current size calibration information including date of last
calibration and the UUID of the queue used to run the calibration
samples.

Returns

  - eSuccess
  - eNoneFound – no calibration is set

## Administrator Interface

## `HawkeyeError SetSystemSecurityFeaturesState (bool enabled)`

Allows an administrator to enable and disable the system’s security
features (including password expiration, inactivity timeouts and
per-user cell type limitations)

Returns

  - eSuccess
  - eNotPermittedByUser
  - eValidationFailed - credentials validation failed
  - eNoneFound - user not found
  

## `HawkeyeError SetUserInactivityTimeout (uint16_t minutes)`

Allows an administrator to set the desired inactivity timeout period.
This setting DOES NOT DIRECTLY AFFECT THE API BEHAVIOR as inactivity
timeouts MUST BE ENFORCED BY THE HOST (the API has no visibility to the
user’s activity on the instrument). The HOST IS RESPONSIBLE FOR TRACKING
USER ACTIVITY AND LOGGING AN INACTIVE USER OUT OF THE SYSTEM.

This feature is only active if the system security features are enabled.

Returns

  - eSuccess
  - eNotPermittedByUser – Logged-in user lacks sufficient permissions

## `HawkeyeError SetUserPasswordExpiration (uint16_t days)`

Allows an administrator to set the desired password expiration period.
The HOST IS RESPONSIBLE for tracking the password periods using the UserGetCurrentWorkQueueItem
Properties.

This feature is only active if the system security features are enabled.

Returns

  - eSuccess
  - eNotPermittedByUser – Logged-in user lacks sufficient permissions

## `HawkeyeError SetConcentrationCalibration(double slope, double intercept, uint32_t cal_image_count, uuid__t queue_id, uint16_t num_consumables, calibration_consumable* consumables)`

Sets the concentration calibration data. Host must provide the UUID of
the work queue used to run the concentration samples for this
calibration and traceability information for the consumables used.
Requires Service access.

Returns
  - eSuccess
  - eNotPermittedByUser – Logged in user lacks sufficient privileges
  - eStorageFault - failed to write calibration history

## `HawkeyeError SetSizeCalibration(double slope, double intercept, uuid__t queue_id, uint16_t num_consumables, calibration_consumable* consumables)`

Sets the size calibration data. Host must provide the UUID of the work
queue used to run the size samples for this calibration and tracability
information for the consumables used.

Returns

  - eSuccess
  - eNotPermittedByUser – Logged in user lacks sufficient privileges
  - eStorageFault - Failed to write calibration history

# Instrument Direct Control

## `HawkeyeError EjectSampleStage()`

Requests that the system put the sample stage into an easy-to-access
orientation. Carousel will move out from the instrument to the furthest
possible extent while maintaining the current rotational orientation.
Plate will move out from the instrument to the furthest possible extent
and rotate to present the plate to the user in a known orientation.

This API only requests that the motion occur – it will return
immediately after the request has been submitted, but before any move
completes. The Host must monitor the system status information to track
the progress of this request.

Returns

  - eSuccess - System ejected the sample stage
  - eNotPermittedByUser - Logged in user lacks sufficient permissions / no user logged in
  - eHardwareFault - the system is experiencing a fault which prevents execution of this request

  
## `HawkeyeError RotateCarousel(SamplePosition& tubeNum)`

When system is IDLE, request that the CAROUSEL be rotated to the next position.  This will fail if:
   - Carousel is not present
   - Sample stage has not been homed
   - System is not idle
   - Sample tube is detected in the active position (system will not intentionally discard a tube that it has not emptied)

Returns

  - eSuccess - System has rotated the carousel and "tubeNum" has been updated with the current position
  - eNotPermittedByUser - Logged in user lacks sufficient permissions / no user logged in
  - eNotPermittedAtThisTime - Carousel is not detected or Sample Tube is detected.
  - eHardwareFault - the system is experiencing a fault which prevents execution of this request
  - eStageNotRegistered - Sample carrier is not registered  

# Work Queue Creation / Control / Status

The primary feature of the Instrument is the automated preparation and
analysis of a set of cell samples. The end user (though the Host)
creates a queue of work that defines the number of samples to be
processed and the identifier, location, selected analysis and settings
for each sample. The Host delivers that work queue to the API which does
some validation and then begins processing the individual tasks,
periodically returning “answers” to the Host for each sample.

## Status

## `HawkeyeError GetWorkQueueStatus(uint32_t& queue_length, WorkQueueItem*& wq)`

Gets the current status of the last work queue given to the API. The
work queue will be sorted in the order of task execution which may
differ from the original order in which the queue was given to the API
(ex: the work queue began with a task in Carousel position \#1, but at
the time of execution the Carousel was oriented at position \#7. Any
work queue items between 7 and 24 will be executed before position \#1
is available for processing). WorkQueueItem pointers must be released
through a call to FreeWorkQueueItem(…).

Returns

  - eSuccess

## `HawkeyeError GetCurrentWorkQueueItem(WorkQueueItem*&)`

Gets the work queue item that is currently being processed by the
Instrument. WorkQueueItem pointers must be released through a call to
FreeWorkQueueItem(…).

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions / no user logged in
  - eNotPermittedAtThisTime - No queue / item in progress
  - eNoneFound - Work queue is empty

## Control

## `HawkeyeError SetWorkQueueImagePreference(eImageOutputType image_type)`

Sets the preference for the type of images returned in the work queue image result callback.
Should be set prior to calling `StartWorkQueue`, but may be changed during work queue execution (changes take effect on next processed image).
Setting is retained between work queues, but NOT across DLL sessions.

Default: `eImageAnnotated`

Returns:
   - eSuccess
   - eNotPermittedByUser - Logged in user lacks sufficient permissions / no user logged in
   

## `HawkeyeError SetWorkQueue(const WorkQueue& wq)`

Sets a work queue for execution by the system. Any previous queue must
be completed prior to calling this function. This call gives the API a
chance to reject the queue for invalid entries. Processing of the work
queue is requested through StartWorkQueue (…).

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions / no user logged in
  - eBusy - An existing work queue is already in progress.
  - eNotPermittedAtThisTime - System health is not okay/Insufficient disk space
  - eInvalidArgs - One or more items in the work queue are invalid (malformed or do not match hardware configuration)
  - eHardwarefault - Reagent pack error
  
## `HawkeyeError AddItemToWorkQueue(const WorkQueueItem& wqi)`

Adds a new WorkQueueItem to the current work queue. May be called while
a queue is executing. Request will be rejected if the queue cannot
support a new item or if the item calls out a sample position that is
already occupied.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions / no user logged in
  - eNotPermittedAtThisTime - No work queue is active.
  - eInvalidArgs - One or more items in the work queue are invalid (malformed or do not match hardware configuration)
	
## `HawkeyeError StartWorkQueue (workqueue_status_callback onWQIStart, workqueue_status_callback onWQIComplete, workqueue_completion_callback onQComplete, workqueue_image_result_callback onWQIImageProcessed)`

Requests that the API begin processing of the work queue most recently
set through SetWorkQueue(…). Queue status is periodically returned to
the Host through the provided callback functions:

  - When execution of a work queue item begins: onWQIStart
      - The WorkQueueItem sent in this callback must be freed by calling FreeWorkQueueItem.
  - When execution of a work queue item completes: onWQICOmplete
      - The WorkQueueItem sent in this callback must be freed by calling FreeWorkQueueItem.
  - When execution of the queue as a whole completes: onQComplete
  - When processing completes on an image acquired from a sample: onWQIImageProcessed

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions / no user logged in
  - eNotPermittedAtThisTime - Sample disposal tray is filled/ Carousel not present.
  - eBusy - An existing work queue/work flow operation is already in progress
  - eInvalidArgs - Callbacks are not set/ Invalid work queue/Empty plate work queue.
  - ePlateNotFound - Plate not present
  - eStageNotRegistered - Sample carrier is not registered
  
## `HawkeyeError SkipCurrentWorkQueueItem()`

*Support: none.  This API is no longer supported*

Requests that the Instrument skip processing of the remainder of the
currently-executing queue item.

\!\!\!\!TODO: API required to allow a Host-provided string that will be
recorded with the work queue execution records.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions / no user logged in
  - eBusy - An existing work queue is already in progress
  - eSoftwareFault - Invalid work queue item index
  - eInvalidArgs - No valid work queue has been given.

## `HawkeyeError PauseQueue()`

Requests that the Instrument pause the current work queue after
completing the currently-executing queue item.

An executing item is never interrupted; reaching the pause state may
take a minute or more depending on the remaining number of images to
acquire from the sample.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions / no user logged in
  - eNotPermittedAtThisTime -Work queue is not in progress

## `HawkeyeError ResumeQueue()`

Requests that the Instrument resume processing of a paused work queue.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions / no user logged in
  - eNotPermittedAtThisTime -Work queue is not in progress/ Plate/Carousel is not present.


## `HawkeyeError StopQueue()`

Requests that the instrument abandon processing of the current work
queue after the completion of the current item.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions / no user logged in
  - eNotPermittedAtThisTime -Work queue is not in progress

# Bioprocess Management

Bioprocesses are a way to connect a group of samples/results together to
track the behavior of a population of cells over time. An administrator
must add/manage the bioprocesses, but once created (and active) they are
available to all users.

### Administrator

## `HawkeyeError AddBioprocess(bioprocess_t bioprocess)`

Add a new bioprocess definition to the instrument.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions / no user logged in
  - eAlreadyExists – The bioprocess name is already in use
  - eStorageFault- Failed to update the info file.
  - eInvalidArgs – A parameter of the bioprocess definition is invalid (ex: cell type does not exist)

## `HawkeyeError SetBioprocessActivation(const char* bioprocess_name, bool activated)`

Activates / deactivates a bioprocess. New samples may only be run
against active bioprocesses.

Returns

  - eSuccess
  - eEntryNotFound – No bioprocess matching the supplied name was found.
  - eNotPermittedByUser
  - eStorageFault - Failed to write info file

## `HawkeyeError RemoveBioprocess(const char* bioprocess_name)`

Removes a Bioprocess definition from the instrument.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions / no user logged in
  - eEntryNotFound – No bioprocess matching the supplied name was found.
  - eStorageFault - failed to update configuration file

### Normal User

## `HawkeyeError GetBioprocessList(bioprocess_t*& bioprocesses, uint32_t& num_bioprocesses)`

Returns the list of all currently-defined bioprocesses.
  - eSuccess
  - eNotPermittedByUser - Logged-in user lacks sufficient permissions / no user logged in
  - eNoneFound - No entries found

## `HawkeyeError GetActiveBioprocessList(bioprocess_t*& bioprocesses, uint32_t& num_bioprocesses)`

Returns the list of all ACTIVE currently-defined bioprocesses.
  - eSuccess
  - eNotPermittedByUser - Logged-in user lacks sufficient permissions / no user logged in.
  - eNoneFound - No entries found

Quality Controls are another way to group samples/results together for analysis as a group.

### Administrator

## `HawkeyeError AddQualityControl(qualitycontrol_t qualitycontrol)`

Add a new Quality Control definition to the instrument

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions / no user logged in
  - eAlreadyExists – The quality control name is already in use
  - eStorageFault - Failed to update the info file.

## `HawkeyeError RemoveQualityControl(const char* qc_name)`

Removes a quality control definition from the instrument.

Returns
  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions / no user logged in
  - eEntryNotFound – No quality control matching the supplied name was found.

### Normal User

## `HawkeyeError GetQualityControlList(qualitycontrol_t*& qualitycontrols, uint32_t& num_qcs)`

Returns the list of all currently-defined quality controls
  - eSuccess
  - eNotPermittedByUser
  - eNoneFound - No entries found

## `HawkeyeError GetActiveQualityControlList(qualitycontrol_t*& qualitycontrols, uint32_t& num_qcs)`

Returns the list of all ACTIVE currently-defined quality controls (that is, those which have not expired)
  - eSuccess
  - eNotPermittedByUser - Logged-in user lacks sufficient permissions / no user logged in.
  - eNoneFound - No entries found# Data and Image Retrieval

# Data Retrieval
All data on the instrument is indexed by a UUID (an 16-byte value). The
data is arranged in a hierarchy as follows:

1.  Work Queue
    1.  Samples
        1.  Image Sets
            1.  Images
        2.  Results

Since much of this data can be extremely large, the Business Logic API
is structured to return a smaller "metadata" record at each level (creation dates, user,
etc.) as well as the UUIDs of the data immediately below it. This allows
for faster transactions while drilling down to the data set or images of
interest.

## Retrieve one or more Work Queue Records / Sample Records / Image Set Records / Image Records / Result Records.

`RetrieveXxxRecord` -  Retrieves a single record matching to the given UUID
`RetrieveXxxList` - Retrieves a records macthing to the given UUIDs
`RetrieveXxxRecords` - Retrieves the records based on "Start" and "end" time / Associated user.

"start" and "end" are in seconds since 1/1/1970 00:00:00 UTC
    A ZERO in either field indicates "beginning of time" or "end of time" respectively
`user_id` indicates the user of interest
    A NULL value for user_id indicates "all users"
`RetrieveXxxList` will only return the items which were retrieved successfully (`list_size` may not equal `retrieved_size`)
  The caller should be sure to check the returned set against the requested set - failure mode for any missing item 
  should be determined by calling the `RetrieveXxxRecord(...)` function explicitly for the missing and checking the return value.

## `HawkeyeError RetrieveWorkQueue(uuid__t id, WorkQueueRecord*& rec)`
Return 
  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data   
  - eEntryNotFound - Record/s not found

## `HawkeyeError RetrieveWorkQueueList(uuid__t* ids, uint32_t list_size, WorkQueueRecord*& recs, uint32_t& retrieved_size)`
Return 
  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data   
  - eNoneFound - Record/s not found

## `HawkeyeError RetrieveSampleRecord(uuid__t id, SampleRecord*& rec)`
Return 
  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data   
  - eEntryNotFound - Record/s not found

## `HawkeyeError RetrieveSampleRecordList(uuid__t* ids, uint32_t list_size, SampleRecord*& recs, uint32_t& retrieved_size)`
Return 
  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data   
  - eNoneFound - Record/s not found

## `HawkeyeError RetrieveSampleImageSetRecord(uuid__t id, SampleImageSetRecord*& rec); HawkeyeError RetrieveWorkQueue(uuid__t id, WorkQueueRecord*& rec)`
Return 
  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data   
  - eEntryNotFound - Record/s not found

## `HawkeyeError RetrieveSampleImageSetRecordList(uuid__t* ids, uint32_t list_size, SampleImageSetRecord*& recs, uint32_t& retrieved_size)`
Return 
  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data   
  - eNoneFound - Record/s not found

## `HawkeyeError RetrieveImageRecord(uuid__t id, ImageRecord*& rec)`
Return 
  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data   
  - eEntryNotFound - Record/s not found

## `HawkeyeError RetrieveImageRecordList(uuid__t* ids, uint32_t list_size, ImageRecord*& recs, uint32_t& retrieved_size);
Return 
  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data   
  - eNoneFound - Record/s not found

## `HawkeyeError RetrieveWorkQueues(uint64_t start, uint64_t end, char* user_id, WorkQueueRecord*& reclist, uint32_t& list_size)`
Return
  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data   
  - eNoneFound - No records found

## `HawkeyeError RetrieveSampleRecords(uint64_t start, uint64_t end, char* user_id, SampleRecord*& reclist, uint32_t& list_size)`
Return
  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data   
  - eNoneFound - No records found

## `HawkeyeError RetrieveSampleImageSetRecords(uint64_t start, uint64_t end, char* user_id, SampleImageSetRecord*& reclist, uint32_t& list_size)`
Return
  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data  
  - eNoneFound - No records found

## `HawkeyeError RetrieveImageRecords(uint64_t start, uint64_t end, char* user_id, ImageRecord*& reclist, uint32_t& list_size)`
Return
  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data 
  - eNoneFound - No records found

## `ResultSummary`
ResultSummary will provide summary(Cumulative) of the results of the analysis.

## `ResultRecord`
ResultRecord will provide summary + per image results of the analysis.

## `HawkeyeError RetrieveResultRecord(uuid__t id, ResultRecord*& rec)`
Return
  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data 
  - eEntryNotFound - Record/s not found


## `HawkeyeError RetrieveResultRecordList(uuid__t* ids, uint32_t list_size, ResultRecord*& recs, uint32_t& retrieved_size)`

Return
  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data 
  - eNoneFound - Record/s not found

	
## `HawkeyeError RetrieveResultRecords(uint64_t start, uint64_t end, char* user_id, ResultRecord*& reclist, uint32_t& list_size)`
Retrieves the records based on "Start" and "end" time / Associated user.

Return 

  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data 
  - eNoneFound - No records found

  
## `HawkeyeError RetrieveResultSummary(uuid__t id, ResultSummary*& rec)`
Return
  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data 
  - eEntryNotFound - Record/s not found


## `HawkeyeError RetrieveResultSummaryList(uuid__t* ids, uint32_t list_size, ResultSummary*& recs, uint32_t& retrieved_size)`

Return
  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data 
  - eNoneFound - Record/s not found

	
## `HawkeyeError RetrieveResultSummaries(uint64_t start, uint64_t end, char* user_id, ResultSummary*& reclist, uint32_t& list_size)`
Retrieves the records based on "Start" and "end" time / Associated user.

Return 

  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data 
  - eNoneFound - No records found


## `HawkeyeError RetrieveImage(uuid__t id, imagewrapper_t*& img)`
Retrieves the Image associated with the specific UUID
Returns:
  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data 
  - eValidationFailed - Image conversion failed.
  - eEntryNotFound - Record not found


## `HawkeyeError RetrieveBWImage(uuid__t image_id, imagewrapper_t*& img)`
Retrieves a version of the specified image transformed into a black and
white bi-level image.
Returns:
  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data 
  - eValidationFailed - Image conversion failed.
  - eEntryNotFound - Record not found


## `HawkeyeError RetrieveAnnotatedImage(uuid__t result_id, uuid__t image_id, imagewrapper_t*& img)`
Retrieves an annotated (marked up with feature number, general
population and population of interest) image for the specific image and
result pair
Returns:
  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data 
  - eValidationFailed - Image conversion failed.
  - eEntryNotFound - Record not found
  - eEntryInvalid - Unable to annotate image

## `HawkeyeError RetrieveDetailedMeasurementsForResultRecord(uuid__t id, DetailedResultMeasurements*& measurements)`
Returns:

  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eStorageFault - Software failed to retrieve the data 
  - eEntryNotFound - Record not found
	
## `HawkeyeError RetrieveSampleRecordsForBioprocess(const char* bioprocess_name, SampleRecord*& reclist, uint32_t& list_size)`
## `HawkeyeError RetrieveSampleRecordsForQualityControl(const char* QC_name, SampleRecord*& reclist, uint32_t& list_size)`
Get Sample records for all samples ran against the specified bioprocess/QualityControl.
The bioprocess/QualityControl does NOT have to be active

Returns:
   - eSuccess
   - eNotPermittedByUser - User does not have required permission
   - eStorageFault - Software failed to retrieve the data    
   - eNoneFound - No records found/No qc/bp records found

	
## `HawkeyeError ClearExportDataFolder(char* optional_subfolder, char* confirmation_password)`

Support: *none*

Clears the contents of the data export folder. If `optional_subfolder` is
NULL, then the entire data export folder will be cleared; if it is not
NULL, then only the specified sub-folder will be cleared.

Requires Elevated privileges or higher

Retuns:
   - eNotSupported
   - eSuccess
   - eNotPermittedByUser

## `HawkeyeError GetRootFolderForExportData(char*& folder_location)`

Support: *none*

Gets the root folder for exported data for the current user.

Retuns:
   - eNotSupported

## `HawkeyeError RetrieveHistogramForResultRecord(uuid__t id, bool POI, Hawkeye::Characteristic_t measurement, uint8_t& bin_count, histogrambin_t* bins)`

Retrieves a histogram data for the specified characteristic. The data is split into bin_count bins across the total
range of returned values. The center of each bin is recorded in the histogrambin_t structure that is returned.
"POI Population of Interest" - true for POI, false for GP.

Returns

  - eSuccess
  - eNotPermittedByUser
  - eEntryNotFound - A result record matching that UUID was not found
  - eStorageFault - Failed to retrieve the Image Analysis data 
  - eNoneFound - Image blob data matching to the given measurement not found
  - eValidationFailed - Image blob data not found for the given result record.


# Log File Retrieval and Maintenance

There are several log files available from the system:

  - Audit
  - Instrument Error
  - Sample Activity
  - Calibration Activity

Logs can be retrieved by administrators and may be queried by date
range. The Audit Trail, Instrument Error and Sample Activity logs can
have older data archived to a remote file (format TBD) which can be
validated and displayed by the API. The Calibration Activity log can
only be cleared (not archived) prior to a given date.

## `HawkeyeError RetrieveAuditTrailLog(uint32_t& num_entries, audit_log_entry*& log_entries)`

## `HawkeyeError RetrieveAuditTrailLogRange(uint64_t starttime, uint64_t endtime, uint32_t& num_entries, audit_log_entry*& log_entries)`

## `HawkeyeError RetrieveInstrumentErrorLog(uint32_t& num_entries, error_log_entry*& log_entries)`

## `HawkeyeError RetrieveInstrumentErrorLogRange(uint64_t starttime, uint64_t endtime, uint32_t& num_entries, error_log_entry*& log_entries)`

## `HawkeyeError RetrieveSampleActivityLog(uint32_t& num_entries, sample_activity_entry*& log_entries)`

## `HawkeyeError RetrieveSampleActivityLogRange(uint64_t starttime, uint64_t endtime, uint32_t& num_entries, sample_activity_entry*& log_entries)`

Retrieve entries out of the logs, either the entire log or by a date
range. Specify “from beginning” or “to end” by using ‘0’ in the
appropriate time range parameter.

Returns

  - eSuccess
  - eNotPermittedByUser - a user must be logged in order to Retrieve Calibration Activity Log.
  - eStorageFault - failed to read log

## `HawkeyeError RetrieveCalibrationActivityLog(calibration_type cal_type, uint32_t& num_entries,calibration_history_entry*& log_entries)`

## `HawkeyeError RetrieveCalibrationActivityLogRange(calibration_type cal_type, uint64_t starttime,uint64_t endtime, uint32_t& num_entries, calibration_history_entry*& log_entries)`

Retrieve entries out of the logs, either the entire log or by a date
range. Specify “from beginning” or “to end” by using ‘0’ in the
appropriate time range parameter.

Returns
  - eSuccess
  - eNotPermittedByUser

## `HawkeyeError ArchiveAuditTrailLog(uint64_t archive_prior_to_time, char* verification_password, char*& archive_location)`

## `HawkeyeError ArchiveInstrumentErrorLog(uint64_t archive_prior_to_time, char* verification_password, char*& archive_location)`

## `HawkeyeError ArchiveSampleActivityLog(uint64_t archive_prior_to_time, char* verification_password, char*& archive_location)`

Archives the log prior to a given timestamp. User must provide
their password for verification. API will inform the Host of the
selected archive location.

Returns:
  - eSuccess
  - eNotPermittedByUser - logged in user lacks sufficient permissions
  - eValidationFailed - unable to validate logged-in user's password
  - eStorageFault - error while accessing log file or saving archive file

## `HawkeyeError ClearCalibrationActivityLog(calibration_type cal_type, uint64_t archive_prior_to_time, char* verification_password)`
  
Clears the given type of calibration log prior to a given timestamp.

Returns:

  - eSuccess-
  - eNotPermittedByUser - logged in user lacks sufficient permissions
  - eValidationFailed - unable to validate logged-in user's password
  - eEntryNotFound - log file failed integrity check and should not be trusted.
  - eStorageFault - error while accessing log file
  
## `HawkeyeError ReadArchivedAuditTrailLog(char* archive_location, uint32_t& num_entries, audit_log_entry*& log_entries)`

## `HawkeyeError ReadArchivedInstrumentErrorLog(char* archive_location, uint32_t& num_entries, error_log_entry*& log_entries)`

## `HawkeyeError ReadArchivedSampleActivityLog(char* archive_location, uint32_t& num_entries, sample_activity_entry*& log_entries)`

Read back an archived log file. Log file must be one produced by the
instrument or it will fail the integrity checks. API cannot be used to
ALTER a log file.

Returns:
  - eSuccess
  - eNotPermittedByUser - logged in user lacks sufficient permissions
  - eStorageFault - error while accessing archived log file

# Reanalysis

## `typedef void(*sample_analysis_callback)(HawkeyeError, uuid__t /*sample_id*/, ResultRecord*&/*Analysis data*/);`

## `HawkeyeError ReanalyzeSample(uuid__t sample_id, uint32_t celltype_index, uint16_t analysis_index, bool from_images, sample_analysis_callback onSampleComplete)`

Performs the Reanalysis of the given sample (`sample_id`) using new
`celltype_index` and triggers a callback with complete ResultRecord data
to UI.
If `from_images` is set to `true`, then the new result is created from the retained images from the previous sample.  If it is `false`, then the result is generated from the previous result data.
Tradeoffs: 

 - From images: declustering is re-executed, possibly with new settings; If only a subset of images is retained, accuracy suffers
 - From result: speed increase, full number of cells is considered, but declustering may be innacurate.

 Returns:
 
  - eSuccess
  - eNotPermittedByUser - logged in user lacks sufficient permissions
  - eInvalidArgs – Invalid CellType/Analysis Index 
  - eStorageFault - Software failed to retrieve the data 
  - eEntryNotFound – No record found


# Signatures

A user may attach a “signature” to a Result. The signatures are
customer-defined in accordance with their own internal practices and
policies and have no inherent meaning to the instrument. Once applied, a
signature cannot be removed.

Signatures are intended to be text displayable to the user. The User
Interface is responsible for all conversion/normalization of Unicode
strings.

### Administrator Interface

## `HawkeyeError AddSignatureDefinition(DataSignature_t* signature)`

Create a new signature definition. Signature definitions must be unique
in both short and long text. Caller is responsible for proper unicode
normalization.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eAlreadyExists - unable to add definition because the short or long text was not unique
  - eStorageFault- Failed to update the info file.
  - eInvalidArgs - Invalid signature input(nullptr).

## `HawkeyeError RemoveSignatureDefinition(char* signature_short_text, uint16_t short_text_len)`

Remove a signature definition. This does not affect any previously-applied signatures.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eEntryNotFound - unable to find signature definition matching `signature_short_text`

### Normal User Interface

## `HawkeyeError RetrieveSignatureDefinitions(DataSignature_t*& signatures, uint16_t& num_signatures)`

Return the current list of signature definitions.

Returns

  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eNoneFound - No Signature found
  
## `HawkeyeError SignResultRecord(uuid__t record_id, char* signature_short_text, uint16_t short_text_len)`

Attach a signature to a Result Record. THIS IS A PERMANANT ALTERATION - SIGNATURES CANNOT BE REMOVED ONCE ADDED

Returns

  - eSuccess
  - eNotPermittedByUser - User does not have required permission
  - eEntryNotFound - no result record exist with the given UUID
  - eStorageFault - Failed to apply signature
  - eInvalidArgs - no signature definition matching `signature_short_text` could be found

# Maintenance Tasks

**Adminstrator Only**

## `HawkeyeError ExportInstrumentConfiguration(const char* filename)`

Exports the instrument configuration (users, cell types, analyses,
bioprocesses, quality controls, etc.) to a file for use as a settings
backup or to aid a laboratory manager in “cloning” instruments for
consistency. The resulting file is NON-MODIFIABLE and will be rejected
by an instrument if it is tampered with in any way.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eInvalidArgs - Invalid input config file path/extension
  - eStorageFault - System failed to get the configuration data
  - eValidationFailed - RSA key not valid / failed to encrypt / failed to apply signature

## `HawkeyeError ImportInstrumentConfiguration(const char* filename)`

Imports an instrument configuration (as provided by the
ExportInstrumentConfiguration(…) function).

NOTE: This action will COMPLETELY OVERWRITE THE CONFIGURATION OF THIS
INSTRUMENT.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eValidationFailed - Import failed because file could not be validated
  - eNoneFound - No configuration file found.


## `HawkeyeError  ExportInstrumentData(uuid__t * rs_uuid_list,
                                                 uint32_t num_uuid,
                                                 const  char* export_location_directory,
                                                 eExportImages exportImages,
                                                 uint16_t export_nth_image,
                                                 export_data_completion_callback onExportCompletionCb,
                                                 export_data_progress_callback exportProgressCb = nullptr)`
Support: Hardware/Simulation

Exports the data(Meta data, Result binaries and images) for each result specified, along with configuration and log files.
The images to export for each result are based on the "eExportImage and export_nth_image" inputs.
eExportImages::FirstAndLastOnly - Only the first and last image will be exported.
eExportImages::All              - All available images will be exported.
eExportImages::ExportNthImage   - If all the images are available(i.e save_nth_image filtering is not applied during initial sample analysis)
                                  then only nth image will be exported, else all the available images will be exported.
Call backs:
1. Complete callback : Indicates the completion(Failure/success) of export data.
                       After this callback triggered, there will be no further export data operation.
                       This callback is also triggered when "CancelExportData" is requested.
2. Progress callback : Indicates export completion(Failure/success) of each result.
                       After this callback is triggered, still the export data will be in progress.
                       This callback can be nullptr, so there will be no indication after export completion of each result.
Returns

   - eSuccess
   - eNotPermittedByUser     - Logged in user lacks sufficient permissions
   - eNotPermittedAtThisTime - Previous export or delete data operation is in progress
   - eInvalidArgs            - Invalid input/nullptr
   - eStorageFault           - Failed export images/result binaries/ Failed to Archive /Failed to save meta data
   - eLowDiskSpace           - Low disk space
   
## `HawkeyeError CancelExportData()`

Accepts the request to cancel the export data operation, if export data is in progress.
On success return from this API, caller must wait for "export_data_completion_callback" callback, which confirms the successful completion of export data cancellation.

Returns

   - eSuccess
   - eNotPermittedAtThisTime - Export data is not in progress
   
## `HawkeyeError ImportInstrumentData(const char* import_file_location)`

*NOT CURRENTLY SUPPORTED*

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eNoneFound - File/path not found
  - eValidationFailed - file checksum / signature failed
  - eBusy - instrument must be idle to do a data import

## Autofocus

**Service Only**

Exercise the instrument’s autofocus sequence. Requires focusing media,
and ~45 minutes of settling time for the procedure. The focus motor will
home and then the system will acquire images throughout the potential
focus field, settling on a location. The host will be provided with data
to show a histogram of the focus “goodness” through the range.

## `HawkeyeError svc_CameraAutoFocus(SamplePosition focusbead_location, autofocus_state_callback_t on_status_change countdown_timer_callback_t on_timer_tick)`

NOTE: can only be executed when system is IDLE; can only be executed by
a user with “service” permission.

Callback will be used to inform the host of progress through the
sequence as well as of the results of the focus. Once the state reaches
`af_WaitingForFocusAcceptance`, the Host must call
`svc_CameraAutoFocus_FocusAcceptance`(…) with acceptance, retry
instructions or cancellation of the focus sequence.

Callback `on_timer_tick` will be used to indicate remaining time (in
seconds) once the state reaches `af_SampleSettlingDelay` autofocus state.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eHardwareFault - a hardware fault, focus motor is not initialized.
  - eStageNotRegistered - Sample carrier is not registered
  - eNotPermittedAtThisTime - System is busy.

##	HawkeyeError svc_CameraFocusAdjust (bool direction_up, bool adjustment_fine);
Manually adjust camera focus position.  For coarse adjustment (10x fine step)

Returns:
	- eSuccess - Motion has been requested.
	- eNotPermittedByUser - Logged in user lacks sufficient permissions
	- eHardwareFault - a hardware fault, condition prevents this action.
	
##	`HawkeyeError svc_CameraFocusRestoreToSavedLocation()`

Restore camera to previously-stored position setting (may re-home focus motor)

Returns:
	- eSuccess - Success
	- eNotPermittedByUser - Logged in user lacks sufficient permissions
	- eHardwareFault - Unable to initialize focus controller / Failed to move focus motor to requested position
	
##	`HawkeyeError svc_CameraFocusStoreCurrentPosition(char* password)`

Write the current focus position to disk as the new focus position. 
The service user MUST re-verify their login credentials to perform this action.

Returns:
	- eSuccess - Success; Position has been saved to disk.
	- eNotPermittedByUser - User does not have the service permission.	
	- eNotPermittedAtThisTime - Logged in user is invalidated user
	- eValidationFailed - Unable to validate login credentials
	- eHardwareFault - Focus controller is not initialized / Failed to move focus motor to requested position
	

##	`HawkeyeError svc_SetSystemSerialNumber(char* serial, char* service_password)`

Set the instrument serial number.  This function requires Service permissions
and the re-verification of the service user password.

Returns
	- eSuccess - serial number has been stored
	- eNotPermittedByUser - Logged-in user lacks sufficient permissions
	- eValidationFailed - unable to validate login credentials
	- eInvalidArgs - serial number not appropriate 
	- eNotPermittedAtThisTime - System Hardware is not initialized.
	- eHardwareFault - Failed to set the serial number

## `HawkeyeError svc_CameraAutoFocus_ServiceSkipDelay()`

Skips the settling delay step in the autofocus sequence

Returns
	- eSuccess 
	- eNotPermittedByUser - Logged-in user lacks sufficient permissions
	- eNotPermittedAtThisTime - Autofocus is not in progress.
	
## `HawkeyeError svc_CameraAutoFocus_FocusAcceptance(eAutofocusCompletion decision)`

Instructs the system to accept or reject the proposed focus location.
The Host can accept it, cancel the process, request that the instrument
re-calculate focus based on the current fluid in the flow cell or retry
focus with different cells (will require additional settling time).

Returns
	- eSuccess 
	- eNotPermittedByUser - Logged-in user lacks sufficient permissions
	- eNotPermittedAtThisTime - Autofocus is not in progress/ Failed to accept.
	- eInvalidArgs - Invalid decision input.
	
	
## `HawkeyeError svc_CameraAutoFocus_Cancel()`

Cancels the autofocus sequence (may require a short time to fully cancel
as the flow cell may need to be flushed out.

Returns
	- eSuccess 
	- eNotPermittedByUser - Logged-in user lacks sufficient permissions
	- eNotPermittedAtThisTime - Autofocus is not in progress

## `HawkeyeError svc_GetFlowCellDepthSetting(uint16_t& flow_cell_depth)`
Returns
	- eSuccess 
	- eNotPermittedByUser - Logged-in user lacks sufficient permissions
	
## `HawkeyeError svc_SetFlowCellDepthSetting(uint16_t flow_cell_depth)`
Returns
	- eSuccess 
	- eNotPermittedByUser - Logged-in user lacks sufficient permissions
	- eInvalidArgs - flow cell depth out of range (50-100)	
	
## Brightfield Dust Reference Image

**Administrator Only**

Create a reference image for automatic removal of static dust/debris
from the brightfield images. The image analysis algorithm will use this
reference to “subtract” the debris from the calculations. This process
requires a small amount of buffer reagent.

## `HawkeyeError StartBrightfieldDustSubtract(brightfield_dustsubtraction_callback on_status_change)`

Begin the dust subtraction. Callback will be used to signal the host of
progression through the sequence and of the resulting image. When the
state reaches `bds_WaitingOnUserApproval`, the Host needs to call
AcceptDustReference(…) with the user’s acceptance or rejection of the
data.

Returns:

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eBusy - cannot start the operation unless the system is idle
  - eNotPermittedAtThisTime - system is not able to start the process at this time (no reagents?)
  - eStageNotRegistered - Sample carrier is not registered

## `HawkeyeError AcceptDustReference(bool accepted)`

Accepts/rejects the dust reference setting.

Returns:

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eBusy - cannot start the operation unless the system is idle
  - eNotPermittedAtThisTime - process is not ready yet

             
## `HawkeyeError CancelBrightfieldDustSubtract()`

Cancels the dust reference generation. May take some time to complete as
fluid must be flushed out.

Returns:

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eNotPermittedAtThisTime - No BrightfieldDustSubtract is in progress to cancel.


## Flush Flow Cell

**All Users**

## `HawkeyeError StartFlushFlowCell(flowcell_flush_status_callback on_status_change)`
Flush the flow cell using Buffer and Air. Uses at least 1 activity of
buffer.

Returns:

  - eSuccess
  - eNotPermittedByUser - Logged-in user lacks sufficient permissions / no user logged in.
  - eBusy - system must be idle
  - eNotPermittedAtThisTime - system cannot run this sequence now (no reagents?)

## `HawkeyeError GetFlushFlowCellState(eFlushFlowCellState& state)`

Returns:

  - eSuccess
  - eNotPermittedAtThisTime - Flush flowcell workflow is not in progress.
  
## `HawkeyeError CancelFlushFlowCell()`

Returns:

  - eSuccess
  - eNotPermittedByUser - Logged-in user lacks sufficient permissions / no user logged in.
  - eNotPermittedAtThisTime - Flush flowcell workflow is not in progress.

## Decontaminate Flow Cell

**All Users**

Decontaminate the flow cell using cleaning agent, conditioning solution, buffer
and air. Uses at least 1 activity of each reagent.

## `HawkeyeError StartDecontaminateFlowCell(flowcell_decontaminate_status_callback on_status_change)`

Returns:

  - eSuccess
  - eNotPermittedByUser - Logged-in user lacks sufficient permissions / no user logged in.
  - eBusy - system must be idle
  - eNotPermittedAtThisTime - system cannot run this sequence now (no reagents?)
  - eStageNotRegistered - Sample carrier is not registered  
  
  
## `HawkeyeError GetDecontaminateFlowCellState(eDecontaminateFlowCellState& state)`

Returns:

  - eSuccess
  - eNotPermittedAtThisTime - Decontaminate workflow is not in progress.
  
## `HawkeyeError CancelDecontaminateFlowCell()`

Returns:

  - eSuccess
  - eNotPermittedByUser - Logged-in user lacks sufficient permissions / no user logged in.
  - eNotPermittedAtThisTime - Decontaminate workflow is not in progress.

## Prime Reagent Lines

**All Users**

Prime the reagent lines from tank to valve. Uses up to 3 activities from
each reagent.

## `HawkeyeError StartPrimeReagentLines(prime_reagentlines_callback on_status_change)`

Returns:

  - eSuccess
  - eNotPermittedByUser - Logged-in user lacks sufficient permissions / no user logged in.
  - eBusy - system must be idle
  - eNotPermittedAtThisTime - system cannot run this sequence now (no reagents?)

## `HawkeyeError GetPrimeReagentLinesState(ePrimeReagentLinesState& state)`

Returns:

  - eSuccess
  - eNotPermittedAtThisTime - Prime workflow is not in progress.

## `HawkeyeError CancelPrimeReagentLines()`

Returns:

  - eSuccess
  - eNotPermittedByUser - Logged-in user lacks sufficient permissions / no user logged in.
  - eNotPermittedAtThisTime - Prime workflow is not in progress.

## Drain Reagent Lines

**All Users**

Drains each reagent line to waste. Callback updates Drain activity status
## `HawkeyeError StartDrainReagentPack(drain_reagentpack_callback on_status_change)`

Returns:

  - eSuccess
  - eNotPermittedByUser - Logged-in user lacks sufficient permissions / no user logged in.
  - eBusy - system must be idle
  - eNotPermittedAtThisTime - system cannot run this sequence now/ pack is not installed

## `HawkeyeError GetDrainReagentPackState(eDrainReagentPackState& state)`

Returns:

  - eSuccess
  - eNotPermittedAtThisTime - Drain workflow is not in progress.

## `HawkeyeError CancelDrainReagentPack()`

Returns:

  - eSuccess
  - eNotPermittedByUser - Logged-in user lacks sufficient permissions / no user logged in.
  - eNotPermittedAtThisTime - Drain workflow is not in progress.

  
## Direct Instrument Control

This section of code is limited to Service Users and provides very
direct control of the instrument.

### Camera Lamp

Turn the camera lamp on and off and adjust its intensity

## `HawkeyeError svc_SetCameraLampState(bool lamp_on, float intensity_0_to_100)`

Turn bright-field LED ON/OFF and if ON then set the LED intensity to requested value

Returns:

  - eSuccess - Success
  - eNotPermittedByUser - User does not have the service permission.
  - eInvalidArgs - Intensity is out of range (must be 0.0 to 100.0)
  - eHardwareFault - Failed to set LED intensity to requested value

## `HawkeyeError svc_SetCameraLampIntensity(float intensity_0_to_100)`

Set the bright-field LED intensity to requested value

Returns:

  - eSuccess - Success
  - eNotPermittedByUser - User does not have the service permission.
  - eInvalidArgs - Intensity is out of range (must be 0.0 to 100.0)
  - eHardwareFault - LED is turned off / Failed to set LED intensity to requested value

## `HawkeyeError svc_GetCameraLampState( bool& lamp_on, float& intensity_0_to_100 )`

Gets the bright-field Led state (ON/OFF) and it's current intensity.

Returns:

  - eSuccess - Success
  - eNotPermittedByUser - User does not have the service permission.
  - eHardwareFault - Failed to read LED intensity

### “On the fly” Image Analysis

Generation of single-shot results. These functions execute very simple
data collection / analysis tasks suitable for tuning focus or similar
activities. They do not generate permanent data. The base of the
functionality is supported by a "temporary" Cell Type and Analysis type.
The temporary Cell Type / Analysis type can be constructed from scratch
or from an existing Cell Type/Analysis Type.

## `HawkeyeError svc_SetAnalysisImagePreference(eImageOutputType image_type)`

Sets the preference for the type of images returned by the single/continuous analysis functions.
Can be changed while analysis is running.  Next affected image will reflect changed setting.
Requires Service access.

Default: `eImageAnnotated`

Return
   - eNotPermittedByUser - Logged in user lacks sufficient permissions / no user logged in
   - eSuccess

## `HawkeyeError svc_GenerateSingleShotAnalysis(service_analysis_result_callback callback)`

Generates an single temporary analysis based on the current camera
image.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eBusy - instrument is busy executing other tasks

## `HawkeyeError svc_GenerateContinuousAnalysis(service_analysis_result_callback callback)`

Generates a series of temporary analyses based on the “live” camera
images. This allows for near-real-time evaluation of changes to
illumination or cell type/analysis settings.

Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eBusy - instrument is busy executing other tasks
  
## `HawkeyeError svc_StopContinuousAnalysis()`
Returns

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions

  
### Live Camera Feed

Allows a service user to toggle a live image feed from the camera to
test the effect of various changes in the instrument. The live feed is
implemented as a series of still images which will be returned
sequentially to the Host via a callback mechanism. The live feed can
only be used when no work queue processing or other image gathering
operation is in progress. The live feed CAN be used when the autofocus
workflow is in the "settling delay" state.

## `HawkeyeError svc_StartLiveImageFeed(service_live_image_callback callback)`
Returns:

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eNotPermittedAtThisTime - the instrument is unable to take images at this time
  - eBusy - the instrument is already executing tasks which require the camera

## `HawkeyeError svc_StopLiveImageFeed()`
Returns:

  - eSuccess
  - eNotPermittedByUser - Logged in user lacks sufficient permissions
  - eInvalidArgs - Internal images id mismatch

### Manual Sample Fluidics Control

Automated sample handling activities for the service user. These
functions assist the service user in exercising the system and
evaluating the optical performance of the system (when used with the
live image feed or single-shot analysis/continuous analysis features.

The user is recommended to use the "flush" functions to clear the
remainder of the sample from the flow cell and sample lines after
exercising these functions.

## `HawkeyeError svc_ManualSample_Load()`

Aspirates a sample from the current sample location; dispenses all but `_____` microliters of sample; primes sample into the flow cell

Returns:
  - eSuccess
  - eNotPermittedByUser
  - eBusy - instrument cannot be operated manually at this time
  - eNotPermittedAtThisTime - load failed
  - eHardwareFault - a fault state exists that prevents the execution of this request.

## `HawkeyeError svc_ManualSample_Nudge()`

Dispenses `____` microliters of sample through the flowcell.

Returns:
  - eSuccess
  - eNotPermittedByUser
  - eBusy - instrument cannot be operated manually at this time
  - eNotPermittedAtThisTime - "nudge" failed because no sample remains in the syringe pump
  - eHardwareFault - failed to get syringe position

## `HawkeyeError svc_ManualSample_Expel()`

Expel: dispenses remainder of sample to the Waste.

Returns:
  - eSuccess
  - eNotPermittedByUser
  - eBusy - instrument cannot be operated manually at this time
  - eNotPermittedAtThisTime - "expel" failed because no sample remains in the syringe pump
  - eHardwareFault - failed to get syringe position

### Syringe Pump / Valve Control

Manual control of the syringe pump and rotary valve controls.

## `HawkeyeError svc_GetValvePort(char& port)`

Gets the syringe valve port

Returns

  - eSuccess - Success
  - eHardwareFault - Failed to read the valve port

## `HawkeyeError svc_SetValvePort(const char port)`

Validates the valve port choice (A-H) and moves the port in a clockwise
direction to the selected location.

Returns:

  - eSuccess
  - eNotPermittedByUser - User does not have the service permission.
  - eInvalidArgs - if the given valve is not valid.
  - eHardwareFault - Failed to set the valve port

## `HawkeyeError svc_GetSyringePumpPostion(uint32_t& pos)`

Gets the syringe pump position

Returns

  - eSuccess - Success
  - eNotPermittedByUser - User is not logged in
  - eHardwareFault - Failed to read the syringe position

## `HawkeyeError svc_AspirateSample(uint32_t volume)`

Reads the current syringe volume and port, validates input volume to
aspirate and performs the aspiration. Aspiration is not allowed from
"Waste(F)"

Returns

  - eSuccess - Success
  - eNotPermittedByUser - User does not have the service permission.
  - eInvalidArgs - Given input volume to aspirate is greater than MAX syringe level OR the given the input volume is less than the syringe current volume
  - eHardwareFault - Failed to read current volume/ Failed to read the current valve/ Failed to Aspirate
  - eNotPermittedAtThisTime - If the current active valve is 'F'

## `HawkeyeError svc_DispenseSample(uint32_t volume)`

Reads the current syringe volume and port, validates input volume to
dispense and performs the dispensing. Dispense is allowed only to "Waste
(F)", "Sample (G)" and "Flow Cell(H)".

Returns

  - eSuccess - Success
  - eNotPermittedByUser - User does not have the service permission.
  - eInvalidArgs - Given input volume level to dispense is greater than syringe current volume
  - eHardwareFault - Failed to read current volume / Failed to read the current valve / Failed to Dispense
  - eNotPermittedAtThisTime - If the current active valve is not any out of 'F' 'G' and 'H'

### Sample Probe

## `HawkeyeError svc_GetProbePostion(int32_t &pos)`

Gets the sample probe position.

Returns

 - eSuccess - Success
 - eNotPermittedByUser - User is not logged in

## `HawkeyeError svc_SetProbePostion (bool upDown, uint32_t stepsToMove)`

Moves the sample probe UP or DOWN in given steps

Returns

 - eSuccess - Success
 - eNotPermittedByUser - User does not have the service permission.
 - eHardwareFault - Failed to move sample probe

## `HawkeyeError svc_MoveProbe (bool upDown)`

Moves the Sample Probe UP( Top most probe position) and DOWN (probe maximum reachable bottom position)
position.

Returns

 - eSuccess - Success
 - eNotPermittedByUser - User does not have the service permission.
 - eHardwareFault - Failed to move sample probe

### Sample Stage

## `HawkeyeError svc_GetSampleWellPosition(SamplePosition& pos)`

Gets the sample stage position either for carousel or plate

Returns

  - eSuccess - Success
  - eNotPermittedByUser - User is not logged in
  - eHardwareFault - Stage is not initialized/ Incorrect position reported.

## `HawkeyeError svc_SetSampleWellPosition(SamplePosition pos)`

Validates the input sample position and checks for initialization of the
required carrier to set the Input sample position and proceeds to set
the sample position if the carrier is present/active.

Returns

  - eSuccess - Success
  - eNotPermittedByUser - User does not have the service permission.
  - eInvalidArgs - Invalid sample position.
  - eHardwareFault - Failed to set sample stage position.
  - eStageNotRegistered - Sample carrier is not registered.

## `HawkeyeError InitializeCarrier()`

Initializes the Carousel/Plate, if it is not initialized.

Returns

  - eSuccess - Success
  - eNotPermittedByUser - User does not have the service permission.
  - eNotPermittedAtThisTime - Failed to initialize the carrier, tube found in carousel
  - eHardwareFault - Error in Hardware. Failed to initialize Carousel/Plate.

### Sample Stage Calibration

These functions perform the calibration sequence on the sample stage
with assistance from the user. The user is required to move the stage to
specific orientations and then advance the calibration sequence. Upon
successful calibration of both Plate and Carousel, the system will be
able to accurately position the stage controller.

## `HawkeyeError svc_PerformPlateCalibration(motor_calibration_state_callback)`

Performs plate calibration and send status thru provided callback

Returns

  - eSuccess - Success 
  - eNotPermittedByUser - User does not have the service permission.
  - eInvalidArgs - Callback provided is null/invalid or calibration state is not idle
  - eNotPermittedAtThisTime - If plate is not present

## `HawkeyeError svc_PerformCarouselCalibration(motor_calibration_state_callback)`

Performs carousel calibration and send status thru provided callback

Returns

  - eSuccess - Success 
  - eNotPermittedByUser - User does not have the service permission.
  - eInvalidArgs - Callback provided is null/invalid or calibration state is not idle
  - eNotPermittedAtThisTime - If carousel is not present

## `HawkeyeError svc_CancelCalibration (motor_calibration_state_callback onCalibStateChangeCb)`

Cancels the ongoing calibration if not already completed
and restore the stage to either previously registered or default state
Callback will be triggered with `CalibrationState::eFault` state

Returns

  - eSuccess - Success 
  - eNotPermittedByUser - User does not have the service permission.
  - eInvalidArgs - Callback provided is null/invalid

## `HawkeyeError svc_GetStageCalibrationBacklash (int32_t& thetaBacklash, int32_t& radiusBacklash)`

Gets the stage calibration backlash values set by service user.
If no values have been set previously then default backlash values will be provided

Returns:

  - eSuccess - Success
  - eNotPermittedByUser - User does not have the service permission.

## `HawkeyeError svc_SetStageCalibrationBacklash (int32_t thetaBacklash, int32_t radiusBacklash)`

Sets the stage calibration backlash values to compensate for mechanical
mechanical backlash when performing either "Plate" or "Carousel" calibration

Returns:

  - eSuccess - Success 
  - eNotPermittedByUser - User does not have the service permission.

### Repetitive Actions

Assistance functions for the host in performing various repetitive
actions for the benefit of a service user trying to force repeat of an
issue (or other reasons). Each function performs a single repetition of
the specified action sequence.

## `HawkeyeError svc_PerformSampleRepetition(bool isProbeSelected = false)`

If the probe is selected, then puts the probe down, Sets the default
valve port as "Sample" and aspirates and dispenses the maximum volume
followed by moving the probe UP. If the probe is not selected (false),
perform the sample operation with no probe movement.

Further: Performs the Focus motor home and focus to center. Performs the
Reagent Arm Home, followed by Arm down and Arm up

Support: *deprecated*



## `HawkeyeError svc_PerformValveRepetition()`

Sets next valve in the order/sequence of the syringe pump. Ex: Initial
valve port is "F"; after successful execution of this API valve port
will be "G".
The direction to set valve is clockwise direction.

Returns

 - eSuccess - Success
 - eNotPermittedByUser - User does not have the service permission
 - eHardwareFault - Failed to read current valve location / Failed to set valve

## `HawkeyeError svc_PerformSyringeRepetition()`

Sets "Sample(G)" as active valve. Moves the syringe pump to 0 -Empty
position (No volume) and aspirates 1000ul(max limit) followed by
Dispensing of same volume. Speed of the Syringe pump is 600ul/sec and direction
to set valve is Clockwise.

Returns

 - eSuccess - Success
 - eNotPermittedByUser - User does not have the service permission
 - eHardwareFault - Failed to set syringe valve / Failed to Aspirate or Dispense

## `HawkeyeError svc_PerformFocusRepetition()`

Homes the Focus motor and adjust the focus to center or to place where better visibility can be achieved.

Returns

 - eSuccess - Success
 - eNotPermittedByUser - User does not have the service permission
 - eHardwareFault - Failed to move focus motor to home position or center position

## `HawkeyeError svc_MoveReagentArm( bool up )`

Moves the Reagent arm UP ( to the upper Home limit sensor ) or DOWN ( to the bottom limit position sensor )

Returns

 - eSuccess - Success
 - eNotPermittedByUser - User does not have the service permission.
 - eHardwareFault - Failed to move Reagent arm

## `HawkeyeError svc_PerfomReagentRepetition()`

Homes the Reagent motor and performs reagent ARM down to insert the
Reagent needles into Reagents followed by ARM UP.

Returns

 - eSuccess - Success
 - eNotPermittedByUser - User does not have the service permission
 - eHardwareFault - Failed to move reagent arm to Home position / Failed to move reagent arm UP/DOWN
 - eNotPermittedAtThisTime - Reagent door is Open

# Temporary Cell Type and Analysis APIs

Description: Sets and Retrieves the temporary cell type and analysis
definition. Set Operation can also be performed using existing Cell
Type/Analysis definition.

Temp Cell Type index/ Analysis Definition index = 0xFFFF.

## `HawkeyeError GetTemporaryAnalysisDefinition(AnalysisDefinition*& temp_definition)
## `HawkeyeError GetTemporaryCellType(CellType*& temp_cell)`

Returns

  - eSuccess - Success
  - eNotPermittedByUser - User does not have the permission.
  - eNoneFound - Temporary Cell Type/Analysis is not set.
 
## `HawkeyeError SetTemporaryAnalysisDefinition(AnalysisDefinition* temp_definition)`
## `HawkeyeError SetTemporaryCellType(CellType* temp_cell)`

Returns

  - eSuccess - Success
  - eNotPermittedByUser - User does not have the permission.
  - eInvalidArgs - Invalid input celltype/Analysis definition(nullptr)


## `HawkeyeError SetTemporaryAnalysisDefinitionFromExisting(uint32_t analysis_index)`
## `HawkeyeError SetTemporaryCellTypeFromExisting(uint32_t ct_index)`
Returns

  - eSuccess - Success
  - eNotPermittedByUser - User does not have the permission.
  - eEntryNotFound - No Analysis/Cell type found
  - eInvalidArgs - Invalid Cell Type/Analysis index
 

# Delete and Free APIs

## Delete

## `HawkeyeError DeleteWorkQueueRecord (uuid__t * wq_uuidlist, uint32_t num_uuid, bool retain_results_and_first_image, delete_results_callback onDeleteCompletion)`

## `HawkeyeError DeleteSampleRecord (uuid__t * wqi_uuidlist, uint32_t num_uuid, bool retain_results_and_first_image, delete_results_callback onDeleteCompletion)`

## `HawkeyeError DeleteResultRecord (uuid__t * rr_uuidlist, uint32_t num_uuid, bool cleanup_if_deletinglastresult, delete_results_callback onDeleteCompletion)`

Deletes Work Queue record, Sample Record data.

  - If `retain_results_and_first_image` is 'false' then deletes the complete record data (Work Queue record, Sample Record, Sample Image Set Record and Result Record)
  - If `retain_results_and_first_image` is 'true' then deletes all the images, retaining the first image and result record.
  - If `cleanup_if_deletinglastresult` is `true` and if a UUID in the list is the final result for a sample record, then deletes the Sample Record and Sample Image Set Records associated with it.
  - After each record delete completion, HawkeyeLogic triggers a callback to UI with deletion status and UUID of the record.

Returns
  - eSuccess - Success
  - eNotPermittedByUser - User does not have the permission.
  - eNotPermittedAtThisTime - Export Data is in progress 
  - eInvalidArgs - No UUID/s passed to delete.
  - eEntryNotFound - Record with the given UUID not found
  - eStorageFault - Failed to delete record.
 
## Free APIs

Release memory allocated by the DLL for simple data types.
ANY and ALL pointers returned by the HawkeyeLogicInterface MUST be freed through this interface.
Complex structures (those with internal memory allocation) will have their own deletion function
defined and MUST be deleted through that function.

## `HawkeyeError FreeTaggedBuffer(NativeDataType tag, void* ptr)`

Returns
  - eSuccess - Success
  - eInvalidArgs - Invalid data type.

## `HawkeyeError FreeListofTaggedBuffer(NativeDataType tag, void* ptr, uint32_t numitems)`

Returns
  - eSuccess - Success
  - eInvalidArgs - Invalid data type /Valid "ptr" but zero "numitems"/ Null "ptr" but non-zero "numitems".

  
# List of Image Analysis Characteristics (as of Algorithm v.3.0)

| **Characteristic Label** | **\<Key, Subkey, Sub-Subkey\>** | **Comment**                                                          |
| ------------------------ | ------------------------------- | -------------------------------------------------------------------- |
| `IsCell`                 | \<1,0,0\>                       | (post-calculation)Cannot be used as General Pop. Or P.o.I. Parameter |
| `IsPOI`                  | \<2,0,0\>                       | (post-calculation)Cannot be used as General Pop. Or P.o.I. Parameter |
| `IsBubble`               | \<3,0,0\>                       | (brightfield)                                                        |
| `IsDeclustered`          | \<4,0,0\>                       | (brightfield)                                                        |
| `IsIrregular`            | \<5,0,0\>                       | (brightfield)                                                        |
| `Area`                   | \<6,0,0\>                       | (brightfield)                                                        |
| `DiameterInPixels`       | \<7,0,0\>                       | (brightfield)                                                        |
| `DiameterInMicrons`      | \<8,0,0\>                       | (brightfield)                                                        |
| `Circularity`            | \<9,0,0\>                       | (brightfield)                                                        |
| `Sharpness`              | \<10,0,0\>                      | (brightfield)                                                        |
| `Perimeter`              | \<11,0,0\>                      | (brightfield)                                                        |
| `Eccentricity`           | \<12,0,0\>                      | (brightfield)                                                        |
| `AspectRatio`            | \<13,0,0\>                      | (brightfield)                                                        |
| `MinorAxis`              | \<14,0,0\>                      | (brightfield)                                                        |
| `MajorAxis`              | \<15,0,0\>                      | (brightfield)                                                        |
| `Volume`                 | \<16,0,0\>                      | (brightfield)                                                        |
| `Roundness`              | \<17,0,0\>                      | (brightfield)                                                        |
| `MinEnclosedArea`        | \<18,0,0\>                      | (brightfield)                                                        |
| `Elimination`            | \<19,0,0\>                      | (brightfield)                                                        |
| `CellSpotArea`           | \<20,0,0\>                      | (brightfield)                                                        |
| `AvgSpotBrightness`      | \<21,0,0\>                      | (brightfield)                                                        |
| `FLAvgIntensity`         | \<25,0,0\>                      | (fluorescent) Specify FL channel number as Subkey                    |
| `FLCellArea`             | \<26,0,0\>                      | (fluorescent) Specify FL channel number as Subkey                    |
| `FLSubPeakCount`         | \<27,0,0\>                      | (fluorescent) Specify FL channel number as Subkey                    |
| `FLPeakAvgIntensity`     | \<28,0,0\>                      | (fluorescent) Specify FL channel number as Subkey                    |
| `FLPeakArea`             | \<29,0,0\>                      | (fluorescent) Specify FL channel number as Subkey                    |
| `FLPeakLoc_X`            | \<30,0,0\>                      | (fluorescent) Specify FL channel number as Subkey                    |
| `FLPeakLoc_Y`            | \<31,0,0\>                      | (fluorescent) Specify FL channel number as Subkey                    |
| `FLSubPeakAvgIntensity`  | \<32,0,0\>                      | (fluorescent) Specify FL channel number as Subkey, subpeak as SSK.   |
| `FLSubPeakPixelCount`    | \<33,0,0\>                      | (fluorescent) Specify FL channel number as Subkey, subpeak as SSK.   |
| `FLSubPeakArea`          | \<34,0,0\>                      | (fluorescent) Specify FL channel number as Subkey, subpeak as SSK.   |
| `FLSubPeakLoc_X`         | \<35,0,0\>                      | (fluorescent) Specify FL channel number as Subkey, subpeak as SSK.   |
| `FLSubPeakLoc_Y`         | \<36,0,0\>                      | (fluorescent) Specify FL channel number as Subkey, subpeak as SSK.   |
| `CFG_SpotAreaPercentage` | \<100,0,0\>                     | (config) If used, must be supplied with General Pop. parameters      |
| `CFG_FLPeakPercentage`   | \<101,0,0\>                     | (config) If used, must be supplied with General Pop. Parameters      |
| `CFG_FLScalableROI`      | \<102,0,0\>                     | (config) If used, must be supplied with General Pop. parameters      |

# Revision History (software_version as reported by GetVersionInformation(…)

## 0.1 beta (initial release to L\&T)

## 0.2 beta

  - Changed signature of HawkeyeLogicInterface::Initialize  
    
    From: (void)  
    To: (bool `without_hardware` = false)  
    Reason: allow API use for a stand-alone installation without
    hardware support.
  - Changed signature of HawkeyeLogicInterface::GetUserList  
    
    From: (char**& userList, uint32_t& numUsers)  
    To: (bool only_enabled, char**& userList, uint32_t& numUsers)  
    Reason: allow caller to get list of only users able to log in.
  - Added `is_standalone_mode` to struct SystemStatus  
    Reason: indicate whether the API is running without hardware support
    (true: no hardware support)
  - Added “identifier” to struct ReagentContainerState  
    Reason: needed a static identifier for a container apart from the
    location or list ordering.
  - Major changes Load/UnloadReagentPack(…) interface and behavior  
    Reason: workflow analysis revealed a more complex workflow.

## 0.5 beta

  - Change to SystemStatus struct (SystemStatus.hpp) to add a count of
    the total samples processed in the life of the system
  - Added “AddItemToWorkQueue(…)”. Allows the Host to append a new work
    item within an executing work queue.
  - Added `Retrieve___Record(s)(…)` functions
  - Added `Retrieve___Image(…)` functions – note that these are
    INCOMPLETE pending decisions on how best to pass image data though
    the DLL boundary.
  - Added GetUserProperties(…) function

## 0.17 beta

  - Added Bioprocess functions
  - Added Quality Control functions
  - Added Import/Export of user configuration
  - HawkeyeError changed from enum to class to avoid an internal naming
    conflict; requires “HawkeyeError::” before all instances of the enum
    values.
  - CellType parameters redefined in terms of Characteristics as
    supplied by the image analysis algorithm
  - AnalysisDefinition functions introduced
  - Added list of Image Analysis Algorithm Characteristics
  - Added Signatures
  - Added notation as to the level of support for each API (simulation
    and/or hardware).

## 0.18 beta

  - Added simulation support for several APIs.
      - Each API is annotated whether it supports simulation and/or
        hardware.

## 0.19 beta

  - To reduce confusion with other APIs, the following changes have been
    made:
      - *GetMyCellTypes* to *GetMyCellTypeIndices*
      - *GetMyAnalyses* to *GetMyAnalysisIndices*
      - *SetUserCellTypes* to *SetUserCellTypeIndices*
      - *GetUserCellTypes* to *GetUserCellTypeIndices*
      - *SetUserAnalyses* to *SetUserAnalysisIndices*
      - *GetUserAnalyses* to *GetUserAnalysisIndices*

  - Verified the functionality of *GetAllCellTypes*.
      - Example usage is in *ScoutTestCS.cs*.

  - Added simulation support for:
      - *GetSystemStatus*
      - *Shutdown*
      - *IsShutdownComplete*

  - Added simulation/hardware support for:
      - *AddItemToWorkQueue*
      - *SkipCurrentWorkQueueItem*
      - *FreeSystemStatus*

## 0.20 beta

  - Renamed GetUserDefinedAnalyses to GetUserAnalyses.
  - Renamed SetReagentContainerLocations to SetReagentContainerLocation.
  - Renamed StartQueue to StartWorkQueue.
  - Added completion callback to LoadReagentPack and UnloadReagentPack.
  - Added simulation support for:
      - AddCellType
      - GetUserAnalyses.
      - RemoveCellType
      - AddAnalysis
      - RemoveAnalysis
      - IsAnalysisSpecializedForCellType
      - GetAnalysisForCellType
      - SpecializeAnalysisForCellType
      - RemoveAnalysisSpecializationForCellType
      - GetReagentContainerStatus
      - GetReagentContainerStatusAll
      - GetReagentDefinitions
      - FreeReagentDefinitions
      - FreeReagentContainerState
      - FreeReagentState
      - SetReagentContainerLocation.
      - StartWorkQueue.
      - LoadReagentPack
      - UnloadReagentPack.

## 0.21 beta

  - Fixed GetAnalysisForCellType API definition.
  - Successfully tested StartWorkQueue.
  - Updated SetUserCellTypeIndices to support deleting a user’s celltype indices.
  - Added simulation support for:
      - SetMyDisplayName
      - SetUserDisplayName

## 0.22 beta

  - Deleted SetMyDisplayName
  - Added simulation support for:
      - ModifyBaseAnalysis
      - ModifyCellType

## 0.XX beta

  - Add APIs
      - GetUserInactivityTimeout
      - `GetUserInactivityTimeout_Setting`
      - GetUserPasswordExpiration
      - `GetUserPasswordExpiration_Setting`
      - `LogoutUser_Inactivity`
      - SetSystemSecurityFeaturesState
      - SetUserInactivityTimeout
      - SetUserPasswordExpiration
      - EjectSampleStage
      - RotateCarousel
      - ReanalyzeSample
      - SetConcentrationCalibration
      - GetConcentrationCalibrationStatus
  - Update APIs
      - `workqueue_status_callback`
      - `workqueue_completion_callback`
      - StartWorkQueue
      - RetrieveImage
      - RetrieveAnnotatedImage
      - RetrieveBWImage

## 0.YY beta

  - Add APIs
      - `svc_CameraAutoFocus_FocusAcceptance`
      - `svc_CameraAutoFocus_Cancel`
      - SetSizeCalibration
      - StartBrightfieldDustSubtract
      - GetBrightfieldDustSubtractState
      - AcceptDustReference
      - CancelBrightfieldDustSubtract
      - RetrieveAuditTrailLog
      - RetrieveAuditTrailLogRange
      - ArchiveAuditTrailLog
      - ReadArchivedAuditTrailLog
      - RetrieveInstrumentErrorLog
      - RetrieveInstrumentErrorLogRange
      - ArchiveInstrumentErrorLog
      - ReadArchivedInstrumentErrorLog
      - RetrieveSampleActivityLog
      - RetrieveSampleActivityLogRange
      - ArchiveSampleActivityLog
      - ReadArchivedSampleActivityLog
      - RetrieveCalibrationActivityLog
      - RetrieveCalibrationActivityLogRange
      - ClearCalibrationActivityLog
      - ClearExportDataFolder
      - ClearSelectedSampleData
      - StartFlushFlowCell
      - GetFlushFlowCellState
      - CancelFlushFlowCell
      - StartDecontaminateFlowCell
      - GetDecontaminateFlowCellState
      - CancelDecontaminateFlowCell
      - StartPrimeReagentLines
      - GetPrimeReagentLinesState
      - CancelPrimeReagentLines
  - Update APIs
      - GetSupportedAnalysisCharacteristics
      - RetrieveDetailedMeasurementsForResultRecord
      - SetConcentrationCalibration
      - `svc_CameraAutoFocus`

## 0.23 Beta

  - Added APIs
      - GetSystemSecurityFeaturesState
      - `svc_CameraAutoFocus_ServiceSkipDelay`
      - `svc_SetCameraLampState`
      - `svc_SetCameraLampIntensity`
      - `svc_GetCameraLampState`
      - `svc_GetTemporaryAnalysisDefinition`
      - `svc_SetTemporaryAnalysisDefinition`
      - `svc_SetTemporaryAnalysisDefinitionFromExisting`
      - `svc_GetTemporaryCellType`
      - `svc_SetTemporaryCellType`
      - `svc_SetTemporaryCellTypeFromExisting`
      - `svc_GenerateSingleShotAnalysis`
      - `svc_GenerateContinuousAnalysis`
      - `svc_StopContinuousAnalysis`
      - `svc_StartLiveImageFeed`
      - `svc_StopLiveImageFeed`
      - `svc_ManualSample_Load`
      - `svc_ManualSample_Nudge`
      - `svc_ManualSample_Expel`
      - `svc_GetValvePort`
      - `svc_SetValvePort`
      - `svc_GetSyringePumpPostion`
      - `svc_AspirateSample`
      - `svc_DispenseSample`
      - `svc_GetProbePostion`
      - `svc_SetProbePostion`
      - `svc_MoveProbe`
      - `svc_GetSampleWellPosition`
      - `svc_SetSampleWellPosition`
      - RotateSampleTray
      - EjectSampleTray
      - `svc_PerformSampleRepetition`
      - `svc_PerformValveRepetition`
      - `svc_PerformSyringeRepetition`
      - `svc_PerformFocusRepetition`
      - `svc_PerfomReagentRepetition`
      - `svc_PerformPlateCalibration`
      - `svc_PerformCarouselCalibration`
      - `svc_CancelCalibration`
      - ExportInstrumentData
      - ImportInstrumentData
      - RetrieveHistogramForResultRecord

## 0.24 Beta

  - No APIs added.
  - Some integration w/ user interface.

## 0.25 Beta

  - Supported APIs
      - GetSupportedAnalysisCharacteristics
      - GetNameForCharacteristic

  - Issues
      - UI is sending analysis index of 1.
          - DLL forces all analysis indices to 0x0000 for now.
      - UI sends substrate of 0 for Carousel and 1 for Plate.
          - DLL forces these to 1 for Carousel and 2 for Plate since DLL
            uses zero to indicate that no substrate has been selected.

## 0.26 Beta

  - Added APIs
      - RetrieveWorkQueueList
      - RetrieveSampleRecordList
      - RetrieveSampleImageSetRecordList
      - RetrieveImageRecordList
      - RetrieveResultRecordList
  - Changed signature of HawkeyeLogicInterface::Initialize  
    
    From: (bool `without_hardware` = false)  
    To: (bool `with_hardware` = true)  
    Reason: to prevent the name confusion with business logic
    initialization.
  - Changed the Workqueue enum eSamplePostWash : `uint16_t`
    
    From: { eSamplePostWash:eMinWash = 0; eSamplePostWash:eMaxWash}
    To: { eSamplePostWash:eNormalWash = 0; eSamplePostWash:FastWash}
  - Changed signature of HawkeyeLogicInterface:: `svc_CameraAutoFocus`
    
    From: (SamplePosition focusbead_location,
    `autofocus_state_callback_t` on_status_change)  
    To: (SamplePosition focusbead_location,
    `autofocus_state_callback_t` on_status_change,
    `countdown_timer_callback_t` on_timer_tick)
    
    Reason: To update User interface about the how much time (in
    seconds) is remaining for “Sample Settling Delay” to complete.

## 0.27 Beta

  - Added logging to SetWorkQueue.

## 0.28 Beta

  - Added initialization state parameter to IsInitializationComplete.
      - See InitializationState.hpp.
  - Removed `roi_x_pixels` and `roi_y_pixels` from CellType.hpp.
  - Supports saving/retrieving of results.

## 0.29 Beta

  - Quick turnaround for UI testing. DLL software was not tagged.
  - Put `roi_x_pixels` and `roi_y_pixels` back into CellType.hpp. These
    values are set from the CellCounting parameters in HawkeyeDLL.info.

## 0.30 Beta

  - Put IsInitializationComplete with the status as a parameter back in.

## 0.31 Beta

  - Changed the signature of IsInitializationComplete. Moved return
    status from a parameter to the method return.

## 0.32 Beta

  - Added support for Host login when security is disabled,
    Host-directed disabling of user accounts.
  - Added API FreeImageSetWrapper to release `imagesetwrapper_t`
    structure.
  - Added API FreeImageWrapper to release `imagewrapper_t` structure.
  - Added API FreeAutofocusResults to release `AutofocusResults_t`
    structure.
  - Changed signature of HawkeyeLogicInterface::
    GetReagentContainerStatus
    
    From: (uint16_t container_num, ReagentContainerState& status)  
    To: (uint16_t container_num, ReagentContainerState*& status)
  - Changed signature of HawkeyeLogicInterface:: RetrieveImage
    From: (uuid__t id, imagewrapper_t& img)  
    To: (uuid__t id, imagewrapper_t*& img)
  - Changed signature of HawkeyeLogicInterface:: RetrieveAnnotatedImage
    
    From: (uuid__t result_id, `uuid__t` image_id, imagewrapper_t& img)  
    To: (uuid__t result_id, `uuid__t` image_id, imagewrapper_t*& img)
  - Changed signature of HawkeyeLogicInterface:: RetrieveBWImage
    
    From: (uuid__t image_id, imagewrapper_t& img)  
    To: (uuid__t image_id, imagewrapper_t*& img)
  - Changed signature of `HawkeyeLogicInterface::service_analysis_result_callback`
    
    From: (HawkeyeError, BasicResultAnswers, imagewrapper_t)  
    To: (HawkeyeError, BasicResultAnswers, imagewrapper_t*)
  - Changed signature of HawkeyeLogicInterface::
    `service_live_image_callback`
    
    From: (HawkeyeError, imagewrapper_t)  
    To: (HawkeyeError, imagewrapper_t*)
  - Changed signature of HawkeyeLogicInterface::
    `workqueue_image_result_callback`
    
    From: (WorkQueueItem*, uint16_t, imagesetwrapper_t, BasicResultAnswers, BasicResultAnswers)  
    To: (WorkQueueItem*, uint16_t, imagesetwrapper_t*,
    BasicResultAnswers, BasicResultAnswers)
  - Added API FreeAuditLogEntry to release `audit_log_entry` structure.
  - Added API FreeWorkqueueSampleEntry to release
    `workqueue_sample_entry` structure.
  - Added API FreeSampleActivityEntry to release `sample_activity_entry`
    structure.
  - Added API FreeCalibrationHistoryEntry to release
    `calibration_history_entry` structure.
  - Added API FreeErrorLogEntry to release `error_log_entry` structure.
  - Added API FreeCalibrationConsumable to release
    `calibration_consumable` structure.
  - Added API FreeDataSignature to release `DataSignature_t` structure.
  - Added API FreeDataSignatureInstance to release
    `DataSignatureInstance_t` structure.
  - Removed the Following API’s
      - `svc_PerformSampleRepetition`
      - EjectSampleTray
      - RotateSampleTray
  - Updated the Definition of EjectSampleStage and RotateCarousel
  - Signature change RotateCarousel() to RotateCarousel(SamplePosition&
    tubeNum)
  - Signature change RotateCarousel() to RotateCarousel(SamplePosition&
    tubeNum)
  - Signature change `svc_GetSampleWellPosition(char row, uint32_t`
    `&pos)` to `svc_GetSampleWellPosition(SamplePosition& pos)`
  - Signature change `svc_SetSampleWellPosition(const char row,`
    `uint32_t pos)` to `svc_SetSampleWellPosition(SamplePosition pos)`
  - Added new API `svc_InitializeCarrier()`

## 0.33 Beta

  - Added new APIs for deletion of “Work Queue Record” and “Sample
    Record”
    
      - DeleteWorkQueueRecord(…)
      - DeleteSampleRecord(…)

## 0.37 Beta

  - Fixed the following issues:
      - We are not getting Sample status (like eMixing, eDilution,
        eCompleted) from the call back value, only eCompleted we are
        getting.
      - StopQueueExecution API is not working. Throwing exception.
      - Flow cell depth range is 50-100 but we are getting 1377 from the
        `svc_getflowcelldepthsetting()` API.
      - Service user not able to run sample. SetWorkQueue() API giving
        invalidArgs.
      - Below APIs returning default data whenever we restart the
        application. ***These should be fixed, though they have not been
        tested.***
          - GetSystemSecurityFeaturesState()
          - GetUserInactivityTimeout()
          - GetUserPasswordExpiration()

## 0.38 Beta

  - Changed return variable of GenerateHostPassword from char* to
    uint32_t.

## 0.39 Beta

  - Bug fixes to reagent pack code.

  - API name change from `svc_InitializeCarrier` to InitializeCarrier and
    API permission is changed from Service to any logged in User.

  - Permission level of `svc_GetTemporaryCellType`,
    `svc_SetTemporaryCellType`, `svc_StartLiveImageFeed`
    `svc_StopLiveImageFeed` APIs are changed to elevated.

## 0.40 Beta

  - Implements NightClean.

  - Fixed GetSystemStatus.

## 0.41 Beta

  - Added simulation mode for PlateController::ProbeUp to fix crash when
    running in simulation mode.

## 0.42 Beta

  - Added simulation mode for CarouselController::ProbeUp to fix crash
    when running in simulation mode.

## 0.43 Beta

  - Fixed timezone issue.

## 0.49 Beta

  - There are two known data structure misalignments between UI v0.60
    and this DLL code.
    
      - `calibration_history_entry` structure
        
          - This causes the UI code that calls
            RetrieveCalibrationActivityLogRange to crash.
    
      - SystemStatus structure
        
          - This does not crash the software, it results in erroneous
            values in the UI.

  - Completed firmware update.

  - Completed implementation of calibration history.

  - Implemented saving binary files.

  - SystemStatus is logged to HawkeyeDLL.log when GetSystemStatus is
    called.

  - Replaced simulation images.

## 0.50 Beta

  - Add “SwapUser” API call.

  - Change name of Host access account

  - Added FreeSimpleArrayTypeMemory
    
      - Used to free memory returned:
        
          - GetUserAnalysisTypeIndices
        
          - GetMyAnalysisTypeIndices
        
          - GetUserCellTypeIndices
        
          - GetMyCellTypeIndices

  - Changed signature of the callback in ReanalyzeSample API from: 
    ```
    typedef void(*sample_analysis_callback)(HawkeyeError,
    `uuid__t *sample_id*, `uuid__t` *result_id*,
    **BasicResultAnswers** *result answer*);
    ```
    to:
    ```
    typedef void(*sample_analysis_callback)(HawkeyeError,
    uuid__t *sample_id*,**ResultRecord*&** *Analysis Data*);
    ```
    
    Purpose: To provide the complete ResultRecord data, instead of just
    cumulative data.
    
    NOTE: Now UI is not required to call a retrieve API for complete
    result record. After the sample reanalysis (“ReanlzeSample”) UI must
    invoke the FreeResultRecord
API

| FROM                                             | TO                                           |
| ------------------------------------------------ | -------------------------------------------- |
| `svc_GetTemporaryAnalysisDefinition`             | `GetTemporaryAnalysisDefinition`             |
| `svc_SetTemporaryAnalysisDefinition`             | `SetTemporaryAnalysisDefinition`             |
| `svc_SetTemporaryAnalysisDefinitionFromExisting` | `SetTemporaryAnalysisDefinitionFromExisting` |
| `svc_GetTemporaryCellType`                       | `GetTemporaryCellType`                       |
| `svc_SetTemporaryCellType`                       | `SetTemporaryCellType`                       |
| `svc_SetTemporaryCellTypeFromExisting`           | `SetTemporaryCellTypeFromExisting`           |

  - Following APIs are renamed

  - Permission of the following APIs is changed from service to elevated
    
      - GetTemporaryAnalysisDefinition
    
      - SetTemporaryAnalysisDefinition
    
      - SetTemporaryAnalysisDefinitionFromExisting
    
      - SetTemporaryCellTypeFromExisting

## 0.51 Beta

  - "StartDecontaminateFlowCell(...)" method call will initialize the
    available carrier (Carousel/Plate) and move the carrier to following
    location
    
      - Plate As Carrier : "A-1" position
    
      - Carousel As Carrier : Position where any tube is detected while
        moving in clockwise direction
        
        If system is unable to initialize the carrier or find tube (in
        case of Carousel) it will either return
        "HawkeyeError::eHardwarefault" or failed State
        “eDecontaminateFlowCellState::dfc_Failed”

  - "StartBrightfieldDustSubtract(...)" method call will initialize the
    available carrier (Carousel/Plate) and move the carrier to following
    location
    
      - Plate As Carrier: "A-1" position
    
      - Carousel As Carrier : Position where any tube is detected while
        moving in clockwise direction
        
        If system is unable to initialize the carrier or find tube (in
        case of Carousel) it will either return
        `HawkeyeError::eHardwarefault` or failed State
        `eBrightFieldDustSubtractionState::bds_Failed`

  - `svc_CameraAutoFocus(...)` method call will initialize the
    available carrier (Carousel/Plate) and move the carrier to following
    location
    
      - Plate As Carrier: "A-1" position
    
      - Carousel As Carrier : Position where any tube is detected while
        moving in clockwise direction
        
        If system is unable to initialize the carrier or find tube (in
        case of Carousel) it will either return
        "HawkeyeError::eHardwarefault" or failed State
        “eAutofocusState::af_Failed”

# 0.54 Beta

  - Add `uint8_t aspiration_cycles` to CellType structure
    
      - This should be user-settable as part of the cell type
        definition.

  - Add `uint8_t mixing_cycles` to AnalysisDefinition structure
    
      - This can be specialized for a particular cell type.

  - Motor Status Changed structure of HawkeyeLogicInterface::
    MotorStatus
    
    From:
    ```
    (eMotorFlags (flags), int32_t (position), char
    (position_description[16]))  
    ```
    To:
    ```
    (uint16_t (flags), int32_t (position), char
    (position_description[16]))
    ```
    
    Reason for change: Now MotorStatus::flags will contains multiple
    states (using bitmask of eMotorFlags)

# 0.57 Beta

  - Changes to HawkeyeError enum list
  - Changes to CalibrationState enum list
  - Changes to InitializationState enum list
  - Changes to BasicResultAnswers struct
    - Add a success/failure code for the result (`E_ERRORCODE eProcessedStatus`) which indicates the validity of the result
    - Add an indication for how many images have been included in the averaged/cumulative analysis data (`nTotalCumulative_Imgs`)
  - Signature changes to
    - Change Analysis Index from 32 to 16 bits
      - AddAnalysis
      - RemoveAnalysis
      - IsAnalysisSpecializedForCellType
      - GetAnalysisForCellType
      - RemoveAnalysisSpecializationForCellType
            
# Sprint 4

  - Change in `audit_event_type` enum list (ReportsCommon.hpp)
    - Added new audit entry (`evt_manualfocuschange`) for manual focus change

  - Added two more Draining states (eULDraining4 & eULDraining5) to the Reagent Unload Sequence enum, that now has following states, with indeces startting at 0
      - eULIdle
      - eULDraining1
      - eULPurging1
      - eULDraining2
      - eULPurging2
      - eULDraining3
      - eULDraining4
      - eULDraining5
      - eULPurging3
      - eULRetractingProbes
      - eULUnlatchingDoor
      - eULComplete
      - `eULFailed_DrainPurge`
      - `eULFailed_ProbeRetract`
      - `eULFailed_DoorUnlatch`
      - `eULFailure_StateMachineTimeout`
      

# Sprint 6
 - SetConcentrationCalibration changed to require Service level.
 

# Sprint 8
 - Added a new enum for work queue status (`eWorkQueueStatus wqStatus`)
 - Changed signature of HawkeyeLogicInterface::GetWorkQueueStatus
      From: (uint32_t& queue_length, WorkQueueItem*& wq)
      To: (uint32_t& queue_length, WorkQueueItem*& wq, eWorkQueueStatus& wqStatus)
 
# Kyle's refactoring

 - Change `FreeSimple...` functions to `Free[ListOf]<CharBuffer|TaggedType>`
 - NEVER 'free' non-char memory with `Free(ListOf)CharBuffer`
 - Other renaming to ensure `Free` functions correspond to `Retriev` or `Get`
   functions
 - Add `FreeHistogramBins`
 
 # Sprint 18
 - `RetrieveCalibrationActivityLog` changed to user level from elevated user level.
 - `RetrieveCalibrationActivityLogRange` changed to user level from elevated user level.

  # Sprint 19
 Added the following new HawkeyeError
 - ePlateNotFound    : Currently not used
 - eCarouselNotFound : Currently not used
 - eLowDiskSpace     : Currently not used
 - eReagentError     : Currently not used
 - eSpentTubeTray    : Currently not used
 
# Sprint 21
 Added support for following APIs
 - ArchiveAuditTrailLog
 - ReadArchivedAuditTrailLog
 - ArchiveInstrumentErrorLog
 - ReadArchivedInstrumentErrorLog
 - ArchiveSampleActivityLog
 - ReadArchivedSampleActivityLog
 
 Update return error type for `GetWorkQueueStatus` API. It will return `HawkeyeError::eSuccess` always
 
 Update Return type for following APIs
 - RetrieveAuditTrailLog
 - RetrieveAuditTrailLogRange
 - RetrieveInstrumentErrorLog
 - RetrieveInstrumentErrorLogRange
 - RetrieveSampleActivityLog
 - RetrieveSampleActivityLogRange
 These APIs will no longer return `HawkeyeError::eNoneFound` if there are no entries found, instead these will return `HawkeyeError::eSuccess`
 with zero entries
 
# Sprint 22

 Added a new API to enable user without administrator log in
 `HawkeyeError AdministrativeUserEnable(const char* administrator_account, const char* administrator_password, const char* user_account)`
 
 Removed the API
 `HawkeyeError  ClearSelectedSampleData(uint32_t n_samples, uuid__t* sample_list, bool clear_images_only)`


# Appendix A: Permission Levels for API calls

| **Function **                                    | **Access Level** | **Allowed For Service** |
| ------------------------------------------------ | ---------------- | ----------------------- |
| `Initialize`                                     | none             |                         |
| `IsInitializationComplete`                       | none             |                         |
| `Shutdown`                                       | none             |                         |
| `IsShutdownComplete`                             | none             |                         |
| `GetSystemStatus`                                | none             |                         |
| `FreeSystemStatus`                               | none             |                         |
| `SystemErrorCodeToString`                        | none             |                         |
| `SystemErrorCodeToExpandedStrings`               | none             |                         |
| `ClearSystemErrorCode`                           | user             |                         |
| `GetVersionInformation`                          | none             |                         |
| `FreeCharBuffer`                                 | none             |                         |
| `FreeListOfCharBuffer`                           | none             |                         |
| `FreeTaggedBuffer`                               | none             |                         |
| `FreeListOfTaggedBuffers`                        | none             |                         |
| `GetUserList`                                    | none             |                         |
| `GetCurrentUser`                                 | none             |                         |
| `GetUserDisplayName`                             | none             |                         |
| `LoginUser`                                      | none             |                         |
| `LogoutUser`                                     | none             |                         |
| `SwapUser`                                       | none             |                         |
| `GenerateHostPassword`                           | none             |                         |
| `AdministrativeUserDisable`                      | none             |                         |
| `GetSystemSecurityFeaturesState`                 | none             |                         |
| `GetUserInactivityTimeout`                       | none             |                         |
| `GetUserInactivityTimeout_Setting`               | none             |                         |
| `GetUserPasswordExpiration`                      | none             |                         |
| `GetUserPasswordExpiration_Setting`              | none             |                         |
| `LogoutUser_Inactivity`                          | user             |                         |
| `GetReagentContainerStatus`                      | none             |                         |
| `GetReagentContainerStatusAll`                   | none             |                         |
| `FreeReagentContainerState`                      | none             |                         |
| `GetReagentDefinitions`                          | none             |                         |
| `FreeReagentDefinitions`                         | none             |                         |
| `FreeCellType`                                   | none             |                         |
| `FreeAnalysisDefinitions`                        | none             |                         |
| `FreeUserProperties`                             | none             |                         |
| `svc_CameraFocusAdjust`                          | service          |                         |
| `svc_CameraFocusRestoreToSavedLocation`          | elevated         |                         |
| `svc_CameraFocusStoreCurrentPosition`            | service          |                         |
| `svc_CameraAutoFocus`                            | elevated         |                         |
| `svc_CameraAutoFocus_ServiceSkipDelay`           | elevated         |                         |
| `svc_CameraAutoFocus_FocusAcceptance`            | elevated         |                         |
| `svc_CameraAutoFocus_Cancel`                     | elevated         |                         |
| `svc_GetFlowCellDepthSetting`                    | service          |                         |
| `svc_SetFlowCellDepthSetting`                    | service          |                         |
| `svc_SetCameraLampState`                         | service          |                         |
| `svc_SetCameraLampIntensity`                     | service          |                         |
| `svc_GetCameraLampState`                         | service          |                         |
| `svc_GetTemporaryAnalysisDefinition`             | elevated         |                         |
| `svc_SetTemporaryAnalysisDefinition`             | elevated         |                         |
| `svc_SetTemporaryAnalysisDefinitionFromExisting` | elevated         |                         |
| `svc_GetTemporaryCellType`                       | elevated         |                         |
| `svc_SetTemporaryCellType`                       | elevated         |                         |
| `svc_SetTemporaryCellTypeFromExisting`           | elevated         |                         |
| `svc_GenerateSingleShotAnalysis`                 | service          |                         |
| `svc_GenerateContinuousAnalysis`                 | service          |                         |
| `svc_StopContinuousAnalysis`                     | service          |                         |
| `svc_StartLiveImageFeed`                         | elevated         |                         |
| `svc_StopLiveImageFeed`                          | elevated         |                         |
| `svc_ManualSample_Load`                          | service          |                         |
| `svc_ManualSample_Nudge`                         | service          |                         |
| `svc_ManualSample_Expel`                         | service          |                         |
| `svc_GetValvePort`                               | none             |                         |
| `svc_SetValvePort`                               | service          |                         |
| `svc_GetSyringePumpPostion`                      | user             |                         |
| `svc_AspirateSample`                             | service          |                         |
| `svc_DispenseSample`                             | service          |                         |
| `svc_GetProbePostion`                            | user             |                         |
| `svc_SetProbePostion`                            | service          |                         |
| `svc_MoveProbe`                                  | service          |                         |
| `svc_GetSampleWellPosition`                      | user             |                         |
| `svc_SetSampleWellPosition`                      | service          |                         |
| `svc_PerformSampleRepetition`                    | service          |                         |
| `svc_PerformValveRepetition`                     | service          |                         |
| `svc_PerformSyringeRepetition`                   | service          |                         |
| `svc_PerformFocusRepetition`                     | service          |                         |
| `svc_PerfomReagentRepetition`                    | service          |                         |
| `svc_PerformPlateCalibration`                    | service          |                         |
| `svc_PerformCarouselCalibration`                 | service          |                         |
| `svc_CancelCalibration`                          | service          |                         |
| `InitializeCarrier`                              | user             |                         |
| `SetSystemSecurityFeaturesState`                 | admin            | No                      |
| `AddUser`                                        | admin            | No                      |
| `RemoveUser`                                     | admin            | No                      |
| `EnableUser`                                     | admin            |                         |
| `ChangeUserPassword`                             | admin            |                         |
| `ChangeUserDisplayName`                          | admin            | No                      |
| `ChangeUserPermissions`                          | admin            | No                      |
| `SetUserProperty`                                | elevated         |                         |
| `GetUserProperty`                                | elevated         |                         |
| `GetUserProperties`                              | elevated         |                         |
| `SetUserCellTypeIndices`                         | elevated         |                         |
| `GetUserCellTypeIndices`                         | elevated         |                         |
| `SetUserAnalysisIndices`                         | elevated         |                         |
| `GetUserAnalysisIndices`                         | elevated         |                         |
| `GetUserFolder`                                  | admin            |                         |
| `SetUserFolder`                                  | admin            |                         |
| `SetUserInactivityTimeout`                       | admin            | No                      |
| `SetUserPasswordExpiration`                      | admin            | No                      |
| `GetFactoryCellTypes`                            | user             |                         |
| `GetUserCellTypes`                               | user             |                         |
| `AddCellType`                                    | elevated         |                         |
| `ModifyCellType`                                 | elevated         |                         |
| `RemoveCellType`                                 | elevated         |                         |
| `GetFactoryAnalyses`                             | user             |                         |
| `GetUserAnalyses`                                | user             |                         |
| `AddAnalysis`                                    | elevated         |                         |
| `ModifyBaseAnalysis`                             | elevated         |                         |
| `RemoveAnalysis`                                 | elevated         |                         |
| `IsAnalysisSpecializedForCellType`               | user             |                         |
| `GetAnalysisForCellType`                         | user             |                         |
| `SpecializeAnalysisForCellType`                  | elevated         |                         |
| `RemoveAnalysisSpecializationForCellType`        | elevated         |                         |
| `ExportInstrumentConfiguration`                  | admin            | No                      |
| `ImportInstrumentConfiguration`                  | admin            | No                      |
| `ExportInstrumentData`                           | admin            | No                      |
| `ImportInstrumentData`                           | admin            | No                      |
| `AddBioprocess`                                  | elevated         |                         |
| `SetBioprocessActivation`                        | elevated         |                         |
| `RemoveBioprocess`                               | elevated         | No                      |
| `AddQualityControl`                              | elevated         |                         |
| `RemoveQualityControl`                           | elevated         | No                      |
| `AddSignatureDefinition`                         | admin            | No                      |
| `RemoveSignatureDefinition`                      | admin            | No                      |
| `SetConcentrationCalibration`                    | service          |                         |
| `SetSizeCalibration`                             | elevated         |                         |
| `StartBrightfieldDustSubtract`                   | elevated         |                         |
| `GetBrightfieldDustSubtractState`                | elevated         |                         |
| `AcceptDustReference`                            | elevated         |                         |
| `CancelBrightfieldDustSubtract`                  | elevated         |                         |
| `RetrieveAuditTrailLog`                          | elevated         |                         |
| `RetrieveAuditTrailLogRange`                     | elevated         |                         |
| `ArchiveAuditTrailLog`                           | elevated         |                         |
| `ReadArchivedAuditTrailLog`                      | elevated         |                         |
| `RetrieveInstrumentErrorLog`                     | elevated         |                         |
| `RetrieveInstrumentErrorLogRange`                | elevated         |                         |
| `ArchiveInstrumentErrorLog`                      | elevated         |                         |
| `ReadArchivedInstrumentErrorLog`                 | elevated         |                         |
| `RetrieveSampleActivityLog`                      | elevated         |                         |
| `RetrieveSampleActivityLogRange`                 | elevated         |                         |
| `ArchiveSampleActivityLog`                       | elevated         |                         |
| `ReadArchivedSampleActivityLog`                  | elevated         |                         |
| `RetrieveCalibrationActivityLog`                 | user             |                         |
| `RetrieveCalibrationActivityLogRange`            | user             |                         |
| `ClearCalibrationActivityLog`                    | elevated         |                         |
| `ClearExportDataFolder`                          | elevated         |                         |
| `ClearSelectedSampleData`                        | elevated         |                         |
| `ChangeMyPassword`                               | user             | No                      |
| `ValidateMe`                                     | user             |                         |
| `GetMyProperty`                                  | user             |                         |
| `GetMyCellTypeIndices`                           | user             |                         |
| `GetMyAnalysisIndices`                           | user             |                         |
| `GetMyFolder`                                    | user             |                         |
| `GetMyDisplayName`                               | user             |                         |
| `GetMyProperties`                                | user             |                         |
| `SetWorkQueue`                                   | user             |                         |
| `AddItemToWorkQueue`                             | user             |                         |
| `StartWorkQueue`                                 | user             |                         |
| `FreeWorkQueueItem`                              | none             |                         |
| `GetWorkQueueStatus`                             | user             |                         |
| `GetCurrentWorkQueueItem`                        | user             |                         |
| `SkipCurrentWorkQueueItem`                       | user             |                         |
| `PauseQueue`                                     | user             |                         |
| `ResumeQueue`                                    | user             |                         |
| `StopQueue`                                      | user             |                         |
| `GetErrorAsStr`                                  | none             |                         |
| `GetPermissionLevelAsStr`                        | none             |                         |
| `GetReagentPackLoadStatusAsStr`                  | none             |                         |
| `GetReagentPackUnloadStatusAsStr`                | none             |                         |
| `GetReagentFlushFlowCellsStatusAsStr`            | none             |                         |
| `GetReagentDecontaminateFlowCellStatusAsStr`     | none             |                         |
| `GetWorkQueueStatusAsStr`                        | none             |                         |
| `SampleTubeDiscardTrayEmptied`                   | user             |                         |
| `EjectSampleStage`                               | user             |                         |
| `RotateCarousel`                                 | user             |                         |
| `UnloadReagentPack`                              | user             |                         |
| `LoadReagentPack`                                | user             |                         |
| `SetReagentContainerLocation`                    | user             |                         |
| `StartFlushFlowCell`                             | user             |                         |
| `GetFlushFlowCellState`                          | user             |                         |
| `CancelFlushFlowCell`                            | user             |                         |
| `StartDecontaminateFlowCell`                     | user             |                         |
| `GetDecontaminateFlowCellState`                  | user             |                         |
| `CancelDecontaminateFlowCell`                    | user             |                         |
| `StartPrimeReagentLines`                         | user             |                         |
| `GetPrimeReagentLinesState`                      | user             |                         |
| `CancelPrimeReagentLines`                        | user             |                         |
| `GetAllCellTypes`                                | user             |                         |
| `GetAllAnalyses`                                 | user             |                         |
| `GetBioprocessList`                              | user             |                         |
| `GetActiveBioprocessList`                        | user             |                         |
| `GetQualityControlList`                          | user             |                         |
| `GetActiveQualityControlList`                    | user             |                         |
| `FreeBioprocess`                                 | none             |                         |
| `FreeQualityControl`                             | none             |                         |
| `GetConcentrationCalibrationStatus`              | user             |                         |
| `GetSizeCalibrationStatus`                       | user             |                         |
| `GetSupportedAnalysisParameterNames`             | user             |                         |
| `GetSupportedAnalysisCharacteristics`            | user             |                         |
| `GetNameForCharacteristic`                       | user             |                         |
| `FreeListOfBlobMeasurement`                      | none             |                         |
| `FreeListOfResultRecord`                         | none             |                         |
| `FreeListOfImageRecord`                          | none             |                         |
| `FreeListOfImageSetRecord`                       | none             |                         |
| `FreeListOfSampleRecord`                         | none             |                         |
| `FreeListOfWorkQueueRecord`                      | none             |                         |
| `FreeDetailedResultMeasurement`                  | none             |                         |
| `RetrieveWorkQueue`                              | user             |                         |
| `RetrieveWorkQueues`                             | user             |                         |
| `RetrieveWorkQueueList`                          | user             |                         |
| `RetrieveSampleRecord`                           | user             |                         |
| `RetrieveSampleRecords`                          | user             |                         |
| `RetrieveSampleRecordList`                       | user             |                         |
| `RetrieveSampleImageSetRecord`                   | user             |                         |
| `RetrieveSampleImageSetRecords`                  | user             |                         |
| `RetrieveSampleImageSetRecordList`               | user             |                         |
| `RetrieveImageRecord`                            | user             |                         |
| `RetrieveImageRecords`                           | user             |                         |
| `RetrieveImageRecordList`                        | user             |                         |
| `RetrieveResultRecord`                           | user             |                         |
| `RetrieveResultRecords`                          | user             |                         |
| `RetrieveResultRecordList`                       | user             |                         |
| `RetrieveResultSummary`                          | user             |                         |
| `RetrieveResultSummaries`                        | user             |                         |
| `RetrieveResultSummaryList`                      | user             |                         |
| `RetrieveDetailedMeasurementsForResultRecord`    | user             |                         |
| `RetrieveImage`                                  | user             |                         |
| `RetrieveAnnotatedImage`                         | user             |                         |
| `RetrieveBWImage`                                | user             |                         |
| `ReanalyzeSample`                                | user             |                         |
| `RetrieveHistogramForResultRecord`               | user             |                         |
| `FreeHistogramBins`                              | none             |                         |
| `RetrieveSampleRecordsForBioprocess`             | user             |                         |
| `RetrieveSampleRecordsForQualityControl`         | user             |                         |
| `RetrieveSignatureDefinitions`                   | none             |                         |
| `SignResultRecord`                               | user             | No                      |
| `GetRootFolderForExportData`                     | user             |                         |
| `FreeImageSetWrapper`                            | none             |                         |
| `FreeImageWrapper`                               | none             |                         |
| `FreeAutofocusResults`                           | none             |                         |
| `FreeAuditLogEntry`                              | none             |                         |
| `FreeWorkqueueSampleEntry`                       | none             |                         |
| `FreeSampleActivityEntry`                        | none             |                         |
| `FreeCalibrationHistoryEntry`                    | none             |                         |
| `FreeErrorLogEntry`                              | none             |                         |
| `FreeCalibrationConsumable`                      | none             |                         |
| `FreeDataSignature`                              | none             |                         |
| `FreeDataSignatureInstance`                      | none             |                         |
| `StartDrainReagentPack`                          | none             |                         |
| `GetDrainReagentPackState`                       | none             |                         |
| `CancelDrainReagentPack`                         | none             |                         |
| `DeleteWorkQueueRecord`                          | admin            |                         |
| `DeleteSampleRecord`                             | admin            |                         |
