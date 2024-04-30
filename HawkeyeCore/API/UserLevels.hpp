#pragma once
#include <cstdint>
/*
 * UserPermissionLevel
 *
 * Describes the permission level associated with a user.
 * NOTE: While "eService" is described as a permission level, it SHALL NOT
 *       be possible to construct a new user with "eService" permissions
 */

const int NO_PWD_EXPIRATION	= 0;		// password expiration days value representing the 'No expiration' state

enum UserPermissionLevel
{
	eNormal = 0,
	eElevated,
	eAdministrator,
	eService,
};

enum SwapUserRole
{
	eAdminOnly = 0,
	eServiceOnly,
	eServiceOrAdmin
};
