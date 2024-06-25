PROCESSOR 18F8722

#include <xc.inc>

; CONFIGURATION (DO NOT EDIT)
; CONFIG1H
CONFIG OSC = HSPLL      ; Oscillator Selection bits (HS oscillator, PLL enabled (Clock Frequency = 4 x FOSC1))
CONFIG FCMEN = OFF      ; Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor disabled)
CONFIG IESO = OFF       ; Internal/External Oscillator Switchover bit (Oscillator Switchover mode disabled)
; CONFIG2L
CONFIG PWRT = OFF       ; Power-up Timer Enable bit (PWRT disabled)
CONFIG BOREN = OFF      ; Brown-out Reset Enable bits (Brown-out Reset disabled in hardware and software)
; CONFIG2H
CONFIG WDT = OFF        ; Watchdog Timer Enable bit (WDT disabled (control is placed on the SWDTEN bit))
; CONFIG3H
CONFIG LPT1OSC = OFF    ; Low-Power Timer1 Oscillator Enable bit (Timer1 configured for higher power operation)
CONFIG MCLRE = ON       ; MCLR Pin Enable bit (MCLR pin enabled; RE3 input pin disabled)
; CONFIG4L
CONFIG LVP = OFF        ; Single-Supply ICSP Enable bit (Single-Supply ICSP disabled)
CONFIG XINST = OFF      ; Extended Instruction Set Enable bit (Instruction set extension and Indexed Addressing mode disabled (Legacy mode))
CONFIG DEBUG = OFF      ; Disable In-Circuit Debugger


GLOBAL var1
GLOBAL var2
GLOBAL var3
GLOBAL result
GLOBAL counter
GLOBAL light_flag
GLOBAL btn_flag
GLOBAL c_is_active
GLOBAL counter_c 
GLOBAL debug_flag
GLOBAL debug_flag_b
GLOBAL b_is_active
GLOBAL btn_flag_b
GLOBAL b_final
GLOBAL c_final
    
; Define space for the variables in RAM
PSECT udata_acs
var1:
    DS 1 ; Allocate 1 byte for var1
var2:
    DS 1 
var3: 
    DS 1
counter:
    DS 1 
temp_result:
    DS 1   
result: 
    DS 1
light_flag:
    DS 1
btn_flag:
    DS 1
c_is_active:
    DS 1
counter_c:
    DS 1
debug_flag:
    DS 1
debug_flag_b:
    DS 1
b_is_active:
    DS 1
btn_flag_b:
    DS 1
b_final:
    DS 1
c_final:
    DS 1


PSECT resetVec,class=CODE,reloc=2
resetVec:
    goto       main

PSECT CODE
main:
    clrf var1	; var1 = 0		
    clrf var2   ; var2 = 0		
    clrf var3
    clrf result ; result = 0
    clrf counter
    clrf light_flag
    clrf btn_flag
    clrf c_is_active
    clrf counter_c
    setf debug_flag
    setf debug_flag_b
    clrf b_is_active
    clrf btn_flag_b
    clrf b_final
    clrf c_final
    
    clrf TRISB
    clrf TRISC
    clrf TRISD
    setf TRISE ; port e is input, set
    
    setf PORTB
    setf LATC
    setf LATD
    
    call busy_wait 
	
    clrf PORTB
    clrf LATC
    clrf LATD
    clrf TRISE
    
    ;incf counter
    
    
    
main_loop:
    
    movlw 252     
    movwf var3      
    outer_most_loop_start1:
	infsnz var3
	goto continue
	movlw 40
	movwf var2
	outer_loop_start1:
	    setf var1
	    loop_start1:
	    call check_buttons
	    decf var1
		bnz loop_start1
	    incfsz var2
	    bra outer_loop_start1
	    goto outer_most_loop_start1 

    continue:
	call update_display
	goto main_loop
;------------------------- 
    
re1_pressed:
    setf btn_flag_b
    return
  
re1_released:
    clrf btn_flag_b
    comf b_is_active
    return
    
start_b:
    movff PORTB, b_final
    movlw 11111111B
    cpfseq b_final	    
    goto cont_b
    clrf b_final;PORTB
    setf debug_flag_b
    movff b_final, PORTB
    return
    cont_b:			; normal durum
	bsf b_final, 0
	rlncf b_final
	bsf b_final, 0
	tstfsz debug_flag_b
	bcf b_final, 1
	clrf debug_flag_b
	movff b_final, PORTB
	return

end_b:
    clrf PORTB 
    clrf b_is_active
    clrf b_final
    setf debug_flag_b
    return
    
;-------------------------    
    
re0_pressed:
    setf btn_flag
    return
  
re0_released:
    clrf btn_flag
    comf c_is_active
    return
    
start_c:
    movff PORTC, c_final
    movlw 11111111B
    cpfseq c_final	    
    goto cont_c
    clrf c_final;PORTc
    setf debug_flag
    movff c_final, PORTC
    return
    cont_c:			; normal durum
	bsf c_final, 0
	rlncf c_final
	bsf c_final, 0
	tstfsz debug_flag
	bcf c_final, 1
	clrf debug_flag
	movff c_final, PORTC
	return
    
end_c:
    clrf PORTC
    clrf c_is_active
    clrf c_final
    setf debug_flag
    return
    
;------------------------------   
 
check_buttons:
    btfsc PORTE, 0	; check RE0, skip if 0
    call re0_pressed
    tstfsz btn_flag
    call re0_released
    
    btfsc PORTE, 1
    call re1_pressed
    tstfsz btn_flag_b
    call re1_released
    
    return

    
update_display:
    tstfsz light_flag
    call turn_off_light
    call turn_on_light
    tstfsz c_is_active	; ?
    call start_c	; ?
    call end_c
    tstfsz b_is_active
    call start_b
    tstfsz b_is_active
    return
    call end_b
    return	
	
turn_on_light:
    movlw 00000001B
    movwf PORTD
    incf light_flag
    return
    
    

turn_off_light:
    clrf PORTD
    decf light_flag
    return

hereson:
    movff b_final, PORTB
    movff c_final, PORTC
    return
    
    
busy_wait:
    movlw 250       
    movwf var3      
    outer_most_loop_start:
	infsnz var3
	return
	clrf var2
	outer_loop_start:
	    setf var1
	    loop_start:
		decf var1
		bnz loop_start
	    incfsz var2
	    bra outer_loop_start
	    goto outer_most_loop_start
  

	
	    
end resetVec