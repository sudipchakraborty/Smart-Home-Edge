#define LowPowerTimerModule
// #define PRJ_Autonomous_Gate
// #define PRJ_Template
// # define Prj_Server
// #define DispenserModule
////////////////////////////////////////////////////////////////////////////////////
#ifdef LowPowerTimerModule
#include <PRJ_LPTM.h>
void setup() { LPTM_setup(); }
void loop(){ LPTM_loop();}
#endif
////////////////////////////////////////////////////////////////////////////////////
#ifdef DispenserModule
#include <PRJ_Dispenser.h>    
void setup() { DispenserModule_setup(); }
void loop(){ DispenserModule_loop();}
#endif
////////////////////////////////////////////////////////////////////////////////////
#ifdef Prj_Server
#include <PRJ_Server.h>    
void setup() {  Server_setup(); }
void loop(){ Server_loop();}
#endif
////////////////////////////////////////////////////////////////////////////////////
#ifdef PRJ_Autonomous_Gate
#include <PRJ_Autonomous_Gate.h>  
    Autonomous_Gate gate;  
    void setup() {gate.PRJ_Autonomous_Gate_SetUp(); }
    void loop(){ gate.PRJ_Autonomous_Gate_Loop();}
#endif
/////////////////////////////////////////////////////////////////////////////////
#ifdef PRJ_Template
    #include <PRJ_Template.h>    
    void setup() {PRJ_Template_SetUp(); }
    void loop(){ PRJ_Template_Loop();}
#endif
/////////////////////////////////////////////////////////////////////////////////
