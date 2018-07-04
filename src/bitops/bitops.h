#pragma once

#include <climits>
#include <cstdint>
#include <limits>

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

namespace details
{
// Given value and bitwidth size, return lowest set bit;
template <typename Storage>
constexpr int
lowestSetBit(Storage value, int size)
{
    if (size == 1)
        return value & 1 ? 0
                         : (std::numeric_limits<int>::max() -
                            bitWidth<Storage>() + 1);
    int halfSize = size / 2;
    int mask = (1 << halfSize) - 1;
    if (value & mask)
        return lowestSetBit(value, halfSize);
    else
        return halfSize + lowestSetBit(value >> halfSize, halfSize);
}

// Given value and bitwidth size, return lowest clear bit, which do not have a
// set bit above it.
template <typename Storage>
constexpr int
lowestClearBit(Storage value, int size)
{
    if (size == 1)
        return value & 1;

    int halfSize = size / 2;
    int mask = ~((1 << halfSize) - 1);
    if (value & mask)
        return halfSize + lowestClearBit(value >> halfSize, halfSize);
    else
        return lowestClearBit(value, halfSize);
}


}

/**
 * maskLowBit
 * Return the lowest bit position that contains a '1' in param mask.
 * If none is set, return INT_MAX;
 */
template <typename Storage>
constexpr int
maskLowBit(Storage mask)
{
    return details::lowestSetBit<Storage>(mask, bitWidth<Storage>());
}

template <typename Storage, Storage mask>
constexpr int
maskLowBit()
{
    return details::lowestSetBit<Storage>(mask, bitWidth<Storage>());
}

/**
 * maskEndBit.
 * Return one beyond highest set bit in the bitmask.
 */
template <typename Storage>
constexpr int
maskEndBit(Storage mask)
{
    return details::lowestClearBit<Storage>(mask, bitWidth<Storage>());
}

template <typename Storage, Storage mask>
constexpr int
maskEndBit()
{
    return details::lowestClearBit<Storage>(mask, bitWidth<Storage>());
}

/**
 * Return the width of a supplied bitmask. E.g. 0x38
 * return 3 bit width.
 */
template<typename Storage, Storage mask>
constexpr int
maskWidth()
{
    return maskEndBit<Storage, mask>() - maskLowBit<Storage, mask>();
};

template<typename Storage>
constexpr int
maskWidth(Storage mask)
{
    return maskEndBit(mask) - maskLowBit(mask);
};


/**
 * Set a bit in an integral type.
 *
 * @param Storage Integral type to set a bit in.
 * @param bitNo bit number to set to '1'.
 * @param val Reference to value to update.
 */
template <typename Storage, int bitNo>
constexpr void
setBit(Storage& val)
{
    static_assert(bitNo < bitWidth<Storage>(), "");
    val |= (1 << bitNo);
}

/**
 * Set a bit in an integral type.
 *
 * @param Storage Integral type to set a bit in.
 * @param val Reference to value to update.
 * @param bitNo bit number to set to '1'.
 */
template <typename Storage>
constexpr void
setBit(Storage& val, int bitNo)
{
    val |= (1 << bitNo);
}

/**
 * Clear a bit in an integral type.
 *
 * @param Storage Integral type to set a bit in.
 * @param bitNo bit number to set to '1'.
 * @param val Reference to value to update.
 */
template <typename Storage, int bitNo>
constexpr void
clearBit(Storage& val)
{
    static_assert(bitNo < bitWidth<Storage>(), "");
    val &= ~static_cast<Storage>(1 << bitNo);
}

/**
 * Clear a bit in an integral type.
 *
 * @param Storage Integral type to set a bit in.
 * @param val Reference to value to update.
 * @param bitNo bit number to set to '1'.
 */
template <typename Storage>
constexpr void
clearBit(Storage& val, int bitNo)
{
    val &= ~static_cast<Storage>(1 << bitNo);
}

/**
 * Set a number of bits in an integral type.
 * Bits set to '1' in setValue are set to '1' in val.
 *
 * @param Storage Integral type to set a bit in.
 * @param val Reference to value to update.
 * @param setValue Bitmask with bits to set.
 */
