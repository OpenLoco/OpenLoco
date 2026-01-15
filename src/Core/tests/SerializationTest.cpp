#include <OpenLoco/Core/BinaryStream.h>
#include <OpenLoco/Core/DataSerialization.h>
#include <array>
#include <gtest/gtest.h>

using namespace OpenLoco;

struct TestStruct
{
    uint8_t a;
    uint16_t b;
    uint32_t c;
    int8_t d;
    int16_t e;
    int32_t f;
    bool g;
};

struct NestedStruct
{
    TestStruct a;
    uint64_t b;
};

struct ArrayStruct
{
    uint16_t arr[5];
};

template<>
struct DataSerialization<TestStruct>
{
    static void encode(const TestStruct& src, DataSerilizer& ds)
    {
        ds.encode(src.a);
        ds.encode(src.b);
        ds.encode(src.c);
        ds.encode(src.d);
        ds.encode(src.e);
        ds.encode(src.f);
        ds.encode(src.g);
    }

    static void decode(TestStruct& dest, DataSerilizer& ds)
    {
        ds.decode(dest.a);
        ds.decode(dest.b);
        ds.decode(dest.c);
        ds.decode(dest.d);
        ds.decode(dest.e);
        ds.decode(dest.f);
        ds.decode(dest.g);
    }
};

template<>
struct DataSerialization<NestedStruct>
{
    static void encode(const NestedStruct& src, DataSerilizer& ds)
    {
        ds.encode(src.a);
        ds.encode(src.b);
    }

    static void decode(NestedStruct& dest, DataSerilizer& ds)
    {
        ds.decode(dest.a);
        ds.decode(dest.b);
    }
};

template<>
struct DataSerialization<ArrayStruct>
{
    static void encode(const ArrayStruct& src, DataSerilizer& ds)
    {
        ds.encode(src.arr);
    }

    static void decode(ArrayStruct& dest, DataSerilizer& ds)
    {
        ds.decode(dest.arr);
    }
};

TEST(SerializationTest, misc)
{
    TestStruct src{ 0x12, 0x3456, 0x789ABCDE, -0x12, -0x3456, -0x789ABCDE, true };
    TestStruct dest{};

    std::array<uint8_t, 128> buffer{};
    auto bs = BinaryStream(buffer.data(), std::size(buffer));
    auto ds = DataSerilizer(bs);

    ds.encode(src);
    bs.setPosition(0);
    ds.decode(dest);

    EXPECT_EQ(src.a, dest.a);
    EXPECT_EQ(src.b, dest.b);
    EXPECT_EQ(src.c, dest.c);
    EXPECT_EQ(src.d, dest.d);
    EXPECT_EQ(src.e, dest.e);
    EXPECT_EQ(src.f, dest.f);
    EXPECT_EQ(src.g, dest.g);
}

TEST(SerializationTest, nested)
{
    NestedStruct src{ TestStruct{ 0x12, 0x3456, 0x789ABCDE, -0x12, -0x3456, -0x789ABCDE, true }, 0x123456789ABCDEULL };
    NestedStruct dest{};

    std::array<uint8_t, 128> buffer{};
    auto bs = BinaryStream(buffer.data(), std::size(buffer));
    auto ds = DataSerilizer(bs);

    ds.encode(src);
    bs.setPosition(0);
    ds.decode(dest);

    EXPECT_EQ(src.a.a, dest.a.a);
    EXPECT_EQ(src.a.b, dest.a.b);
    EXPECT_EQ(src.a.c, dest.a.c);
    EXPECT_EQ(src.a.d, dest.a.d);
    EXPECT_EQ(src.a.e, dest.a.e);
    EXPECT_EQ(src.a.f, dest.a.f);
    EXPECT_EQ(src.a.g, dest.a.g);
    EXPECT_EQ(src.b, dest.b);
}

TEST(SerializationTest, array)
{
    ArrayStruct src{ { 0, 1, 2, 3, 4 } };
    ArrayStruct dest{};

    std::array<uint8_t, 128> buffer{};
    auto bs = BinaryStream(buffer.data(), std::size(buffer));
    auto ds = DataSerilizer(bs);

    ds.encode(src);
    bs.setPosition(0);
    ds.decode(dest);

    for (auto i = 0; i < std::size(src.arr); ++i)
    {
        EXPECT_EQ(src.arr[i], dest.arr[i]);
    }
}
