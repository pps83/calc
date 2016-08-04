#include <assert.h>
#include <iostream>
#include <sstream>
#include <string>
#include "expression_parser.h"
#ifndef _WIN32
#include <readline/readline.h>
#include <readline/history.h>
#endif

static int ok_count = 0, err_count = 0;
static void eval(const char *expr, double &res, std::string &err_msg, int &err_pos, bool test_serialize = false)
{
    try
    {
        expression_parser parser(expr);
        if (test_serialize)
        {
            std::stringstream ss;
            ss << parser;
            parser.parse(ss.str().c_str());
        }
        res = parser.solve();
        err_msg.clear();
        err_pos = -1;
    }
    catch (const expression_error &e)
    {
        res = 0;
        err_msg = e.what();
        err_pos = (int)(e.p ? e.p - expr : -1);
    }
}

void TEST(const char *expr, double expected_res, const char *err_msg = "", int err_pos = 0)
{
    double res0, res1, resX;
    std::string err_msg0, err_msg1, err_msgX;
    int err_pos0, err_pos1, err_posX;
    eval(expr, res0, err_msg0, err_pos0);
    if (err_pos0==-1)
    {
        eval(expr, resX, err_msgX, err_posX, true);
        assert(res0==resX && err_msg0==err_msgX);
    }

    const char *pos = strchr(expr, '=');
    if (pos)
    {
        std::string expr_m(pos+1);
        expr_m += '=';
        expr_m.append(expr, pos);
        eval(expr_m.c_str(), res1, err_msg1, err_pos1);
        if (err_pos1 != -1)
        {
            int e_pos = expr_m.size() - (pos - expr);
            if (err_pos1 >= e_pos)
                err_pos1 -= e_pos;
            else
                err_pos1 += (expr_m.size() - e_pos) + 1;
        }
        if (res0!=res1 || err_msg0!=err_msg1 ||
            (err_msg0!="multiple variables in linear equation" && err_pos0!=err_pos1))
        {
            fprintf(stderr, "error: linear equation parsing\n");
            err_count++;
        }
    }
    try
    {
        double res = expression_parser(expr).solve();
        char str[256];
        sprintf(str, "%.10f %.10f", res, expected_res);
        double d1, d2;
        sscanf(str, "%lf %lf", &d1, &d2);
        if (d1 == d2 && err_msg[0]=='\0' && err_pos==0)
            ok_count++;
        else
            err_count++;
    }
    catch (const expression_error &e)
    {
        if (0==strcmp(err_msg, e.what()) && err_pos == (int)(e.p ? e.p - expr : -1))
            ok_count++;
        else
        {
            err_count++;
            fprintf(stderr, "expression error: %s (at pos=%d)\n", e.what(), (int)(e.p ? e.p - expr : -1));
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
    TEST("log(log(1000))", 0.47712125472);
    TEST("log log(1000)", 0.47712125472);
    TEST("log log 1000", 0.47712125472);
    TEST("-0.123/(3+log 100)log(10)(1+1*5/6(3*2))", -0.1476);
    TEST("(0.1/3+.1/3+10/300)/0.001e10", 1e-8);
    TEST("(0.1/3+.1/3+10/300+3.14159265359)/1e8+10", 10.0000000324);
    TEST("log((((((0.1/3+.1/3+1/30+3.14159265359)/1e8+10)))))", 1.00000000141);
    TEST("-5log(((((.2/3+1/3+3.1415926535)/1e8+10))))(0+.1)", -0.50000000076);
    TEST("-5log(((((.2/3+1/3+3.1415926535)/1e8+10))))(0+.1)+1", 0.49999999923);

    TEST("2x=log 100", 1);
    TEST("2x+5-x*10 = log 100/2", 0.5);
    TEST("2 * x + 0.5 = 1", 0.25);
    TEST("1*(2 * x) + 0.5 = 1", 0.25);
    TEST("2x + 1 = 2(1-x)", 0.25);
    TEST("2x + 1 = 2(1-0.5*((2x+10)))", -2.25);

    TEST("", 0, "expected a value", 0);
    TEST("abc", 0, "expected a value", 0);
    TEST("log100", 0, "expected a value", 0);
    TEST("()", 0, "expected a value", 1);
    TEST("1/", 0, "expected a value", 2);
    TEST("1//2", 0, "expected a value", 2);
    TEST("1*-", 0, "expected a value", 3);
    TEST("1/()", 0, "expected a value", 3);
    TEST("2(1+", 0, "expected a value", 4);
    TEST("0..123", 0, "unexpected input", 2);
    TEST("1 1", 0, "unexpected input", 2);
    TEST("1*(1+3 1)", 0, "unexpected input", 7);
    TEST("1/(1", 0, "expected ')'", 4);
    TEST("1/(((1", 0, "expected ')'", 6);
    TEST("1*(1+3", 0, "expected ')'", 6);
    TEST("1*(2 * 2*(((x+1))) + 0.5 = 1", 0, "expected ')'", 25);
    TEST("1/0", 0, "division by 0", -1);
    TEST("log -1", 0, "log of negative or 0", -1);
    TEST("1=1", 0, "linear equation missing 'x'", -1);
    TEST("x", 0, "linear equation missing right hand side", 1);
    TEST("1/x=1", 0, "division or log in linear equation", 2);
    TEST("log(x)=1", 0, "division or log in linear equation", 4);
    TEST("10/((1+1(1+x)))=1", 0, "division or log in linear equation", 11);
    TEST("log((1+1(1+x)))=1", 0, "division or log in linear equation", 11);
    TEST("x*x=1", 0, "non-linear equation", 3);
    TEST("1x+log 100x*5(1+x)=1", 0, "non-linear equation", 18);
    TEST("(1+(1+x))*log(100)*((1+(1+x)))=1", 0, "non-linear equation", 30);
    TEST("x=y", 0, "multiple variables in linear equation", 2);
    TEST("x=6(1+y)", 0, "multiple variables in linear equation", 6);
    TEST("1(x)=1(y)", 0, "multiple variables in linear equation", 7);
    TEST("x=x", 0, "linear equation always true", -1);
    TEST("x=x+1", 0, "linear equation has no solution", -1);

    if (err_count)
        printf("%d tests passed, %d tests failed\n", ok_count, err_count);
    else
        printf("OK (%d tests passed)\n", ok_count);
    return err_count;
}

bool read_line(const char *msg, std::string &ret)
{
#ifndef _WIN32
    rl_bind_key('\t', rl_insert);
    char *str = readline(msg);
    if (!str)
        return false;
    ret = str;
    if (*str)
        add_history(str);
    free(str);
#else
    std::cout << msg;
    std::cout.flush();
    if (!std::getline(std::cin, ret))
        return false;
#endif
    return true;
}

bool read_expr(std::string &expr)
{
    do
    {
        if (!read_line(">>> ", expr))
            return false;
    }
    while (expr.empty());
    while (expr[expr.size() - 1] == '\\')
    {
        std::string extra;
        if (!read_line("... ", extra))
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
        std::cout << to_string(expression_parser(expr.c_str()).solve());
    }
    catch (const expression_error &e)
    {
        std::cout << "expression error: " << e.what();
        if (e.p)
            std::cout <<" (at pos=" << (int)(e.p - expr.c_str()) << ")";
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
