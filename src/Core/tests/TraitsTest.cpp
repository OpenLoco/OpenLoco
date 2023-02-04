#include <OpenLoco/Core/Traits.hpp>
#include <string>

using namespace OpenLoco;

struct TestStructPlain
{
    int a;
    int b;
    bool c;
    float d;
    double e;
};

static_assert(Traits::IsPOD<TestStructPlain>::value == true);

struct TestStructPtr
{
    int a;
    int b;
    bool c;
    float d;
    double e;
    void* ptr;
};

static_assert(Traits::IsPOD<TestStructPtr>::value == true);

struct TestStructStr
{
    int a;
    int b;
    bool c;
    float d;
    double e;
    std::string str;
};

static_assert(Traits::IsPOD<TestStructStr>::value == false);
