// constexpr_check: turn a failed check inside consteval code into a clear
// compiler error, with your message.
#include <ConstexprCore/constexpr_check.h>
#include <ConstexprCore/fixed_string.h>

#include <cstdio>

using namespace ConstexprCore;

// A validated value type: constructing it with bad input does not compile.
consteval unsigned short port(unsigned value) {
    static_require(value != 0, "port 0 is reserved");
    static_require(value < 65536, "port must fit in 16 bits");
    return static_cast<unsigned short>(value);
}
constexpr auto https = port(443);
// constexpr auto nope = port(70000);   // error: "port must fit in 16 bits"

// Validating text, with the position of the problem reported back.
enum class ident_error { none, empty, bad_start, bad_char };

template <fixed_string Str>
consteval error_result<ident_error> check_identifier() {
    if (Str.size() == 0) return {ident_error::empty, 0};
    auto ok_start = [](char c) { return c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); };
    auto ok_rest  = [&](char c) { return ok_start(c) || (c >= '0' && c <= '9'); };
    if (!ok_start(Str[0])) return {ident_error::bad_start, 0};
    for (std::size_t i = 1; i < Str.size(); ++i)
        if (!ok_rest(Str[i])) return {ident_error::bad_char, i};
    return {ident_error::none, 0};
}

static_assert(check_identifier<"user_id">().ok());
constexpr auto bad = check_identifier<"user-id">();
static_assert(bad.error == ident_error::bad_char && bad.position == 4);

// require_ok turns that result into a compile error when it must be valid.
template <fixed_string Str>
consteval auto identifier() {
    require_ok(check_identifier<Str>(), "not a valid identifier");
    return Str;
}
constexpr auto name = identifier<"user_id">();
// constexpr auto oops = identifier<"user-id">();   // error: "not a valid identifier"

int main() {
    std::printf("port  : %u\n", https);
    std::printf("ident : %.*s\n", int(name.size()), name.view().data());   // not NUL-terminated
    std::printf("bad   : position %zu\n", bad.position);
    return https == 443 ? 0 : 1;
}
