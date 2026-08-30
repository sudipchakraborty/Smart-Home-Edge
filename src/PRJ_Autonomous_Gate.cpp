
////Default 
#include "rs485.h"
#include "modbusASCII.h"
#include "modbusResponseBuilder.h"
#include "memoryMap.h"
#include "DEBUG.h"
#include "LED.h"
#include "RELAY.h"
#include "Buzzer.h"
#include "StatusBlink.h"
#include "Rtc1307.h"
#include "AppTime.h"
#include "Utils.h"
#include "SimulatedClock.h"
#include "eepromStorage.h"
#include "IO.h"
////////////////////////////////////

//// Project specific include //////
#include <PRJ_Autonomous_Gate.h>
#include "PWM.h"
///////////////////////////////////

/// default declaration ///////////
extern StatusBlink st;
extern Autonomous_Gate gate;  
extern debug dbg;
///////////////////////////////////


////specific object ///////////////
PWM Lpwm(PIN_L_PWM,25000,10);          // 1kHz, 10-bit
PWM Rpwm(PIN_R_PWM,25000,10);          // 1kHz, 10-bit
///////////////////////////////////

//=====assign for input=========================
IO home(PIN_SEN_HOME,IO_INPUT_PULLUP,ACTIVE_LOW);
IO end(PIN_SEN_TERMINAL,IO_INPUT_PULLUP,ACTIVE_LOW);
IO indoor(PIN_SW_INDOOR,IO_INPUT_PULLUP,ACTIVE_LOW);

IO outdoor(PIN_SW_OUTDOOR,IO_INPUT_PULLUP,ACTIVE_LOW);


// input assignment  
IO en(PIN_EN,IO_OUTPUT,ACTIVE_HIGH);

IO calling_Bell(PIN_CALLING_BELL,IO_OUTPUT,ACTIVE_LOW);


// PWM Lpwm(PIN_L_PWM,25,ACTIVE_LOW);
// IO Rpwm(PIN_R_PWM,IO_OUTPUT,ACTIVE_LOW);

IO fault(PIN_LED_FAULT,IO_OUTPUT,ACTIVE_LOW);
IO dir(PIN_RE_DE,IO_OUTPUT,ACTIVE_LOW);
Buzzer bz(PIN_BZR,ACTIVE_LOW);
////////////////////////////////


