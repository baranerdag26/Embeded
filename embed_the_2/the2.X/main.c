#include <xc.h>
#include <stdint.h>

// ============================ //
// Do not edit this part!!!!    //
// ============================ //
// 0x300001 - CONFIG1H
#pragma config OSC = HSPLL // Oscillator Selection bits (HS oscillator,
                           // PLL enabled (Clock Frequency = 4 x FOSC1))
#pragma config FCMEN = OFF // Fail-Safe Clock Monitor Enable bit
                           // (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF  // Internal/External Oscillator Switchover bit
                           // (Oscillator Switchover mode disabled)
// 0x300002 - CONFIG2L
#pragma config PWRT = OFF  // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = OFF // Brown-out Reset Enable bits (Brown-out
                           // Reset disabled in hardware and software)
// 0x300003 - CONFIG1H
#pragma config WDT = OFF // Watchdog Timer Enable bit
                         // (WDT disabled (control is placed on the SWDTEN bit))
// 0x300004 - CONFIG3L
// 0x300005 - CONFIG3H
#pragma config LPT1OSC = OFF // Low-Power Timer1 Oscillator Enable bit
                             // (Timer1 configured for higher power operation)
#pragma config MCLRE = ON    // MCLR Pin Enable bit (MCLR pin enabled;
                             // RE3 input pin disabled)
// 0x300006 - CONFIG4L
#pragma config LVP = OFF   // Single-Supply ICSP Enable bit (Single-Supply
                           // ICSP disabled)
#pragma config XINST = OFF // Extended Instruction Set Enable bit
                           // (Instruction set extension and Indexed
                           // Addressing mode disabled (Legacy mode))

#pragma config DEBUG = OFF // Disable In-Circuit Debugger

#define KHZ 1000UL
#define MHZ (KHZ * KHZ)
#define _XTAL_FREQ (40UL * MHZ)
#define MIN_BUTTON_COUNT 1

// ============================ //
//         Declarations         //
// ============================ //

volatile uint8_t prevB;
int prev_piece = 2;
int port_c_decimal;
int blink_flag = 0;
int two_seconds_counter = 0;
uint32_t active_piece = 0;
uint32_t active_piece_record;
uint8_t LATC_value;
uint8_t LATD_value;
uint8_t LATE_value;
uint8_t LATF_value;
int LATC_flag = 0;
int LATD_flag = 0;
int LATE_flag = 0;
int LATF_flag = 0;
uint8_t LATC_old;
uint8_t LATD_old;
uint8_t LATE_old;
uint8_t LATF_old;
int is_active = 0;
int total_pieces = 0;

volatile uint8_t lastRBState;
int can_submit = 1;
int rotate_counter = 0;
uint32_t active_piece_first = 0x00000000;

typedef enum
{
    Waiting,
    Detected,
    WaitForRelease,
    Update
} ButtonState;

typedef union
{
    struct
    {
        unsigned RG0 : 1;
        unsigned RG1 : 1;
        unsigned RG2 : 1;
        unsigned RG3 : 1;
        unsigned RG4 : 1;
        unsigned RG5 : 1;
        unsigned : 2; // Padding for the unused higher bits
    };
    uint8_t Full;
} Button_Type;

Button_Type Button_Press;
Button_Type Temp_Press;
ButtonState buttonState = Waiting;
uint8_t buttonCount = 0;

const uint8_t segmentCodes[10] = {
    0x3F, // 0 -> ABCDEF
    0x06, // 1 -> BC
    0x5B, // 2 -> ABDEG
    0x4F, // 3 -> ABCDG
    0x66, // 4 -> BCFG
    0x6D, // 5 -> ACDFG
    0x7D, // 6 -> ACDEFG
    0x07, // 7 -> ABC
    0x7F, // 8 -> ABCDEFG
    0x6F  // 9 -> ABCDFG
};

void shift_down_piece(uint32_t *active_piece);
void parse_and_assign(uint32_t value, uint8_t *ports);
void check_buttons_rg();
void spawn_piece();
void show_piece();
void submit_piece();
void rotate_piece();
void move_right();
void move_left();
void move_up();
void move_down();
void displayNumber();

// ============================ //
//        Initialization        //
// ============================ //

