#ifndef PROJECT_SELECTION_H
#define PROJECT_SELECTION_H

#if defined(LowPowerTimerModule_V1) && defined(LowPowerTimerModule_V2)
  #error "Select only one Low Power Timer Module version"
#endif

#if !defined(LowPowerTimerModule_V1) && !defined(LowPowerTimerModule_V2)
  #error "Select LowPowerTimerModule_V1 or LowPowerTimerModule_V2 in platformio.ini"
#endif

#ifdef LowPowerTimerModule_V1
  #define LowPowerTimerModule
#endif

#endif // PROJECT_SELECTION_H
