#pragma once

#include <climits>
#include <cstdint>

/**
 * Bitops. Collects a number of operations to manipulate bit fields
 * within a larger storage type, typically an integral type.
 */

namespace bitops
{

/**
 * Return the number of bits in a given type,
 *
 * @param Storage Type to check number of bits for.
 * @return number of bits.
 */
template <typename Storage>
constexpr int
bitWidth()
{
    // Ruling out exotic platforms.
    static_assert(CHAR_BIT == 8, "");
    return 8 * sizeof(Storage);
}

/**
 * Set a bit in an integral type.
 *
 * @param Storage Integral type to set a bit in.
 * @param bitNo bit number to set to '1'.
 * @param val Reference to value to modify.
 */
template <typename Storage, int bitNo>
void
setBit(Storage& val)
{
    static_assert(bitNo < bitWidth<Storage>(), "");
    val |= (1 << bitNo);
}

/**
 * Set a bit in an integral type.
 *
 * @param Storage Integral type to set a bit in.
 * @param val Reference to value to modify.
 * @param bitNo bit number to set to '1'.
 */
template <typename Storage>
void
setBit(Storage& val, int bitNo)
{
    val |= (1 << bitNo);
}

/**
 * Clear a bit in an integral type.
 *
 * @param Storage Integral type to set a bit in.
 * @param bitNo bit number to set to '1'.
 * @param val Reference to value to modify.
 */
template <typename Storage, int bitNo>
void
clearBit(Storage& val)
{
    static_assert(bitNo < bitWidth<Storage>(), "");
    val &= ~static_cast<Storage>(1 << bitNo);
}

/**
 * Clear a bit in an integral type.
 *
 * @param Storage Integral type to set a bit in.
 * @param val Reference to value to modify.
 * @param bitNo bit number to set to '1'.
 */
template <typename Storage>
void
clearBit(Storage& val, int bitNo)
{
    val &= ~static_cast<Storage>(1 << bitNo);
}

/**
 * Set a number of bits in an integral type.
 * Bits set to '1' in setValue are set to '1' in val.
 *
 * @param Storage Integral type to set a bit in.
 * @param val Reference to value to modify.
 * @param setValue Bitmask with bits to set.
 */
template <typename Storage, Storage setValue>
void
setBits(Storage& val)
{
    val |= setValue;
}

/**
 * Set a number of bits in an integral type.
 * Bits set to '1' in setValue are set to '1' in val.
 *
 * @param Storage Integral type to set a bit in.
 * @param val Reference to value to modify.
 * @param setValue Bitmask with bits to set.
 */
template <typename Storage>
void
setBits(Storage& val, Storage setValue)
{
    val |= setValue;
}

/**
 * Clear a number of bits in an integral type.
 * Bits set to '1' in setValue are cleared to '0' in val.
 *
 * @param Storage Integral type to set a bit in.
 * @param val Reference to value to modify.
 * @param setValue Bitmask with bits to set.
 */
template <typename Storage, Storage clearValue>
void
clearBits(Storage& val)
{
    val &= ~clearValue;
}


/**
 * Clear a number of bits in an integral type.
 * Bits set to '1' in setValue are cleared to '0' in val.
 *
 * @param Storage Integral type to set a bit in.
 * @param val Reference to value to modify.
 * @param setValue Bitmask with bits to set.
 */
template <typename Storage>
void
clearBits(Storage& val, Storage clearValue)
{
    val &= ~clearValue;
}

/**
 * Structure for collecting bit clear and set operations.
 * Bits set to one indicate it should be modified.
 * The clear will be applied before the set.
 */
template <class Storage>
struct BitModifications
{
    BitModifications() = default;
    BitModifications(Storage clear, Storage set) : toClear(clear), toSet(set)
    {
    }

    /// Bits set to '1' are forced to 0 in the final write.
    Storage toClear = static_cast<Storage>(0);

