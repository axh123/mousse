#ifndef CORE_PRIMITIVES_STRINGS_STRING_OPS_HPP_
#define CORE_PRIMITIVES_STRINGS_STRING_OPS_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
//   string_ops.cpp

#include "string.hpp"
#include "dictionary.hpp"
#include "hash_table.hpp"


namespace mousse {
namespace stringOps {

//- Expand occurences of variables according to the mapping
//  Expansion includes:
//  -# variables
//    - "$VAR", "${VAR}"
//
//  Supports default values as per the Bourne/Korn shell.
//  \code
//      "${parameter:-defValue}"
//  \endcode
//  If parameter is unset or null, the \c defValue is substituted.
//  Otherwise, the value of parameter is substituted.
//
//  Supports alternative values as per the Bourne/Korn shell.
//  \code
//      "${parameter:+altValue}"
//  \endcode
//  If parameter is unset or null, nothing is substituted.
//  Otherwise the \c altValue is substituted.
//
//  Any unknown entries are removed silently.
//
//  Malformed entries (eg, brace mismatch, sigil followed by bad character)
//  are left as is.
//
//  \note the leading sigil can be changed to avoid conflicts with other
//  string expansions
string expand
(
  const string&,
  const HashTable<string, word, string::hash>& mapping,
  const char sigil = '$'
);

//- Inplace expand occurences of variables according to the mapping
//  Expansion includes:
//  -# variables
//    - "$VAR", "${VAR}"
//
//  Supports default values as per the Bourne/Korn shell.
//  \code
//      "${parameter:-defValue}"
//  \endcode
//  If parameter is unset or null, the \c defValue is substituted.
//  Otherwise, the value of parameter is substituted.
//
//  Supports alternative values as per the Bourne/Korn shell.
//  \code
//      "${parameter:+altValue}"
//  \endcode
//  If parameter is unset or null, nothing is substituted.
//  Otherwise the \c altValue is substituted.
//
//  Any unknown entries are removed silently.
//
//  Malformed entries (eg, brace mismatch, sigil followed by bad character)
//  are left as is.
//
//  \note the leading sigil can be changed to avoid conflicts with other
//  string expansions
string& inplaceExpand
(
  string&,
  const HashTable<string, word, string::hash>& mapping,
  const char sigil = '$'
);

//- Expand occurences of variables according to the dictionary
//  Expansion includes:
//  -# variables
//    - "$VAR", "${VAR}"
//
//  Any unknown entries are left as-is
//
//  \note the leading sigil can be changed to avoid conflicts with other
//  string expansions
string expand
(
  const string&,
  const dictionary& dict,
  const char sigil = '$'
);

//- Get dictionary or (optionally) environment variable
string getVariable
(
  const word& name,
  const dictionary& dict,
  const bool allowEnvVars,
  const bool allowEmpty
);

//- Recursively expands (dictionary or environment) variable
//  starting at index in string. Updates index.
string expand
(
  const string& s,
  string::size_type& index,
  const dictionary& dict,
  const bool allowEnvVars,
  const bool allowEmpty
);

//- Inplace expand occurences of variables according to the dictionary
//  and optionally environment variables
//  Expansion includes:
//  -# variables
//    - "$VAR", "${VAR}"
//
//  with the "${}" syntax doing a recursive substitution.
//  Any unknown entries are left as-is
//
//  \note the leading sigil can be changed to avoid conflicts with other
//  string expansions
string& inplaceExpand
(
  string& s,
  const dictionary& dict,
  const bool allowEnvVars,
  const bool allowEmpty,
  const char sigil = '$'
);

//- Inplace expand occurences of variables according to the dictionary
//  Expansion includes:
//  -# variables
//    - "$VAR", "${VAR}"
//
//  Any unknown entries are left as-is
//
//  \note the leading sigil can be changed to avoid conflicts with other
//  string expansions
string& inplaceExpand
(
  string&,
  const dictionary& dict,
  const char sigil = '$'
);

//- Expand initial tildes and all occurences of environment variables
//  Expansion includes:
//  -# environment variables
//    - "$VAR", "${VAR}"
//  -# current directory
//    - leading "./" : the current directory
//  -# tilde expansion
//    - leading "~/" : home directory
//    - leading "~user" : home directory for specified user
//    - leading "~OpenFOAM" : site/user OpenFOAM configuration directory
//
//  Supports default values as per the Bourne/Korn shell.
//  \code
//      "${parameter:-defValue}"
//  \endcode
//  If parameter is unset or null, the \c defValue is substituted.
//  Otherwise, the value of parameter is substituted.
//
//  Supports alternative values as per the Bourne/Korn shell.
//  \code
//      "${parameter:+altValue}"
//  \endcode
//  If parameter is unset or null, nothing is substituted.
//  Otherwise the \c altValue is substituted.
//
//  Any unknown entries are removed silently, if allowEmpty is true.
//
//  Malformed entries (eg, brace mismatch, sigil followed by bad character)
//  are left as is.
//
//  \sa
//  mousse::findEtcFile
string expand
(
  const string&,
  const bool allowEmpty = false
);

//- Expand initial tildes and all occurences of environment variables
//  Expansion includes:
//  -# environment variables
//    - "$VAR", "${VAR}"
//  -# current directory
//    - leading "./" : the current directory
//  -# tilde expansion
//    - leading "~/" : home directory
//    - leading "~user" : home directory for specified user
//    - leading "~OpenFOAM" : site/user OpenFOAM configuration directory
//
//  Supports default values as per the Bourne/Korn shell.
//  \code
//      "${parameter:-defValue}"
//  \endcode
//  If parameter is unset or null, the \c defValue is substituted.
//  Otherwise, the value of parameter is substituted.
//
//  Supports alternative values as per the Bourne/Korn shell.
//  \code
//      "${parameter:+altValue}"
//  \endcode
//  If parameter is unset or null, nothing is substituted.
//  Otherwise the \c altValue is substituted.
//
//  Any unknown entries are removed silently, if allowEmpty is true.
//
//  Malformed entries (eg, brace mismatch, sigil followed by bad character)
//  are left as is.
//
//  Any unknown entries are removed silently if allowEmpty is true.
//  \sa
//  mousse::findEtcFile
string& inplaceExpand
(
  string&,
  const bool allowEmpty = false
);

//- Return string trimmed of leading whitespace
string trimLeft(const string&);

//- Trim leading whitespace inplace
string& inplaceTrimLeft(string&);

//- Return string trimmed of trailing whitespace
string trimRight(const string&);

//- Trim trailing whitespace inplace
string& inplaceTrimRight(string&);

//- Return string trimmed of leading and trailing whitespace
string trim(const string&);

//- Trim leading and trailing whitespace inplace
string& inplaceTrim(string&);

}  // namespace stringOps
}  // namespace mousse

#endif
