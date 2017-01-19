Abilities

lex.l:
    Use the one from Project2, and add support for pragma symbol.
parser.y:
    Use the one from Project2, and add sementic check.
symbol_table.c:
    The data strcuture for symbol_table
symchk.c:
    Check the semantic errors. Including:
    1. Function definition, declaration, and usage error
    2. Expression type checking
    3. Coercion
    4. Boolean expression for loop and if-else
codegen.c:
    Generate java bytecode

Platform

1. My computer (Ubuntu 16.04) (lex 2.6.0, bison 3.0.4, gcc 5.4.0)
2. NCTU Linux workstation

How to
	make
	./compiler [filename] // generate output.j

