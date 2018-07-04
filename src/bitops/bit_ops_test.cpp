/*
 * cpp98_nullptr_test.cpp
 *
 *  Created on: 14 okt. 2017
 *      Author: mikaelr
 */

#include <bitops.h>

#include <gtest/gtest.h>

using bitops::WordUpdate;
using bitops::WordUpdate;
using bitops::bitFieldMask;
using bitops::bitWidth;
using bitops::clearBit;
using bitops::clearBits;
using bitops::decodeBitField;
using bitops::encodeBitField;
using bitops::maskEndBit;
using bitops::maskLowBit;
using bitops::resize_cast;
using bitops::setBit;
using bitops::setBits;
using bitops::updateBits;
// using bitops::encodeBitFieldTyped;
// using bitops::decodeBitFieldTyped;

TEST(bitops, mask2bitNo)
{
    EXPECT_EQ(maskLowBit(0x10), 4);
    EXPECT_EQ(maskLowBit(uint8_t(0x80)), 7);
    EXPECT_EQ(maskLowBit(uint64_t(1ll << 63)), 63);
    EXPECT_EQ(maskLowBit(0), INT_MAX);

    EXPECT_EQ(maskEndBit(0x10), 5);
    EXPECT_EQ(maskEndBit(0), 0);
    EXPECT_EQ(maskEndBit(uint8_t(0x80)), 8);
    EXPECT_EQ(maskEndBit(uint64_t(1ll << 63)), 64);
    EXPECT_EQ(maskEndBit(0), 0);
    EXPECT_EQ(maskEndBit(0xff0), 12);
}

TEST(bitops, setBit1)
{
    uint32_t t = 0xf0f0f0;

    setBit<uint32_t, 0>(t);
    EXPECT_EQ(t, 0xf0f0f1u);
    setBit<uint32_t, 17>(t);
    EXPECT_EQ(t, 0xf2f0f1u);
}

TEST(bitops, setBit2)
{
    uint32_t t = 0xf0f0f0;

    setBit(t, 0);
    EXPECT_EQ(t, 0xf0f0f1u);
    setBit(t, 17);
    EXPECT_EQ(t, 0xf2f0f1u);
}

TEST(bitops, clearBit1)
{
    uint32_t t = 0xf0f0f0;
    clearBit<uint32_t, 5>(t);
    EXPECT_EQ(t, 0xf0f0d0u);
    clearBit<uint32_t, 20>(t);
    EXPECT_EQ(t, 0xe0f0d0u);
}

TEST(bitops, clearBit2)
{
    uint32_t t = 0xf0f0f0;
    clearBit(t, 5);
    EXPECT_EQ(t, 0xf0f0d0u);
    clearBit(t, 20);
    EXPECT_EQ(t, 0xe0f0d0u);
}

TEST(bitops, setBits)
{
    uint32_t t = 0xf0f0f0;
    setBits<uint32_t, 0xf0000ff>(t);
    EXPECT_EQ(t, 0xff0f0ffu);

    uint32_t t2 = 0xf0f0f0;
    setBits(t2, 0xf0000ffu);
    EXPECT_EQ(t2, 0xff0f0ffu);
}

TEST(bitops, clearBits)
{
    uint32_t t = 0xf0f0f0;
    clearBits<uint32_t, 0xf0000ff>(t);
    EXPECT_EQ(t, 0xf0f000u);

    uint32_t t2 = 0xf0f0f0;
    clearBits(t2, 0xf0000ffu);
    EXPECT_EQ(t2, 0xf0f000u);
}

TEST(bitops, updateBits)
{
    uint32_t t = 0xf0f0f0f0;
    updateBits<uint32_t, 0xff0000ff, 0x0ff00ff0>(t);
    EXPECT_EQ(t, 0x0ff0fff0u);

    uint32_t t2 = 0xf0f0f0f0;
    updateBits(t2, 0xff0000ffu, 0x0ff00ff0u);
    EXPECT_EQ(t2, 0x0ff0fff0u);
}

TEST(bitops, bitWidth)
{
    EXPECT_EQ(bitWidth<uint16_t>(), 16);
    EXPECT_EQ(bitWidth<uint32_t>(), 32);
}

TEST(bitops, WordUpdate)
{
    auto wu = WordUpdate<uint32_t>();
    EXPECT_EQ(wu.toClear, 0);
    EXPECT_EQ(wu.toSet, 0);

    uint32_t val = 0x5555aaaa;
    update(val, wu);
    EXPECT_EQ(val, 0x5555aaaa);

    wu.setBit(31);
    wu.setBit(0);
    wu.clearBit(30);
    wu.clearBit(1);

    update(val, wu);
    EXPECT_EQ(val, 0x9555aaa9);
}

