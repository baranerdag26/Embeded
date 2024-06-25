#include <xc.h>
#include <stdio.h>
#include <string.h>
#include "pragmas.h"

/**
 *
 * Defines LED and button macros, and button state variables.
 *
 * This file contains preprocessor macros for controlling LEDs and reading button
 * states, as well as variables to track the state of each button.
 *
 * The LED macros provide a convenient way to set the state of each LED. The
 * BUTTON_STATE_IDLE and BUTTON_STATE_PRESSED macros define the possible states
 * for each button.
 *
 * The button state variables track the current state of each button, which can
 * be used to detect button presses and releases.
 */
#define LED_CLEAR       \
    LATDbits.LATD0 = 0; \
    LATCbits.LATC0 = 0; \
    LATBbits.LATB0 = 0; \
    LATAbits.LATA0 = 0;

#define LED_1 LATDbits.LATD0
#define LED_2 LATCbits.LATC0
#define LED_3 LATBbits.LATB0
#define LED_4 LATAbits.LATA0

#define BUTTON_1 PORTBbits.RB4
#define BUTTON_2 PORTBbits.RB5
#define BUTTON_3 PORTBbits.RB6
#define BUTTON_4 PORTBbits.RB7

// Button states
#define BUTTON_STATE_IDLE 0
#define BUTTON_STATE_PRESSED 1

// Button state variables
unsigned char button1_state = BUTTON_STATE_IDLE;
unsigned char button2_state = BUTTON_STATE_IDLE;
unsigned char button3_state = BUTTON_STATE_IDLE;
unsigned char button4_state = BUTTON_STATE_IDLE;

int prs4_flag = 0;
int prs5_flag = 0;
int prs6_flag = 0;
int prs7_flag = 0;

/**
 * Defines the system clock frequency as 40 MHz.
 *
 * This macro sets the system clock frequency to 40 MHz, which is used by various
 * peripheral modules and functions that require a clock reference.
 */
#define _XTAL_FREQ 40000000 // 40 MHz

/**
 * Defines the possible states for the application.
 * The current state is tracked using the `current_state` variable.
 */
enum STATE
{
    IDLE,
    ACTIVE,
    END
};

enum STATE current_state = IDLE;

unsigned int total_distance = 0;
unsigned int remaining_distance = 0;
unsigned int current_speed = 0;
unsigned int adc_value = 0;
unsigned char manual_control = 0;
unsigned char button_state = 0x00; // Initial state of PORTB (all
int timer_counter = 0;
int altitude_flag = 0;
unsigned int altitude_period = 0;

// buttons not pressed)

// Function prototypes
void initUSART(void);
void initADC(void);
void initButtons(void);
void sendUSART(char *message);
void processCommand(char *command);
void sendDistanceResponse(void);
void sendAltitudeResponse(void);
void handleButtonPress(unsigned char button);
void __interrupt() ISR(void);

/**
 * The main function initializes the USART, ADC, and buttons, then enters an infinite loop.
 */
void main(void)
{
    initUSART();
    initADC();
    initButtons();

    while (1)
    {
    }
}

/**
 *  Initializes the USART peripheral for asynchronous communication at 115200 baud.
 *
 * This function configures the USART peripheral to operate in asynchronous mode at 115200 baud. It sets up the baud rate generator, enables reception and transmission, and enables the receive interrupt. It also configures Timer0 to be used for other purposes.
 */
