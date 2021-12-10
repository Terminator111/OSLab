run:
	@nasm -f elf32 -o my_print.o my_print.asm
	@g++ -g -m32 -std=c++11 main.cpp my_print.o -o final
	@./final

.PHONY : clean
clean:
	-rm final *.o
