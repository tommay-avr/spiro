#define F_CPU (9.6e6 / 64)

#include <avr/io.h>
#include <util/delay.h>
#include <util/delay_basic.h>
#include <avr/fuse.h>
#include <stdint.h>

/*
  PB0/OCOA pin 5: motor pwm
  PB3 pin 2: switch
  PB4/ADC2 pin 3: knob
*/

// If we make the PWM too low the motor will stop.  So we scale the
// values 0 -> 255 to PWM_MIN -> 255.  The average voltage from the
// PWM is equal to the ADC voltage since they're both linear from 0 to
// 3.3V.  PWM_MIN corresponds to 0.8V which makes sense since the
// motor is spec'd to run down to 1V.

#define PWM_MIN (62)

static uint8_t
read_adc(void)
{
  ADCSRA |= _BV(ADSC);
  loop_until_bit_is_clear(ADCSRA, ADSC);
  return ADCH;
}

static inline void
set_pwm(uint8_t pwm)
{
  OCR0A = pwm;
}

// Scale 0 -> 255 to PWM_MIN -> 255
static uint8_t
scale_pwm(uint8_t in)
{
  return (uint8_t)(((uint16_t)(255 - PWM_MIN) * in + 127) / 255) + PWM_MIN;
}

int
main(void)
{
  // Clock is 9.6MHz.  Prescale by 64 to get 150kHz which allows
  // us to use the ADC at the default prescale of /2 which gives
  // 75kHz which is within 50-200kHz.
  // Interrupts must be disabled for these two lines.  They are.

  CLKPR = _BV(CLKPCE);		// Enable prescaler to be set.
  CLKPR = 6;			// Divide by 64 (150kHz).

  // Switch (PB3) is input (default) with pull-up enabled.

  PORTB |= _BV(PB3);		// Enable pull-up.

  // Knob (PB4/ADC2) is input (default) with pull-up disabled (default)
  // and digital input buffer disabled.

  DIDR0 |= _BV(ADC2D);		// Disable digital input buffer.

  // ADC setup:

  // Select ADC2.
  ADMUX |= _BV(MUX1);
  // Left adjust ADC result so it appears in ADCH.
  ADMUX |= _BV(ADLAR);
  // Clock prescaler is /2 (default).
  // Enable the ADC.
  ADCSRA |= _BV(ADEN);

  // PWM setup:

  // Fast PWM mode, TOP = 0xFF.

  TCCR0A = 0x83;

  // Select clock = CPU/1 which starts the timer.

  TCCR0B |= _BV(CS00);

  DDRB |= _BV(DDB0);		// Pin 4 (OC0A) is output.

  // Enable pull-ups on unused/floating input pins.

  PORTB |= _BV(PB1) | _BV(PB2) | _BV(PB5);

  uint8_t adc = read_adc();
  uint16_t rnd = adc << 8;	/* "Entropy". */

  // Set the motor to full power briefly to make sure it starts up.

  uint8_t pwm = 0xFF;
  set_pwm(pwm);

  _delay_ms(250);

  for (;;) {
    if ((PINB & (_BV(PB3))) == 0) {
      // Switch is off, copy ADC to PWM.
      uint8_t adc = read_adc();
      rnd += adc;
      pwm = scale_pwm(adc);
      set_pwm(pwm);
    }
    else {
      // Switch is on.  Ramp between random pwm values with ramp rate
      // controlled by ADC.

      /* Generate a new random rnd value, and ramp to it at a rate
	 controlled by adc.  Higher adc = faster rate.  */

      rnd = (rnd << 2) + rnd + 0x3333;
      uint8_t to_pwm = scale_pwm(rnd >> 8);

#define delta_t (255)
      int16_t delta_p = to_pwm - pwm;
      int8_t ip = 1;
      if (delta_p < 0) {
	ip = -1;
	delta_p = -delta_p;
      }
      delta_p <<= 1;
      int16_t error = -delta_t;
 
      for (int16_t t = delta_t; t >= 0; t--) {
	error += delta_p;
	if (error >= 0) {
	  error -= delta_t << 1;
	  pwm += ip;
	  set_pwm(pwm);
	}

	int16_t counter = 0x2000;
	int16_t counter_delta = (int16_t)read_adc() + 10;
	while ((counter -= counter_delta) >= 0) {
	  _delay_loop_1(6);
	}
      }
    }
  }
}

FUSES = {
  // Might want to set BOD level.
  .low = LFUSE_DEFAULT,
  .high = HFUSE_DEFAULT,
};

/*
x[n+1] = (x[n]*a + b) mod m
If b is nonzero, the maximum possible period m
is obtained if and only if:
- Integers m and b are relatively prime, that is, have no common  
factors other than 1.
- Every prime number that is a factor of m is also a factor of a-1.
- If integer m is a multiple of 4, a-1 should be a  multiple of 4.
- Notice that all of these conditions are met if m=2^k, a = 4c + 1,
and b is odd. Here, c, b, and k are positive integers.
*/

/*
TMR2 counts at Fosc/4, and may be further reduced by the prescaler.
The period is set by PR2.  The pulse width is a fraction of this period.
With Fosc = 250KHz, clock is 62.5KHz
With PR2 = 0xFF this gives a period 256 clocks = 4ms (244Hz) which
may be ok.  For testing with a flashing LED, using prescalar of 1:64
gives 3.8Hz.  1:16 is 15.25Hz, 1:4 is 61 Hz, 1:1 is 244Hz.
*/