void initUSART(void)
{
    TXSTAbits.SYNC = 0;    // Asynchronous mode
    TXSTAbits.BRGH = 1;    // High-speed mode
    BAUDCONbits.BRG16 = 0; // 8-bit baud rate generator
    SPBRG = 21;            // Baud rate setting for 115200 bps with
                           // 40MHz clock

    RCSTAbits.SPEN = 1; // Enable serial port
    RCSTAbits.CREN = 1; // Enable reception
    TXSTAbits.TXEN = 1; // Enable transmission

    INTCONbits.GIE = 1;  // Enable global interrupts
    INTCONbits.PEIE = 1; // Enable peripheral interrupts
    PIE1bits.RCIE = 1;   // Enable receive interrupt
    PIE1bits.TXIE = 0;   // Disable transmit interrupt

    // TIMER
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    INTCONbits.TMR0IF = 0; // Timer0 Overflow Interrupt Flag
    INTCONbits.TMR0IE = 0;
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // T0CON
    T0CONbits.TMR0ON = 1; // Timer0 On/Off Control
    T0CONbits.T08BIT = 0; // Timer0 8-bit/16-bit Control
    T0CONbits.T0CS = 0;   // Timer0 Clock Source Select
    T0CONbits.T0SE = 0;   // Timer0 Source Edge Select
    T0CONbits.PSA = 0;    // Timer0 Prescaler Assignment
    T0CONbits.T0PS2 = 1;  // Timer0 Prescaler Select bits
    T0CONbits.T0PS1 = 0;  // Timer0 Prescaler Select bits
    T0CONbits.T0PS0 = 1;  // Timer0 Prescaler Select bits

    // Initial Timer0 Value
    TMR0L = 0xFF;
    TMR0H = 0xC2;

    IPR1bits.RC1IP = 1;
}

/**
 * Sends a null-terminated string to the USART peripheral.
 *
 * The null-terminated string to be transmitted.
 */
void sendUSART(char *message)
{
    while (*message != '\0')
    {
        while (!TXSTAbits.TRMT)
            ;             // Wait until the transmit shift
                          // register is empty
        TXREG = *message; // Transmit character
        message++;
    }
}

/**
 * Initializes the Analog-to-Digital Converter (ADC) module.
 *
 * This function configures the ADC module to use channel 12 (AN12) as the input
 * and sets all analog pins. It also enables the ADC interrupt and clears the
 * ADC interrupt flag.
 */
void initADC(void)
{
    TRISH = 0x10; // AN12 input RH4
    // Configure ADC
    ADCON0 = 0x31; // Channel 12; Turn on AD Converter
    ADCON1 = 0x0F; // All analog pins
    ADCON2 = 0xA2;
    ADRESH = 0x00;
    ADRESL = 0x00;

    PIE1bits.ADIE = 1; // Enable ADC interrupt
    PIR1bits.ADIF = 0; // Clear ADC interrupt flag
}

/**
 * Sends the distance response message over the USART interface.
 *
 * This function is responsible for updating the remaining distance and
 * formatting the distance message to be sent over the USART interface.
 * The message is formatted as "$DST%04X#" where the 4-digit hexadecimal
 * value represents the remaining distance.
 */

void sendDistanceResponse(void)
{

    remaining_distance -= current_speed;
    char distance_msg[12];
    sprintf(distance_msg, "$DST%04X#", remaining_distance);
    sendUSART(distance_msg);
}

/**
 * Sends the altitude response message over the USART interface.
 * The altitude value is determined based on the current ADC reading,
 * and the message is formatted as "$ALT<4-digit-hex>#".
 */
void sendAltitudeResponse(void)
{

    unsigned int altitude;
    if (adc_value >= 768)
    {
        altitude = 12000;
    }
    else if (adc_value >= 512)
    {
        altitude = 11000;
    }
    else if (adc_value >= 256)
    {
        altitude = 10000;
    }
    else
    {
        altitude = 9000;
    }
    char altitude_msg[9];
    sprintf(altitude_msg, "$ALT%04X#", altitude);

    sendUSART(altitude_msg);
    remaining_distance -= current_speed;
}

/**
 * Initializes the button inputs and related peripherals.
 *
 * This function sets up the necessary configuration for the button inputs and
 * related peripherals, such as:
 * - Configuring PORTB pins RB4-RB7 as inputs
 * - Clearing the LED
 * - Configuring PORTD0, PORTC0, PORTB0, and PORTA0 as outputs
 * - Enabling PORTB pull-ups
 * - Disabling PORTB change interrupt
 * - Clearing various interrupt flags
 * - Enabling Timer0 overflow interrupt
 * - Initializing the button state variables to BUTTON_STATE_IDLE
 */