void Init()
{

    // ADCON 0x7 gibi biÅey olacak
    // port init sÄ±rasÄ± Ã¶nemli
    // A
    TRISA = 0x0F; // Controller
    LATA = 0x00;
    PORTA = 0x00;

    // B
    TRISB = 0x0F; // Interrupts
    PORTB = 0x00;
    LATB = PORTB;

    // Game Area

    // C
    TRISC = 0x00;
    LATC = 0x00;
    PORTC = 0x00;

    // D
    TRISD = 0x00;
    LATD = 0x00;
    PORTD = 0x00;

    // E
    TRISE = 0x00;
    LATE = 0x00;
    PORTE = 0x00;

    // F
    TRISF = 0x00;
    LATF = 0x00;
    PORTF = 0x00;

    // G
    TRISG = 0x3F; // Buttons
    LATG = 0x00;
    PORTG = 0x00;

    TRISH = 0x00;
    TRISJ = 0x00;

    PORTH = 0x00;
    PORTJ = 0x00;

    lastRBState = PORTB;
}

void InitializeTimerAndInterrupts()
{
    // Enable Interrupts
    //  INTCON

    INTCONbits.RBIF = 0;   // RB Port Change Interrupt Flag
    INTCONbits.TMR0IF = 0; // Timer0 Overflow Interrupt Flag
    INTCONbits.INT0IF = 0; // External Interrupt 0 Flag

    // RCON
    RCONbits.IPEN = 0; // Interrupt Priority Enable

    // T0CON
    T0CONbits.TMR0ON = 1; // Timer0 On/Off Control
    T0CONbits.T08BIT = 0; // Timer0 8-bit/16-bit Control
    T0CONbits.T0CS = 0;   // Timer0 Clock Source Select
    T0CONbits.T0SE = 0;   // Timer0 Source Edge Select
    T0CONbits.PSA = 0;    // Timer0 Prescaler Assignment
    T0CONbits.T0PS2 = 1;  // Timer0 Prescaler Select bits
    T0CONbits.T0PS1 = 0;  // Timer0 Prescaler Select bits
    T0CONbits.T0PS0 = 1;  // Timer0 Prescaler Select bits

    // INTCON3
    INTCON3bits.INT3IF = 0; // External Interrupt 3 Flag
    INTCON3bits.INT3IE = 1; // External Interrupt 3 Enable

    // Initial Timer0 Value
    TMR0L = 0x69;
    TMR0H = 0x67;

    INTCONbits.GIE = 1;     // Global Interrupt Enable
    INTCONbits.PEIE = 1;    // Peripheral Interrupt Enable
    INTCONbits.TMR0IE = 1;  // Timer0 Overflow Interrupt Enable
    INTCONbits.INT0IE = 1;  // External Interrupt 0 Enable
    INTCON3bits.INT3IE = 1; // External Interrupt 3 Enable
    INTCONbits.RBIE = 0;    // RB Port Change Interrupt Enable
}

// ============================ //
//   INTERRUPT SERVICE ROUTINE  //
// ============================ //

__interrupt(high_priority) void HandleInterrupt()
{

    if (INTCON3bits.INT3IF == 1)
    {
        // Check which button is pressed
        // RB3
        if (PORTBbits.RB3 == 1)
        {
            submit_piece();
        }

        // External Interrupt 3
        INTCON3bits.INT3IF = 0; // Clear the External Interrupt 3 flag
    }

    if (INTCONbits.INT0IF == 1)
    {
        // RB0
        if (PORTBbits.RB0 == 1)
        {
            if (active_piece_first == 0x01030000)
            {
                show_piece();
                rotate_piece();
                show_piece();
                // FULL TIMER
                TMR0H = 0xFF;
                TMR0L = 0xFF;
            }
        }
        INTCONbits.INT0IF = 0; // Clear the External Interrupt 0 flag
    }

    if (INTCONbits.RBIF == 1)
    {
        INTCONbits.RBIF = 0; // Clear the RB port change interrupt flag
    }

    if (INTCONbits.TMR0IF == 1)
    {

        two_seconds_counter++;
        INTCONbits.TMR0IF = 0; // Clear the Timer0 overflow interrupt flag
        TMR0L = 0x69;
        TMR0H = 0x67;
        if (blink_flag == 0)
        {
            // Light is on
            blink_flag = 1;
            active_piece_record = active_piece;
            active_piece = 0x00000000;
        }
        else
        {
            // Light is off
            blink_flag = 0;
            active_piece = active_piece_record;
            active_piece_record = 0x00000000;
        }

        if (two_seconds_counter == 8)
        {

            move_down();

            two_seconds_counter = 0;

            if (LATC_flag == 1)
            {
                LATC_value = LATC_old;
                LATC_flag = 0;
            }
            if (LATD_flag == 1)
            {
                LATD_value = LATD_old;
                LATD_flag = 0;
            }
            if (LATE_flag == 1)
            {
                LATE_value = LATE_old;
                LATE_flag = 0;
            }
            if (LATF_flag == 1)
            {
                LATF_value = LATF_old;
                LATF_flag = 0;
            }
        }
    }
}

