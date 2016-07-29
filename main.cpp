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
    TEST("1", 1);
    TEST("0.123", 0.123);
    TEST("-0.123", -0.123);
    TEST("-0.123(3+4)", -0.861);
    TEST("-0.123/(3+log(100))", -0.0246);
    TEST("1.0/3", 0.33333333333);
    TEST("-0.123/(3+log(100))log(10)(1+1*5/6(3*2))", -0.1476);
    TEST("(0.1/3+.1/3+10/300)/0.001e10", 1e-8);
    TEST("(0.1/3+.1/3+10/300+3.14159265359)/1e8+10", 10.000000032);
    TEST("log((((((0.1/3+.1/3+10/300+3.14159265359)/1e8+10)))))", 1.00000000141);
    TEST("-5log((((((0.1/3+.1/3+10/300+3.14159265359)/1e8+10)))))(0+.1)", -0.5000000007);
}
