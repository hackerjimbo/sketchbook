#include <Arduino.h>

volatile byte state (0);
volatile byte buffer;
volatile byte count;

const byte tx (3);
const byte sensor (A2);

void
setup ()
{
  pinMode (tx, OUTPUT);
  pinMode (sensor, INPUT_PULLUP);
  
  cli();                      // Stop interrupts for a bit
  
  TCCR1 = 0;                  // stop the timer
  TCNT1 = 0;                  // zero the timer
  GTCCR = _BV (PSR1);         // reset the prescaler
  OCR1A = 208;                // set the compare value
  OCR1C = 208;
  
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
  
  // This gives 2MHz clock ticks

  TCCR1 = _BV (CTC1) | /* _BV (CS13) | _BV (CS12)  | */ _BV(CS11) | _BV(CS10);

  sei();
}

void
loop ()
{
  char buf[5];
  int v (analogRead (sensor));
  
  for (signed char b = 3; b >= 0; --b)
  {
    buf[b] = '0' + (v % 10);
    v /= 10;
  }
  
  buf[4] = '\0';
  
  say (buf);
  say ("\r\n");
  
  delay (200);
}

void
say (const char *what)
{
  const char *upto (what);
  
  while (*upto != '\0')
  {
    while (state != 0)
      ;
      
    buffer = *upto;
    state = 1;
    upto += 1;
  }
}

ISR (TIMER1_COMPA_vect)
{
  switch (state)
  {
    case 0:                       // Idle
      break;
      
    case 1:                       // Start bit
      count = 0;
      state = 2;
      digitalWrite (tx, LOW);
      break;
      
    case 2:                       // Clock data
      if (count == 8)
      {
        state = 3;
      }
      else
      {
        digitalWrite (tx, ((buffer >> count) & 1) ? HIGH : LOW);
        count += 1;
      }
      
      break;
      
    case 3:                       // Stop bit
      digitalWrite (tx, HIGH);
      state = 0;
      break;
      
    default:                      // Just in case
      state = 0;
      break;
  }
}

