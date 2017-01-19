TARGET = compiler 
OBJECT = lex.yy.c y.tab.c y.tab.o lex.yy.o y.output y.tab.h main.o 
JFILE = output.j output.class
CC = gcc -g
LEX = flex
YACC = yacc -v
YACCFLAG = -d
LIBS = -lfl

all: $(TARGET)

compiler: y.tab.o lex.yy.o main.o
	$(CC) -o $(TARGET) symbol_table.c helper.c symchk.c codegen.c y.tab.o lex.yy.o main.o $(LIBS)


symbol_table.o: symbol_table.c
	$(CC) -c symbol_table.c

symchk.o: symchk.c
	$(CC) -c symchk.c

helper.o: helper.c
	$(CC) -c helper.c

codegen.o: codegen.c
	$(CC) -c codegen.c

y.tab.o: y.tab.c
	$(CC) -c y.tab.c

y.tab.c: parser.y
	$(YACC) $(YACCFLAG) parser.y

lex.yy.o: lex.yy.c
	$(CC) -c lex.yy.c

lex.yy.c: lex.l
	$(LEX) lex.l

main.o: main.c
	$(CC) -c main.c

test:
	java -jar jasmin.jar output.j
	java output

clean:
	rm -f $(TARGET) $(OBJECT) $(JFILE)