template <typename Storage, Storage setValue>
constexpr void
setBits(Storage& val)
{
    val |= setValue;
}

template <typename Storage, Storage setValue>
constexpr void
setBits(volatile Storage& val)
{
    if (setValue)
        val |= setValue;
}

/**
 * Set a number of bits in an integral type.
 * Bits set to '1' in setValue are set to '1' in val.
 *
 * @param Storage Integral type to set a bit in.
 * @param val Reference to value to update.
 * @param setValue Bitmask with bits to set.
 */
template <typename Storage>
constexpr void
setBits(Storage& val, const Storage& setValue)
{
    val |= setValue;
}

template <typename Storage>
constexpr void
setBits(volatile Storage& val, const Storage& setValue)
{
    val |= setValue;
}

/**
 * Clear a number of bits in an integral type.
 * Bits set to '1' in clearValue are cleared to '0' in val.
 *
 * @param Storage Integral type to set a bit in.
 * @param val Reference to value to update.
 * @param clearValue Bitmask with bits to set.
 */
template <typename Storage, Storage clearValue>
constexpr void
clearBits(Storage& val)
{
    val &= ~clearValue;
}

template <typename Storage, Storage clearValue>
constexpr void
clearBits(volatile Storage& val)
{
    if (clearValue)
        val &= ~clearValue;
}

/**
 * Clear a number of bits in an integral type.
 * Bits set to '1' in clearValue are cleared to '0' in val.
 *
 * @param Storage Integral type to set a bit in.
 * @param val Reference to value to update.
 * @param clearValue Bitmask with bits to set.
 */
template <typename Storage>
constexpr void
clearBits(Storage& val, Storage clearValue)
{
    val &= ~clearValue;
}

template <typename Storage>
constexpr void
clearBits(volatile Storage& val, Storage clearValue)
{
    val &= ~clearValue;
}

/**
 * Structure for collecting bit clear and set operations.
 * Bits set to one indicate it should be modified.
 * The clear will be applied before the set.
 */
template <class Storage>
struct WordUpdate
{
    WordUpdate() = default;
    WordUpdate(Storage clear, Storage set) : toClear(clear), toSet(set) {}

    /// Bits set to '1' are forced to 0 in the final write.
    Storage toClear = static_cast<Storage>(0);

    /// Bits set to '1' are forced to 1 in the final write.
    Storage toSet = static_cast<Storage>(0);

    /// Merge 2 BitModification structures into 1. Bit set have priority
    /// over bit cleared.
    static void apply(WordUpdate& lhs, const WordUpdate& rhs)
    {
        lhs.toClear |= rhs.toClear;
        lhs.toSet |= rhs.toSet;
    }

    // Create a new storage with a different width, preserving those
    // bits that remains. When casting to a smaller size, this is a
    // potentially destructive operation.
    template <class DestStore>
    WordUpdate<DestStore> resize_cast() const
    {
        DestStore dummy;
        return makeResize(*this, dummy);
    }

    // When applying the WordUpdate, set bit 'bitNo'
    WordUpdate& setBit(int bitNo)
    {
        bitops::setBit(toSet, bitNo);
        bitops::clearBit(toClear, bitNo);
        return *this;
    }

    // When applying the WordUpdate, clear bit 'bitNo'
    WordUpdate& clearBit(int bitNo)
    {
        bitops::clearBit(toSet, bitNo);
        bitops::setBit(toClear, bitNo);
        return *this;
    }

    // When applying the WordUpdate, Set all bits == 1 in bitMask, to 1.
    WordUpdate& setBits(const Storage& bitMask)
    {
        bitops::setBits(toSet, bitMask);
        bitops::clearBits(toClear, bitMask);
        return *this;
    }

    // When applying the WordUpdate, Clear, all bits == 1 in bitMask, to 0.
    WordUpdate& clearBits(const Storage& bitMask)
    {
        bitops::clearBits(toSet, bitMask);
        bitops::setBits(toClear, bitMask);
        return *this;
    }

