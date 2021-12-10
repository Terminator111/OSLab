run: 
	@nasm -f elf -g main.asm
	@ld -m elf_i386 main.o -o main
	@./main

clean:
	rm main *.o