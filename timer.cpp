/*
 * timer.cpp - class to do accurate timing
 */

#include "timer.h"
#include <Arduino.h>

timer::timer()
{
  m_SetTime = micros();
}

void timer::set()
{
  m_SetTime = micros();
}

uint32_t timer::getDeltaMicro()
{
  uint32_t now = micros();
  if (now < m_SetTime)
  {
    // handle time rollover
    uint32_t remainder = UINT32_MAX - m_SetTime;
    return (now + remainder);
  }

  return (now - m_SetTime);
}

