/* MIT License

Copyright (c) 2020 Matthieu Baumann

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

volatile uint32_t toggle = 0;
volatile uint64_t dividing = 0;
// dividing step
volatile uint8_t d = 1;
// rotate step
volatile uint8_t h = 0;

volatile bool rst_triggered = false;

void setup() {
  // Switch off the UART call from the bootloader
  // We do that to be able to use PD0 and PD1 as output pins
  UCSR0B = 0;
  
  // Enable Global Interrupts
  SREG |= (1 << 7);

  /* Setup the Timer/Counter1 */
  // Enable Timer/Counter1 Overflow Interrupt
  TIMSK1 |= (1 << TOIE1);

  // Enable normal mode where the Timer/Counter1 counts to
  // 0xFF, overflows and repeats.
  TCCR1A = 0;

  // Set the prescaler to 1024 so that the Timer/Counter1
  // has a frequency of 16MHz/1024 ~= 15625Hz
  TCCR1B &= ~(1 << CS12);
  TCCR1B |= (1 << CS11);
  TCCR1B &= ~(1 << CS10);
  
  /* Setup the analog comparator */
  //  Compare the input voltage with a Bandgap voltage send on the positive input of the comparator.
  ACSR |= (1 << ACBG);
  // Enable analog comparator interrupt on failing edge (bit 1) which would actually capture a rising edge of the signal.
  ACSR &= ~(1 << ACIS1);
  ACSR &= ~(1 << ACIS0);
  delay(5); // A short wait for bandgap voltage to stabilize.

  // Analog Comparator Interrupt Enable 
  ACSR |= (1 << ACIE);

  // PD7 is connected to AIN7 (Analog Comparator Negative Input).
  //DDRD &= ~(1 << DDD7);

  /* Setup the ADC to trigger mode */
  // convert GND 0V to digital
  ADMUX |= 0x40;

  // ADC Enable
  ADCSRA |= (1 << ADEN);

  // ADC start first conversion for running mode
  ADCSRA |= (1 << ADSC);

  // ADC Auto Trigger Enable
  //ADCSRA |= (1 << ADATE);

  // ADC Interrupt Enable
  ADCSRA |= (1 << ADIE);

  // ADC Auto Trigger Source set to Analog Comparator Interrupt
  //ADCSRB = (1 << ADTS1) | (1 << ADTS2);
  //ADCSRB = 0x0;

  // Wait some cycles for the first conversion to complete
  __builtin_avr_delay_cycles(128);

  /* Clock dividing outputs */
  DDRB = 0xFF;

  /* Set PD0, PD1 as led outputs */
  /* Set PD4 as input (reset) */
  /* Set PD7 as input (master clk signal) */
  DDRD = 0x6B;

  //Serial.begin(9600);
}

void loop() {
  // Check if a reset occurs on PORTD4
  bool reset = !(PIND & (1 << PIND4));
  if(reset) {
    if(!rst_triggered) {
      resetClock();
    }
    rst_triggered = true;
  } else {
    rst_triggered = false;
  }
}

// Timer 1 overflow ISR
ISR(TIMER1_OVF_vect)
{
  if(PORTD & (1 << PORTD2)) {
    PORTD &= ~(1 << PORTD2);
  } else {
    PORTD |= (1 << PORTD2);
  }
}

