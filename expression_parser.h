#ifndef expression_parser_h_
#define expression_parser_h_

#include <stdio.h>
#include <string.h>
#include <assert.h>
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
        bool div, has_func;
        double num_value;
        t_func func_value;
    };
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
            if (!next('+') && !next('-'))
                break;
            prod();
        }
    }
    void prod()
    {
        term();
        for (;;)
        {
            skip_ws();
            if (!next('/') && !next('*'))
                break;
            term();
        }
    }
    void term()
    {
        skip_ws();
        bool neg = false;
        while (next('-'))
        {
            skip_ws();
            neg = !neg;
        }
        char c = *p;
        // last comma for locales that use comma as decimal mark
        if (c >= '0' && c <= '9' || c == '.' || c == ',')
            num();
        while (func());
    }
    bool func()
    {
        skip_ws();
        bool is_log = next("log");
        if (is_log)
            skip_ws();
        if (!next('('))
        {
            if (!is_log)
                return false;
            throw "expr paren start";
        }
        expression_parser parser(p);
        p = parser.p;
        skip_ws();
        if (!next(')'))
            throw "expr paren end";
        return true;
    }
    void num()
    {
        int n;
        double f;
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
};


#endif /* expression_parser_h_ */
