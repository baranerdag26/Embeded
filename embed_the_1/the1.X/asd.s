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
    
    incf counter
    
    
    
main_loop:
    call check_buttons
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
    movlw 11111111B
    cpfseq b_final
    goto cont_b
    clrf b_final;PORTB
    setf debug_flag_b
    goto hereson
    cont_b:
	bsf b_final, 0
	rlncf b_final
	bsf b_final, 0
	tstfsz debug_flag_b
	bcf b_final, 1
	clrf debug_flag_b
	goto hereson

end_b:
    clrf b_final 
    setf debug_flag_b
    goto hereson
    
;-------------------------    
    
re0_pressed:
    setf btn_flag
    return
  
re0_released:
    clrf btn_flag
    comf c_is_active
    return
    
start_c:
    movlw 11111111B
    cpfseq c_final
    goto cont
    clrf c_final
    setf debug_flag
    goto hereb
    cont:
	bsf c_final, 7
	rrncf c_final
	bsf c_final, 7
	tstfsz debug_flag
	bcf c_final, 6
	clrf debug_flag
	goto hereb
    
end_c:
    clrf c_final  ; Clear PORTC
    setf debug_flag
    goto hereb
    
;------------------------------   
 
check_buttons:
    btfsc PORTE, 0	; check RE0, skip if 0
    call re0_pressed
    tstfsz btn_flag
    goto re0_released
    
    btfsc PORTE, 1
    goto re1_pressed
    tstfsz btn_flag_b
    goto re1_released
    
    return

update_display:
    incfsz counter
    return
    goto counter_overflowed

adjust_light:
    tstfsz light_flag
    goto turn_off_light
    goto turn_on_light
    here:   
	tstfsz c_is_active	; ?
	goto start_c	; ?
	goto end_c
	hereb:    
	    tstfsz b_is_active
	    goto start_b
	    goto end_b
	    goto hereson
	    
	 
    
;--------------------------------	
	
turn_on_light:
    movlw 00000001B
    movwf PORTD
    incf light_flag
    goto here
    
    

turn_off_light:
    clrf PORTD
    decf light_flag
    goto here

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
  
counter_overflowed:
    movlw 252     
    movwf var3      
    outer_most_loop_start1:
	infsnz var3
	goto adjust_light
	movlw 40
	movwf var2
	outer_loop_start1:
	    setf var1
	    loop_start1:
		decf var1
		bnz loop_start1
	    incfsz var2
	    bra outer_loop_start1
	    goto outer_most_loop_start1 
	
	    
end resetVec