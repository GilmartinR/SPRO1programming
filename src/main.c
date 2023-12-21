#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include "usart.h"

#define encdr_holes 8
#define base_OCR 40 // OCR0A value at which vehicle begins movement
#define pi 3.1416
#define diameter 65
#define k 4 // RPS per 1 OCR0A increase post base_OCR

int main(void) {  
  uart_init(); // open the communication to the microcontroller
  io_redirect(); // redirect input and output to the communication

  // Optocoupler Setup
  TCCR1A = 0x00;
  TCCR1B = 0xC5;
  DDRB &= ~0x01;
  PORTB |= 0x01;

  // PWM Setup

  DDRD |= 0x60;
  TCCR0A |= 0xA3;
  TCCR0B |= 0x05;

  // ADC Setup

  ADMUX = ADMUX | 0x40;
  ADCSRB = ADCSRB & (0xF8);
  ADCSRA = ADCSRA | 0xE7;

  // Prerequisites for speed determination

  double saved_OCR, new_OCR;
  int sector = 0;
  double d_rem[2], t_rem[2]; // distances and times remaining in each sector - initial values inputted via Nextion

  uint32_t count = 0; // Total amount of signals
  uint32_t adclow;
  
  //Flags
  int runflag = 1;
  int battery_flag = 1;
  int SLDflg1 = 1, SLDflg2 = 1, BUTTON_flag = 1;
  
  // ENTERING DETAILS
    char rb[7]; // Touch release reader
    char rb2[8]; // get val reader
    printf("page 0%c%c%c", 0xff,0xff,0xff);

    while(SLDflg1 || SLDflg2){ // Sliders for Sector 1, reacting on Touch Release Events
        for(int i = 0; i < 7; i++){
            scanf("%c", &rb[i]);
        }
        if((rb[0] == 0x65) && (rb[2] == 0x02) && SLDflg1){            
            printf("get sect1.h1.val%c%c%c", 0xff, 0xff, 0xff);            
            for(int i = 0; i < 8; i++){
                scanf("%c", &rb2[i]);
            }
            if(rb2[0]==0x71){
                printf("sect1.n0.val=%d%c%c%c", rb2[1], 0xff, 0xff, 0xff);
                d_rem[0] = rb2[1];
            }
            else{
                printf("sect1.n0.val=%d%c%c%c", -5, 0xff, 0xff, 0xff);
            }
            SLDflg1 -= 1;
            if(!SLDflg1){
                SLDflg1 = 1;
            }
        }
        if((rb[0] == 0x65) && (rb[2] == 0x01) && SLDflg2){            
            printf("get sect1.h0.val%c%c%c", 0xff, 0xff, 0xff);            
            for(int i = 0; i < 8; i++){
                scanf("%c", &rb2[i]);
            }
            if(rb2[0]==0x71){
                printf("sect1.n1.val=%d%c%c%c", rb2[1], 0xff, 0xff, 0xff);
                t_rem[0] = rb2[1];
            }
            else{
                printf("sect1.n1.val=%d%c%c%c", -5, 0xff, 0xff, 0xff);
            }
            SLDflg2 -= 1;
            if(!SLDflg2){
                SLDflg2 = 1;
            }
        }

        if((rb[0] == 0x65) && (rb[2] == 0x05) && BUTTON_flag){    
            SLDflg1 = 0;
            SLDflg2 = 0;
            BUTTON_flag = 0;
            break;
        }
    }

    SLDflg1 = 1;
    SLDflg2 = 1;
    BUTTON_flag = 1;
    printf("page 1%c%c%c", 0xff,0xff,0xff);

    while(SLDflg1 || SLDflg2){ // Sliders for Sector 2, reacting on Touch Release Events
        for(int i = 0; i < 7; i++){
            scanf("%c", &rb[i]);
        }
        if((rb[0] == 0x65) && (rb[2] == 0x02) && SLDflg1){            
            printf("get sect2.h1.val%c%c%c", 0xff, 0xff, 0xff);            
            for(int i = 0; i < 8; i++){
                scanf("%c", &rb2[i]);
            }
            if(rb2[0]==0x71){
                printf("sect2.n0.val=%d%c%c%c", rb2[1], 0xff, 0xff, 0xff);
                d_rem[1] = rb2[1];
            }
            else{
                printf("sect2.n0.val=%d%c%c%c", -5, 0xff, 0xff, 0xff);
            }
            SLDflg1 -= 1;
            if(!SLDflg1){
                SLDflg1 = 1;
            }
        }
        if((rb[0] == 0x65) && (rb[2] == 0x01) && SLDflg2){            
            printf("get sect2.h0.val%c%c%c", 0xff, 0xff, 0xff);            
            for(int i = 0; i < 8; i++){
                scanf("%c", &rb2[i]);
            }
            if(rb2[0]==0x71){
                printf("sect2.n1.val=%d%c%c%c", rb2[1], 0xff, 0xff, 0xff);
                t_rem[1] = rb2[1];
            }
            else{
                printf("sect2.n1.val=%d%c%c%c", -5, 0xff, 0xff, 0xff);
            }
            SLDflg2 -= 1;
            if(!SLDflg2){
                SLDflg2 = 1;
            }
        }

        if((rb[0] == 0x65) && (rb[2] == 0x05) && BUTTON_flag){            
            SLDflg1 = 0;
            SLDflg2 = 0;
            BUTTON_flag = 0;
            break;
        }
    }

    //Change distance to mm and add extra 1 second to times (avoid accidental 0 and add time buffer)
    d_rem[0] = d_rem[0] * 1000;
    d_rem[1] = d_rem[1] * 1000;
    t_rem[0] += 1;
    t_rem[1] += 1;

    printf("page 2%c%c%c", 0xff, 0xff, 0xff);
    printf("mainPage.n1.val=%d%c%c%c", (int) d_rem[sector], 0xff, 0xff, 0xff);
    printf("mainPage.x0.val=%d%c%c%c", (int) (t_rem[sector] * 100), 0xff, 0xff, 0xff);
    TCNT1 = 0;
    saved_OCR = 50;
    while(runflag && battery_flag) {
        
        OCR0A = saved_OCR; //PWM

        if ((TIFR1 & 0x20) == 0x20 ){ // Optocoupler signal Reading
            count += 1;
            TCNT1 = 0;
            TIFR1 |= 0x20;
            d_rem[sector] -= (pi * diameter) / encdr_holes;
            t_rem[sector] -= ICR1 * 0.000064;
            printf("mainPage.n1.val=%d%c%c%c", (int) d_rem[sector], 0xff, 0xff, 0xff);
            printf("mainPage.x0.val=%d%c%c%c", (int) (t_rem[sector] * 100), 0xff, 0xff, 0xff);
            if((d_rem[sector] <= 0) && (t_rem[sector] >= 0)){ // Checking if task is completed in given time/distance
                if(sector == 1){ // Finished
                    runflag = 0;
                    OCR0A = 0;
                }
                else{ // Next Sector
                    sector += 1;
                }
            }
            if(t_rem[sector]<=0){ // only requirement of task failing, due to previous 'if' removing all other variants
                runflag = 0;
                OCR0A = 0;
                printf("page 4%c%c%c", 0xff, 0xff, 0xff);
            }
            if(count>=2){ // Calculating Required Speed
                new_OCR = (16 * d_rem[sector] / (t_rem[sector] * pi * k * diameter)) + base_OCR;
                if(new_OCR > saved_OCR){ // Acceleration
                    printf("mainPage.t4.txt=%s%c%c%c", "Accelerating", 0xff,0xff,0xff);
                }
                if(new_OCR < saved_OCR){ // Deceleration
                    printf("mainPage.t4.txt=%s%c%c%c", "Decelerating", 0xff,0xff,0xff);
                }
                saved_OCR = new_OCR;
            }
        } 

        adclow = ADCL; // Battery Voltage measurement
        adclow = 300 * (adclow + ((ADCH & 0x03) << 8)) / 1024 * 5;
        printf("mainPage.x1.val=%lu%c%c%c", adclow, 0xff, 0xff, 0xff);
        // if ( adclow <= 680 ){ // Stop Program on Less than set value on Voltage - WORKING
        //     battery_flag = 0; 
        //     printf("page 3%c%c%c", 0xff, 0xff, 0xff);
        //     OCR0A = 0;
        // }

    }
    return 0;
}