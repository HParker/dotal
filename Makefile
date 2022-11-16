FLEX = /usr/bin/flex
CC := /usr/bin/gcc
CFLAGS := $(CFLAGS) -Wextra

lang: main.o parser.o scanner.o compiler.o debug.o emit.o ast.o lut.o
	$(CC) -o lang main.o parser.o scanner.o compiler.o debug.o emit.o ast.o lut.o

color: color.o scanner.o compiler.o debug.o
	$(CC) -o color color.o scanner.o compiler.o debug.o

color.o: color.c parser.h scanner.h lang.h

main.o: main.c parser.h scanner.h lang.h emit.c

compiler.o: compiler.c compiler.h parser.h scanner.h lang.h

parser.o: parser.h parser.c lang.h

scanner.o: scanner.h

debug.o: debug.c compiler.h parser.h scanner.h lang.h

emit.o: emit.c lang.h debug.h compiler.o lut.o debug.o

ast.o: ast.c ast.h

lut.o: lut.c lut.h

# Parser
parser.h parser.c: parser.y lemon
	./lemon parser.y

# Lexer
scanner.c scanner.h: scanner.l
	$(FLEX) scanner.l

lemon: lemon.c
	$(CC) -o lemon lemon.c

format:
	clang-format -i color.c compiler.c debug.c emit.c main.c
	clang-format -i debug.h emit.h lang.h compiler.h

check: parser.o scanner.o compiler.o debug.o emit.o ast.o lut.o
	$(CC) -o test/run test/test.c parser.o scanner.o compiler.o debug.o emit.o ast.o lut.o `pkg-config --cflags --libs check`
	./test/run

.PHONY: clean
clean:
	rm -f *.o
	rm -f scanner.c scanner.h
	rm -f parser.c parser.h parser.out
	rm -f lang lemon
