#ifndef RELAY_H
#define RELAY_H

#include "Arduino.h"
//////////////////////
class relay
{
    public:
           relay(byte pinNo);
      void begin(void);
      void on(void);
      void off(void);
      void toggle(void);
      void test(void);
      void test(long delayTime);
    private: 
      byte pin;      
};
///////////////////////











#endif
