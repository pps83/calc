#ifndef expression_parser_h_
#define expression_parser_h_

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <list>


//  EXPR          ::= PROD+EXPR | PROD-EXPR | PROD             PROD([+\-]PROD)*
//  PROD          ::= TERM*PROD | TERM/PROD | TERM             TERM([*\/]TERM)*
//  TERM          ::= -TERM | TERM FUNC | FUNC | NUM           -*(NUM|FUNC)(FUNC)*
//  FUNC          ::= log(EXPR) | (EXPR)                       (log)?\(EXPR\)

namespace
{
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
}

class expression_parser
{
public:
    expression_parser(const char *expr = nullptr) : p(nullptr)
    {
        if (expr)
            parse(expr);
    }
    bool parse(const char *expression)
    {
        p = expression;
        parsed_expression.clear();
        expr();
        skip_ws();
        return *p == '\0';
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
        func_terms();
        for (;;)
        {
            skip_ws();
            if (!next('/') && !next('*'))
                break;
            term(true, p[-1]=='/');
            func_terms();
        }
    }
    void func_terms()
    {
        for (;;)
        {
            const char *tmp = p;
            if (!term(false))
            {
                p = tmp;
                break;
            }
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
            if (*p >= '0' && *p <= '9' || *p == '.' || *p == ',')
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
                        throw "expected a value";
                    parsed_expression.back().resize(parsed_expression.back().size()-1);
                    return false;
                }
                throw "expr paren start";
            }
            expression_parser parser;
            parser.parse(p);
            p = parser.p;
            t.func_value.expr.swap(parser.parsed_expression);
            skip_ws();
            if (!next(')'))
                throw "expr paren end";
        }
        return true;
    }
    void num(double &f)
    {
        int n;
        if (sscanf(p, "%lf%n", &f, &n) < 1)
            throw "cannot parse number";
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

private:
    const char *p;
    t_expr parsed_expression;
};


#endif /* expression_parser_h_ */
