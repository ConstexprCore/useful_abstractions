// constexpr_ptr: view memory as another type where reinterpret_cast cannot go.
#include <ConstexprCore/constexpr_ptr.h>

#include <cstdint>
#include <cstdio>

using namespace ConstexprCore;

// reinterpret_cast is not allowed during constant evaluation. reinterpret_ptr
// gives the same "read these chars as bytes" view, legally, via bit_cast on
// each element.
constexpr std::uint8_t checksum(const char* data, std::size_t n) {
    reinterpret_ptr<std::uint8_t, char> bytes{data};
    std::uint8_t sum = 0;
    for (std::size_t i = 0; i < n; ++i) sum = static_cast<std::uint8_t>(sum + bytes[i]);
    return sum;
}

constexpr char payload[] = {'\x01', '\x02', '\xFF'};
static_assert(checksum(payload, 3) == 2);                  // 1 + 2 + 255, mod 256
static_assert(*reinterpret_ptr<std::uint8_t, char>{payload + 2} == 255);

// It is a random-access iterator, so it works with algorithms and ranges.
constexpr bool all_nonzero() {
    auto first = make_reinterpret_ptr<std::uint8_t>(payload);
    for (auto it = first; it != first + 3; ++it) if (*it == 0) return false;
    return true;
}
static_assert(all_nonzero());

// The writable counterpart, for building byte buffers in constexpr code.
constexpr std::uint32_t little_endian_word() {
    char buffer[4] = {};
    writable_ptr<std::uint8_t, char> out{buffer};
    out[0] = 0x78; out[1] = 0x56; out[2] = 0x34; out[3] = 0x12;
    std::uint32_t word = 0;
    reinterpret_ptr<std::uint8_t, char> in{buffer};
    for (int i = 3; i >= 0; --i) word = (word << 8) | in[i];
    return word;
}
static_assert(little_endian_word() == 0x12345678);

int main() {
    std::printf("checksum : %u\n", checksum(payload, 3));
    std::printf("word     : 0x%08x\n", little_endian_word());
    return checksum(payload, 3) == 2 ? 0 : 1;
}
