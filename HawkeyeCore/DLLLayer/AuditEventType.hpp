#pragma once

#include <cstdint>

enum audit_event_type : uint32_t
{
	evt_login = 0,
	evt_logout,
	evt_logoutforced,
	__UNUSED__3,
	__UNUSED__4,
	__UNUSED__5,
	evt_loginfailure,
	evt_accountlockout,

	evt_useradd,
	evt_userremove,
	evt_userenable,			// 10
	evt_userdisable,
	evt_passwordchange,
	evt_passwordreset,
	evt_userpermissionschange,
	__UNUSED__15,

	evt_securityenable,
	evt_securitydisable,

	evt_celltypecreate,
	evt_celltypemodify,
	evt_celltypedelete,		// 20

	evt_analysiscreate,
	evt_analysismodify,
	evt_analysisdelete,

	evt_bioprocesscreate,
	evt_bioprocessdelete,

	evt_qcontrolcreate,
	evt_qcontroldelete,

	evt_fluidicsflush,
	evt_fluidicsprime,
	evt_fluidicsdrain,		// 30
	evt_fluidicsdecontaminate,
	evt_fluidicspurge,
	evt_fluidicsnightlyclean,	// This is the "standard" (legacy) nightly clean.

	evt_reagentload,
	evt_reagentunload,
	evt_reagentinvalid,
	evt_reagentunusable,

	evt_firmwareupdate,

	evt_auditlogarchive,
	evt_errorlogarchive,	// 40
	evt_samplelogarchive,

	__UNUSED__42,

	evt_datavalidationfailure,

	evt_signaturedefinitionadd,
	evt_signaturedefinitionremove,

	evt_concentrationinterceptset,
	evt_concentrationinterceptnotset,
	evt_concentrationslopeset,
	evt_concentrationslopenotset,
	evt_sizeinterceptset,	// 50
	evt_sizeinterceptnotset,
	evt_sizeslopeset,
	evt_sizeslopenotset,

	evt_notAuthorized,

	evt_instrumentconfignotfound,
	evt_instrumentconfigimported,
	evt_instrumentconfigexported,

	evt_autofocusaccepted,

	evt_clearedexportdata,
	__UNUSED__60,			// 60

	evt_clearedcalibrationfactors,

	evt_dustsubtractionaccepted,

	evt_deletesamplerecord,
	evt_deleteworklistrecord,

	evt_manualfocusoperation,

	evt_bioprocessactivate,
	evt_bioprocessdeactivate,

	evt_datasignatureapplied,
	evt_deleteresultrecord,

	evt_sampleprocessingerror,// 70

	evt_offlinemode,
	evt_sampleresultcreated,
	evt_Instrumentdataexported,

	evt_setuserpasswordexpiration,
	evt_setuserinactivitytimeout,

	evt_setusercfg,

	evt_updatefailed,
	evt_fluidicsautomationnightlyclean,
	evt_acupusingstandardconcentrationintercept,
	evt_serialnumber,		// 80
	evt_worklist,
	evt_automation,
	evt_acup,
	evt_qcontrolmodify,
	evt_deletecampaigndata,
	evt_flowcelldepthupdate,
	evt_automationlocked,
	evt_automationunlocked,
};
