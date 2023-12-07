#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include "usart.h"

#define encdr_holes 8

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



  uint32_t count = 0; // Total amount of signals
  unsigned count_in_cycle = 0; // Security measure to avoid "0" results when calculatng avg RPM
  double time_rotation = 0; // Time for a full rotation
  double time_diffs[encdr_holes]; // Last 8 time differences
  uint32_t RPM_for_screen; // RPM in UNSIGNED LONG INT format for NEXTION
  uint32_t adclow;

  for(int i = 0; i < encdr_holes; i++){
    time_diffs[i] = 0;
  }

  //Test of screen connection
  printf("page0.n0.val=0%c%c%c", 0xff, 0xff, 0xff);
  printf("page0.x0.val=66000%c%c%c", 0xff, 0xff, 0xff);
  
  while(1) {
    OCR0A = 255;
		if ((TIFR1 & 0x20) == 0x20 ){ // Optocoupler has received a signal
      
      count += 1;
      TCNT1 = 0;
      TIFR1 |= 0x20;
      time_diffs[count % encdr_holes] = ICR1 * 0.000064; //ticks to seconds

      // Using function is too slow
      count_in_cycle = 0;
      time_rotation = 0;
      if(count > 1){ // Security measure against first count
        for(int i = 0; i < encdr_holes; i++){ // Calculating RPM via 1 (or however many is needed) last cycle and adjusting for '0' measurements and 1st cycle
          if (time_diffs[i] != 0){
            count_in_cycle += 1;
            time_rotation += time_diffs[i];
          }
        }
        RPM_for_screen = (60 / time_rotation) * (encdr_holes/count_in_cycle) * 1000; //Getting RPM with a accuracy of 3 digits after dot for Nextion
        printf("page0.x0.val=%lu%c%c%c", RPM_for_screen, 0xff, 0xff, 0xff);
      }  
    }
    adclow = ADCL;

     printf("page0.n0.val=%lu%c%c%c", 300 * (adclow + ((ADCH & 0x03) << 8)) / 1024 * 5, 0xff, 0xff, 0xff);
  }
 
  return 0;
}