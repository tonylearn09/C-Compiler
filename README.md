## Abilities

  * lex.l:
    - Use the one from Project2, and add support for pragma symbol.
  * parser.y:
    - Use the one from Project2, and add sementic check.
    * symbol_table.c:
    - The data strcuture for symbol_table
    * symchk.c:
  -Check the semantic errors. Including:
    - Function definition, declaration, and usage error
    - Expression type checking
  - Coercion
    - Boolean expression for loop and if-else
  * codegen.c:
    - Generate java bytecode

## Platform

  * Ubuntu 16.04 (lex 2.6.0, bison 3.0.4, gcc 5.4.0)

## How to
	```
  make
	./compiler [filename]   // generate output.j
  java -jar jasmin.jar output.j   // generate output.class
  java output
  ```

