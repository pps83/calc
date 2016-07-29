#include <assert.h>
#include "expression_parser.h"

#define TEST(expr, expected_res)                             \
do {                                                         \
    expression_parser parser;                                \
    bool parse_result = parser.parse(expr);                  \
    assert(parse_result);                                    \
    double res = eval(parser);                               \
    char str[256];                                           \
    sprintf(str, "%f %f", res, (double)expected_res);        \
    double d1, d2;                                           \
    sscanf(str, "%lf %lf", &d1, &d2);                        \
    printf("%s\n", d1==d2 ? "OK" : "ERROR");                 \
} while(0)                                                   \


int main()
{
    TEST("0.123", 0.123);
    TEST("-0.123", -0.123);
    TEST("-0.123(3+4)", -0.861);
    TEST("-0.123/(3+log(100))", -0.0246);
}
