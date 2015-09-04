
#include <gtest/gtest.h>

#include "app_helper.h"


int main_argc;
char ** main_argv;


int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    main_argc = argc;
    main_argv = argv;

    auto result = RUN_ALL_TESTS();

    getchar();

    return result;
}
