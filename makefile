LIBS = ./raylib-6.0_linux_amd64/lib/libraylib.a
FLAGS = -Wall -Wextra
OUT=qcfs

$(OUT): main.c makefile *.h
	gcc main.c $(FLAGS) -o $@ $(LIBS) -lm -lX11 

install: ~/.local/bin/$(OUT)

~/.local/bin/$(OUT): $(OUT)
	@ mkdir -p ~/.local/bin/
	cp $^ $@

