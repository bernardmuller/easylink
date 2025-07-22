# Parsing RESP

## Overview
Compilers, in this case an interpreter, uses a series of tokens to terminals in the grammar of the language
to which rules could generate that sequence of tokens. For this we need to build a syntax/parsing tree with 
the [protocol specifications][https://redis.io/docs/latest/develop/reference/protocol-spec/] of RESP. 
Once we have the protocol mapped into a rules tree, we check whether the tokens array we received from the
lexer matches the rules.

## RESP GRAMMAR

### Key Features
- Recursive structure
- Length prefixed data
- Null representations with "-1"
- Type indications

### Properties
- LL(1): Can be parsed with single lookahead (the first character determines type)
- Unambiguous: Each valid RESP string has exactly one parse tree
- Context-Free: No context-dependent parsing rules
- Recursive: Arrays can contain nested structures of any depth

```
resp_value     → simple_string
               | error
               | integer
               | bulk_string
               | array ;

simple_string  → "+" string_data CRLF ;

error          → "-" string_data CRLF ;

integer        → ":" number CRLF ;

bulk_string    → "$" number CRLF string_data CRLF
               | "$" "-1" CRLF ;                    // null bulk string

array          → "*" number CRLF array_elements
               | "*" "-1" CRLF ;                    // null array

array_elements → resp_value array_elements
               | ε ;                                // empty (base case)

string_data    → PRINTABLE_CHARS ;                 // any printable characters

number         → DIGIT+ 
               | "-" DIGIT+ ;                       // negative numbers allowed

CRLF           → "\r\n" ;
```

## Parse Tree Examples

### Simple String `+OK\r\n`
```
resp_value
└── simple_string
    ├── "+"
    ├── string_data("OK")
    └── CRLF
```

### Array `*2\r\n+hello\r\n:42\r\n`

Arrays look a bit strange at first, but this is just the way a recursive structure looks. Each array element
contains a resp_value and an array_elements case, which makes it look very similar to a linked list.

```
resp_value
└── array
    ├── "*"
    ├── number("2")
    ├── CRLF
    └── array_elements
        ├── resp_value
        │   └── simple_string
        │       ├── "+"
        │       ├── string_data("hello")
        │       └── CRLF
        └── array_elements
            ├── resp_value
            │   └── integer
            │       ├── ":"
            │       ├── number("42")
            │       └── CRLF
            └── ε
```
