#include <gtest/gtest.h>

// 这是一个演示用的测试用例
// 证明测试框架可以正常工作
TEST(DemoTest, BasicMath)
{
	EXPECT_EQ(1 + 1, 2);
	EXPECT_TRUE(10 > 5);
}