.section .data
MEM_FD:          .asciz "/dev/mem"                @ Caminho para o arquivo /dev/mem
FPGA_BRIDGE:     .word 0xff200                    @ endereco da ponte
HW_REGS_SPAN:          .word 0x100                @ tamanho da pagina do mapeamento (256 kb)
dataA: .word 0x80                                 @Barramento do DATA_A
dataB: .word 0x70                                 @Barramento do DATA_B
ADRESS_MAPPED:  .space   4
ADRESS_FD:      .space   4

  .section .rodata
msg_mmap_error:      .asciz "Mapping error\n"
msg_mmap_sucess:     .asciz "Successful mapping\n"
mmsg_mmap_error2:    .asciz "Mmap error\n"

  .section .text
  .global _start
  .type _start, %function

  .global mapm
  .type mapm, %function

  .global draw_box
  .type draw_box, %function

  .global draw_text
  .type draw_text, %function

  .global limpar_tela
  .type limpar_tela, %function

  .global button_pressed
  .type button_pressed, %function

  .global draw_polygon
  .type draw_polygon, %function

  .global draw_sprite
  .type draw_sprite, %function

  .global set_sprite
  .type set_sprite, %function
  
mapm:

  sub sp, sp, #28
  str lr, [sp, #24]
  str r7, [sp, #20]
  str r5, [sp, #16]
  str r3, [sp, #12]
  str r2, [sp, #8]
  str r1, [sp, #4]
  str r0, [sp, #0]


  @abre o arquivo /dev/mem
  MOV r7, #5          @ syscall open
  LDR r0, =MEM_FD     @ caminho do arquivo
  MOV r1, #2          @ para leitura e escrita 
  MOV r2, #0          @ sem flags
  SWI 0               @ chama o sistema para executar

  ldr r1, =ADRESS_FD
  str r0, [r1]
  mov r4, r0              @guarda em r4

  @ configurar o mmap
  mov r7, #192        @ syscall do mmap2
  mov r0, #0          @ para deixar o kernel decidir o enderço virtual
  ldr r1, =HW_REGS_SPAN @ tamanho da pagina
  ldr r1, [r1]
  mov r2, #3          @ leitura/escrita
  mov r3, #1          @ compartilhado com outros processos
  ldr r5, =FPGA_BRIDGE @carrega o endereço base da FPGA 
  ldr r5, [r5]        @ carrega o valor real do enderço da FPGA
  svc 0               @ kernel é chamado para executar a syscall

  ldr r1, =ADRESS_MAPPED  @endereco e carregado aqui
  str r0, [r1]

  ldr lr, [sp, #24]
  ldr r7, [sp, #20]
  ldr r5, [sp, #16]
  ldr r3, [sp, #12]
  ldr r2, [sp, #8]
  ldr r1, [sp, #4]
  ldr r0, [sp, #0]
  add sp, sp, #28

  bx lr


draw_polygon:
  sub sp, sp, #36
  str lr, [sp, #32]
  str r8, [sp, #28]
  str r6, [sp, #24]
  str r5, [sp, #20]
  str r4, [sp, #16]
  str r3, [sp, #12]
  str r2, [sp, #8]
  str r1, [sp, #4]
  str r0, [sp, #0]   
  ldr r3, =ADRESS_MAPPED
  ldr r3, [r3]
   
  bl verifica_fifo

  @Zera o start
  mov r0, #0
  strd r0, [r3, #0xc0]

  @Data A
  mov r1, #0
  lsl r1, r1, #4
  mov r0, #0b0011 @opcode -> DP
  add r1, r1, r0
  str r1, [r3, #0x80]

  @Data B 
  ldr r2, [sp, #0] @forma
  lsl r2, r2, #31
  ldr r4, [sp, #4] @RGB
  lsl r4, r4, #22
  add r2, r2, r4
  ldr r5, [sp, #8] @tamanho
  lsl r5, r5, #18
  add r2, r2, r5
  ldr r6, [sp, #12] @posição y
  lsl r6, r6, #9
  add r2, r2, r6  
  ldr r8, [sp, #36] @posição x
  add r2, r2, r8
  str r2, [r3, #0x70] 


  @Inicia o start para escrever na tela
  mov r0, #1
  strd r0, [r3, #0xc0]

  ldr lr, [sp, #32]
  ldr r8, [sp, #28]
  ldr r6, [sp, #24]
  ldr r5, [sp, #20]
  ldr r4, [sp, #16]
  ldr r3, [sp, #12]
  ldr r2, [sp, #8]
  ldr r1, [sp, #4]
  ldr r0, [sp, #0]
  add sp, sp, #36


  bx lr

draw_sprite:
  sub sp, sp, #32
  str lr, [sp, #28]
  str r6, [sp, #24]
  str r5, [sp, #20]
  str r4, [sp, #16]
  str r3, [sp, #12]
  str r2, [sp, #8]
  str r1, [sp, #4]
  str r0, [sp, #0]   
  ldr r3, =ADRESS_MAPPED
  ldr r3, [r3]
   
  bl verifica_fifo

  @Zera o start
  mov r0, #0
  strd r0, [r3, #0xc0]

  @Data A
  ldr r1, [sp, #0]
  lsl r1, r1, #4 @Registrador
  mov r0, #0b0000 @opcode -> WBR
  add r1, r1, r0
  str r1, [r3, #0x80]

  @Data B 
  ldr r2, [sp, #4] @Sp
  lsl r2, r2, #29
  ldr r4, [sp, #8] @Coordenada X
  lsl r4, r4, #19
  add r2, r2, r4
  ldr r5, [sp, #12] @Coordenada Y
  lsl r5, r5, #9
  add r2, r2, r5
  ldr r6, [sp, #32] @Offset
  add r2, r2, r6
  str r2, [r3, #0x70] 


  @Inicia o start para escrever na tela
  mov r0, #1
  strd r0, [r3, #0xc0]

  ldr lr, [sp, #28]
  ldr r6, [sp, #24]
  ldr r5, [sp, #20]
  ldr r4, [sp, #16]
  ldr r3, [sp, #12]
  ldr r2, [sp, #8]
  ldr r1, [sp, #4]
  ldr r0, [sp, #0]
  add sp, sp, #32

  bx lr

set_sprite:
  sub sp, sp, #20
  str lr, [sp, #16]
  str r3, [sp, #12]
  str r2, [sp, #8]
  str r1, [sp, #4]
  str r0, [sp, #0]   
  ldr r3, =ADRESS_MAPPED
  ldr r3, [r3]
   
  bl verifica_fifo

  @Zera o start
  mov r0, #0
  strd r0, [r3, #0xc0]

  @Data A
  ldr r1, [sp, #0]
  lsl r1, r1, #12 @Endereco de memoria
  mov r0, #0b0001 @opcode -> WBR
  add r1, r1, r0
  str r1, [r3, #0x80]

  @Data B 
  ldr r2, [sp, #4] @RGB
  str r2, [r3, #0x70]


  @Inicia o start para escrever na tela
  mov r0, #1
  strd r0, [r3, #0xc0]

  ldr lr, [sp, #16]
  ldr r3, [sp, #12]
  ldr r2, [sp, #8]
  ldr r1, [sp, #4]
  ldr r0, [sp, #0]
  add sp, sp, #20

  bx lr


draw_box:
  sub sp, sp, #24
  str lr, [sp, #20]
  str r3, [sp, #16]
  str r7, [sp, #12]
  str r2, [sp, #8]
  str r1, [sp, #4]
  str r0, [sp, #0]
  ldr r3, =ADRESS_MAPPED
  ldr r3, [r3]

  @Desenha o primeiro bloco
  bl verifica_fifo

  @Zera o start
  mov r0, #0
  strd r0, [r3, #0xc0]

  @dataA
  mov r0, #0b0010 @opcode -> WBM
  ldr r1, [sp, #0] @Bloco escolhido
  lsl r1, r1, #4
  add r1, r1, r0
  str r1, [r3, #0x80]

  @dataB
  ldr r2, [sp, #4] @RGB
  str r2, [r3, #0x70]

  @Inicia o start para escrever na tela
  mov r0, #1
  strd r0, [r3, #0xc0]

  @Desenhar o segundo bloco
  bl verifica_fifo

  @Zera o start
  mov r0, #0
  strd r0, [r3, #0xc0]

  @dataA
  mov r0, #0b0010 @opcode -> WBM
  ldr r1, [sp, #0] @Bloco escolhido
  add r1, r1, #1
  lsl r1, r1, #4
  add r1, r1, r0
  str r1, [r3, #0x80]

  @dataB
  ldr r2, [sp, #4] @RGB
  str r2, [r3, #0x70]

  @Inicia o start para escrever na tela
  mov r0, #1
  strd r0, [r3, #0xc0]

  @Desenhar o terceiro bloco
  bl verifica_fifo

  @Zera o start
  mov r0, #0
  strd r0, [r3, #0xc0]

  @dataA
  mov r0, #0b0010 @opcode -> WBM
  ldr r1, [sp, #0] @Bloco escolhido
  add r1, r1, #80
  lsl r1, r1, #4
  add r1, r1, r0
  str r1, [r3, #0x80]

  @dataB
  ldr r2, [sp, #4] @RGB
  str r2, [r3, #0x70]

  @Inicia o start para escrever na tela
  mov r0, #1
  strd r0, [r3, #0xc0]

  @Desenhar o quarto bloco
  bl verifica_fifo

  @Zera o start
  mov r0, #0
  strd r0, [r3, #0xc0]

  @dataA
  mov r0, #0b0010 @opcode -> WBM
  ldr r1, [sp, #0] @Bloco escolhido
  add r1, r1, #81
  lsl r1, r1, #4
  add r1, r1, r0
  str r1, [r3, #0x80]

  @dataB
  ldr r2, [sp, #4] @RGB
  str r2, [r3, #0x70]

  @Inicia o start para escrever na tela
  mov r0, #1
  strd r0, [r3, #0xc0]

  ldr lr, [sp, #20]
  ldr r3, [sp, #16]
  ldr r7, [sp, #12]
  ldr r2, [sp, #8]
  ldr r1, [sp, #4]
  ldr r0, [sp, #0]
  add sp, sp, #24

  bx lr

draw_text:
  sub sp, sp, #24
  str lr, [sp, #20]
  str r3, [sp, #16]
  str r7, [sp, #12]
  str r2, [sp, #8]
  str r1, [sp, #4]
  str r0, [sp, #0]
  ldr r3, =ADRESS_MAPPED
  ldr r3, [r3]

  @Desenha o primeiro bloco
  bl verifica_fifo

  @Zera o start
  mov r0, #0
  strd r0, [r3, #0xc0]

  @dataA
  mov r0, #0b0010 @opcode -> WBM
  ldr r1, [sp, #0] @Bloco escolhido
  lsl r1, r1, #4
  add r1, r1, r0
  str r1, [r3, #0x80]

  @dataB
  ldr r2, [sp, #4] @RGB
  str r2, [r3, #0x70]

  @Inicia o start para escrever na tela
  mov r0, #1
  strd r0, [r3, #0xc0]

  ldr lr, [sp, #20]
  ldr r3, [sp, #16]
  ldr r7, [sp, #12]
  ldr r2, [sp, #8]
  ldr r1, [sp, #4]
  ldr r0, [sp, #0]
  add sp, sp, #24

  bx lr

limpar_tela:
  sub sp, sp, #28
  str lr, [sp, #24]
  str r3, [sp, #20]
  str r5, [sp, #16]
  str r4, [sp, #12]
  str r2, [sp, #8]
  str r1, [sp, #4]
  str r0, [sp, #0]
  ldr r3, =ADRESS_MAPPED
  ldr r3, [r3]

  mov r4, #4800 @número de linhas
  mov r5, #0b0 @contador de pixel

loop:
  bl verifica_fifo
  @Zera o start
  mov r0, #0
  strd r0, [r3, #0xc0]

  @dataA
  mov r0, #0b0010 @opcode -> WBM
  mov r1, r5 @Bloco escolhido
  lsl r1, r1, #4
  add r1, r1, r0
  str r1, [r3, #0x80]

  @dataB
  mov r0, #0b000 @R
  mov r1, #0b000 @G
  mov r2, #0b000 @B
  lsl r2, r2, #6
  lsl r1, r1, #3
  add r2, r2, r1
  add r2, r2, r0
  str r2, [r3, #0x70]

  @Inicia o start para escrever na tela
  mov r0, #1
  strd r0, [r3, #0xc0]

  add r5, r5, #0b1  @Incrementar 1 no valor que representa o pixel

  sub r4, r4, #1
  cmp r4, #0
  bgt loop

  ldr lr, [sp, #24]
  ldr r3, [sp, #20]
  ldr r5, [sp, #16]
  ldr r4, [sp, #12]
  ldr r2, [sp, #8]
  ldr r1, [sp, #4]
  ldr r0, [sp, #0]
  add sp, sp, #28

  bx lr 

verifica_fifo:
  ldr r0, [r3, #0xb0]
  cmp r0, #0
  bne verifica_fifo

  bx lr

button_pressed:         @botão 1 = 7 // botão 2 = 11 // botão 3 = 13 // botão 4 = 14 // nada = 15
  sub sp, sp, #4
  str r1, [sp, #0]

  ldr r1, =ADRESS_MAPPED
  ldr r1, [r1]
  ldr r0, [r1, #0x0]

  ldr r1, [sp, #0]
  add sp, sp, #4
  
  bx lr
