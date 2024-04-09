// ReSharper disable CppInconsistentNaming
#pragma once
#include "ExpandedUser.hpp"

/**
 * \brief Interface for security principal. Provides access to an ExpandedUser instance.
 */
class ISecurityPrincipal  // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
	virtual ~ISecurityPrincipal() = default;

	/*
	 * Return an ExpandedUser for the current security principal. The return value
	 * is used in UserInterface.
	 */
	virtual ExpandedUser *GetExpandedUser() = 0;
};
