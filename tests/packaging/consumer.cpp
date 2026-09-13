// Consumed the CMake way: the include path is the same whether
// useful_abstractions came from add_subdirectory/FetchContent or from an
// installed package.
#include <ConstexprCore/useful_abstractions.h>

#include <cstdio>
#include <string>

using namespace ConstexprCore;

template <fixed_string Name>
struct tagged { static constexpr std::string_view tag = Name.view(); };

static_assert(tagged<"widget">::tag == "widget");
static_assert(type_name<double>() == "double");
static_assert(fnv1a("a") != fnv1a("b"));

int main() {
    if (tagged<"widget">::tag != "widget") return 1;
    std::printf("ok: useful_abstractions %s, type_name<double> = %s\n",
                useful_abstractions_version, std::string(type_name<double>()).c_str());
    return 0;
}
