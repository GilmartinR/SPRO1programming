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


  double time_for_count = 0; // Time difference between signals
  uint32_t count = 0; // Total amount of signals
  unsigned count_in_cycle = 0; // Security measure to avoid "0" results when calculatng avg RPM
  double time_rotation = 0; // Time for a full rotation
  double time_diffs[encdr_holes]; // Last 8 time differences
  double RPM = 0; // RPM in double format
  uint32_t RPM_for_screen; // RPM in UNSIGNED LONG INT format for NEXTION

  for(int i = 0; i < encdr_holes; i++){
    time_diffs[i] = 0;
  }

  //Test of screen connection
  printf("page0.n0.val=0%c%c%c", 0xff, 0xff, 0xff);
  printf("page0.x0.val=66000%c%c%c", 0xff, 0xff, 0xff);
  
  while(1) {
		if ((TIFR1 & 0x20) == 0x20 ){ // Optocoupler has received a signal
      count += 1;
      TCNT1 = 0;
      TIFR1 |= 0x20;
      time_for_count = ICR1 * 0.000064; //ticks to seconds
      time_diffs[count % encdr_holes] = time_for_count;

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
        RPM = (60 / time_rotation) * (encdr_holes/count_in_cycle) * 1000; //Getting RPM with a accuracy of 3 digits after dot for Nextion
        RPM_for_screen = RPM;
        printf("page0.x0.val=%lu%c%c%c", RPM_for_screen, 0xff, 0xff, 0xff);
      }  
    }
  }
  
  return 0;
}