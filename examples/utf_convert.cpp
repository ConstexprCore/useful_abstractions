// utf_convert: validate UTF-8 and convert it to UTF-16 / UTF-32 at compile time.
#include <ConstexprCore/utf_convert.h>

#include <cstdio>

using namespace ConstexprCore;

// "€" is E2 82 AC in UTF-8. Written as bytes so the source file's own
// encoding does not matter.
constexpr auto euro = validate_utf8<"Price: \xE2\x82\xAC">();
static_assert(euro.ok());

// Invalid input is reported with a reason and a byte position, not a bool.
constexpr auto bad = validate_utf8<"ok\xFF">();
static_assert(!bad.ok());
static_assert(bad.error == utf8_error::invalid_lead_byte);
static_assert(bad.position == 2);

constexpr auto cut = validate_utf8<"\xE2\x82">();          // sequence cut short
static_assert(cut.error == utf8_error::truncated_sequence);

// Conversion to UTF-16 and UTF-32. The result is a fixed_string of the
// target character type, sized exactly.
constexpr auto utf16 = utf8_to_utf16<"Price: \xE2\x82\xAC">();
static_assert(utf16.view() == u"Price: €");
static_assert(utf16.size() == 8);

constexpr auto utf32 = utf8_to_utf32<"\xE2\x82\xAC">();
static_assert(utf32.size() == 1 && utf32[0] == U'€');

// Counting code points rather than bytes.
static_assert(utf8_code_point_count<"Price: \xE2\x82\xAC">() == 8);
static_assert(sizeof("Price: \xE2\x82\xAC") - 1 == 10);       // bytes

// The _checked variants refuse to compile on invalid input:
//   constexpr auto oops = utf8_to_utf16_checked<"\xFF">();   // error

int main() {
    std::printf("valid           : %s\n", euro.ok() ? "yes" : "no");
    std::printf("bad at position : %zu\n", bad.position);
    std::printf("utf16 units     : %zu\n", utf16.size());
    std::printf("code points     : %zu\n", utf8_code_point_count<"Price: \xE2\x82\xAC">());
    return euro.ok() && !bad.ok() ? 0 : 1;
}