// ============================ //
//       Helper Functions      //
// ============================ //

void rotate_piece()
{
    uint8_t port[4];
    if (active_piece == 0x00000000)
    {
        parse_and_assign(active_piece_record, port);
    }
    else
    {
        parse_and_assign(active_piece, port);
    }

    if (rotate_counter % 4 == 0)
    {
        if (port[0] != 0x00)
        {
            port[0] = port[0] << 1;
        }
        else if (port[1] != 0x00)
        {
            port[1] = port[1] << 1;
        }
        else if (port[2] != 0x00)
        {
            port[2] = port[2] << 1;
        }
        else if (port[3] != 0x00)
        {
            port[3] = port[3] << 1;
        }
    }
    else if (rotate_counter % 4 == 1)
    {
        if (port[0] != 0x00)
        {
            uint8_t temp = port[0];
            port[0] = port[1];
            port[1] = temp;
        }
        else if (port[1] != 0x00)
        {
            uint8_t temp = port[1];
            port[1] = port[2];
            port[2] = temp;
        }
        else if (port[2] != 0x00)
        {
            uint8_t temp = port[2];
            port[2] = port[3];
            port[3] = temp;
        }
        else if (port[3] != 0x00)
        {
            uint8_t temp = port[3];
            port[3] = port[0];
            port[0] = temp;
        }
    }
    else if (rotate_counter % 4 == 2)
    {
        if (port[3] != 0x00)
        {
            port[3] = port[3] >> 1;
        }
        else if (port[2] != 0x00)
        {
            port[2] = port[2] >> 1;
        }
        else if (port[1] != 0x00)
        {
            port[1] = port[1] >> 1;
        }
        else if (port[0] != 0x00)
        {
            port[0] = port[0] >> 1;
        }
    }
    else
    {
        if (port[0] != 0x00)
        {
            uint8_t temp = port[0];
            port[0] = port[1];
            port[1] = temp;
        }
        else if (port[1] != 0x00)
        {
            uint8_t temp = port[1];
            port[1] = port[2];
            port[2] = temp;
            uint32_t active_piece_first = 0x00000000;
        }
        else if (port[2] != 0x00)
        {
            uint8_t temp = port[2];
            port[2] = port[3];
            port[3] = temp;
        }
        else if (port[3] != 0x00)
        {
            uint8_t temp = port[3];
            port[3] = port[0];
            port[0] = temp;
        }
    }

    rotate_counter++;
    // Reconstruct the active_piece value using bitwise operations
    active_piece = ((uint32_t)port[0] << 24) | ((uint32_t)port[1] << 16) | ((uint32_t)port[2] << 8) | ((uint32_t)port[3]);
    active_piece_record = active_piece;
}

void submit_piece()
{

    if (can_submit == 0)
    {
        return;
    }

    uint8_t ports[4];
    if (active_piece == 0x00000000)
    {
        parse_and_assign(active_piece_record, ports);
    }
    else
    {
        parse_and_assign(active_piece, ports);
    }

    LATC_value = PORTC | LATC_old | ports[0] | LATC_value;
    LATD_value = PORTD | LATD_old | ports[1] | LATD_value;
    LATE_value = PORTE | LATE_old | ports[2] | LATE_value;
    LATF_value = PORTF | LATF_old | ports[3] | LATF_value;

    prev_piece = (prev_piece + 1) % 3;

    is_active = 0;

    // Reset collision flags and restore old values if necessary
    if (LATC_flag == 1)
    {
        LATC = LATC_old;
        LATC_flag = 0;
    }
    if (LATD_flag == 1)
    {
        LATD = LATD_old;
        LATD_flag = 0;
    }
    if (LATE_flag == 1)
    {
        LATE = LATE_old;
        LATE_flag = 0;
    }
    if (LATF_flag == 1)
    {
        LATF = LATF_old;
        LATF_flag = 0;
    }
    total_pieces++;
    spawn_piece();
}

