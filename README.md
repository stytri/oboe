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

Delimited by `"`. 

Can contain escape sequences (see **Characters** section)

#### Characters

Delimited by `'`.

Can contain an escape sequence.

##### Escape sequence

Initiated by `\`, followed by:

- `0` inserts a nul character.
- `n` inserts a new-line character.
- `t` inserts a horizontal-tab character.
- `U` or `u` followed by up to 8 hexadecimal digits specifying  a Unicode code-point.
- `W` or `w` followed by up to 4 hexadecimal digits specifying  a Unicode code-point.
- `X` or `x` followed by up to 2 hexadecimal digits specifying  a Unicode code-point.
- End-of-Line characters; these are elided, including CR-LF and LF-CR pairings.
- for other characters, acts as a quoting mechanism.

#### Sub-expressions

Initiated by one of `(`, `[`,  `{`, terminated by the corresponding `)`, `]`, `}`.

`(` `)` are elided, replaced in the syntax tree by the bracketed **sub-expression**.

`[` `]` and `{` `}` are represented in the syntax tree by distinct operators.

`[` `]` is used to define **arrays/environments**.

`{` `}` is used to designate an evaluation **block**.

Where the bracketed expression is an _operand_-less **operator** sans space, then this forms a distinct **operator**.

#### Arrays

An **array** can be indexed, associative, or a mix of both. They can also act as an **environment** (aka Name-space or, scope).

Indexes are zero based. Assigning to to the last + 1 index, appends a new entry.

##### Environments

The following environments are predefined:

**local** which is the default scope within a function. It can be specifically invoked using the `(:)` operator. There are no predefined identifiers in the **local** environment.

**static** which is the default scope within a source file. It can be specifically invoked using the `{:}` operator. There are no predefined identifiers in the **static** environment.

**global**, which is available to all. It can be specifically invoked using the `[:]` operator. Unless **oboe** is invoked with the `--math` option, there are no predefined identifiers in the **global** environment.

**system**, which can be accessed via the **sigil** operator.

When an **environment** is applied to an expression or, expression-list, it is automatically linked to the current **environment**.

An anonymous **environment** can be utilized to limit the scope of variables.

#### Block

Used to demark a block of code; primarily this will be used with conditional expressions to isolate a block of code to avoid unwanted interaction with the `;` operator which is utilized to designate alternate program flow paths.

#### Operators

##### Predefined operators

- _applicate_, has no lexical representation, but is invoked by adjacency.
- `,` _sequence_, creates a list of expressions.
- `;` _assemblage_, creates a list of sequences/expressions.

##### User-defined operators

See `lex.h` for permitted lexeme characters.

Where an _operand_-less **operator** is bracketed sans space, then this forms a distinct **operator**.

##### Named operators

User-defined operators can be named by prefixing an **identifier** with '`' and can also be terminated with another back-tick.

##### Unary operators

All operators are inherently binary; when used as a unary operator, the operator is still parsed at the same precedence level; therefore, when an operator is used as a unary operator in a sub-expression, the sub-expression should be parenthesized.

#### Identifiers

See `lex.h` for permitted lexeme characters.

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
- Interstitial
- Sequence
- Assemblage

Although the goal is for only binary operators, the simplicity of the implementation of parsing gives us unary operators for free - it would require more code to enforce binary only. However, in the syntax tree all operators are binary, unary operations being represented by having a non-value operand (internally this is the **Zen** type - Zero/Empty/Null). The empty parenthesis `()` operator can be used to specify **Zen** explicitly.

The more detailed grammar (e.g. declaration, selection, iteration) is handled at runtime; but is built from binary operators.

#### Operator Grammar and Operation

##### assemblage

_left-operand_ ; _right-operand_

An _assemblage_ may be evaluated differently when used as an operand, but is otherwise evaluated thus:

_left-operand_ is evaluated, then _right-operand_ is evaluated, the result of evaluating the _right-operand_ is returned.

##### sequence

_left-operand_ `,` _right-operand_

