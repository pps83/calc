#include <assert.h>
#include <iostream>
#include <string>
#include "expression_parser.h"

static int ok_count = 0, err_count = 0;
void TEST(const char *expr, double expected_res, const char *err_msg = "", int err_pos = 0)
{
    try
    {
        double res = eval(expression_parser(expr));
        char str[256];
        sprintf(str, "%.10f %.10f", res, expected_res);
        double d1, d2;
        sscanf(str, "%lf %lf", &d1, &d2);
        if (d1 == d2)
            ok_count++;
        else
            err_count++;
    }
    catch (const expression_error &e)
    {
        if (0==strcmp(err_msg, e.what()) && err_pos == (int)(e.p - expr))
            ok_count++;
        else
        {
            err_count++;
            fprintf(stderr, "expression error: %s (at pos=%d)\n", e.what(), (int)(e.p - expr));
        }
    }
}

int test()
{
    TEST("1", 1);
    TEST("1.", 1);
    TEST("1.0", 1);
    TEST("1.0000", 1);
    TEST("0.123", 0.123);
    TEST("-0.123", -0.123);
    TEST("-0.123(3+4)", -0.861);
    TEST("-0.123/(3+log(100))", -0.0246);
    TEST("1.0/3", 0.33333333333);
    TEST("log 100", 2);
    TEST("log(100)", 2);
    TEST("log--100", 2);
    TEST("5log--100", 10);
    TEST("-0.123/(3+log 100)log(10)(1+1*5/6(3*2))", -0.1476);
    TEST("(0.1/3+.1/3+10/300)/0.001e10", 1e-8);
    TEST("(0.1/3+.1/3+10/300+3.14159265359)/1e8+10", 10.0000000324);
    TEST("log((((((0.1/3+.1/3+1/30+3.14159265359)/1e8+10)))))", 1.00000000141);
    TEST("-5log(((((.2/3+1/3+3.1415926535)/1e8+10))))(0+.1)", -0.50000000076);
    TEST("-5log(((((.2/3+1/3+3.1415926535)/1e8+10))))(0+.1)+1", 0.49999999923);

    TEST("2x=log 100", 1);
    TEST("2x+5-x*10 = log 100/2", 0.5);
    TEST("2 * x + 0.5 = 1", 0.25);

    TEST("", 0, "expected a value", 0);
    TEST("abc", 0, "expected a value", 0);
    TEST("log100", 0, "expected a value", 0);
    TEST("()", 0, "expected a value", 1);
    TEST("1/", 0, "expected a value", 2);
    TEST("1//2", 0, "expected a value", 2);
    TEST("1*-", 0, "expected a value", 3);
    TEST("1/()", 0, "expected a value", 3);
    TEST("2(1+", 0, "expected a value", 4);
    TEST("0a", 0, "unexpected input", 1);
    TEST("0..123", 0, "unexpected input", 2);
    TEST("1 1", 0, "unexpected input", 2);
    TEST("1*(1+3 1)", 0, "unexpected input", 7);
    TEST("1/(1", 0, "expected ')'", 4);
    TEST("1/(((1", 0, "expected ')'", 6);
    TEST("1*(1+3", 0, "expected ')'", 6);

    if (err_count)
        printf("%d tests passed, %d tests failed\n", ok_count, err_count);
    else
        printf("OK (%d tests passed)\n", ok_count);
    return err_count;
}

bool read_expr(std::string &expr)
{
    do
    {
        std::cout << ">>> ";
        std::cout.flush();
        if (!std::getline(std::cin, expr))
            return false;
    }
    while (expr.empty());
    while (expr[expr.size() - 1] == '\\')
    {
        std::string extra;
        std::cout << "... ";
        std::cout.flush();
        if (!std::getline(std::cin, extra))
            return false;
        expr.resize(expr.size()-1);
        expr.append(extra);
    }
    return true;
}

static std::string to_string(double d)
{
    char str[128];
    sprintf(str, "%.12g", d);
    return str;
}

void calc_help()
{
    std::cout << "TODO: add help";
}

static int calc_eval(const std::string &expr)
{
    if (expr == "q" || expr == "exit")
        return 0;
    else if (expr == "help" || expr == "--help" || expr == "?" || expr == "/?")
        calc_help();
    else if (expr == "test")
        return test();
    else try
    {
        std::cout << to_string(eval(expression_parser(expr.c_str())));
    }
    catch (const expression_error &e)
    {
        std::cout << "expression error: " << e.what() <<
            " (at pos=" << (int)(e.p - expr.c_str()) << ")";
    }
    return 1;
}

void calc()
{
    for (std::string expr; read_expr(expr) && calc_eval(expr);)
        std::cout << std::endl;
}

int main(int argc, const char **argv)
{
    if (argc>1)
        return calc_eval(argv[1]);
    calc();
    return 0;
}