void check_buttons_rg()
{
    static ButtonState state = Waiting;
    static uint8_t buttonCount = 0;
    Button_Type currentButtonRead;

    // Read each button bit directly from PORTG
    currentButtonRead.RG0 = PORTGbits.RG0;
    currentButtonRead.RG1 = PORTGbits.RG1;
    currentButtonRead.RG2 = PORTGbits.RG2;
    currentButtonRead.RG3 = PORTGbits.RG3;
    currentButtonRead.RG4 = PORTGbits.RG4;
    currentButtonRead.RG5 = PORTGbits.RG5;

    if (state == Waiting)
    {
        if (currentButtonRead.Full != 0)
        {
            state = Detected;
            Temp_Press = currentButtonRead;
            buttonCount = 0;
        }
    }
    else if (state == Detected)
    {
        if (currentButtonRead.Full == Temp_Press.Full)
        {
            buttonCount++;
            if (buttonCount >= MIN_BUTTON_COUNT)
            {
                state = WaitForRelease;
            }
        }
        else
        {
            state = Waiting;
        }
    }
    else if (state == WaitForRelease)

    {
        if ((currentButtonRead.Full & Temp_Press.Full) == 0) // Check if any pressed button has been released
        {
            state = Update;
        }
    }
    else if (state == Update)
    {
        Button_Press = Temp_Press;
        state = Waiting;
        buttonCount = 0;

        if (Button_Press.RG2 == 1)
        {

            move_right();
        }
        else if (Button_Press.RG4 == 1)
        {
            move_left();
        }
        else if (Button_Press.RG3 == 1)
        {
            move_up();
        }
        else if (Button_Press.RG0 == 1)
        {
            move_down();
        }
    }
}

void move_right()
{
    uint8_t ports[4];
    if (active_piece == 0x00000000)
    {
        parse_and_assign(active_piece_record, ports);
    }
    else
    {
        parse_and_assign(active_piece, ports);
    }

    // Check if the piece is on the far left (in port F)
    if (ports[3] == 0)
    {
        // Shift all ports to the right if the piece is not on the far left
        uint8_t temp2 = ports[2];
        uint8_t temp1 = ports[1];
        uint8_t temp0 = ports[0];

        ports[0] = 0x00;
        ports[1] = temp0;
        ports[2] = temp1;
        ports[3] = temp2;

        // Reconstruct the active_piece value using bitwise operations
        active_piece = ((uint32_t)ports[0] << 24) | ((uint32_t)ports[1] << 16) | ((uint32_t)ports[2] << 8) | ((uint32_t)ports[3]);
        active_piece_record = active_piece;
    }
}

void move_left()
{
    uint8_t ports[4];
    if (active_piece == 0x00000000)
    {
        parse_and_assign(active_piece_record, ports);
    }
    else
    {
        parse_and_assign(active_piece, ports);
    }

    // Check if the piece is on the far right (in port C)
    if (ports[0] == 0)
    {
        // Shift all ports to the left if the piece is not on the far right
        uint8_t temp2 = ports[2];
        uint8_t temp1 = ports[1];
        uint8_t temp0 = ports[0];

        ports[0] = temp1;
        ports[1] = temp2;
        ports[2] = ports[3];
        ports[3] = 0x00;

        // Reconstruct the active_piece value using bitwise operations
        active_piece = ((uint32_t)ports[0] << 24) | ((uint32_t)ports[1] << 16) | ((uint32_t)ports[2] << 8) | ((uint32_t)ports[3]);
        active_piece_record = active_piece;
    }
}

void move_up()
{
    uint8_t ports[4];
    if (active_piece == 0x00000000)
    {
        parse_and_assign(active_piece_record, ports);
    }
    else
    {
        parse_and_assign(active_piece, ports);
    }

    if ((ports[0] & 0x01) == 0 && (ports[1] & 0x01) == 0 && (ports[2] & 0x01) == 0 && (ports[3] & 0x01) == 0)
    {
        ports[0] = ports[0] >> 1;
        ports[1] = ports[1] >> 1;
        ports[2] = ports[2] >> 1;
        ports[3] = ports[3] >> 1;

        active_piece = ((uint32_t)ports[0] << 24) | ((uint32_t)ports[1] << 16) | ((uint32_t)ports[2] << 8) | ((uint32_t)ports[3]);
        active_piece_record = active_piece;
    }
}