TEST(bitops, WordUpdate2)
{
    auto wu = WordUpdate<uint32_t>().setBits(0xff000000).clearBits(0xff);

    uint32_t val = 0x5555aaaa;
    update(val, wu);
    EXPECT_EQ(val, 0xff55aa00);

    wu.setBits(0xf).clearBits(0xf00);
    uint32_t val2 = 0x5555aaaa;
    update(val2, wu);
    EXPECT_EQ(val2, 0xff55a00f);

    auto wu2 = WordUpdate<uint8_t>().setBit(1).clearBit(4);
    uint8_t val3 = '\x55';
    update(val3, wu2);
    EXPECT_EQ(val3, 0x47);
}

TEST(bitops, WordUpdate_operators)
{
    WordUpdate<uint32_t> wu;
    EXPECT_EQ(wu.toClear, 0);
    EXPECT_EQ(wu.toSet, 0);

    uint32_t val = 0x5555aaaa;
    val %= wu;
    EXPECT_EQ(val, 0x5555aaaa);

    wu.setBit(31);
    wu.clearBit(30);

    WordUpdate<uint32_t> wu2;
    wu2.setBit(0);
    wu2.clearBit(1);

    val %= wu % wu2;

    EXPECT_EQ(val, 0x9555aaa9);

    uint16_t val2 = 0x4466;
    val2 %= wu2.resize_cast<uint16_t>();
    EXPECT_EQ(val2, 0x4465);

    uint16_t val3 = 0x4466;
    val3 %= resize_cast<uint16_t>(wu2);
    EXPECT_EQ(val3, 0x4465);
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

/**
 * Helper array to set up constexpr arrays.
 */
template <typename T, std::size_t N>
class array_result
{
    constexpr static std::size_t size_ = N;
    T data_[N]{};

  public:
    constexpr std::size_t size() const
    {
        return N;
    }
    constexpr T& operator[](std::size_t n)
    {
        return data_[n];
    }
    constexpr const T& operator[](std::size_t n) const
    {
        return data_[n];
    }
    using iterator = T*;
    constexpr iterator begin()
    {
        return &data_[0];
    }
    constexpr iterator end()
    {
        return &data_[N];
    }
};

#define STM32F42_43xxx 1
#include <stm32f4xx.h>

// Our devices in the MCU.
enum Dev_e
{
    rcc,
    gpioa,
    gpiob,
    gpioc,
    gpiod,
    gpioe,
    gpiof,
    gpiog,
    gpioh,
    gpioi,
    gpioj,
    gpiok,

    usart1,
    usart2,
    spi1,
    spi2,
    dev_endIndex
};

// Registers in the RCC
enum RccChangesetIndex
{
    ci_ahb1,
    ci_ahb2,
    ci_ahb3,
    ci_apb1,
    ci_apb2,
    ci_endIndex,
};

// Set of changes to apply to the RCC regs.
struct RccChangeSet
{
    constexpr RccChangeSet() {}
    uint32_t vals[ci_endIndex] = {};
};

struct DeviceEntry
{
    constexpr DeviceEntry() : reg(ci_endIndex), bitMask(0) {}
    constexpr DeviceEntry(Dev_e dev, RccChangesetIndex reg, uint32_t bitMask)
        : reg(reg), bitMask(bitMask)
    {
    }

    RccChangesetIndex reg; // Which register is this device connected to.
    uint32_t bitMask;      // Which bit control the device.
};

static constexpr auto
calcDevTable() -> array_result<DeviceEntry, dev_endIndex>
{
    array_result<DeviceEntry, dev_endIndex> res;

    res[rcc] = DeviceEntry(rcc, ci_endIndex, 0);
    res[gpioa] = DeviceEntry(gpioa, ci_ahb1, RCC_AHB1ENR_GPIOAEN);
    res[gpiob] = DeviceEntry(gpiob, ci_ahb1, RCC_AHB1ENR_GPIOBEN);
    res[gpioc] = DeviceEntry(gpioc, ci_ahb1, RCC_AHB1ENR_GPIOCEN);
    res[gpiod] = DeviceEntry(gpiod, ci_ahb1, RCC_AHB1ENR_GPIODEN);
    res[gpioe] = DeviceEntry(gpioe, ci_ahb1, RCC_AHB1ENR_GPIOEEN);
    res[gpiof] = DeviceEntry(gpiof, ci_ahb1, RCC_AHB1ENR_GPIOFEN);
    res[gpiog] = DeviceEntry(gpiog, ci_ahb1, RCC_AHB1ENR_GPIOGEN);
    res[gpioh] = DeviceEntry(gpioh, ci_ahb1, RCC_AHB1ENR_GPIOHEN);
    res[gpioi] = DeviceEntry(gpioi, ci_ahb1, RCC_AHB1ENR_GPIOIEN);
    res[gpioj] = DeviceEntry(gpioj, ci_ahb1, RCC_AHB1ENR_GPIOJEN);
    res[gpiok] = DeviceEntry(gpiok, ci_ahb1, RCC_AHB1ENR_GPIOKEN);

    res[usart1] = DeviceEntry(usart1, ci_apb2, RCC_APB2ENR_USART1EN);
    res[usart2] = DeviceEntry(usart2, ci_apb1, RCC_APB1ENR_USART2EN);
    res[spi1] = DeviceEntry(spi1, ci_apb2, RCC_APB2ENR_SPI1EN);
    res[spi2] = DeviceEntry(spi2, ci_apb1, RCC_APB1ENR_SPI2EN);

#if 0
#define RCC_AHB1ENR_CRCEN ((uint32_t)0x00001000)
#define RCC_AHB1ENR_BKPSRAMEN ((uint32_t)0x00040000)
#define RCC_AHB1ENR_CCMDATARAMEN ((uint32_t)0x00100000)
#define RCC_AHB1ENR_DMA1EN ((uint32_t)0x00200000)
#define RCC_AHB1ENR_DMA2EN ((uint32_t)0x00400000)
#define RCC_AHB1ENR_DMA2DEN ((uint32_t)0x00800000)
#define RCC_AHB1ENR_ETHMACEN ((uint32_t)0x02000000)
#define RCC_AHB1ENR_ETHMACTXEN ((uint32_t)0x04000000)
#define RCC_AHB1ENR_ETHMACRXEN ((uint32_t)0x08000000)
#define RCC_AHB1ENR_ETHMACPTPEN ((uint32_t)0x10000000)
#define RCC_AHB1ENR_OTGHSEN ((uint32_t)0x20000000)
#define RCC_AHB1ENR_OTGHSULPIEN ((uint32_t)0x40000000)
#endif

    return res;
}

static const constexpr array_result<DeviceEntry, dev_endIndex> devTable =
    calcDevTable();

// Given list of devices, calculate RCC register modification masks.
template <Dev_e... devices>
constexpr auto
rccCalcRccMask() -> RccChangeSet
{
    RccChangeSet res;
    Dev_e devs[] = {devices...};
    for (auto i : devs)
    {
        const DeviceEntry& e = devTable[i];
        if (e.reg == ci_endIndex)
            throw 1; // Will fail at compile time if device lack rcc register.
        res.vals[e.reg] |= e.bitMask;
    }
    return res;
}

template <Dev_e... devices>
void
rccEnableClock(RCC_TypeDef* rcc)
{
    constexpr RccChangeSet cset = rccCalcRccMask<devices...>();

    setBits<uint32_t, cset.vals[ci_ahb1]>(rcc->AHB1ENR);
    setBits<uint32_t, cset.vals[ci_ahb2]>(rcc->AHB2ENR);
    setBits<uint32_t, cset.vals[ci_ahb3]>(rcc->AHB3ENR);
    setBits<uint32_t, cset.vals[ci_apb1]>(rcc->APB1ENR);
    setBits<uint32_t, cset.vals[ci_apb2]>(rcc->APB2ENR);
}

template <Dev_e... devices>
void
rccReset(RCC_TypeDef* rcc)
{
    constexpr RccChangeSet cset = rccCalcRccMask<devices...>();

    setBits<uint32_t, cset.vals[ci_ahb1]>(rcc->AHB1RSTR);
    clearBits<uint32_t, cset.vals[ci_ahb1]>(rcc->AHB1RSTR);

    setBits<uint32_t, cset.vals[ci_ahb2]>(rcc->AHB2RSTR);
    clearBits<uint32_t, cset.vals[ci_ahb2]>(rcc->AHB2RSTR);

    setBits<uint32_t, cset.vals[ci_ahb3]>(rcc->AHB3RSTR);
    clearBits<uint32_t, cset.vals[ci_ahb3]>(rcc->AHB3RSTR);

    setBits<uint32_t, cset.vals[ci_apb1]>(rcc->APB1RSTR);
    clearBits<uint32_t, cset.vals[ci_apb1]>(rcc->APB1RSTR);

    setBits<uint32_t, cset.vals[ci_apb2]>(rcc->APB2RSTR);
    clearBits<uint32_t, cset.vals[ci_apb2]>(rcc->APB2RSTR);
}

TEST(bitops, demo_rcc_expansion)
{
    RCC_TypeDef rccS = {};
    RCC_TypeDef* rcc = &rccS;

    rccEnableClock<gpioa, gpioc, gpiof, usart1, usart2, spi1>(rcc);

    printf("mask : %xu\n", rcc->AHB1ENR);
    printf("mask : %xu\n", rcc->AHB2ENR);
    printf("mask : %xu\n", rcc->AHB3ENR);
    printf("mask : %xu\n", rcc->APB1ENR);
    printf("mask : %xu\n", rcc->APB2ENR);
}

int
main(int argc, char** argv)
{
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