void initButtons(void)
{
    TRISB = 0xF0; // Set RB4-RB7 as inputs

    LED_CLEAR;
    LATB = PORTB;
    TRISDbits.TRISD0 = 0;
    TRISCbits.TRISC0 = 0;
    TRISBbits.TRISB0 = 0;
    TRISAbits.TRISA0 = 0;

    INTCON2bits.RBPU = 0; // Enable PORTB pull-ups
    INTCONbits.RBIE = 0;  // Enable PORTB change interrupt
    INTCONbits.RBIF = 0;  // Clear PORTB change interrupt flag
    PIR1bits.RCIF = 0;    // Clear receive interrupt flag
    PIR1bits.ADIF = 0;    // Clear ADC interrupt flag
    INTCONbits.TMR0IE = 1;
    INTCONbits.TMR0IF = 0; // Timer0 Overflow Interrupt Flag

    button1_state = BUTTON_STATE_IDLE;
    button2_state = BUTTON_STATE_IDLE;
    button3_state = BUTTON_STATE_IDLE;
    button4_state = BUTTON_STATE_IDLE;
}

/**
 * Handles a button press event by sending a message to the USART interface.
 *
 * The button that was pressed, represented as an unsigned char value.
 */
void handleButtonPress(unsigned char button)
{
    char button_msg[7];
    sprintf(button_msg, "$PRS%02X#", button);
    sendUSART(button_msg);
}

int adc_flag = 0;

/**
 * Interrupt Service Routine (ISR) that handles various interrupt sources, including:
 * - UART receive overflow error
 * - UART receive interrupt
 * - ADC conversion complete interrupt
 * - PORTB change interrupt
 * - Timer0 overflow interrupt
 *
 * This ISR is responsible for processing received UART data, handling button presses,
 * managing the ADC conversion, and updating the timer-related functionality.
 */
void __interrupt() ISR(void)
{
    if (RCSTA1bits.OERR)
    {
        RCSTA1bits.CREN = 0;
        RCSTA1bits.CREN = 1;
    }
    if (PIR1bits.RCIF)
    {
        // Handle received character

        PIR1bits.RCIF = 0; // Clear receive interrupt flag
        char received = RCREG;
        static char command[10];
        static unsigned char index = 0;

        if (received == '$')
        {
            index = 0;
        }

        command[index++] = received;

        if (received == '#')
        {
            command[index] = '\0';

            processCommand(command);
        }
    }

    if (PIR1bits.ADIF)
    {
        PIR1bits.ADIF = 0; // Clear ADC interrupt flag

        // Read ADC result
        adc_value = (ADRESH << 8) | ADRESL;
        adc_flag = 1;
    }

    if (INTCONbits.RBIF)
    {
        volatile uint8_t dummy = PORTB; // Read PORTB to clear RBIF
        LATB = LATB;
        INTCONbits.RBIF = 0; // Clear PORTB change interrupt flag

        if (manual_control)
        {
            // Button 1
            if (BUTTON_1 == 0 && button1_state == BUTTON_STATE_IDLE)
            {
                button1_state = BUTTON_STATE_PRESSED;
            }
            else if (BUTTON_1 == 1 && button1_state == BUTTON_STATE_PRESSED)
            {
                button1_state = BUTTON_STATE_IDLE;
                prs4_flag = 1;
                LED_1 = 0;
            }

            // Button 2
            if (BUTTON_2 == 0 && button2_state == BUTTON_STATE_IDLE)
            {
                button2_state = BUTTON_STATE_PRESSED;
            }
            else if (BUTTON_2 == 1 && button2_state == BUTTON_STATE_PRESSED)
            {
                button2_state = BUTTON_STATE_IDLE;
                prs5_flag = 1;
                LED_2 = 0;
            }

            // Button 3
            if (BUTTON_3 == 0 && button3_state == BUTTON_STATE_IDLE)
            {
                button3_state = BUTTON_STATE_PRESSED;
            }
            else if (BUTTON_3 == 1 && button3_state == BUTTON_STATE_PRESSED)
            {
                button3_state = BUTTON_STATE_IDLE;
                prs6_flag = 1;
                LED_3 = 0;
            }

            // Button 4
            if (BUTTON_4 == 0 && button4_state == BUTTON_STATE_IDLE)
            {
                button4_state = BUTTON_STATE_PRESSED;
            }
            else if (BUTTON_4 == 1 && button4_state == BUTTON_STATE_PRESSED)
            {
                button4_state = BUTTON_STATE_IDLE;
                prs7_flag = 1;
                LED_4 = 0;
            }
        }
    }

    if (INTCONbits.TMR0IF == 1)
    {
        INTCONbits.TMR0IF = 0; // Clear the Timer0 overflow interrupt flag
        TMR0L = 0xFF;
        TMR0H = 0xC2;
        timer_counter++;

        if (current_state == ACTIVE)
        {

            if (altitude_flag)
            {
                if (timer_counter == altitude_period)
                {
                    GODONE = 1;
                    if (adc_flag)
                    {
                        sendAltitudeResponse();
                        timer_counter = 0;
                        adc_flag = 0;
                    }
                }
                else
                {
                    sendDistanceResponse();
                }
            }
            else
            {

                if (prs4_flag)
                {
                    handleButtonPress(4);
                    prs4_flag = 0;
                    remaining_distance -= current_speed;
                    goto here;
                }
                if (prs5_flag)
                {
                    handleButtonPress(5);
                    prs5_flag = 0;
                    remaining_distance -= current_speed;
                    goto here;
                }
                if (prs6_flag)
                {
                    handleButtonPress(6);
                    prs6_flag = 0;
                    remaining_distance -= current_speed;
                    goto here;
                }
                if (prs7_flag)
                {
                    handleButtonPress(7);
                    prs7_flag = 0;
                    remaining_distance -= current_speed;
                    goto here;
                }
                sendDistanceResponse();
            here:
                printf("here\n");
            }
        }
    }
}

