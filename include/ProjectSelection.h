#ifndef PROJECT_SELECTION_H
#define PROJECT_SELECTION_H

// Default project selection. A PlatformIO environment can override this.
#if !defined(LowPowerTimerModule_V1) && !defined(LowPowerTimerModule_V2)
#define LowPowerTimerModule_V1
#endif
// #define PRJ_Autonomous_Gate
// #define PRJ_Template
// #define Prj_Server
// #define DispenserModule

// Internal defines for backwards compatibility with project files
#ifdef LowPowerTimerModule_V1
  #define LowPowerTimerModule
#endif

#ifdef LowPowerTimerModule_V2
  // V2 already uses this public selection name directly.
#endif

#if defined(LowPowerTimerModule_V1) && defined(LowPowerTimerModule_V2)
#error "Select only one Low Power Timer Module version"
#endif

#endif // PROJECT_SELECTION_H
