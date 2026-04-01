src = $(wildcard *.c)
targets = $(patsubst %.c, %, $(src))

ALL: $(targets)

%: %.c
	gcc $< -o $@ -Wall -g

clean:
	rm -f $(targets)

.PHONY: all clean
