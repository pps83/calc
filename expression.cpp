#include "expression.h"


static void clear(expr_t &e)
{
    e.clear();
    e.xprods.clear();
}
static void expand_x(expr_t &expr);

void expression::parse(const char *expression, char expect, bool x_allowed, char x_name)
{
    this->x_allowed = x_allowed;
    this->x_name = x_name;
    p = expression;
    clear(parsed_expression);
    clear(lhs_parsed_expression);
    expr();
    skip_ws();
    if (expect == '\0' && *p == '=')
    {
        ++p;
        lhs_parsed_expression.swap(parsed_expression);
        lhs_parsed_expression.xprods.swap(parsed_expression.xprods);
        expr();
        skip_ws();
    }
    if (*p && *p != expect && !(*p=='=' && expect==')'))
        err("unexpected input");
    if (!lhs_parsed_expression.empty())
    {
        if (lhs_parsed_expression.xprods.empty() && parsed_expression.xprods.empty())
            err("linear equation missing 'x'", 0);
    }
    else if (expect == '\0' && !parsed_expression.xprods.empty())
        err("linear equation missing right hand side");
}

void expression::expr()
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
void expression::prod()
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
bool expression::term(bool num_allowed, bool div, bool log)
{
    parsed_expression.back().resize(parsed_expression.back().size()+1);
    term_t &t = parsed_expression.back().back();
    t.num_value = 1;
    t.div = div;
    t.log = false;
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
    if(!has_value && next_term("log"))
    {
        std::unique_ptr<expression> parser(new expression(nullptr, '\0', false, x_name));
        parser->parsed_expression.resize(1);
        parser->p = p;
        parser->term(true, false, true);
        p = parser->p;
        t.expr_value.swap(parser->parsed_expression);
        t.expr_value.xprods.swap(parser->parsed_expression.xprods);
        t.log = true;
        has_value = true;
    }
    if(!has_value)
    {
        bool x_allowed = this->x_allowed && !t.div && !log;
        if (((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) && check_term(p[1]))
        {
            if (!x_allowed)
                err("division or log in linear equation");
            if (x_name && x_name != *p)
                err("multiple variables in linear equation");
            t.x = x_name = *p++;
        }
        else
        {
            if (!next('('))
            {
                if (num_allowed)
                    err("expected a value");
                parsed_expression.back().resize(parsed_expression.back().size() - 1);
                return false;
            }
            std::unique_ptr<expression> parser(new expression);
            parser->parse(p, ')', x_allowed, x_name);
            p = parser->p;
            t.expr_value.swap(parser->parsed_expression);
            t.expr_value.xprods.swap(parser->parsed_expression.xprods);
            x_name = parser->x_name;
            skip_ws();
            if (!next(')'))
                err("expected ')'");
        }
        if (t.x || !t.expr_value.xprods.empty())
        {
            if (parsed_expression.xprods.empty() || parsed_expression.xprods.back() != --parsed_expression.end())
                parsed_expression.xprods.push_back(--parsed_expression.end());
            if (!parsed_expression.back().xterms.empty())
                err("non-linear equation");
            parsed_expression.back().xterms.push_back(--parsed_expression.back().end());
        }
    }
    while (num_allowed && !log)
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
void expression::num(double &f)
{
    int n;
    if (sscanf(p, "%lf%n", &f, &n) < 1)
        err("cannot parse number");
    p += n;
}
void expression::skip_ws()
{
    while (*p == ' ' || *p == '\t')
        ++p;
}
bool expression::next(char c)
{
    if (*p != c)
        return false;
    ++p;
    return true;
}
bool expression::next_term(const char *str)
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
bool expression::check_term(char c)
{
    return !((c >= 'a' && c <= 'z') || (c >= 'A'&& c <= 'Z')
        || (c >= '0' && c <= '1') || c == '_');
}
void expression::err(const char *msg, const char *pos)
{
    throw expression_error(msg, pos);
}
void expression::err(const char *msg)
{
    err(msg, p);
}
void expression::simplify()
{
    expand_x(lhs_parsed_expression);
    expand_x(parsed_expression);
    // move terms containing x to lhs
    for (auto &it : parsed_expression.xprods)
    {
        it->front().num_value *= -1;
        lhs_parsed_expression.splice(lhs_parsed_expression.end(), parsed_expression, it);
    }
    lhs_parsed_expression.xprods.splice(lhs_parsed_expression.xprods.end(), parsed_expression.xprods);
    // move terms that don not contain x to rhs
    auto it_x = lhs_parsed_expression.xprods.begin();
    for (auto it1 = lhs_parsed_expression.begin(); it1!=lhs_parsed_expression.end();)
    {
        auto it = it1;
        ++it1;
        if (it_x!=lhs_parsed_expression.xprods.end() && *it_x == it)
        {
            ++it_x;
            continue;
        }
        it->front().num_value *= -1;
        parsed_expression.splice(parsed_expression.end(), lhs_parsed_expression, it);
    }
}
double expression::solve()
{
    double lhs_res = 0, res;
    bool linear = !lhs_parsed_expression.empty();
    if (linear)
    {
        simplify();
        lhs_res = eval(lhs_parsed_expression);
    }
    res = eval(parsed_expression);
    if (!linear)
        return res;
    if (lhs_res == 0.0)
        err(res == 0.0 ? "linear equation always true" : "linear equation has no solution", 0);
    return res / lhs_res;
}

double eval(const term_t &term)
{
    double ret = term.expr_value.empty() ? 1 : eval(term.expr_value);
    if (term.log)
    {
        if (ret <= 0)
            throw expression_error("log of negative or 0", nullptr);
        ret = log10(ret);
    }
    return term.num_value * ret;
}
double eval(const prod_t &terms)
{
    double ret = 1;
    for (const auto &term : terms)
    {
        double x = eval(term);
        if (term.div && !x)
            throw expression_error("division by 0", nullptr);
        if (term.div)
            ret /= x;
        else
            ret *= x;
    }
    return ret;
}
double eval(const expr_t &expr)
{
    double ret = 0;
    for (const auto &prod : expr)
        ret += eval(prod);
    return ret;
}
double eval(const char *expr)
{
    try
    {
        return expression(expr).solve();
    }
    catch (const expression_error &e)
    {
        fprintf(stderr, "expression error: %s (at pos=%d)\n", e.what(), (int)((e.p ? e.p - expr : -1)));
    }
    return 0;
}
std::ostream& operator<<(std::ostream &os, const term_t &term)
{
    if (term.num_value != 1.0)
        os << std::setprecision(15) << term.num_value;
    if (term.x)
        os << term.x;
    if (term.log)
    {
        os << "log";
        if (term.x)
            os << ' ';
    }
    if (!term.expr_value.empty())
        os << term.expr_value;
    if (!term.x && term.expr_value.empty() && term.num_value == 1.0)
        os << std::setprecision(15) << term.num_value;
    return os;
}
std::ostream& operator<<(std::ostream &os, const prod_t &prod)
{
    bool first = true;
    for (auto &it : prod)
    {
        if (!first && !it.div)
            os << '*';
        else if (it.div)
            os << '/';
        os << it;
        first = false;
    }
    return os;
}
std::ostream& operator<<(std::ostream &os, const expr_t &expr)
{
    os << '(';
    bool first = true;
    for (auto &it : expr)
    {
        if (!first && it.front().num_value >= 0)
            os << '+';
        os << it;
        first = false;
    }
    return os << ')';
}
// (1*2 + 3*x + 5*6 + x*7) => (x*(3+7) + (1*2 + 5*6))
static void compact_x(expr_t &expr)
{
    expr_t e;
    e.resize(2);
    e.front().resize(1);
    term_t &x = e.front().back();
    x.div = x.log = false;
    x.num_value = expr.xprods.empty() ? 0 : 1.0;
    for (auto &it : expr.xprods)
        x.expr_value.splice(x.expr_value.end(), expr, it);
    expr.xprods.clear();

    e.back().resize(1);
    term_t &y = e.back().back();
    y.div = y.log = false;
    y.num_value = expr.empty() ? 0 : 1.0;
    y.expr_value.swap(expr);
    expr.swap(e);
    expr.xprods.push_back(expr.begin());
}
// (1*2*(3*4 + x*5 + 6*7 + 8*x)*9) => (1*2*x*(5+8)*9 + 1*2*(3*4+6*7)*9)
static void expand_x(expr_t &expr)
{
    for (auto &it : expr.xprods)
    {
        assert(it->xterms.size()<2);
        if (it->xterms.size() == 1 && !it->xterms.front()->expr_value.xprods.empty())
        {
            auto &p = it->xterms.front();
            expand_x(p->expr_value);
            compact_x(p->expr_value);
            expr_t e;
            e.splice(e.end(), p->expr_value, p->expr_value.begin());
            expr.push_back(*it);
            p->expr_value.splice(p->expr_value.end(), e, e.begin());
            e.splice(e.end(), p->expr_value, p->expr_value.begin());
        }
    }
}
