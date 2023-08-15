#ifndef __BPLUS_TEST_H
#define __BPLUS_TEST_H

#include <stdio.h>
#define FILENAME    "/home/shuiyihang/NDSL_Practice/week_5/test/info.txt"

#define do_func(name)   {   \
    printf("%s test start...\n", #name);\
    name();\
}

#endif