    /// Bits set to '1' are forced to 1 in the final write.
    Storage toSet = static_cast<Storage>(0);

    /// Merge 2 BitModification structures into 1. Bit set have priority
    /// over bit cleared.
    static void apply(BitModifications& lhs, const BitModifications& rhs)
    {
        lhs.toClear |= rhs.toClear;
        lhs.toSet |= rhs.toSet;
    }
};

/**
 * Merge 2 BitModifications structs, using the apply function.
 * Bit sets have priority over bit clear.
 */
template <class Storage>
BitModifications<Storage>
operator%(const BitModifications<Storage>& lhs, const BitModifications<Storage>& rhs)
{
    BitModifications<Storage> bm = lhs;
    BitModifications<Storage>::apply(bm, rhs);
    return bm;
}

/**
 * Apply a BitModification struct to an integral type. Perform first
 * the clear bits and then the set bits.
 *
 * @param Storage  Type of the integral to perform operations on.
 * @lhs Storage value to apply the bit modifications to.
 * @rhs Describe which bits to clear and set.
 */
template <class Storage>
Storage&
operator%=(Storage& lhs, const BitModifications<Storage>& rhs)
{
    clearBits<Storage>(lhs, rhs.toClear);
    setBits<Storage>(lhs, rhs.toSet);
    return lhs;
}

/**
 * Apply a BitModification struct to a volatile integral type. Perform first
 * the clear bits and then the set bits.
 *
 * @param Storage  Type of the integral to perform operations on.
 * @lhs Storage value to apply the bit modifications to.
 * @rhs Describe which bits to clear and set.
 */
template <class Storage>
Storage&
operator%=(volatile Storage& lhs, const BitModifications<Storage>& rhs)
{
	Storage t = lhs;
    clearBits<Storage>(t, rhs.toClear);
    setBits<Storage>(t, rhs.toSet);
    lhs = t;
    return lhs;
}

/**
 * Modify a number of bits.
 * First perform clear operation then set operation.
 */
template <typename Storage, Storage clearValue, Storage setValue>
void
modifyBits(Storage& val)
{
    clearBits<Storage, clearValue>(val);
    setBits<Storage, setValue>(val);
}

template <typename Storage>
void
modifyBits(Storage& val, Storage clearValue, Storage setValue)
{
    clearBits<Storage>(val, clearValue);
    setBits<Storage>(val, setValue);
}

// Common case, clearing a fixed field and then setting run time
// Variable bits.
template <typename Storage, Storage clearValue>
void
modifyBitsSetField(Storage& val, Storage setValue)
{
    clearBits<clearValue>(val);
    setBits(val, setValue);
}

/**
 * Type BitField:
 *
 * Contain information about a sub-field within a larger bit storage
 * type. Typical case a few bits within a larger word.
 * It has support for keeping track of a user defined type for the
 * bitfield.
 *
 * @param Storage_ The integral type for the storage.
 * @param FieldType_ Type than represent the bitfield.
 * @param offset_  The bit offset into the storage where the field begins.
 * @param width_ The bit width of the field.
 *
 * Member: offset : Bit number for the lowest bit.
 *         width  : Number of bits in the value.
 *         FieldType : Type of the bitfield.
 */
template <typename Storage_, typename FieldType_, int offset_, int width_>
class BitField
{
  public:
    enum
    {
        offset = offset_,
        width = width_,
        storageWidth = bitWidth<Storage_>(),
    };
    static_assert(storageWidth >= offset + width, "");

    using FieldType = FieldType_;
    using Storage = Storage_;
    using BitFieldType = BitField<Storage_, FieldType_, offset_, width_>;

    /// Return given value in 'bit modification form', suitable to be aggregated.
    static BitModifications<Storage> value(FieldType t);

