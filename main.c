/* Power Electronics Project
 Control Circuit Software runs on PIC16F1825
 November 20, 2014
 */

// Include the following header files
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <xc.h>

// PIC16F1825 Configuration Bit Settings
#pragma config FOSC = HS        // High Speed External Oscillator
#pragma config WDTE = OFF       // Watchdog timer disabled
#pragma config MCLRE = ON       // MCLR Pin Function Select 
#pragma config BOREN = OFF      // Brown-out Reset Disabled
#pragma config PLLEN = OFF      // PLL Disabled (4x PLL disabled)
#pragma config LVP = OFF        // Low-Voltage Programming Disable

// Define Ceramic Oscillator Frequency
#define _XTAL_FREQ  (20000000UL)

// Declare and Initialize Counters/Variables
unsigned int us_200 = 0; //200 uSeconds variables
float phase = 0; //This variable stores the current phase of wave
float alpha = 30; //Stores the delay angle (desired turn on angle)
int pulse = 0; //Stores the pulse width counter (to change pulse width look in main())
int pulseflag = 0; //Lets us know if a pulse has occured this half cycle
int adcread = 0x0000; //stores ADC read value
float alpha1=0; //temporary storage for alpha used in read_adc()


// Function Prototypes
void generate_pulse(void);
float read_pot(void);

// Interrupt Service Routine
void interrupt ISR1(void) {
    // UART Flag High?
//    if (PIR1bits.RCIF == 1) { //Received UART data
//
//        PIR1bits.RCIF = 0; //Reset UART Receive flag
//    }
    // Timer0 Overflow Flag High?
    if (INTCONbits.TMR0IF == 1) { //If timer0 Overflows (which should happen every 200us)
        us_200++; //Increment 200 uSeconds counter
        pulse++; //increment pulse width counter
        phase = phase + 2; //Sum the phase with the number of 200us since zero crossing
        //For debugging:
        //LATCbits.LATC1^=1;       //toggle LED on pin RC1
        INTCONbits.TMR0IF = 0; //Reset the timer0 interrupt flag
    }
    /* 200us is about equal to 2 degrees, assuming that one half of the 60Hz line cycle is
     * 8.333ms, then 8333us/90=92.58 which we will round to about 100, so every interrupt there is 2degrees of phase added on to
     * the previous phase.  */

    // External Interrupt Pin Flag High?
    if (INTCONbits.INTF == 1) { //If there is an external interrupt
        OPTION_REGbits.INTEDG ^= 1; //toggle rising or falling edge interrupt generation
        phase = 0; //reset current phase to zero
        us_200 = 0; //Reset us counter
        pulseflag = 0; //Reset the pulse flag
        INTCONbits.INTF = 0; //Reset the enternal interrupt flag
    }
}


// MAIN
void main(void) {

    // Setup Clock, GPIO, Peripherals, Timers, etc...

    // Oscillator Configure
    OSCCONbits.SCS = 0b00; // Clock Determined by FOSC in CONFIG WORD

    // Clock Configure
    OPTION_REG = 0b11000001; //Timer0 overflow every 1 uSecond
    OPTION_REGbits.INTEDG = 1; //rising egde interrupt generation

    // Interrupt Configure
    INTCON = 0b01110000; //Select Interrupt Conditions,UART INT, Timer0 OF & EXT INT

    // Peripheral Interrupt Configuration
    PIE1 = 0b00100000; //Enable UART Interrupt
    PIE2 = 0x00; //Disable all other peripheral interrupts
    PIE3 = 0x00; //Disable all other peripheral interrupts

    // GPIO Configure
    TRISCbits.TRISC1 = 0; // RC1/LED1 as output
    TRISAbits.TRISA2 = 1; // RA2 as input (interrupt Pin)
    ANSELAbits.ANSA2 = 0;
    TRISCbits.TRISC0 = 0; // RC0 as output to trigger triac
    TRISCbits.TRISC2 = 1; //RC2 as input for pot to set alpha
    ANSELCbits.ANSC2 = 1; //Set RC2 as analog input


    //ADC Configure
    ADCON0 = 0b00011001;  //Enable ADC on AN6(RC2), but do not start conversion
    ADCON1 = 0b11010000;  //Configure Vref+- and AD Clock

    // Enable Global Interrupts
    INTCONbits.GIE = 1; //Enable Global Interrupts


    // Main Loop
    while (1) {
        alpha = read_pot();
        // Check if phase is less than alpha
        if ((phase < alpha)) { //if phase angle is less than delay angle and a pulse hasnt happend yet
            LATCbits.LATC0 = 0; //Keep the output LOW
            LATCbits.LATC1=0;//Keep the output 2 low
        } else {

            // Check if Phase is greater than alpha and pulse has not been fired
            if ((pulseflag == 0) && (phase > alpha)) { 
                generate_pulse(); //generate a pulse 
                pulseflag = 1; //set the flag to know we have pulsed for this half wave cycle
            } else {
                LATCbits.LATC0 = 0; //Turn off output
                LATCbits.LATC1=0; //Turn off output 2
            }
        }
    }

}



/* This function generates a pulse for a time specified in the while(pulse< x) function */
void generate_pulse(void) {

    // Generate Pulse
    pulse = 0;  // Reset pulse counter
    while (pulse < 4) { //While pulse < x   ***CHANGE FOR PULSE WIDTH***
        if(OPTION_REGbits.INTEDG==1){
        LATCbits.LATC0 = 1; // Set output High
        }else{LATCbits.LATC1 =1;}//Set output 2 HIGH
    }
    LATCbits.LATC1=0;// Set output 2 low
    LATCbits.LATC0 = 0; //Set output LOW
}


float read_pot(void){
    adcread = 0; //reset adcread temp storage
    ADCON0bits.ADON =1;  //Enable ADC
    ADCON0bits.GO_nDONE =1;  //Start conversion
    while(ADCON0bits.GO_nDONE ==1){} //While ADC is reading, wait. Automatically set LOW when done
    adcread = (ADRESH<<8)|ADRESL;  //store high and low bytes of adcread
    alpha1 = 0.0625*adcread; //64/1024*adcread
    return alpha1;
}
