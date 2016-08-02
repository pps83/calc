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
//  FUNC          ::= log(EXPR) | log TERM | x                 \(EXPR\) | log TERM | x

typedef std::list<struct t_term> t_prod;
typedef std::list<t_prod> t_expr;
typedef std::list<struct t_prod_x> t_expr_x;
struct t_prod_x
{
    t_expr::iterator x;
    std::list<t_prod::iterator> xx;
};
struct t_term
{
    bool div, log;
    char x;
    double num_value;
    t_expr expr_value;
    t_expr_x expr_value_x;
};
double eval(const t_expr &expr);
double eval(const t_term &term);
double eval(const t_prod &terms);
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
        parsed_expression_x.clear();
        lhs_parsed_expression.clear();
        lhs_parsed_expression_x.clear();
        expr();
        skip_ws();
        if (expect == '\0' && *p == '=')
        {
            ++p;
            lhs_parsed_expression.swap(parsed_expression);
            lhs_parsed_expression_x.swap(parsed_expression_x);
            expr();
            skip_ws();
        }
        if (*p && *p != expect)
        {
            err("unexpected input");
        }
        if (!lhs_parsed_expression.empty())
            simplify();
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
    bool term(bool num_allowed, bool div = false, bool log = false)
    {
        parsed_expression.back().resize(parsed_expression.back().size()+1);
        t_term &t = parsed_expression.back().back();
        t.num_value = 1;
        t.div = div;
        t.log = log;  // TODO: if possible, verify that log argument isn't negative
        t.x = 0;
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
            if (next_term("log"))
            {
                term(true, false, true);
                return true;
            }
            if (((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) && check_term(p[1]))
                t.x = *p++;
            else
            {
                if (!next('('))
                {
                    if (num_allowed)
                        err("expected a value");
                    parsed_expression.back().resize(parsed_expression.back().size() - 1);
                    return false;
                }
                expression_parser parser(p, ')');
                p = parser.p;
                t.expr_value.swap(parser.parsed_expression);
                t.expr_value_x.swap(parser.parsed_expression_x);
                skip_ws();
                if (!next(')'))
                    err("expected ')'");
            }
            if (t.x || !t.expr_value_x.empty())
            {
                if (parsed_expression_x.empty() || parsed_expression_x.back().x != --parsed_expression.end())
                {
                    parsed_expression_x.resize(parsed_expression_x.size() + 1);
                    parsed_expression_x.back().x = --parsed_expression.end();
                }
                parsed_expression_x.back().xx.push_back(--parsed_expression.back().end());
            }
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
    bool next_term(const char *str)
    {
        size_t len = strlen(str);
        if (0 != memcmp(str, p, len))
            return false;
        // check that function or variable has to end after len chars:
        if (!check_term(p[len]))
            return false;
        p += len;
        return true;
    }
    static bool check_term(char c)
    {
        return !((c >= 'a' && c <= 'z') || (c >= 'A'&& c <= 'Z')
            || (c >= '0' && c <= '1') || c == '_');
    }
    void err(const char *msg)
    {
        throw expression_error(msg, p);
    }
    void simplify()
    {
        // move terms containing x to lhs
        for (auto &it : parsed_expression_x)
        {
            it.x->front().num_value *= -1;
            lhs_parsed_expression.splice(lhs_parsed_expression.end(), parsed_expression, it.x);
        }
        lhs_parsed_expression_x.splice(lhs_parsed_expression_x.end(), parsed_expression_x);
        // move terms that don not contain x to rhs
        auto it_x = lhs_parsed_expression_x.begin();
        for (auto it1 = lhs_parsed_expression.begin(); it1!=lhs_parsed_expression.end();)
        {
            auto it = it1;
            ++it1;
            if (it_x!=lhs_parsed_expression_x.end() && it_x->x == it)
            {
                ++it_x;
                continue;
            }
            it->front().num_value *= -1;
            parsed_expression.splice(parsed_expression.end(), lhs_parsed_expression, it);
        }
        if (parsed_expression.empty())
        {
            t_expr expr;
            expr.resize(1);
            expr.front().resize(1);
            auto &x = expr.front().front();
            x.div = false;
            x.log = false;
            x.x = 0;
            x.num_value = 0;
            expr.swap(parsed_expression);
        }
        // divide rhs by lhs
        // TODO: implement proper product expansion
        parenthesize(parsed_expression, false);
        parenthesize(lhs_parsed_expression);
        parsed_expression.front().splice(parsed_expression.front().end(), lhs_parsed_expression.front());
        parsed_expression.front().back().div ^= true;
    }
    static void parenthesize(t_expr &expr, bool force_parenths = true)
    {
        if (expr.size()<=1 && !force_parenths)
            return;
        t_expr ex;
        ex.resize(1);
        ex.front().resize(1);
        auto &x = ex.front().front();
        x.div = false;
        x.log = false;
        x.x = 0;
        x.num_value = 1;
        x.expr_value.swap(expr);
        ex.swap(expr);
    }

private:
    const char *p;
    t_expr lhs_parsed_expression, parsed_expression;
    t_expr_x lhs_parsed_expression_x, parsed_expression_x;
};


double eval(const t_term &term)
{
    double ret = term.num_value;
    if (!term.expr_value.empty())
        ret *= eval(term.expr_value);
    if (term.log && ret <= 0)
        throw expression_error("log of negative or 0", nullptr);
    return term.log ? log10(ret) : ret;
}
double eval(const t_prod &terms)
{
    double ret = 1;
    for (const auto &term : terms)
    {
        double x = eval(term);
        if (term.div && !x)
            throw expression_error("division by 0", nullptr);
        if (term.div)
            ret /= eval(term);
        else
            ret *= eval(term);
    }
    return ret;
}
double eval(const char *expr)
{
    try
    {
        return eval(expression_parser(expr));
    }
    catch (const expression_error &e)
    {
        fprintf(stderr, "expression error: %s (at pos=%d)\n", e.what(), (int)((e.p ? e.p - expr : -1)));
    }
    return 0;
}


#endif /* expression_parser_h_ */
