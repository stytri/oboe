# OBOE: Only Binary Operator Expressions



## About

This is a [toy language](https://en.wikipedia.org/wiki/Esoteric_programming_language) inspired by the [C ternary operator](https://en.wikipedia.org/wiki/%3F:).

It originated when I was thinking it would be nice to have an equivalent to the C ternary operator for the `switch`  statement, this was then expanded to _why not make everything an operator and eliminate keywords_.

Further to this I arrived at the following guides for implementation:

-	Operators not keywords
-	Binary operators only
-	Only use Standard C
-	Use it as a sandbox for other ideas



## Implementation

Implemented as an executable syntax tree.

#### Lexicon

UTF-8 encoding.

#### Comments

Comments are started with `#`

If followed by one of `(`, `[`,  `{`, the comment is terminated by the corresponding `)`, `]`, `}`. The active brackets can be nested.

Otherwise comments are terminated at the end of line.

#### Integers

##### Decimal

`[0-9]+([Ee][0-9]+)?`

##### Hexadecimal

`(0x|0X)[0-9]+([Pp][0-9]+)?`

#### Floats

##### Decimal

`[0-9]+\.[0-9]*([Ee][-+]?[0-9]+)?`

##### Hexadecimal

`(0x|0X)[0-9A-Fa-f]+\.[0-9A-Fa-f]*([Pp][-+]?[0-9]+)?`

#### Strings

##### Double-quote strings

Delimited by `"`. 

No escape sequences.

Can not contain `"`.

##### Single-quote strings

Delimited by `'`.

Can contain escape sequences (see **Characters** section)

#### Characters

Delimited by ```.

Can contain an escape sequence.

##### Escape sequence

Initiated by `\`, followed by:

- `n` inserts a new-line character.
- `t` inserts a horizontal-tab character.
- `U` or `u` followed by up to 8 hexadecimal digits specifying  a Unicode code-point.
- `W` or `w` followed by up to 4 hexadecimal digits specifying  a Unicode code-point.
- `X` or `x` followed by up to 2 hexadecimal digits specifying  a Unicode code-point.
- for other characters, acts as a quoting mechanism.

#### Sub-expressions

Initiated by one of `(`, `[`,  `{`, terminated by the corresponding `)`, `]`, `}`.

`{` `}` and `[` `]` are represented in the syntax tree by distinct operators.

`(` `)` are elided, replaced in the syntax tree by the bracketed sub-expression.

#### Operators

##### Predefined operators

- _applicate_, has no lexical representation, but is invoked by adjacency.
- `,` _sequence_, creates a list of operations.
- `;` _assemblage_, creates a list of sequences.

##### User-defined operators

See `lex.h` for definition.

#### Identifiers

See `lex.h` for definition.

### Grammar

#### Basic Grammar

The current grammar is fairly flat, consisting of five precedence levels with left to right evaluation. The precedence levels, in decreasing order, are:

- Primary (Values, Identifiers, Sub-expressions)
- Applicate
- Operators
- Sequence
- Assemblage

Although the goal is for only binary operators, the simplicity of the implementation of parsing gives us unary operators for free - it would require more code to enforce binary only. However, in the syntax tree all operators are binary, unary operations being represented by having a non-value operand (internally this is the **Zen** type - Zero/Empty/Null). The empty parenthesis `()` operator can be used to specify **Zen** explicitly.

The more detailed grammar (e.g. declaration, selection, iteration) is handled at runtime; but is built from binary operators.

