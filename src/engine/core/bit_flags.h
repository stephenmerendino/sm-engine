#pragma once

#include "engine/core/types.h"

constexpr inline bool is_bit_set(u8 bits, u8 flag) { return (bits & flag); }
constexpr inline void set_bit(u8& bits, u8 flag) { bits |= flag; }
constexpr inline void unset_bit(u8& bits, u8 flag) { bits = bits & ~flag; }

constexpr inline bool is_bit_set(u16 bits, u16 flag) { return (bits & flag); }
constexpr inline void set_bit(u16& bits, u16 flag) { bits |= flag; }
constexpr inline void unset_bit(u16& bits, u16 flag) { bits = bits & ~flag; }

constexpr inline bool is_bit_set(u32 bits, u32 flag) { return (bits & flag); }
constexpr inline void set_bit(u32& bits, u32 flag) { bits |= flag; }
constexpr inline void unset_bit(u32& bits, u32 flag) { bits = bits & ~flag; }

constexpr inline bool is_bit_set(u64 bits, u64 flag) { return (bits & flag); }
constexpr inline void set_bit(u64& bits, u64 flag) { bits |= flag; }
constexpr inline void unset_bit(u64& bits, u64 flag) { bits = bits & ~flag; }
