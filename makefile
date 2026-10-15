OBJ := $(patsubst %.c, %.o, $(wildcard *.c))

mp3_tag: $(OBJ)
	gcc -o $@ $^

clean:
	rm -f *.exe *.o