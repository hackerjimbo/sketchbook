#include <Arduino.h>

const byte bits (4);          // Bit resolution
const byte leds (6);          // In our case 6 LEDs

volatile byte values[leds];   // value for each LED

void
setup ()
{
  cli();                      // Stop interrupts for a bit
  
  TCCR1 = 0;                  // stop the timer
  TCNT1 = 0;                  // zero the timer
  GTCCR = _BV (PSR1);         // reset the prescaler
  OCR1A = 49;                 // set the compare value
  OCR1C = 49;
  
  //interrupt on Compare Match A, save Timer0's values
  TIMSK = (TIMSK & (_BV(OCIE0A)|_BV(OCIE0B)|_BV(TOIE0))) | _BV(OCIE1A);        
  
  //start timer, ctc mode, prescaler clk/16384
  // It divides by 2^(n-1) where n is CS13-CS10. So, this gives us:
  //                       div    freq     max time (uS)
  //                CS10     1 8000000.00       32.0
  //           CS11          2 4000000.00       64.0
  //           CS11 CS10     4 2000000.00      128.0
  //      CS12               8 1000000.00      256.0
  //      CS12      CS10    16  500000.00      512.0
  //      CS12 CS11         32  250000.00     1024.0
  //      CS12 CS11 CS10    64  125000.00     2048.0
  // CS13                  128   62500.00     4096.0
  // CS13           CS10   256   31250.00     8192.0
  // CS13      CS11        512   15625.00    16384.0
  // CS13      CS11 CS10  1024    7812.50    32768.0
  // CS13 CS12            2048    3906.25    65536.0
  // CS13 CS12      CS10  4096    1953.12   131072.0
  // CS13 CS12 CS11       8192     976.56   262144.0
  // CS13 CS12 CS11 CS10 16384     488.28   524288.0
  
  // This gives 1MHz clock ticks

  TCCR1 = _BV (CTC1) | /* _BV (CS13) |*/ _BV (CS12) /* | _BV(CS11) | _BV(CS10)*/;

  sei();
  
  // Heartbeat each LED in turn
  
  for (byte i (0); i < leds; ++i)
  {
    values[i] = 0xff;
    delay (100);
    values[i] = 0x00;
    delay (100);
    values[i] = 0xff;
    delay (100);
    values[i] = 0x00;
    delay (700);
  }
  
  // Now fade in/out each LED in turn
  
  for (byte i (0); i < leds; ++i)
  {
    for (byte j (0); j < 255; ++j)
    {
      values[i] = j;
      delay (2);
    }
    for (byte j (0); j < 255; ++j)
    {
      values[i] = 254 - j;
      delay (2);
    }
  }
}

void
loop ()
{
  static const byte step[leds] =
  {
    1, 3, 5, 7, 11, 13
  };
  
  static boolean up[leds] =
  {
    true, true, true, true, true, true
  };
  
  static byte count[leds] =
  {
    0, 0, 0, 0, 0, 0
  };
  
  for (byte i (0); i < leds; ++i)
  {
    count[i] += 1;
    
    if (count[i] == step[i])
    {
      if (up[i])
      {
        if (values[i] == 0xff)
        {
          values[i] = 0xfe;
          up[i] = false;
        }
        else
        {
          values[i] += 1;
        }
      }
      else
      {
        if (values[i] == 0x00)
        {
          values[i] = 0x01;
          up[i] = true;
        }
        else
        {
          values[i] -= 1;
        }
      }
      
      count[i] = 0;
    }
  }
  
  delay (1);
}

ISR (TIMER1_COMPA_vect)
{
  static const byte pbits[leds] =
  {
    _BV (PORTB0),             // LED 0
    _BV (PORTB1),             // LED 1
    _BV (PORTB0),             // LED 2
    _BV (PORTB2),             // LED 3
    _BV (PORTB1),             // LED 4
    _BV (PORTB2)              // LED 5
  };
  
  static const byte dbits[leds] =
  {
    _BV (DDB0) | _BV (DDB1),  // LED 0
    _BV (DDB0) | _BV (DDB1),  // LED 1
    _BV (DDB0) | _BV (DDB2),  // LED 2
    _BV (DDB0) | _BV (DDB2),  // LED 3
    _BV (DDB1) | _BV (DDB2),  // LED 4
    _BV (DDB1) | _BV (DDB2)   // LED 5
  };
  
  static byte bitno (0);      // Bit we're currently looking at [0, bits)
  static byte frame (0);      // Frame inside that bit
  static byte led (0);        // LED we're looking at [0, leds)
  
  const byte scan (1 << (bitno + (8 - bits)));
  
  PORTB = 0;
  DDRB = ((values[led] & scan)) ? dbits[led] : 0;
  PORTB = pbits[led];

#if 0
  frame += 1;
  
  // If we've done all the frames for this bit then advance onto the next bit
  
  if (frame == (1 << bitno))
  {
    frame = 0;
    bitno += 1;

    // If we've done all the bits then start again
    
    if (bitno == bits)
    {
      bitno = 0;
      led += 1;
      
      if (led == leds)
        led = 0;
    }
  }
#else
  led += 1;
  
  if (led == leds)
  {
    led = 0;
    frame += 1;
  
    // If we've done all the frames for this bit then advance onto the next bit
  
    if (frame == (1 << bitno))
    {
      frame = 0;
      bitno += 1;

      // If we've done all the bits then start again
    
      if (bitno == bits)
        bitno = 0;
    }
  }
#endif
}

