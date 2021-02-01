// This file is part of www.nand2tetris.org
// and the book "The Elements of Computing Systems"
// by Nisan and Schocken, MIT Press.
// File name: projects/04/Fill.asm

// Runs an infinite loop that listens to the keyboard input. 
// When a key is pressed (any key), the program blackens the screen,
// i.e. writes "black" in every pixel. When no key is pressed, the
// program clears the screen, i.e. writes "white" in every pixel.

//Setting the number of screen words for looping later
@8192
D=A // D = 32 words (512 pixels) * 256 screen dimensions
@screensize
M=D // screensize = 512 * 256

(LOOP) // while(true)
	@color
	M=0 // color = 0
	
	@KBD
	D=M // D = Keyboard
	
	@FILL
	D; JEQ // if keyboard == 0 goto fill
	
	@0
	D=A-1
	@color
	M=D // else color = -1

(FILL)
	@i
	M=0 // i = 0
	
(FILLLOOP)
	@screensize
	D=M 
	@i
	D=D-M // D = screensize - i
	
	@LOOP
	D;JLE // if(screensize - i <=0) goto LOOP
	
	@SCREEN
	D=A
	
	@i
	D=D+M
	
	@pixel
	M=D // pixel = screen + i
	
	@color
	D=M
	
	@pixel
	A=M
	M=D // M[pixel] = color
	
	@i
	M=M+1 // i += 1
	
	@FILLLOOP
	0;JMP // goto FILLLOOP