void move_down()
{
    uint8_t ports[4];
    if (active_piece == 0x00000000)
    {
        parse_and_assign(active_piece_record, ports);
    }
    else
    {
        parse_and_assign(active_piece, ports);
    }

    if ((ports[0] & 0x80) == 0 && (ports[1] & 0x80) == 0 && (ports[2] & 0x80) == 0 && (ports[3] & 0x80) == 0)
    {
        ports[0] = ports[0] << 1;
        ports[1] = ports[1] << 1;
        ports[2] = ports[2] << 1;
        ports[3] = ports[3] << 1;

        // Reconstruct the active_piece value using bitwise operations
        active_piece = ((uint32_t)ports[0] << 24) | ((uint32_t)ports[1] << 16) | ((uint32_t)ports[2] << 8) | ((uint32_t)ports[3]);
        active_piece_record = active_piece;
    }
}

void parse_and_assign(uint32_t value, uint8_t *ports)
{
    ports[0] = (value >> 24) & 0xFF;
    ports[1] = (value >> 16) & 0xFF;
    ports[2] = (value >> 8) & 0xFF;
    ports[3] = (value & 0xFF);
}

void spawn_piece()
{

    if (prev_piece == 2)
    {
        // DOT
        active_piece = 0x01000000;
    }
    else if (prev_piece == 1)
    {
        // L
        active_piece = 0x01030000;
        active_piece_first = 0x01030000;
    }
    else
    {
        // SQUARE
        active_piece = 0x03030000;
    }
    is_active = 1;
    active_piece_record = active_piece;
    rotate_counter = 0;
    active_piece_first = active_piece;
}

void show_piece()
{
    uint8_t ports[4];
    parse_and_assign(active_piece, ports);

    LATC = ports[0] | LATC_value;
    LATD = ports[1] | LATD_value;
    LATE = ports[2] | LATE_value;
    LATF = ports[3] | LATF_value;

    // COLLISION
    if ((ports[0] & LATC_value) && LATC_flag == 0)
    {
        LATC_flag = 1;
        LATC_old = LATC_value;
        LATC_value = PORTC & ~ports[0];
        can_submit = 0;
    }

    if ((ports[1] & LATD_value) && LATD_flag == 0)
    {
        LATD_flag = 1;
        LATD_old = LATD_value;
        LATD_value = PORTD & ~ports[1];
        can_submit = 0;
    }
    if ((ports[2] & LATE_value) && LATE_flag == 0)
    {
        LATE_flag = 1;
        LATE_old = LATE_value;
        LATE_value = PORTE & ~ports[2];
        can_submit = 0;
    }
    if ((ports[3] & LATF_value) && LATF_flag == 0)
    {
        LATF_flag = 1;
        LATF_old = LATF_value;
        LATF_value = PORTF & ~ports[3];
        can_submit = 0;
    }

    // if no collision, can submit = 1
    if (LATC_flag == 0 && LATD_flag == 0 && LATE_flag == 0 && LATF_flag == 0)
    {
        can_submit = 1;
    }
}

void displayDigit(int digit, uint8_t position)
{
    PORTH = 0x00;                // Turn off all displays to prevent ghosting during update
    PORTH = (1 << position);     // Enable the display corresponding to the position
    PORTJ = segmentCodes[digit]; // Set segments for the digit
    __delay_ms(1);               // Display for a short time
}

void displayNumber(int number)
{
    int tens = number / 10;  // Calculate tens digit
    int units = number % 10; // Calculate units digit

    displayDigit(tens, 2);  // Display tens on D1 (connected to PORTH2)
    displayDigit(units, 3); // Display units on D0 (connected to PORTH3)
}

// ============================ //
//            MAIN              //
// ============================ //
void main()
{
    Init();

    // Wait 1 second
    //__delay_ms(1000);

    InitializeTimerAndInterrupts();
    TMR0L = 0x69;
    TMR0H = 0x67;

    spawn_piece();
    // Main Loop
    while (1)
    {
        check_buttons_rg();
        show_piece();
        displayNumber(total_pieces);
    }
}