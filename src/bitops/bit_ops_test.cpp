/*
 * cpp98_nullptr_test.cpp
 *
 *  Created on: 14 okt. 2017
 *      Author: mikaelr
 */

#include <bitops.h>

#include <gtest/gtest.h>

using bitops::setBit;
using bitops::setBits;
using bitops::clearBit;
using bitops::clearBits;
using bitops::modifyBits;
using bitops::bitWidth;
using bitops::bitFieldMask;
using bitops::encodeBitField;
using bitops::decodeBitField;
// using bitops::encodeBitFieldTyped;
// using bitops::decodeBitFieldTyped;

TEST(bitops, setBit)
{
    uint32_t t = 0xf0f0f0;
    setBit<uint32_t, 0>(t);
    EXPECT_EQ(t, 0xf0f0f1u);
    setBit<uint32_t, 17>(t);
    EXPECT_EQ(t, 0xf2f0f1u);
}

TEST(bitops, clearBit)
{
    uint32_t t = 0xf0f0f0;
    clearBit<uint32_t, 5>(t);
    EXPECT_EQ(t, 0xf0f0d0u);
    clearBit<uint32_t, 20>(t);
    EXPECT_EQ(t, 0xe0f0d0u);
}

TEST(bitops, setBits)
{
    uint32_t t = 0xf0f0f0;
    setBits<uint32_t, 0xf0000ff>(t);
    EXPECT_EQ(t, 0xff0f0ffu);
}

TEST(bitops, clearBits)
{
    uint32_t t = 0xf0f0f0;
    clearBits<uint32_t, 0xf0000ff>(t);
    EXPECT_EQ(t, 0xf0f000u);
}

TEST(bitops, modifyBits)
{
    uint32_t t = 0xf0f0f0f0;
    modifyBits<uint32_t, 0xff0000ff, 0x0ff00ff0>(t);
    EXPECT_EQ(t, 0x0ff0fff0u);
}

TEST(bitops, bitWidth)
{
    EXPECT_EQ(bitWidth<uint16_t>(), 16);
    EXPECT_EQ(bitWidth<uint32_t>(), 32);
}

TEST(bitops, bitFieldMask)
{
    using myField = bitops::BitField<uint32_t, int, 4, 7>;
    uint32_t val = bitFieldMask<myField>();
    EXPECT_EQ(val, 0x7f0u);

    // Also compile time valid.
    static_assert(bitFieldMask<myField>() == 0x7f0u, "");
}

TEST(bitops, encodeBitField)
{
    using myField = bitops::BitField<uint32_t, int, 4, 7>;
    uint32_t val = encodeBitField<myField>(0x34u);
    EXPECT_EQ(val, 0x340u);
}

TEST(bitops, encodeBitFieldTyped)
{
    enum class TestEnum
    {
        testVal = 34
    };
    using myField = bitops::BitField<uint32_t, TestEnum, 4, 7>;

    uint32_t val = encodeBitField<myField>(TestEnum::testVal);
    EXPECT_EQ(val, 16u * 34u);
}

TEST(bitops, decodeBitField)
{
    using myField = bitops::BitField<uint32_t, int, 4, 7>;
    int val = decodeBitField<myField>(0x340u);
    EXPECT_EQ(val, 0x34);
}

TEST(bitops, decodeBitFieldTyped)
{
    enum class TestEnum
    {
        testVal = 34
    };
    using myField = bitops::BitField<uint32_t, TestEnum, 4, 7>;
    TestEnum val = bitops::decodeBitField<myField>(34u * 16u);
    EXPECT_EQ(val, TestEnum::testVal);
}

TEST(bitops, readWrite)
{
    enum class TestEnum
    {
        null = 0,
        testVal = 34
    };
    using myField = bitops::BitField<uint32_t, TestEnum, 4, 7>;
    uint32_t t = 0;
    bitops::write<myField>(t, TestEnum::testVal);

    EXPECT_EQ(t, 34 * 16u);

    EXPECT_EQ(bitops::read<myField>(t), TestEnum::testVal);
}

TEST(bitops, test_clear_set_functions_of_bitfield)
{
    enum class TestEnum
    {
        null = 0,
        testVal = 34
    };
    using myField = bitops::BitField<uint32_t, TestEnum, 4, 7>;
    using myField2 = bitops::BitField<uint32_t, TestEnum, 16, 6>;

    auto test = myField::value(TestEnum::testVal);

    EXPECT_EQ(test.toClear, 0x07f0u);
    EXPECT_EQ(test.toSet, 0x0220u);

    auto test2 = myField2::value<TestEnum::testVal>();

    EXPECT_EQ(test2.toClear, 0x01d0000u);
    EXPECT_EQ(test2.toSet, 0x0220000u);

    auto test3 = test % test2;

    EXPECT_EQ(test3.toClear, 0x001d07f0u);
    EXPECT_EQ(test3.toSet, 0x00220220u);

    uint32_t allSet = 0xffffffff;
    allSet %= test3;
    EXPECT_EQ(allSet, 0xFFE2FA2Fu);

    uint32_t allClear = 0x0;
    allClear %= test3;
    EXPECT_EQ(allClear, 0x00220220u);

    auto test4 = myField::set();
    EXPECT_EQ(test4.toClear, 0x0u);
    EXPECT_EQ(test4.toSet, 0x000007f0u);

    auto test5 = myField::clear();
    EXPECT_EQ(test5.toClear, 0x07f0u);
    EXPECT_EQ(test5.toSet, 0x0u);

    EXPECT_TRUE(true);
}

int
main(int argc, char** argv)
{
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
