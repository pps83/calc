### Description
Simple scientific calculator app with the following features:

- the calculator supports addition, subtraction, multiplication, division, log on floating point numbers
- the calculator can solve simple linear equations with a single variable (namely, x), for simplicity, only addition, subtraction and multiplication operations are allowed
- the calculator supports parentheses in both modes
- the calculator has a language parser
- does not use any library that can accomplish any of the listed requirements
- the calculator handles all error cases (by carefully indicating the errors to the user)

**class expression** implements recursive descent parser with the following
grammar rules:
```
  EXPR          ::= PROD+EXPR | PROD-EXPR | PROD
  PROD          ::= TERM*PROD | TERM/PROD | TERM
  TERM          ::= -TERM | TERM FUNC | FUNC | NUM
  FUNC          ::= (EXPR) | log TERM | x
```
or in pseudo-regex:
```
  EXPR      ::=  PROD([+\-]PROD)*
  PROD      ::=  TERM([*\/]TERM)*
  TERM      ::=  -*(NUM|FUNC)(FUNC)*
  FUNC      ::=  \(EXPR\) | log TERM | x
```

Grammar should be very similar to google calculator.
[See detailed diagram](../blob/master/diagram/index.html).

### Licensing 
For licensing this code see LICENSE
