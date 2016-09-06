
#include <gtest/gtest.h>

#include <TestEnvironment.h>


int main(int argc, char* argv[])
{
    TestEnvironment::init(argc, argv);

    ::testing::InitGoogleTest(&argc, argv);

    auto result = RUN_ALL_TESTS();

    TestEnvironment::release();

    getchar();

    return result;
}