//__________________________________________________________________________________________________________________________
void Autonomous_Gate::PRJ_Autonomous_Gate_SetUp()
{
    // Default setup///////////////
    st.begin(PIN_ST_LED, 50000);
    dbg.begin(Serial1, 115200, 4);
    dbg.println("system started..");
    /////////////////////////////////
    bz.begin();
    en.begin(); 
    en.on();

    Lpwm.begin();
    Lpwm.setDuty(30);
    Lpwm.disable();

    Rpwm.begin();
    Rpwm.setDuty(30);
    Rpwm.disable();

    gate.FSM = FSM_init;
    home.begin();
    end.begin();
    bz.begin();
    bz.beep();
}
//_________________________________________________________________________________________________________
void Autonomous_Gate::PRJ_Autonomous_Gate_Loop()
{
    st.blink();
    FSM_Handler();
}
//_________________________________________________________________________________________________________
void Autonomous_Gate::FSM_Handler()
{
    switch (gate.FSM)
    {
    case FSM_init:
         if (Go_Home())
         {
             gate.FSM = FSM_Read_Indoor_switch;
         }
        break;
    ////////////////////////////////////////////
    case FSM_Read_Indoor_switch:
         if (indoor.read() == true)
         {
             dbg.println("Indoor switch pressed");
            
            //  // JOG
            int speed=25;
            do{
                Motor_Rotate_Towards_Terminal(speed);   
                delay(400);
                speed+=5;
            }while(speed<50);

            
            speed=40;
            do{
                Motor_Rotate_Towards_Terminal(speed);   
                delay(300);
                speed-=5;
            }while(speed>25);

            Motor_Rotate_Towards_Terminal(25); 

            // ACCLn.
            // for(int k=25;k<50;k++)
            // {
            //     Motor_Rotate_Towards_Terminal(k);          delay(500);       
            // }
            
            // // JOG
            // Motor_Rotate_Towards_Terminal(50);   delay(3000);


            // DECLn.
             // ACCLn.
            // for(int k=50;k<25;k--)
            // {
            //     Motor_Rotate_Towards_Terminal(k); delay(500);       
            // }
             
             gate.FSM = FSM_Door_Opening;
         }
         else
         {
             dbg.println("Indoor switch not pressed:",gate.loop_counter++);
             delay(10);
         }
        
        break;
    /////////////////////////////////////////////
    case FSM_Door_Opening:
        dbg.println("Door opening:",gate.loop_counter++);

        if (end.read() == true)
        {
            dbg.println("Terminal sensor triggered");
            Motor_Stop();
            gate.FSM = FSM_Wait_For_Closing;
        }
        else
        {
            dbg.println("Terminal sensor not triggered:",gate.loop_counter++);
            // delay(1000);
        }
        break;
    /////////////////////////////////////////////
    case FSM_Wait_For_Closing:
        gate.loop_counter = 0;
        for(int i=0;i<10;i++)
        {
            dbg.println("Waiting for door to close:",gate.loop_counter++);
            delay(1000);
        }
        dbg.println("Waiting time over. Going to close door");
        Motor_Rotate_Towards_Home(25);
        // delay(1000);
        // Motor_Rotate_Towards_Home(30);
        // delay(5000);
        // Motor_Rotate_Towards_Home(Speed_JoG);
        // delay(1000);
        FSM = FSM_Door_Closing;     
        break;
    /////////////////////////////////////////////
    case FSM_Door_Closing:
        dbg.println("Door closing:",gate.loop_counter++);
        if (home.steady_read() == true)
        {
            dbg.println("Home sensor triggered");
            Motor_Stop();
            gate.FSM = FSM_Read_Indoor_switch;
        }
        else
        {
            dbg.println("Home sensor not triggered:",gate.loop_counter++);
            // delay(1000);
        }
        break;  
    /////////////////////////////////////////////
    
    default:
        break;
    }
}
//_________________________________________________________________________________________________________
bool Autonomous_Gate::Go_Home()
{
    dbg.println("Checking home sensor...");
    delay(1000);
 
    if(home.steady_read()==true)
    {
       dbg.println("The door is in Home position. No need to move.");
       return true;
    }
    else
    {      
        dbg.println("The door is not in Home position. Moving towards home...");
        delay(1000);
        Motor_Rotate_Towards_Home(Speed_JoG);
    }   

    do
    {
        dbg.println("waiting for home sensor to be triggered:",gate.loop_counter++);
        bz.beep();
    }while(home.steady_read()==false);

    Motor_Stop();
    return true;
}
//_________________________________________________________________________________________________________
void Autonomous_Gate::Motor_Rotate_Towards_Home(char dutyCycle)
{
    Lpwm.disable();

    Rpwm.setDuty(dutyCycle);
    Rpwm.enable();
}
//_________________________________________________________________________________________________________
void Autonomous_Gate::Motor_Rotate_Towards_Terminal(char dutyCycle)
{
    Rpwm.disable();

    Lpwm.setDuty(dutyCycle);
    Lpwm.enable();
}
//_________________________________________________________________________________________________________
void Autonomous_Gate::Motor_Stop()
{
    Lpwm.disable();
    Rpwm.disable();
}
//_________________________________________________________________________________________________________







//   if(home_tirggered())
//     {
//         dbg.println("Home sensor triggered");
//     }
//     else
//     {
//         dbg.println("Home sensor not triggered");
//     }


    // // Lpwm.enable();
    // // Rpwm.disable();
    
    // dbg.println("Left PWM ON");
    
    // delay(5000);
    
    // Lpwm.disable();
    // delay(5000);

    // Rpwm.enable();
    // delay(5000);
    // Rpwm.disable();
    // delay(5000);

    // Lpwm.test();

    // en.test();
    // Lpwm.test();




    

    // calling_Bell.begin();
    // Lpwm.begin(); 

    // Rpwm.begin(); 
    // Ren.begin(); 
    // fault.begin(); 
    // dir.begin(); 
    // bzr_gt.begin();
