;-------------------------------------------------------------------------------
; MSP430 Assembler Code Template for use with TI Code Composer Studio
;
;
;-------------------------------------------------------------------------------
            .cdecls C,LIST,"msp430.h"       ; Include device header file
            
;-------------------------------------------------------------------------------
            .def    RESET                   ; Export program entry-point to
                                            ; make it known to linker.
;-------------------------------------------------------------------------------
            .text                           ; Assemble into program memory.
            .retain                         ; Override ELF conditional linking
                                            ; and retain current section.
            .retainrefs                     ; And retain any sections that have
                                            ; references to current section.

;-------------------------------------------------------------------------------
RESET       mov.w   #__STACK_END,SP         ; Initialize stackpointer
StopWDT     mov.w   #WDTPW|WDTHOLD,&WDTCTL  ; Stop watchdog timer


;-------------------------------------------------------------------------------
; Main loop here
;-------------------------------------------------------------------------------
;experimento 3 - visto 1

RT_TAM					.equ	6	;tamanho dos rotores
CONF1					.equ	1	;configura��o do rotor1


exp3:
	mov.w #MSG,R5	;R5 -> mensagem clara
	mov.w #GSM,R6	;R6 -> mensagem cifrada
	call #enigma3	;chamada da sub-rotina

	mov.w #GSM,R5	;R5 -> mensagem cifrada
	mov.w #DCF,R6	;R6 -> mensagem decifrada
	call #enigma3

init_enigma3:
	mov.w #RT1, R7
	mov.w #CONF1, R8
	mov.w #config_rotor, R15
	call  #app_rotor
	mov.w R15, R8 ; R8 = configura��o do rotor

enigma3:
	mov.b	@R5+,R7	;acha o primeiro caracter, incrementa e move para R7
	tst.b	R7		;testa R7
	jz fim			;chamada da sub-rotina para finalizar a execu��o
	mov.w #RT1,R8	;move RT1 para R8
	call #cifra		;chamada da sub-rotina

	mov.b R12,R7	;move o conte�do de R12 para R7 -> caracter cifrado
	mov.b R8,R9		;
	mov.w #RF1,R8	;move o conte�do do refletor para R8
	mov.w #RT1,R9	;move o conte�do do rotor para R9
	call #refletor	;chamada da sub-rotina

	call #caracter_refletido ;chamada da sub-rotina
	mov.b R12,0(R6)	;move o conte�do de R12 para a primeira posi��o de R6
	inc.w R6		;incrementa R6
	mov.w R9, R8 	;move R9 para R8
	jmp enigma3		;vai para o engima

cifra:
	push.b R7				;salva contexto do caracter a ser cifrado
	cmp.b LAST_LETTER,R7	;compara o caracter cifrado com a �ltima letra
	jge caracter_invalido	;Verifica se o caracter pode ser cifrado. Ele n�o pode ser cifrado se for R7 for maior do que a LAST_LETTER
	sub.b FIRST_LETTER,R7	;R7 = caracter - 'A'
	jn caracter_invalido	;caracter eh invalido se R7 for menor do que a FIRST_LETTER
	jmp caracter_valido		;vai para a sub-rotina de caracter_valido

caracter_valido:
	add.w R8,R7			;R7 = RT1[caracter - 'A']
	mov.b FIRST_LETTER,R12	;move FIRST_LETTER para R12
	add.b @R7,R12			;soma o conte�do de R7 em R12
	pop.b R7				;libera o contexto de R7
	jmp	fim					;vai para a sub-rotina fim

caracter_invalido:
	pop.b R7	;libera a pilha em R7
	mov.b R7,R12	;move R7 para R12
	jmp fim		;vai para a sub-rotina fim

refletor:
	push.w R8	;salva contexto para o refletor
	call #cifra	;chamada da sub-rotina da cifra
	mov.b R12,R7 ;move o conteudo de R12 para R7
	mov.w R9,R8	;move o conte�do para R8
	call #busca_elementos ;chama a sub-rotina da busca de elementos
	add.b FIRST_LETTER,R12 ;adiciona o primeito elemento em R12
	pop.w R8	;libera a pilha
	jmp fim	;vai para a sub-rotina fim

busca_elementos:
	push.w R7	;salva o contexto
	push.w R9
	push.w R10

	sub.b FIRST_LETTER,R7	;subtrai o conte�do
	mov.w #0,R9				;inicializa R9 com 0
	add.b LAST_LETTER,R9	;adiciona em R9
	sub.b FIRST_LETTER,R9	;subtrai em R9
	mov.w R8,R10			;move o elemento em R8 para R10
	mov.w #0,R12			;inicializa R12 com 0

loop:
	cmp.b R12,R9
	jz elemento_nao_encontrado	;se o elemento n�o for encontrado
	cmp.b 0(R10),R7				;compara o primeiro elemento de R10 com R7
	jz elemento_encontrado		;se o elemento for encontrado
	inc.w R10					;incrementa R10
	inc.w R12					;incrementa R12
	jmp loop					;volta para o loop

elemento_encontrado:

elemento_nao_encontrado:
	jmp	encontrar_elemento
	nop

encontrar_elemento:
	pop.w R10	;libera a pilha
	pop.w R9
	pop.w R7
	jmp fim		;vai para a sub-rotina fim

caracter_refletido:
app_rotor:
fim:
	ret


		.data

MSG: .byte "CABECAFEFACAFAD",0	;Mensagem em claro
GSM: .byte "XXXXXXXXXXXXXXX",0 	;Mensagem cifrada
DCF: .byte "XXXXXXXXXXXXXXX",0	;Mensagem decifrada
RT1: .byte 2, 4, 1, 5, 3, 0 	;Trama do Rotor 1
RF1: .byte 3, 5, 4, 0, 2, 1		;Trama do Refletor
config_rotor: .byte 0, 0, 0, 0, 0, 0 ; rotor configurado
FIRST_LETTER: .byte 0x41
LAST_LETTER:  .byte 0x47


                                            

;-------------------------------------------------------------------------------
; Stack Pointer definition
;-------------------------------------------------------------------------------
            .global __STACK_END
            .sect   .stack
            
;-------------------------------------------------------------------------------
; Interrupt Vectors
;-------------------------------------------------------------------------------
            .sect   ".reset"                ; MSP430 RESET Vector
            .short  RESET
            