    /// Return given value in 'bit modification form', suitable to be aggregated.
    template <FieldType_ f>
    static BitModifications<Storage> value()
    {
        const Storage toSet = static_cast<Storage>(static_cast<int>(f) << offset);
        const Storage toClear = static_cast<Storage>(((1 << width) - 1) << offset) & ~toSet;

        return BitModifications<Storage_>(toClear, toSet);
    }

    /// Return modification to set all bits in field.
    static BitModifications<Storage> set();

    /// Return modification to clear all bits in field.
    static BitModifications<Storage> clear();
};

/**
 * Return a mask with all the bits set corresponding to the bit field.
 *
 * @param BitField The BitField description class.
 * @return A value in type Storage with the mask for the BitField.
 */
template <typename BitField>
constexpr typename BitField::Storage
bitFieldMask()
{
    using Storage = typename BitField::Storage;
    const int offset = BitField::offset;
    const int width = BitField::width;
    static_assert(bitWidth<Storage>() >= offset + width, "");
    return static_cast<Storage>(((1 << width) - 1) << offset);
}

/**
 * Return a mask with all the bits set to '1' where bits needs to
 * be cleared to get the correct value.
 *
 * @param BitField The BitField description class.
 * @param value. The user value from the bitfield.
 * @return A value in type Storage with the cleared mask mask.
 */
template <typename BitField, typename BitField::FieldType value>
constexpr typename BitField::Storage
bitFieldClearBits()
{
    using Storage = typename BitField::Storage;
    const int offset = BitField::offset;
    const int width = BitField::width;
    static_assert(bitWidth<Storage>() >= offset + width, "");
    Storage t = ~value & ((1u << width) - 1u);
    return static_cast<Storage>(t << offset);
}

/**
 * Return the bit field value with the correct type from the
 * storage.
 *
 * @param BitField The BitField description class.
 * @param bits storage value to read out bitfield from.
 * @return Field value read from storage.
 */
template <typename BitField>
typename BitField::FieldType
decodeBitField(typename BitField::Storage bits)
{
    using Storage = typename BitField::Storage;
    using FieldType = typename BitField::FieldType;
    const int offset = BitField::offset;
    const int width = BitField::width;
    static_assert(bitWidth<Storage>() >= offset + width, "");
    return static_cast<FieldType>((bits >> offset) & ((1u << width) - 1u));
}

/**
 * Return a value suitable to be OR:ed into the backing store.
 *
 * @param BitField The BitField description class.
 * @param value suitable for a bit field.
 * @return Field value read from storage.
 */
template <typename BitField>
typename BitField::Storage
encodeBitField(typename BitField::FieldType value)
{
    int val = static_cast<int>(value);
    using Storage = typename BitField::Storage;
    const int offset = BitField::offset;
    const int width = BitField::width;
    static_assert(bitWidth<Storage>() >= offset + width, "");
    return static_cast<Storage>(val << offset);
}

/**
 * Return a value suitable to be OR:ed into the backing store.
 *
 * @param BitField The BitField description class.
 * @param value suitable for a bit field.
 * @return Field value read from storage.
 */
template <typename BitField, typename BitField::FieldType value>
constexpr typename BitField::Storage
encodeBitField()
{
    using Storage = typename BitField::Storage;
    const int offset = BitField::offset;
    const int width = BitField::width;
    static_assert(bitWidth<Storage>() >= offset + width, "");
    return static_cast<Storage>(value << offset);
}

template <typename Storage_, typename FieldType_, int offset_, int width_>
BitModifications<Storage_>
BitField<Storage_, FieldType_, offset_, width_>::value(FieldType_ t)
{
    const Storage toClear = bitFieldMask<BitField>();
    const Storage toSet = encodeBitField<BitField>(t);
    return BitModifications<Storage_>(toClear, toSet);
}

template <typename Storage_, typename FieldType_, int offset_, int width_>
BitModifications<Storage_>
BitField<Storage_, FieldType_, offset_, width_>::set()
{
    return BitModifications<Storage_>(0u, bitFieldMask<BitField>());
}

template <typename Storage_, typename FieldType_, int offset_, int width_>
BitModifications<Storage_>
BitField<Storage_, FieldType_, offset_, width_>::clear()
{
    return BitModifications<Storage_>(bitFieldMask<BitField>(), 0u);
}

template <typename Storage, uint32_t clearBits_, uint32_t setBits_>
struct WriteImplSpecializeSetClear
{
    static void write(Storage& s)
    {
        modifyBits<Storage, clearBits_, setBits_>(s);
    }
};

template <typename Storage, uint32_t clearBits_>
struct WriteImplSpecializeSetClear<Storage, clearBits_, 0>
{
    static void write(Storage& s)
    {
        clearBits<Storage, clearBits_>(s);
    }
};

template <typename Storage, uint32_t setBits_>
struct WriteImplSpecializeSetClear<Storage, 0, setBits_>
{
    static void write(Storage& s)
    {
        setBits<Storage, setBits_>(s);
    }
};

// Base case.
template <typename BitField, int value, int width>
struct WriteImplSpecialize1Bit
{
    static void write(typename BitField::Storage& s)
    {
        using Storage = typename BitField::Storage;
        WriteImplSpecializeSetClear<Storage, bitFieldClearBits<BitField, value>(),
                                    encodeBitField<BitField, value>()>::write(s);
    }
};

template <typename BitField>
struct WriteImplSpecialize1Bit<BitField, 1, 1>
{
    static void write(typename BitField::Storage& s)
    {
        using Storage = typename BitField::Storage;
        setBit<Storage, BitField::offset>(s);
    }
};

template <typename BitField>
struct WriteImplSpecialize1Bit<BitField, 0, 1>
{
    static void write(typename BitField::Storage& s)
    {
        using Storage = typename BitField::Storage;
        clearBit<Storage, BitField::offset>(s);
    }
};

template <typename BitField, typename BitField::FieldType value>
void
write(typename BitField::Storage& s)
{
    WriteImplSpecialize1Bit<BitField, value, BitField::width>::write(s);
}

// Write a value to a bitfield.
template <typename BitField>
void
write(typename BitField::Storage& s, typename BitField::FieldType value)
{
    using Storage = typename BitField::Storage;
    modifyBits<Storage>(s, bitFieldMask<BitField>(), encodeBitField<BitField>(value));
}

// Write a value to a bitfield.
template <typename Storage>
void
write(Storage& s, const BitModifications<Storage>& bm)
{
    modifyBits<Storage>(s, bm.toClear, bm.toSet);
}

template <typename Storage>
void
write(volatile Storage& s, const BitModifications<Storage>& bm)
{
    Storage t = s;
    modifyBits<Storage>(t, bm.toClear, bm.toSet);
    s = t;
}

template <typename Storage>
void
write(volatile uint16_t& s, const BitModifications<Storage>& bm)
{
    auto ptr = reinterpret_cast<volatile uint32_t*>(&s);
    Storage t = *ptr;
    modifyBits<Storage>(t, bm.toClear, bm.toSet);
    *ptr = t;
}

template <typename BitField>
typename BitField::FieldType
read(typename BitField::Storage bits)
{
    using Storage = typename BitField::Storage;
    using FieldType = typename BitField::FieldType;
    const int offset = BitField::offset;
    const int width = BitField::width;
    static_assert(bitWidth<Storage>() >= offset + width, "");
    return static_cast<FieldType>((bits >> offset) & ((1u << width) - 1u));
}

template <typename BitField>
BitModifications<typename BitField::Register::RegStorage>
value(typename BitField::Field::FieldType t)
{
    return BitField::Field::value(t);
}

template <typename BitField>
BitModifications<typename BitField::Register::RegStorage>
set()
{
    return BitField::Field::set();
}

template <typename BitField>
BitModifications<typename BitField::Register::RegStorage>
clear()
{
    return BitField::Field::clear();
}
}
