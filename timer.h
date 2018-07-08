/*
  timer.h - 
 */
 
#ifndef timer_h
#define timer_h

#include <stdint.h>

class timer
{
public:
  timer();
  void set();
  uint32_t getDeltaMicro();
  
private:
  uint32_t m_SetTime;
};

#endif