A _sequence_ may be evaluated differently when used as an operand, but is otherwise evaluated thus:

_left-operand_ is evaluated, then _right-operand_ is evaluated, and a new sequence of the results is created. Individual operators [e.g. **conditional**, **iteration** or, **selection**] may handle sequences differently in certain instances.

##### range

_operand_ `..` _operand_

##### declaration

either:

_referent_ `:` _operand_

or:

_referent_ `::` _operand_

or:

_referent_ `:^` _reference_

or:

_referent_ `(` _parameter_? (`,` _parameter_)* `)` `:` _operand_

or:

[_precedence-operator-string_]? _operator-string_ `(` _parameter_? (`,` _parameter_)* `)` `:` _operand_

or:

_operator-string_ `:` _operator-string_

##### global declaration

Normally, non-operator declarations are made in the static environment (_source-file_, _function_, ...); if the global environment operator `[:]` is applied to the declaration then it is made in the global environment. Operator declarations are always made in the global environment.

##### static declaration

Declarations within a non static-scope (e.g. within a function), can be made static by applying the static environment operator `(:)`. They will be visible to all functions that share the same static scope; e.g. within the same source file.

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

_condition_ `?` `(`_true-operand_ `;` _false-operand_`)`

_condition_ is evaluated, and if the result, when cast to a boolean value, evaluates to **true**, then _true-operand_ is evaluated, otherwise _false-operand_ id evaluated.

**sequences** in _condition_ are evaluated as a simple list of expressions, each evaluated in turn; with the result of the evaluation of the final expression in the sequence is used to determine the condition.

_operand_ `?` **Zen** 

**Zen** `?` _operand_

evaluates _operand_ and returns  its boolean value.

The `!` operator is as above, except the condition is inverted.

##### selection

either:

_condition_ `?:` `(` (_case-expression_ `:` _action-expression_ `;`)+ _default-action-expression_?`)`

or:

 **Zen** `?:` `(` (_case-expression_ `:` _action-expression_ `;`)+ _default-action-expression_?`)`

**sequences** in _condition_ are evaluated as a simple list of expressions, each evaluated in turn; with the result of the evaluation of the final expression in the sequence is used to determine the condition.

##### iteration

either:

_iteration-control_ `?*` _iteration-operand_

or:

_iteration-control_ `?*` `(` _iteration-expression `;` _no-iteration-expression_ `)`

_no-iteration-operand_ is evaluated if the controlling _condition_ never evaluates **true**

or:

_iteration-control_ `?*` **Zen**

where _iteration-control_ is either:

_condition_

or:

`(` _initialization_ `;` _condition_ `)`

or:

`(` _initialization_ `;` _condition_ `;` _recalculation_ `)`

or:

`(` _identifier_ `:` _range_ `)`   

or:

`(` _identifier_ `=` _range_ `)`

or:

`(` _identifier_ `:` _sequence_ `)`   

or:

`(` _identifier_ `=` _sequence_ `)`

or:

`(` _identifier_ `:` _array_`[`_range_`]` `)`

or:

`(` _identifier_ `=` _array_`[`_range_`]` `)`

or:

`(` _identifier_ `:` `[`_initializer_`]` `)`

or:

`(` _identifier_ `=` `[`_initializer_`]` `)`

The `!*` operator is as above, except the condition is inverted; does **not** apply to **range**s.

**sequences** in _initialization_, _condition_ and, _recalculation_ are evaluated as a simple list of expressions, each evaluated in turn; in the case of _condition_ with the result of the evaluation of the final expression in the sequence is used to determine the condition.

##### evaluation

_left-operand_ _operator_ _right-operand_

The following operators are built-in:

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

The non-comparative operators also have a self-assigning form: e.g. 

_reference_ `+=` _operand_

##### applicate

either:

_left-operand_ _right-operand_

or:

_operator_ _right-operand_

##### sigil

_operand_ `@` _identifier_

Used to access the attributes and functions of _operand_ (e.g. type query, type conversion).

When **Zen** is the _operand_, it provides access to the system library.

