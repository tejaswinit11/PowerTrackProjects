OBJ := $(patsubst %.c, %.o, $(wildcard *.c))

lexical_analyser: $(OBJ)
	gcc -o $@ $^

clean:
	rm -f *.exe *.o