// escape_string: JSON, URL and HTML escaping done by the compiler.
#include <ConstexprCore/escape_string.h>

#include <cstdio>
#include <string>

using namespace ConstexprCore;

// Escaping a literal at compile time means the escaped bytes live in the
// binary's read-only data: nothing to compute, nothing to allocate.
constexpr auto json  = json_escape<"say \"hi\"\n">();
static_assert(json.view() == R"(say \"hi\"\n)");

constexpr auto quoted = json_quoted<"say \"hi\"">();    // adds the quotes too
static_assert(quoted.view() == R"("say \"hi\"")");

constexpr auto url = percent_encode<"Hello World!">();
static_assert(url.view() == "Hello%20World%21");

constexpr auto html = html_escape<"<b>&</b>">();
static_assert(html.view() == "&lt;b&gt;&amp;&lt;/b&gt;");

// Sizes are available without materialising the result.
static_assert(json_escape_size<"say \"hi\"\n">() == json.size());

// Composing at compile time: a URL with an encoded query.
constexpr auto query = "https://example.com/search?q="_fs + percent_encode<"c++ & you">();
static_assert(query.view() == "https://example.com/search?q=c%2b%2b%20%26%20you");

int main() {
    std::printf("json   : %s\n", std::string(json.view()).c_str());
    std::printf("quoted : %s\n", std::string(quoted.view()).c_str());
    std::printf("url    : %s\n", std::string(url.view()).c_str());
    std::printf("html   : %s\n", std::string(html.view()).c_str());
    std::printf("query  : %s\n", std::string(query.view()).c_str());
    return url.view() == "Hello%20World%21" ? 0 : 1;
}