int alt_array[10];
int ind = 0;

/*
 * Processes a command received from the USART interface.
 * This function handles various commands that can be received.
 */

void processCommand(char *command)
{
    if (strncmp(command, "$GOO", 4) == 0)
    {
        INTCONbits.TMR0IE = 1;
        current_state = ACTIVE;
        sscanf(&command[4], "%4X", &total_distance);
        remaining_distance = total_distance;
        current_speed = 0;
    }
    else if (strncmp(command, "$SPD", 4) == 0)
    {
        sscanf(&command[4], "%4X", &current_speed);
    }
    else if (strncmp(command, "$ALT", 4) == 0)
    {

        sscanf(&command[4], "%4X", &altitude_period);

        //  alt_array[ind++] = altitude_period;

        altitude_period /= 100;

        // Start a new conversion
        if (!altitude_flag && altitude_period != 0)
        {

            ADCON0bits.GO = 1; // Start conversion

            altitude_flag = 1;

            timer_counter = 0;
        }
        else if (altitude_period == 0)
        {
            altitude_flag = 0;
        }
        else
        {
            ADCON0bits.GO = 1; // Start conversion

            timer_counter = 0;
        }
    }
    else if (strncmp(command, "$MAN", 4) == 0)
    {
        if (command[4] == '0' && command[5] == '1')
        {
            INTCONbits.RBIE = 1; // Enable PORTB change interrupt
            manual_control = 1;
            LED_CLEAR;
        }
        else if (command[4] == '0' && command[5] == '0')
        {
            manual_control = 0;
            INTCONbits.RBIE = 0; // Enable PORTB change interrupt
            LED_CLEAR;
        }
    }
    else if (strncmp(command, "$LED", 4) == 0)
    {
        if (manual_control)
        {
            unsigned char led_number;
            sscanf(&command[4], "%2X", &led_number);

            switch (led_number)
            {
            case 0:
                LED_CLEAR;
                break;
            case 1:
                LED_1 = 1;
                break;
            case 2:
                LED_2 = 1;
                break;
            case 3:
                LED_3 = 1;
                break;
            case 4:
                LED_4 = 1;
                break;
            }
        }
    }
    else if (strncmp(command, "$END", 4) == 0)
    {
        current_state = END;
        INCONbits.GIE = 0;   // Disable interrupts
        INTCONbits.PEIE = 0; // Disable peripheral interrupts
        sendUSART("$END#");
    }
}