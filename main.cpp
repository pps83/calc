#include <assert.h>
#include "expression_parser.h"

#define TEST(expr, expected_result)                             \
do {                                                            \
    expression_parser parser;                                   \
    bool result = parser.parse(expr);                           \
    printf("%s\n", expected_result == result ? "OK" : "ERROR"); \
} while(0)


int main()
{
    TEST("0.123", 1);
}
