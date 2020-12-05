;kernel.asm
;nasm directive - 32 bit

bits 32
section .text
  ;multiboot specification
  align 4
  dd 0x1BADB002               ;magic number
  dd 0x00                     ;flag
  dd - (0x1BADB002 + 0x00)    ;check summa. mn + f + ch == 0 should be zero

global start
global keyboardHandler
global readPort
global writePort
global loadIDT

extern kernelMain           ;kernel.c
extern keyboardHandlerMain  

readPort:
  mov   edx, [esp + 4]
  in    al, dx ;al - lower 8 bits of eax, dx -  lower 16 bits of edx
  ret

writePort:
  mov   edx, [esp + 4]
  mov   al, [esp + 4 + 4]
  out   dx, al
  ret

loadIDT:
  mov   edx, [esp + 4]
  lidt  [edx]
  sti
  ret

keyboardHandler:
  call  keyboardHandlerMain
  iretd  

start:
  cli                     ;interrupt blocking
  mov   esp, stack_space  ;stack pointer
  call  kernelMain
  hlt                     ;stopping the processor

section .bss
  resb 8192               ;8 kilobytes per stack
stack_space:

;nasm -f elf32 kernel.asm -o kasm.o
