#ifndef expression_h_
#define expression_h_

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <list>
#include <stdexcept>
#include <ostream>
#include <iomanip>
#include <memory>


//  EXPR          ::= PROD+EXPR | PROD-EXPR | PROD             PROD([+\-]PROD)*
//  PROD          ::= TERM*PROD | TERM/PROD | TERM             TERM([*\/]TERM)*
//  TERM          ::= -TERM | TERM FUNC | FUNC | NUM           -*(NUM|FUNC)(FUNC)*
//  FUNC          ::= log(EXPR) | log TERM | x                 \(EXPR\) | log TERM | x

struct term_t;
struct prod_t : std::list<term_t> { std::list<std::list<term_t>::iterator> xterms; };
struct expr_t : std::list<prod_t> { std::list<std::list<prod_t>::iterator> xprods; };
struct term_t
{
    bool div, log;
    char x;
    double num_value;
    expr_t expr_value;
};
double eval(const expr_t &expr);
double eval(const prod_t &terms);
double eval(const term_t &term);
double eval(const char *expr);

std::ostream& operator<<(std::ostream &os, const expr_t &expr);
std::ostream& operator<<(std::ostream &os, const term_t &term);
std::ostream& operator<<(std::ostream &os, const prod_t &prod);


class expression_error : public std::invalid_argument
{
public:
    explicit expression_error(const char *msg, const char *p) : std::invalid_argument(msg), p(p) {}
    const char *p;
};


class expression
{
public:
    explicit expression(const char *expr = nullptr, char expect = '\0', bool x_allowed = true, char x_name = 0)
    {
        if (expr)
            parse(expr, expect, x_allowed, x_name);
    }
    void parse(const char *expression, char expect = '\0', bool x_allowed = true, char x_name = 0);

protected:
    void expr();
    void prod();
    bool term(bool num_allowed, bool div = false, bool log = false);
    void num(double &f);
    void skip_ws();
    bool next(char c);
    bool next_term(const char *str);
    static bool check_term(char c);
    void err(const char *msg, const char *pos);
    void err(const char *msg);
    void simplify();
    friend std::ostream& operator<<(std::ostream &os, const expression &ep)
    {
        if (!ep.lhs_parsed_expression.empty())
            os << ep.lhs_parsed_expression << '=';
        return os << ep.parsed_expression;
    }

public:
    double solve();

private:
    const char *p;
    expr_t lhs_parsed_expression, parsed_expression;
    bool x_allowed;
    char x_name;
};


#endif /* expression_h_ */