    // Helper function to perform resize operation.
    template <class DestStore>
    static WordUpdate<DestStore> makeResize(const WordUpdate<Storage>& wuFrom,
                                            DestStore dummy)
    {
        WordUpdate<DestStore> wu;
        wu.toClear = static_cast<DestStore>(wuFrom.toClear);
        wu.toSet = static_cast<DestStore>(wuFrom.toSet);
        return wu;
    }
};

// Free function variant for WordUpdate rezise_cast.
// Need class/function object to fix type deduction.
// Need inheritance to let this class be part of operator resolution for
// operator %, %=.
template <class DestStore>
struct resize_cast : public WordUpdate<DestStore>
{
    template <typename Storage>
    explicit resize_cast(const WordUpdate<Storage>& wu)
        : WordUpdate<DestStore>(
              WordUpdate<Storage>::makeResize(wu, static_cast<DestStore>(0)))
    {
    }
};

/**
 * Apply a WordUpdate to a value of the relevant type.
 */
template <class Storage>
void
update(Storage& s, const WordUpdate<Storage>& wu)
{
    Storage t = s;
    clearBits<Storage>(t, wu.toClear);
    setBits<Storage>(t, wu.toSet);
    s = t;
}

/**
 * Apply a WordUpdate to a volatile reference the relevant type.
 */
template <class Storage>
void
update(volatile Storage& s, const WordUpdate<Storage>& wu)
{
    Storage t = s;
    clearBits<Storage>(t, wu.toClear);
    setBits<Storage>(t, wu.toSet);
    s = t;
}

/**
 * update a number of bits.
 * First perform clear operation then set operation.
 */
template <typename Storage, Storage clearValue, Storage setValue>
void
update(Storage& val)
{
    clearBits<Storage, clearValue>(val);
    setBits<Storage, setValue>(val);
}

template <typename Storage>
void
update(Storage& val, Storage clearValue, Storage setValue)
{
    clearBits<Storage>(val, clearValue);
    setBits<Storage>(val, setValue);
}

/**
 * Merge 2 WordUpdate structs, using the apply function.
 * Bit sets have priority over bit clear.
 */
