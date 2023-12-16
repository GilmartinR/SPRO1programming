#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include "usart.h"

#define encdr_holes 8
#define base_OCR 140 // OCR0A value at which vehicle begins movement
#define pi 3.1416
#define diameter 65
#define k 1 // RPS per 1 OCR0A increase post base_OCR

void(* resetFunc) (void) = 0;

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

  double saved_OCR;
  int sector = 0;
  double d_rem[2], t_rem[2]; // distances and times remaining in each sector - initial values inputted via Nextion
  //double cycle_distance;

  uint32_t count = 0; // Total amount of signals
  unsigned count_in_cycle = 0; // Security measure to avoid "0" results when calculatng avg RPM
  double time_rotation = 0; // Time for a full rotation
  double time_diffs[encdr_holes]; // Last 8 time differences
  uint32_t RPM_for_screen; // RPM in UNSIGNED LONG INT format for NEXTION
  uint32_t adclow;


  //Flags
  int runflag = 1;
  int battery_flag = 1;
  int start_flag = 1;

  for(int i = 0; i < encdr_holes; i++){
    time_diffs[i] = 0;
  }
  char read;
  printf("page 0%c%c%c", 0xff, 0xff, 0xff);
  
  while(start_flag){
    //     printf("get page2.h0.val%c%c%c", 0xff, 0xff, 0xff);
    //     for(int i = 0; i < 8; i++){
    //         scanf("%c", &read);
    //         read_buff[i] = read;
    //     }
    //     reading = read_buff[1] | (read_buff[2] << 8) | (read_buff[3] << 16) | (read_buff[4] << 24);
    //     printf("page2.n0.val=%lu%c%c%c", reading, 0xff,0xff,0xff);
    scanf("%c", &read);    
    if(read == 0x65){
        start_flag = 0;
        break;
    }
  }

  //Test of screen connection
  printf("page0.n0.val=0%c%c%c", 0xff, 0xff, 0xff);
  printf("page0.x0.val=66000%c%c%c", 0xff, 0xff, 0xff);
  
// Free time setup for drives and shift to mm
  d_rem[0] = 1000;
  d_rem[1] = 3000;
  t_rem[0] = 22;
  t_rem[1] = 52;
    //Starting OCR0A
    printf("page 1%c%c%c", 0xff, 0xff, 0xff);
    printf("page0.n1.val=%d%c%c%c",(int) d_rem[sector], 0xff, 0xff, 0xff);

    saved_OCR = 200;
    while(runflag && battery_flag) {
        
        OCR0A = saved_OCR; //0 - 255 PWM Power Setup

        if ((TIFR1 & 0x20) == 0x20 ){ // Optocoupler signal Reading
        
            count += 1;
            TCNT1 = 0;
            TIFR1 |= 0x20;
            time_diffs[count % (encdr_holes)] = ICR1 * 0.000064; //ticks to seconds, for average RPM calc

            // In case of non-0 time, calculate distance and time remaining, along with required OCR value after each measurement

            d_rem[sector] -= (pi * diameter) / encdr_holes;
            t_rem[sector] -= ICR1 * 0.000064;
            printf("page0.n1.val=%d%c%c%c", (int) d_rem[sector], 0xff, 0xff, 0xff);
            if(d_rem[sector] <= 0){
                if(sector == 1){
                    runflag = 0;
                    OCR0A = 0;
                }
                else{
                    sector += 1;
                }
            }
            if(count>=4){
                saved_OCR = (16 * d_rem[sector] / (t_rem[sector] * pi * k * diameter)) + base_OCR;
            }
            
            if(!(count % (encdr_holes))){ // RPM printing every full wheel turn
                count_in_cycle = 0;
                time_rotation = 0;
                for(int i = 0; i < encdr_holes; i++){ // Calculating RPM via 1 (or however many is needed) last cycle and adjusting for '0' measurements and 1st cycle
                    if (time_diffs[i] != 0){
                        count_in_cycle += 1;
                        time_rotation += time_diffs[i];
                    }
                }
                RPM_for_screen = (60 / time_rotation) * (4 * encdr_holes/count_in_cycle) * 4000; //Getting RPM with a accuracy of 3 digits after dot for Nextion
                printf("page0.x0.val=%ld%c%c%c", RPM_for_screen, 0xff, 0xff, 0xff); 
            }

        } 
        

        adclow = ADCL; // Battery Voltage measurement
        printf("page0.n0.val=%lu%c%c%c", 300 * (adclow + ((ADCH & 0x03) << 8)) / 1024 * 5, 0xff, 0xff, 0xff);
        if ( 300 * (adclow + ((ADCH & 0x03) << 8)) / 1024 * 5 <=670){ // Stop Program on Less than set value on Voltage - WORKING
            battery_flag = 0; 
            printf("page 2%c%c%c", 0xff, 0xff, 0xff);
            OCR0A = 0;
        }

    }
    return 0;
}