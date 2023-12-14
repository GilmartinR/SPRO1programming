#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include "usart.h"

#define encdr_holes 8
#define base_OCR 250 // OCR0A value at which vehicle begins movement
#define pi 3.1416
#define diameter 65
#define k 1 // RPS per 1 OCR0A increase post base_OCR

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

  double new_OCR, saved_OCR;
  int sector = 0;
  double d_rem[2], t_rem[2]; // distances and times remaining in each sector - initial values inputted via Nextion
  //double cycle_distance;

  uint32_t count = 0; // Total amount of signals
  unsigned count_in_cycle = 0; // Security measure to avoid "0" results when calculatng avg RPM
  double time_rotation = 0; // Time for a full rotation
  double time_diffs[encdr_holes]; // Last 8 time differences
  uint32_t RPM_for_screen; // RPM in UNSIGNED LONG INT format for NEXTION
  uint32_t adclow;

  int runflag = 1;
  int battery_flag = 1;

  for(int i = 0; i < encdr_holes; i++){
    time_diffs[i] = 0;
  }

  //Test of screen connection
  printf("page0.n0.val=0%c%c%c", 0xff, 0xff, 0xff);
  printf("page0.x0.val=66000%c%c%c", 0xff, 0xff, 0xff);
  
// Free time setup for drives and shift to mm
  d_rem[0] = 1000;
  d_rem[1] = 3000;
  t_rem[0] = 24;
  t_rem[1] = 54;

  saved_OCR = base_OCR;//d_rem[sector] / (t_rem[sector] * pi * k * diameter) + base_OCR; //Starting OCR0A
    printf("page 0%c%c%c", 0xff, 0xff, 0xff);
    while(runflag && battery_flag) {
    // PWM Power Setup

        OCR0A = saved_OCR; //0 - 255

        // Optocoupler signal Reading
        if ((TIFR1 & 0x20) == 0x20 ){ 
        
            count += 1;
            TCNT1 = 0;
            TIFR1 |= 0x20;
            time_diffs[count % (encdr_holes)] = ICR1 * 0.000064; //ticks to seconds

            // Using function is too slow
            
            if((count > 1) && !(count % (encdr_holes))){ // Security measure against first count
                count_in_cycle = 0;
                time_rotation = 0;
                for(int i = 0; i < encdr_holes; i++){ // Calculating RPM via 1 (or however many is needed) last cycle and adjusting for '0' measurements and 1st cycle
                    if (time_diffs[i] != 0){
                        count_in_cycle += 1;
                        time_rotation += time_diffs[i];
                    }
                }
                t_rem[sector] -= time_rotation;
                d_rem[sector] -= (pi * diameter * count_in_cycle / encdr_holes);
                printf("page0.n1.val=%d%c%c%c", (int) d_rem[sector], 0xff, 0xff, 0xff);
                /*if ((t_rem[sector] <= 0) && (d_rem[sector] > 0)){
                    //printf("FAILURE!");
                    runflag = 0;
                    break;
                }
                if ((d_rem[sector] <= 0) && (t_rem[sector] > 2)){
                    runflag = 0;
                    break;
                }
                if((d_rem[sector]<= 0) && (t_rem[sector] <= 2)){
                    sector += 1;
                }
                new_OCR = 4 * d_rem[sector] / (t_rem[sector]  * k * pi * diameter) + base_OCR;
                if(new_OCR > saved_OCR){
                    //printf("Speeding Up!");
                }
                if(new_OCR < saved_OCR){
                    //printf("Slowing down!");
                }       
                saved_OCR = new_OCR;
                */
                RPM_for_screen = (60 / time_rotation) * (4 * encdr_holes/count_in_cycle) * 4000; //Getting RPM with a accuracy of 3 digits after dot for Nextion
                
            }
            //printf("page0.x0.val=%ld%c%c%c", count * 10, 0xff, 0xff, 0xff); 
        }
        adclow = ADCL;
        printf("page0.n0.val=%lu%c%c%c", 300 * (adclow + ((ADCH & 0x03) << 8)) / 1024 * 5, 0xff, 0xff, 0xff);
        if ( 300 * (adclow + ((ADCH & 0x03) << 8)) / 1024 * 5 <=680){ // Stop Program on Less than set value on Voltage - WORKING
            battery_flag = 0; 
            printf("page 1%c%c%c", 0xff, 0xff, 0xff);
            OCR0A = 0;
        }
    }
    if(battery_flag == 0){
        
    }
    return 0;
}