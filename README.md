# OBOE: Only Binary Operator Expressions



## About

This is a [toy language](https://en.wikipedia.org/wiki/Esoteric_programming_language) inspired by the [C ternary operator](https://en.wikipedia.org/wiki/%3F:).

It originated when I was thinking it would be nice to have an equivalent to the C ternary operator for the `switch`  statement, this was then expanded to _why not make everything an operator and eliminate keywords_.

Further to this I arrived at the following guides for implementation:

-	Operators not keywords
-	Binary operators only
-	Only use Standard C
-	Use it as a sandbox for other ideas

## Development Environment

Windows 7

[jEdit](http://jedit.org/) Editor

[MingGW](https://nuwen.net/mingw.html) ([GCC](https://gcc.gnu.org/)) Compiler

[CodeBlocks](http://www.codeblocks.org/) (to build and debug)

[Git](https://git-scm.com/) Source Control Management

[FreeCommander](https://freecommander.com) File Manager

[ConEmu](https://conemu.github.io/) Console Terminal

## Implementation

Implemented as an executable syntax tree interpreter.

#### Lexicon

[UTF-8](https://en.wikipedia.org/wiki/UTF-8) encoding.

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

- `0` inserts a nul character.
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

Expressions are evaluated left to right.

Precedence levels, in decreasing order, are:

- Primary (Values, Identifiers, Sub-expressions)
- Applicate
- Binding
- Exponential
- Multiplicative
- Additive
- Bitwise
- Relational
- Logical
- Conditional
- Assigning
- Declarative
- Sequence
- Assemblage

Although the goal is for only binary operators, the simplicity of the implementation of parsing gives us unary operators for free - it would require more code to enforce binary only. However, in the syntax tree all operators are binary, unary operations being represented by having a non-value operand (internally this is the **Zen** type - Zero/Empty/Null). The empty parenthesis `()` operator can be used to specify **Zen** explicitly.

The more detailed grammar (e.g. declaration, selection, iteration) is handled at runtime; but is built from binary operators.

#### Operator Grammar and Operation

##### assemblage

_left-operand_ ; _right-operand_

An _assemblage_ may be evaluated differently when used as an operand, but is otherwise evaluated thus:

_left-operand_ is evaluated, then _right-operand_ is evaluated.

##### sequence

_left-operand_ `,` _right-operand_

A _sequence_ may be evaluated differently when used as an operand, but is otherwise evaluated thus:

_left-operand_ is evaluated, then _right-operand_ is evaluated.

##### declaration

either:

_referent_ `:` _operand_

or:

_referent_ `:^` _reference_

or:

_referent_ `(` _parameter_? (`,` _parameter_)* `)` `:` _operand_

or:

_operator-string_ `(` _parameter_? (`,` _parameter_)* `)` `:` _operand_

or:

_operator-string_ `:` _operator-string_

##### assignment

either:

_reference_ `=` _operand_

or:

_reference_ `=^` _reference_

##### conditional

either:

_condition_ `?` _true-operand_

_condition_ is evaluated, and if the result, when cast to a boolean value, evaluates to **true**, then _true-operand_ is evaluated.

or:

_condition_ `?` _true-operand_ `;` _false-operand_

_condition_ is evaluated, and if the result, when cast to a boolean value, evaluates to **true**, then _true-operand_ is evaluated, otherwise _false-operand_ id evaluated.

_operand_ `?` **Zen** 

**Zen** `?` _operand_

evaluates _operand_ and returns  its boolean value.

The `!` operator is as above, except the condition is inverted.

##### selection

either:

_expression_ `?:` `(` (_case-expression_ `:` _case-operand_ `;`)+ _default-operand_?`)`

or:

 **Zen** `?:` `(` (_case-expression_ `:` _case-operand_ `;`)+ _default-operand_?`)`

##### iteration

either:

_iteration-control_ `?*` _iteration-operand_

or:

_iteration-control_ `?*` `(` _iteration-operand_ `;` _no-iteration-operand_ `)`

_no-iteration-operand_ is evaluated if the controlling _condition_ never evaluates **true**

or:

_iteration-control_ `?*` **Zen**

where _iteration-control_ is either:

_condition_

or:

`(` _initialization_ `;` _condition_ `)`

or:

`(` _initialization_ `;` _condition_ `;` _recalculation_ `)`

The `!*` operator is as above, except the condition is inverted.

##### evaluation

_left-operand_ _operator_ _right-operand_

`&&`	logical AND

`||`	logical OR

`<`	less than

`<=`	less than or equal

`==`	equal

`<>`	not equal

`>=`	greater than or equal

`>`	greater than

`&`	bitwise AND

`|`	bitwise OR

`~`	bitwise XOR

`+`	add

`-`	subtract

`*`	multiply

`/`	divide

`//`	modulo

`<<`	shift left

`>>`	shift right

`<<<`	extract left

`>>>`	extract right

`<<>`	rotate left

`<>>`	rotate right

##### applicate

either:

_left-operand_ _right-operand_

or:

_operator_ _right-operand_

##### sigil

_operand_ `@` _identifier_

Used to access the attributes and functions of _operand_ (e.g. type query, type conversion).

When **Zen** is the _operand_, it provides access to the system library.

