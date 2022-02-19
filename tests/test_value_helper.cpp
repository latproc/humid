#include "gtest/gtest.h"
#include <nanogui/common.h>
#include <value.h>
#include <valuehelper.h>

bool isNull(const Value &value) { return value.kind == Value::t_empty; }

namespace {

TEST(ValueHelper, CanGetDefaultForKnownProperty) {
    auto value = defaultForProperty("border");
    EXPECT_EQ(value.kind, Value::t_integer);
    EXPECT_EQ(value.iValue, 0);
}

TEST(ValueHelper, CanGetTypeForKnownProperty) {
    auto kind = typeForProperty("value_scale");
    EXPECT_EQ(kind, Value::t_float);
}

TEST(ValueHelper, CanGetDefaultForType) {
    auto value = defaultForType(Value::t_bool);
    EXPECT_EQ(value.kind, Value::t_bool);
    EXPECT_EQ(value, false);
}

} // namespace