// Analog -> Digital converter
ISR(ADC_vect) {
  // PC1 enabled to read
  // PC1 is connected to the clock division ratio, can be in {1, 2, 4, 8}
  if (ADMUX == 0x41) {
      // Retrieve rotate value from the ADC
      uint16_t D = 0;
      D |= ADCL | (ADCH << 8);
    
      // Compute dividing value 
      d = (D >> 8) + 1; // value between [0; 3] + 1 = [1; 4]
      // Setting the ADMUX to PC0 input
      ADMUX = 0x40;
  // PC0 enabled to read
  // PC0 reads a voltage between 0 and 5 volts that encodes the rotation factor applied to the output clocks.
  } else if (ADMUX == 0x40) {
      // Retrieve rotate value from the ADC
      uint16_t R = 0;
      R |= ADCL | (ADCH << 8);

      /* Compute new rotation value
       * Possible range values:
       *  
       *  [0; 127] -> 0
       *  [128; 255] -> 1
       *  ...
       *  [896; 1023] -> 7
       */
      h = R >> 7; // value between [0; 7]

      // Setting ADMUX to PC1 input
      ADMUX = 0x41;
  }
  // Start a new conversion
  ADCSRA |= (1 << ADSC);
}

/* This checks whether x is a multiple of 2 power of n.
 * 
 * With n = 2 this checks whether x is a power of 2^2 = 4.
 * x is left shifted of 2 bits (present one on the first and second bits are thus removed).
 * We right shift again by 2 bits the result and check if it equal to x. If yes, we know
 * the first and second bits of x were zeros.
*/
bool is_multiple_of_2_power_n(uint64_t x, uint8_t n) {
  return ((x >> n) << n) == x;
}

/* Atmega comparator ISR.
 *
 * A ISR is launched when the input clock goes above the bandgap constant voltage of the comparator.
 * This means a new cycle of the input clock begins!
*/
ISR(ANALOG_COMP_vect)
{
  dividing += 1;
  // Normal speed
  if (d == 1) {
    step_forward();
  // speed / 2
  } else if (d == 2 && dividing & 0x1) {
    step_forward();
  // speed / 4
  } else if (d == 3 && is_multiple_of_2_power_n(dividing, 2)) {
    step_forward();
  // speed / 8
  } else if (d == 4 && is_multiple_of_2_power_n(dividing, 3)) {
    step_forward();
  }
}

void step_forward() {
  toggle += 1;
  if(toggle & 0x1) {
    PORTB &= 0x0;
    // Reset the pins PD0 and PD1
    PORTD &= ~((1 << PORTD0) | (1 << PORTD1));
  } else {
    uint32_t raising_clock = (toggle >> 1);
    
    // Divided by 1 => copy of the clock signal
    triggerClockDivision(0);

    // Divided by 2
    if(!(raising_clock & 0x1)) {
      triggerClockDivision(1);
    }

    // Divided by 3
    if(raising_clock % 3 == 0) {
      triggerClockDivision(2);
    }

    // Divided by 4
    if(raising_clock % 4 == 0) {
      triggerClockDivision(3);
    }

    // Divided by 5
    if(raising_clock % 5 == 0) {
      triggerClockDivision(4);
    }
    
    // Divided by 6
    if(raising_clock % 6 == 0) {
      triggerClockDivision(5);
    }
    
    // Divided by 7
    if(raising_clock % 7 == 0) {
      triggerClockDivision(6);
    }
    
    // Divided by 8
    if(raising_clock % 8 == 0) {
      triggerClockDivision(7);
    }

    // Reset the output clocks 
    if(raising_clock == 5*6*7*8) {
      resetClock();
    }
  }
}

void triggerClockDivision(uint8_t id) {
  // Process rotation
  id = (id + h) % 8;

  // Activate corresponding clock division
  switch(id) {
    case 1:
      PORTB |= (1 << PORTB1);
      break;
    case 2:
      PORTB |= (1 << PORTB2);
      break;
    case 3:
      PORTB |= (1 << PORTB3);
      break;
    case 4:
      PORTB |= (1 << PORTB4);
      break;
    case 5:
      PORTB |= (1 << PORTB5);
      break;
    case 6:
      PORTD |= (1 << PORTD0);
      break;
    case 7:
      PORTD |= (1 << PORTD1);
      break;
    default:
      PORTB |= (1 << PORTB0);
      break;
  }
}

void resetClock() {
  dividing = 0;
  toggle = 0;
}