template <class Storage>
WordUpdate<Storage>
operator%(const WordUpdate<Storage>& lhs, const WordUpdate<Storage>& rhs)
{
    WordUpdate<Storage> bm = lhs;
    WordUpdate<Storage>::apply(bm, rhs);
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
operator%=(Storage& lhs, const WordUpdate<Storage>& rhs)
{
    update<Storage>(lhs, rhs);
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
volatile Storage&
operator%=(volatile Storage& lhs, const WordUpdate<Storage>& rhs)
{
    update<Storage>(lhs, rhs);
    return lhs;
}

// Common case, clearing a fixed field and then setting run time
// Variable bits.
template <typename Storage, Storage clearValue>
void
updateBitsSetField(Storage& val, Storage setValue)
{
    clearBits<clearValue>(val);
    setBits(val, setValue);
}

/**
 * Represent a range of bits within a storage unit. Used for
 * specifying bits in a bit range.
 * @param unsigned integral type acting as backing store for the range.
 * @param lowBit_ index of the lowest bit position for the range.
 * @param width_ Number of bits in the range.
 */
template <class Storage_, int lowBit_, int width_>
struct Range
{
    using Storage = Storage_;
    enum
    {
        lowBit = lowBit_,
        endBit = lowBit_ + width_,
        width = width_,
        maxValue = 1ull << width - 1,
    };
    static_assert(endBit <= bitWidth<Storage>(), "");

    // Return a value suitable for masking the range in a store.
    static constexpr Storage mask()
    {
        return static_cast<Storage>(((1ull << width) - 1) << lowBit);
    }

    // Return value shifted into the proper range position.
    template <Storage value>
    static constexpr Storage value2Storage()
    {
        static_assert(value <= maxValue, "");
        return value << lowBit;
    }
    static constexpr Storage value2Storage(Storage value)
    {
        return value << lowBit;
    }
};

/**
 * Given an integral value that is a bitmask, calculate the appropriate range
 * type.
 */
template <typename Storage, Storage bitMask>
auto constexpr bitmask2Range()
    -> Range<Storage, maskLowBit<Storage, bitMask>(),
             maskWidth<Storage, bitMask>()>
{
    return {};
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
    using Rng = Range<Storage_, offset_, width_>;
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

    /// Return given value in 'bit modification form', suitable to be
    /// aggregated.
    static WordUpdate<Storage> value(FieldType t);

    /// Return given value in 'bit modification form', suitable to be
    /// aggregated.
    template <FieldType_ f>
    static WordUpdate<Storage> value()
    {
        const constexpr Storage sf = static_cast<Storage>(f);
        const constexpr Storage toSet = Rng::value2Storage(sf);
        const constexpr Storage toClear = Rng::mask() & ~toSet;
        return WordUpdate<Storage_>(toClear, toSet);
    }

    /// Return modification to set all bits in field.
    static WordUpdate<Storage> set();

    /// Return modification to clear all bits in field.
    static WordUpdate<Storage> clear();
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
    return BitField::Rng::mask();
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
    return BitField::Rng::mask() &
           ~BitField::Rng::value2Storage(static_cast<Storage>(value));
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
WordUpdate<Storage_>
BitField<Storage_, FieldType_, offset_, width_>::value(FieldType_ t)
{
    const Storage toClear = bitFieldMask<BitField>();
    const Storage toSet = encodeBitField<BitField>(t);
    return WordUpdate<Storage_>(toClear, toSet);
}

template <typename Storage_, typename FieldType_, int offset_, int width_>
WordUpdate<Storage_>
BitField<Storage_, FieldType_, offset_, width_>::set()
{
    return WordUpdate<Storage_>(0u, bitFieldMask<BitField>());
}

template <typename Storage_, typename FieldType_, int offset_, int width_>
WordUpdate<Storage_>
BitField<Storage_, FieldType_, offset_, width_>::clear()
{
    return WordUpdate<Storage_>(bitFieldMask<BitField>(), 0u);
}

template <typename Storage, uint32_t clearBits_, uint32_t setBits_>
struct WriteImplSpecializeSetClear
{
    static void write(Storage& s)
    {
        update<Storage, clearBits_, setBits_>(s);
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
        WriteImplSpecializeSetClear<
            Storage, bitFieldClearBits<BitField, value>(),
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

/**
 * Write a BitField value into a backing store.
 * @tparam BitField Type of the BitField.
 * @param s backing storage where value is written.
 * @param value Value to be written.
 */
template <typename BitField>
void
write(typename BitField::Storage& s, typename BitField::FieldType value)
{
    using Storage = typename BitField::Storage;
    update<Storage>(s, bitFieldMask<BitField>(),
                    encodeBitField<BitField>(value));
}

// Write a value to a bitfield.
template <typename Storage>
void
write(Storage& s, const WordUpdate<Storage>& bm)
{
    update<Storage>(s, bm.toClear, bm.toSet);
}

template <typename Storage>
void
write(volatile Storage& s, const WordUpdate<Storage>& bm)
{
    Storage t = s;
    update<Storage>(t, bm.toClear, bm.toSet);
    s = t;
}

template <typename Storage>
void
write(volatile uint16_t& s, const WordUpdate<Storage>& bm)
{
    // To avoid degenerate gcc code generation. Cast to 32 bit
    // will not generate unneeded instructions on ARM.
    auto ptr = reinterpret_cast<volatile uint32_t*>(&s);
    Storage t = *ptr;
    update<Storage>(t, bm.toClear, bm.toSet);
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
WordUpdate<typename BitField::Register::RegStorage>
value(typename BitField::Field::FieldType t)
{
    return BitField::Field::value(t);
}

template <typename BitField>
WordUpdate<typename BitField::Register::RegStorage>
set()
{
    return BitField::Field::set();
}

template <typename BitField>
WordUpdate<typename BitField::Register::RegStorage>
clear()
{
    return BitField::Field::clear();
}
} // namespace bitops
