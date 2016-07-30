#ifndef expression_parser_h_
#define expression_parser_h_

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <list>
#include <stdexcept>


//  EXPR          ::= PROD+EXPR | PROD-EXPR | PROD             PROD([+\-]PROD)*
//  PROD          ::= TERM*PROD | TERM/PROD | TERM             TERM([*\/]TERM)*
//  TERM          ::= -TERM | TERM FUNC | FUNC | NUM           -*(NUM|FUNC)(FUNC)*
//  FUNC          ::= log(EXPR) | (EXPR)                       (log)?\(EXPR\)

typedef std::list<struct t_term> t_prod;
typedef std::list<t_prod> t_expr;
struct t_func
{
    bool log;
    t_expr expr;
};
struct t_term
{
    bool div;
    double num_value;
    t_func func_value;
};
double eval(const t_expr &expr);
double eval(const t_func &func)
{
    double ret = eval(func.expr);
    return func.log ? log10(ret) : ret;
}
double eval(const t_term &term)
{
    double ret = term.num_value;
    if (!term.func_value.expr.empty())
        ret *= eval(term.func_value);
    return ret;
}
double eval(const t_prod &terms)
{
    double ret = 1;
    for (const auto &term : terms)
    {
        if (term.div)
            ret /= eval(term);
        else
            ret *= eval(term);
    }
    return ret;
}
double eval(const t_expr &expr)
{
    double ret = 0;
    for (const auto &prod : expr)
        ret += eval(prod);
    return ret;
}
double eval(const char *expr);


class expression_error : public std::invalid_argument
{
public:
    explicit expression_error(const char *msg, const char *p) : std::invalid_argument(msg), p(p) {}
    const char *p;
};


class expression_parser
{
public:
    expression_parser(const char *expr = nullptr, char expect = '\0') : p(nullptr)
    {
        if (expr)
            parse(expr, expect);
    }
    void parse(const char *expression, char expect = '\0')
    {
        p = expression;
        parsed_expression.clear();
        expr();
        skip_ws();
        if (*p && *p != expect)
            err("unexpected input");
    }
    operator const t_expr&() const
    {
        return parsed_expression;
    }

protected:
    void expr()
    {
        prod();
        for (;;)
        {
            skip_ws();
            if (!next('+') && *p!='-') // treat (a-b) as (a+-b)
                break;
            prod();
        }
    }
    void prod()
    {
        parsed_expression.resize(parsed_expression.size()+1);
        term(true);
        for (;;)
        {
            skip_ws();
            if (!next('/') && !next('*'))
                break;
            term(true, p[-1]=='/');
        }
    }
    bool term(bool num_allowed, bool div = false)
    {
        parsed_expression.back().resize(parsed_expression.back().size()+1);
        t_term &t = parsed_expression.back().back();
        t.num_value = 1;
        t.div = div;
        skip_ws();
        bool has_value = false;
        if (num_allowed)
        {
            bool neg = false;
            while (next('-'))
            {
                skip_ws();
                neg = !neg;
            }
            // last comma for locales that use comma as decimal mark
            if ((*p >= '0' && *p <= '9') || *p == '.' || *p == ',')
            {
                has_value = true;
                num(t.num_value);
            }
            if (neg)
                t.num_value *= -1;
        }
        if(!has_value)
        {
            t.func_value.log = next("log");
            if (t.func_value.log)
                skip_ws();
            if (!next('('))
            {
                if (!t.func_value.log)
                {
                    if (num_allowed)
                        err("expected a value");
                    parsed_expression.back().resize(parsed_expression.back().size()-1);
                    return false;
                }
                err("expected '('");
            }
            expression_parser parser(p, ')');
            p = parser.p;
            t.func_value.expr.swap(parser.parsed_expression);
            skip_ws();
            if (!next(')'))
                err("expected ')'");
        }
        while (num_allowed)
        {
            const char *tmp = p;
            if (!term(false))
            {
                p = tmp;
                break;
            }
        }
        return true;
    }
    void num(double &f)
    {
        int n;
        if (sscanf(p, "%lf%n", &f, &n) < 1)
            err("cannot parse number");
        p += n;
    }
    void skip_ws()
    {
        while (*p == ' ' || *p == '\t')
            ++p;
    }
    bool next(char c)
    {
        if (*p != c)
            return false;
        ++p;
        return true;
    }
    bool next(const char *str)
    {
        size_t len = strlen(str);
        if (0 != memcmp(str, p, len))
            return false;
        p += len;
        return true;
    }
    void err(const char *msg)
    {
        throw expression_error(msg, p);
    }

private:
    const char *p;
    t_expr parsed_expression;
};


double eval(const char *expr)
{
    try
    {
        return eval(expression_parser(expr));
    }
    catch (const expression_error &e)
    {
        fprintf(stderr, "expression error: %s (at pos=%d)\n", e.what(), (int)(e.p-expr));
    }
    return 0;
}


#endif /* expression_parser_h_ */
