#ifndef CONSTEXPRCORE_USEFUL_ABSTRACTIONS_H
#define CONSTEXPRCORE_USEFUL_ABSTRACTIONS_H

// Umbrella header: everything the library offers, in one include.
//
// Each component is also usable on its own — include just
// <ConstexprCore/fixed_string.h> if that is all you need. This header exists
// so that "give me the whole library" is one line, and so that the
// single-header amalgamation (singleheader/useful_abstractions.h) has a root
// to walk from.

#include <ConstexprCore/useful_abstractions_version.h>

#include <ConstexprCore/constexpr_check.h>
#include <ConstexprCore/constexpr_hash.h>
#include <ConstexprCore/constexpr_ptr.h>
#include <ConstexprCore/escape_string.h>
#include <ConstexprCore/fixed_string.h>
#include <ConstexprCore/type_name.h>
#include <ConstexprCore/utf_convert.h>

#endif // CONSTEXPRCORE_USEFUL_ABSTRACTIONS_H
