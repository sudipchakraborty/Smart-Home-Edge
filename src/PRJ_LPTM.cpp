#include "ProjectSelection.h"
#ifdef LowPowerTimerModule

#include "PRJ_LPTM.h"
#include "rs485.h"
#include "modbusASCII.h"
#include "modbusResponseBuilder.h"
#include "memoryMap.h"
#include "DEBUG.h"
//////////////////////////////////
#include "LED.h"
#include "RELAY.h"
#include "Buzzer.h"
#include "StatusBlink.h"
#include "Rtc1307.h"
#include "AppTime.h"
#include "Utils.h"
#include "SimulatedClock.h"
#include "eepromStorage.h"
#include "RelayScheduleHmsStorage.h"
#include "I2CScanner.h"
#include "at24c32N.h"
/////////////////////////////////////////////////////////////////////////////////////////
uint8_t MY_SLAVE_ID = 1;   // change per device   1
RS485 rs485;
char txBuf[255];
uint8_t rxBuf[255];
ModbusASCII mb;
ModbusASCIIFrame modbusFrame;

debug dbg;
StatusBlink st;
Rtc1307 rtc;
AppTime SecondTick;

AppTime RL1_Time;
AppTime RL2_Time;
////////////////////////////
led ledTx(LED_TX,0);
led ledRx(LED_RX,0);
relay rl1(RL1);
relay rl2(RL2);
Buzzer bzr(BZR, 0);

int FSMState;
SimulatedClock simClock;
EEPROMStorage eeprom;

int countValue = 0;
uint16_t temp[2];
long Modbus_REceive_PacketCount=0;
////////////////////////////////////////////////////////////////////////////////////
void LPTM_setup()
{
    dbg.begin(Serial1, 115200, 4);
    dbg.println("system started..");

    st.begin(LED_PIN, 5000);
    rtc.begin();

    rs485.DefaultSetUp();
    dbg.println("system Initialized..");

    SecondTick.set_time(1000);
    SecondTick.start();

    const bool eepromReady = eeprom.begin(21, 22, LPTM_EEPROM_I2C_ADDR);
    if (eepromReady)
        dbg.println("AT24C32 detected at project I2C address 0x50");
    else
        dbg.println("EEPROM PROBE ERROR: AT24C32 not detected at project I2C address 0x50");

    // Discover_I2C_Devices();
    
    delay(1000);
    /////////////////////////////
    LoadModbusConfigFromEEPROM();
    Update_Time_From_Modbus();
 
    RL1_Time.set_time(10000);
    RL1_Time.reset();
    RL1_Time.start();

    RL2_Time.set_time(10000);
    RL2_Time.reset();
    RL2_Time.start();

    FSMState= IDLE;
    ///////////////////////////
     String s = rtc.readDT_As_ddmmyyyyhhmmss();
     dbg.println("RTC Time: ", s);

     simClock.set(s);
     uint32_t startSec = Utils::DT_String_To_Seconds_From_TimePart(s.c_str());
     dbg.print("Time In second: ", startSec);

     dbg.println("System Initialized..");
     bzr.beep();
     delay(1000);
}
//__________________________________________________________________________________________
uint8_t Discover_I2C_Devices(void)
{
    dbg.println("Checking I2C devices on SDA 21 / SCL 22...");
    const uint8_t deviceCount = I2CScanner::scan(Serial1);

    if (I2CScanner::devicePresent(0x68))
        dbg.println("RTC detected at I2C address 0x68");
    else
        dbg.println("RTC ERROR: no device detected at I2C address 0x68");

    bool eepromFound = false;
    for (uint8_t address = 0x50; address <= 0x57; ++address)
    {
        if (!I2CScanner::devicePresent(address))
            continue;

        eepromFound = true;
        char message[64];
        snprintf(message, sizeof(message),
                 "EEPROM candidate detected at I2C address 0x%02X", address);
        dbg.println(message);

        if (address == LPTM_EEPROM_I2C_ADDR)
            dbg.println("EEPROM address matches project address 0x50");
        else
            dbg.println("EEPROM ADDRESS MISMATCH: update LPTM_EEPROM_I2C_ADDR");
    }

    if (!eepromFound)
        dbg.println("EEPROM ERROR: no device detected from address 0x50 to 0x57");

    return deviceCount;
}
//__________________________________________________________________________________________
void LPTM_loop()
{  
    st.blink();
    StateMachine();
    Modbus_Handler();
}
//__________________________________________________________________________________________
void StateMachine(void)
{  
    switch (FSMState) 
    {
        case IDLE:                       
             FSMState=READING_RTC;
            break;
        ///////////////////
        case READING_RTC:       
            if(SecondTick.timeOut())
            {
                // simClock.updateMin(30);
                dbg.println("==============");
                String s=rtc.readDT_As_ddmmyyyyhhmmss();

                uint16_t rtcYear = 0;
                uint8_t rtcMonth = 0;
                uint8_t rtcDay = 0;
                uint8_t rtcHour = 0;
                uint8_t rtcMinute = 0;
                uint8_t rtcSecond = 0;

                if (Utils::parseDDMMYYYY_HHMMSS(
                        s.c_str(), rtcYear, rtcMonth, rtcDay,
                        rtcHour, rtcMinute, rtcSecond))
                {
                    modbusMemory[ModbusAddr_RTC_Hour_Reg] = rtcHour;
                    modbusMemory[ModbusAddr_RTC_Minute_Reg] = rtcMinute;
                    modbusMemory[ModbusAddr_RTC_Second_Reg] = rtcSecond;
                    modbusMemory[ModbusAddr_RTC_Day_Reg] = rtcDay;
                    modbusMemory[ModbusAddr_RTC_Month_Reg] = rtcMonth;
                    modbusMemory[ModbusAddr_RTC_Year_Reg] = rtcYear;
                }
                else
                {
                    dbg.println("RTC parse ERROR: expected dd-mm-yyyy HH:mm:ss");
                }






                // String s=simClock.readDT_As_ddmmyyyyhhmmss();

                dbg.println("TIME:",s);
                // dbg.println(Utils::DT_String_To_Seconds_From_TimePart(s.c_str()));

                SecondTick.start();
  
                dbg.print("RL1: ");
                dbg.print("ON:",Utils::secondsToHHMMSS(RL1_Time.onTime)," ");
                dbg.println("OFF:",Utils::secondsToHHMMSS(RL1_Time.offTime));
                
                dbg.print("RL2: ");
                dbg.print("ON:",Utils::secondsToHHMMSS(RL2_Time.onTime)," ");
                dbg.println("OFF:",Utils::secondsToHHMMSS(RL2_Time.offTime));
                
                uint32_t tt = Utils::DT_String_To_Seconds_From_TimePart(s.c_str());
                if(RL1_Time.isWithinWindow(tt))
                {
                    rl1.on(); 
                    dbg.print("RL1:ON ");
                }else
                {
                    rl1.off();
                    dbg.print("RL1:OFF ");
                }

                 if(RL2_Time.isWithinWindow(tt))
                 {
                    rl2.on();
                    dbg.println("RL2:ON"); 
                }
                else
                {
                    rl2.off();
                    dbg.println("RL2:OFF");
                }

                dbg.println("==============");
            }
            // FSMState=CHECK_RL1_TIME;
            break;
        ////////////////////
        case CHECK_RL1_TIME:
            
            // if(RL1_Time.Within_Window()){
            //     rl1.on();
            // }
            // else{
            //     rl1.off();
            // }
            FSMState=CHECK_RL2_TIME;
            break;
        /////////////////////
         case CHECK_RL2_TIME:
             FSMState = IDLE;
            break;
        /////////////////////
        default:
            FSMState = IDLE;
    }
}
//__________________________________________________________________________________________
void Modbus_Handler()
 {
     if (rs485.available()){
        dbg.println("Received data on RS485");
        size_t rxLen = rs485.receivePacket(rxBuf, sizeof(rxBuf) - 1, 20);

        if (rxLen == 0) return;
        rxBuf[rxLen] = '\0';
        if (!mb.parseModbusASCII((char*)rxBuf, modbusFrame)) return;
        
        // ✅ Slave ID filtering (IMPORTANT)
        if (modbusFrame.slaveId != MY_SLAVE_ID && modbusFrame.slaveId != 0)
        {
            dbg.println("Packet not for me, or Not Broadcast...ignored");
            return;
        }
        if (!mb.isModbusPacketHealthy(modbusFrame)) return;
        mb.PacketAvailable=true;
        dbg.println("Modbus Packet COunt=", ++Modbus_REceive_PacketCount);

        mb.debugPrintModbusFrame(modbusFrame, dbg);

        // Holding-register reads must return the RTC value at request time.
        // if (modbusFrame.function == 0x03)
        // {
        //     // Update_RTC_Registers();
        // }

        char txLen = Modbus_BuildResponse(&modbusFrame,txBuf,sizeof(txBuf));
        if (txLen > 0){
            rs485.send((uint8_t*)txBuf, txLen);
            // dbg.print("Sent Response: ");
            // dbg.printHex((uint8_t*)txBuf, txLen);
           
        }
         ModbusActionHandler();
    }
 }
//__________________________________________________________________________________________
void ModbusActionHandler(void)
{
    if (!mb.PacketAvailable) return;
     mb.PacketAvailable=false;

    switch (modbusFrame.function){
        case 0x06: // Write Single Register
            switch(modbusFrame.address)
            {
                case ModbusAddr_Output: // controller pin output control
                    modbusMemory[ModbusAddr_Output] = modbusFrame.value;
                    if(modbusFrame.value & Modbus_RL1) 
                    {
                        rl1.on();
                        dbg.println("Relay 1 ON");
                    } else {
                        rl1.off();
                        dbg.println("Relay 1 OFF");
                    }
                    //////////////////////////
                    if(modbusFrame.value & Modbus_RL2) {
                        rl2.on();
                        dbg.println("Relay 2 ON");
                    } else {
                        rl2.off();
                        dbg.println("Relay 2 OFF");
                    }
                    ////////////////////////////////
                    if(modbusFrame.value & Modbus_BZR) {
                        bzr.on();
                        dbg.println("Buzzer ON");
                    } else {
                        bzr.off();
                        dbg.println("Buzzer OFF");
                    }
                    ////////////////////////////////
                    if(modbusFrame.value & Modbus_LED_TX) {
                        ledTx.on();
                        dbg.println("TX LED ON");
                    } else {
                        ledTx.off();
                        dbg.println("TX LED OFF");
                    }
                    ////////////////////////////////
                    if(modbusFrame.value & Modbus_LED_RX) {
                        ledRx.on();
                        dbg.println("RX LED ON");
                    } else {
                        ledRx.off();
                        dbg.println("RX LED OFF");
                    }
                    ////////////////////////////////
                    if(modbusFrame.value & Modbus_LED_STATUS) {
                        st.on();
                        dbg.println("Status LED ON");
                    } else {
                        st.off();
                        dbg.println("Status LED OFF");
                    }
                break;
                ////////////////////////////////////
                case ModbusAddr_Beep:
                    bzr.beep();
                    dbg.println("Buzzer Beep Triggered");
                break;
                ////////////////////////////////////
                case ModbusAddr_RL1_StartTime_hh: modbusMemory[ModbusAddr_RL1_StartTime_hh] = modbusFrame.value; break;
                case ModbusAddr_RL1_StartTime_mm: modbusMemory[ModbusAddr_RL1_StartTime_mm] = modbusFrame.value; break;
                case ModbusAddr_RL1_StartTime_ss: modbusMemory[ModbusAddr_RL1_StartTime_ss] = modbusFrame.value; break;
                case ModbusAddr_RL1_EndTime_hh: modbusMemory[ModbusAddr_RL1_EndTime_hh] = modbusFrame.value; break;
                case ModbusAddr_RL1_EndTime_mm: modbusMemory[ModbusAddr_RL1_EndTime_mm] = modbusFrame.value; break;
                case ModbusAddr_RL1_EndTime_ss: modbusMemory[ModbusAddr_RL1_EndTime_ss] = modbusFrame.value; break;
                
                case ModbusAddr_RL1_Time_Update:
                {
                    if (Update_Relay1_Time_From_Modbus())
                        dbg.println("Relay 1 Schedule Updated from Modbus Registers");
                    else
                        dbg.println("Relay 1 Schedule Update ERROR");

                    modbusMemory[ModbusAddr_RL1_Time_Update] = 0;
                }
                break;
                case ModbusAddr_RL2_StartTime_hh: modbusMemory[ModbusAddr_RL2_StartTime_hh] = modbusFrame.value; break;
                case ModbusAddr_RL2_StartTime_mm: modbusMemory[ModbusAddr_RL2_StartTime_mm] = modbusFrame.value; break;
                case ModbusAddr_RL2_StartTime_ss: modbusMemory[ModbusAddr_RL2_StartTime_ss] = modbusFrame.value; break;
                case ModbusAddr_RL2_EndTime_hh: modbusMemory[ModbusAddr_RL2_EndTime_hh] = modbusFrame.value; break;
                case ModbusAddr_RL2_EndTime_mm: modbusMemory[ModbusAddr_RL2_EndTime_mm] = modbusFrame.value; break;
                case ModbusAddr_RL2_EndTime_ss: modbusMemory[ModbusAddr_RL2_EndTime_ss] = modbusFrame.value; break;
                
                case ModbusAddr_RL2_Time_Update:
                {
                    if (Update_Relay2_Time_From_Modbus())
                        dbg.println("Relay 2 Schedule Updated from Modbus Registers");
                    else
                        dbg.println("Relay 2 Schedule Update ERROR");

                    modbusMemory[ModbusAddr_RL2_Time_Update] = 0;
                }
                break;

                case ModbusAddr_RTC_Hour: modbusMemory[ModbusAddr_RTC_Hour] = modbusFrame.value; break;
                case ModbusAddr_RTC_Minute: modbusMemory[ModbusAddr_RTC_Minute] = modbusFrame.value; break;
                case ModbusAddr_RTC_Second: modbusMemory[ModbusAddr_RTC_Second] = modbusFrame.value; break;
                case ModbusAddr_RTC_Day: modbusMemory[ModbusAddr_RTC_Day] = modbusFrame.value; break;
                case ModbusAddr_RTC_Month: modbusMemory[ModbusAddr_RTC_Month] = modbusFrame.value; break;
                case ModbusAddr_RTC_Year: modbusMemory[ModbusAddr_RTC_Year] = modbusFrame.value; break;
               
                case ModbusAddr_RTC_Update:
                {
                    const uint16_t hour = modbusMemory[ModbusAddr_RTC_Hour];
                    const uint16_t minute = modbusMemory[ModbusAddr_RTC_Minute];
                    const uint16_t second = modbusMemory[ModbusAddr_RTC_Second];
                    const uint16_t day = modbusMemory[ModbusAddr_RTC_Day];
                    const uint16_t month = modbusMemory[ModbusAddr_RTC_Month];
                    const uint16_t shortYear = modbusMemory[ModbusAddr_RTC_Year];

                    bool valid = hour <= 23 && minute <= 59 && second <= 59 &&
                                 month >= 1 && month <= 12 && shortYear <= 99;
                    uint8_t maximumDay = 0;

                    if (valid)
                    {
                        static const uint8_t daysPerMonth[] =
                            {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
                        const uint16_t year = 2000 + shortYear;
                        maximumDay = daysPerMonth[month - 1];
                        const bool leapYear = ((year % 4 == 0) && (year % 100 != 0)) ||
                                              (year % 400 == 0);
                        if (month == 2 && leapYear)
                            maximumDay = 29;
                    }

                    valid = valid && day >= 1 && day <= maximumDay;
                    if (valid && rtc.setDateTime(2000 + shortYear,
                                                 static_cast<uint8_t>(month),
                                                 static_cast<uint8_t>(day),
                                                 static_cast<uint8_t>(hour),
                                                 static_cast<uint8_t>(minute),
                                                 static_cast<uint8_t>(second)))
                    {
                        dbg.println("RTC Updated: ", rtc.readDateTimeString());
                        Update_RTC_Registers();
                    }
                    else
                    {
                        dbg.println("RTC Update ERROR: invalid date/time registers");
                    }

                    // This is a one-shot command register, not persistent state.
                    modbusMemory[ModbusAddr_RTC_Update] = 0;
                }
                break;

                default:
                  break;
            }
    }



        //  if(modbusFrame.function == 0x06) { // Write Single Register
        //     switch(modbusFrame.address) 
        //     {
        //     case ModbusAddr_Output:
        //           if(modbusFrame.value & Modbus_RL1) 
        //           {
        //             rl1.on();
        //             dbg.println("Relay 1 ON");
        //             } else {
        //                 rl1.off();
        //                 dbg.println("Relay 1 OFF");
        //             }
        //             //////////////////////////
        //             if(modbusFrame.value & Modbus_RL2) {
        //             rl2.on();
        //             dbg.println("Relay 2 ON");
        //             } else {
        //                 rl2.off();
        //                 dbg.println("Relay 2 OFF");
        //             }
        //             ////////////////////////////////
        //             if(modbusFrame.value & Modbus_BZR) {
        //             bzr.on();
        //             dbg.println("Buzzer ON");
        //             } else {
        //                 // No direct way to turn off buzzer if using buzz(duration, frequency)
        //                 // You can implement a method in Buzzer class to stop buzzing if needed
        //                 bzr.off();
        //                 dbg.println("Buzzer OFF");
        //             }                
        //         break;
        //         ////////////////////////////////////
        //         case ModbusAddr_RL1_StartTime_hh:
        //         case ModbusAddr_RL1_StartTime_mm:
        //         case ModbusAddr_RL1_StartTime_ss:
        //         case ModbusAddr_RL1_EndTime_hh:
        //         case ModbusAddr_RL1_EndTime_mm:
        //         case ModbusAddr_RL1_EndTime_ss:
        //         case ModbusAddr_RL2_StartTime_hh:
        //         case ModbusAddr_RL2_StartTime_mm:
        //         case ModbusAddr_RL2_StartTime_ss:
        //         case ModbusAddr_RL2_EndTime_hh:
        //         case ModbusAddr_RL2_EndTime_mm:
        //         case ModbusAddr_RL2_EndTime_ss:
        //         {
        //             const RelayScheduleHmsStorage::WriteResult result =
        //                 relayScheduleStorage.handleModbusWrite(modbusFrame.address);
        //             if (result == RelayScheduleHmsStorage::WriteResult::Updated)
        //                 Update_Time_From_Modbus();
        //         }
        //         break;
        //         /////////////////////////////////////
        //         case ModbusAddr_RTC_Hour:
        //         case ModbusAddr_RTC_Minute:
        //         case ModbusAddr_RTC_Second:
        //             if (Set_RTC_From_Modbus())
        //             {
        //                 dbg.println("RTC Time Updated: ", rtc.readTimeString());
        //             }
        //             else
        //             {
        //                 dbg.println("RTC Time Update ERROR");
        //                 Update_RTC_Registers();
        //             }
        //         break;
        //         /////////////////////////////////////
        //         case ModbusAddr_RTC_Day:
        //         case ModbusAddr_RTC_Month:
        //         case ModbusAddr_RTC_Year:
        //             if (Set_RTC_Date_From_Modbus())
        //             {
        //                 dbg.println("RTC Date Updated: ", rtc.readDateTimeString());
        //             }
        //             else
        //             {
        //                 dbg.println("RTC Date Update ERROR");
        //                 Update_RTC_Registers();
        //             }
        //         break;
        //         /////////////////////////////////////
        //         default:
        //         dbg.print("Unhandled Register Address: ");
        //         break;
        //     }
        // } else {
        //     dbg.print("Unhandled Modbus Function: ");
        //     // dbg.println(modbusFrame.function, HEX);
        // }   
    }
//__________________________________________________________________________________________
bool modbusSaveToEEPROM(void)
{
    return false;
} 
//__________________________________________________________________________________________
bool LoadModbusConfigFromEEPROM(void)
{
    byte data1[6];
    eeprom.readBytes(ModbusAddr_RL1_StartTime_hh, data1, sizeof(data1));

    for(int i=0;i<6;i++)
     {
        modbusMemory[ModbusAddr_RL1_StartTime_hh+i] = data1[i];
        // dbg.print(",",data1[i]);
     }
     //////////////////////////////////////////////////////////////////////
    eeprom.readBytes(ModbusAddr_RL2_StartTime_hh, data1, sizeof(data1));
    for(int i=0;i<6;i++)
     {
        modbusMemory[ModbusAddr_RL2_StartTime_hh+i] = data1[i];
     }
    return true;
}
//__________________________________________________________________________________________
void Test_Data_Save_To_Modbus(void)
{
    uint32_t val;

    val=Utils::timeStringToSeconds("17:00:00");
    modbusMemory[ModbusAddr_RL1_StartTime_hh] = val / 3600UL;
    modbusMemory[ModbusAddr_RL1_StartTime_mm] = (val % 3600UL) / 60UL;
    modbusMemory[ModbusAddr_RL1_StartTime_ss] = val % 60UL;

    val=Utils::timeStringToSeconds("19:30:00");
    modbusMemory[ModbusAddr_RL1_EndTime_hh] = val / 3600UL;
    modbusMemory[ModbusAddr_RL1_EndTime_mm] = (val % 3600UL) / 60UL;
    modbusMemory[ModbusAddr_RL1_EndTime_ss] = val % 60UL;
    
    val=Utils::timeStringToSeconds("19:35:00");
    modbusMemory[ModbusAddr_RL2_StartTime_hh] = val / 3600UL;
    modbusMemory[ModbusAddr_RL2_StartTime_mm] = (val % 3600UL) / 60UL;
    modbusMemory[ModbusAddr_RL2_StartTime_ss] = val % 60UL;

    val=Utils::timeStringToSeconds("23:00:00");
    modbusMemory[ModbusAddr_RL2_EndTime_hh] = val / 3600UL;
    modbusMemory[ModbusAddr_RL2_EndTime_mm] = (val % 3600UL) / 60UL;
    modbusMemory[ModbusAddr_RL2_EndTime_ss] = val % 60UL;
}
//__________________________________________________________________________________________
void Update_Time_From_Modbus(void)
{
    uint16_t onHour = modbusMemory[ModbusAddr_RL1_StartTime_hh];
    uint16_t onMinute = modbusMemory[ModbusAddr_RL1_StartTime_mm];
    uint16_t onSecond = modbusMemory[ModbusAddr_RL1_StartTime_ss];
    uint16_t offHour = modbusMemory[ModbusAddr_RL1_EndTime_hh];
    uint16_t offMinute = modbusMemory[ModbusAddr_RL1_EndTime_mm];
    uint16_t offSecond = modbusMemory[ModbusAddr_RL1_EndTime_ss];

    uint32_t onTimeSeconds = static_cast<uint32_t>(onHour) * 3600UL +
                             static_cast<uint32_t>(onMinute) * 60UL + onSecond;
    uint32_t offTimeSeconds = static_cast<uint32_t>(offHour) * 3600UL +
                              static_cast<uint32_t>(offMinute) * 60UL + offSecond;

    RL1_Time.set_on_time(onTimeSeconds);
    RL1_Time.set_off_time(offTimeSeconds);
    /////////////////////////////////////////////
    onHour = modbusMemory[ModbusAddr_RL2_StartTime_hh];
    onMinute = modbusMemory[ModbusAddr_RL2_StartTime_mm];
    onSecond = modbusMemory[ModbusAddr_RL2_StartTime_ss];
    offHour = modbusMemory[ModbusAddr_RL2_EndTime_hh];
    offMinute = modbusMemory[ModbusAddr_RL2_EndTime_mm];
    offSecond = modbusMemory[ModbusAddr_RL2_EndTime_ss];

    onTimeSeconds = static_cast<uint32_t>(onHour) * 3600UL +
                                   static_cast<uint32_t>(onMinute) * 60UL + onSecond;
    offTimeSeconds = static_cast<uint32_t>(offHour) * 3600UL +
                                    static_cast<uint32_t>(offMinute) * 60UL + offSecond;

    RL2_Time.set_on_time(onTimeSeconds);
    RL2_Time.set_off_time(offTimeSeconds);    
}
//__________________________________________________________________________________________
bool Update_Relay1_Time_From_Modbus(void)
{
    const uint16_t onHour = modbusMemory[ModbusAddr_RL1_StartTime_hh];
    const uint16_t onMinute = modbusMemory[ModbusAddr_RL1_StartTime_mm];
    const uint16_t onSecond = modbusMemory[ModbusAddr_RL1_StartTime_ss];
    const uint16_t offHour = modbusMemory[ModbusAddr_RL1_EndTime_hh];
    const uint16_t offMinute = modbusMemory[ModbusAddr_RL1_EndTime_mm];
    const uint16_t offSecond = modbusMemory[ModbusAddr_RL1_EndTime_ss];

    const uint32_t onTimeSeconds = static_cast<uint32_t>(onHour) * 3600UL +
                                   static_cast<uint32_t>(onMinute) * 60UL + onSecond;
    const uint32_t offTimeSeconds = static_cast<uint32_t>(offHour) * 3600UL +
                                    static_cast<uint32_t>(offMinute) * 60UL + offSecond;

    RL1_Time.set_on_time(onTimeSeconds);
    RL1_Time.set_off_time(offTimeSeconds);

    // dbg.println("Relay 1 EEPROM Write Verified");

    uint8_t data[6];

     data[0] = static_cast<uint8_t>(onHour);
     data[1] = static_cast<uint8_t>(onMinute);
     data[2] = static_cast<uint8_t>(onSecond);
     data[3] = static_cast<uint8_t>(offHour);
     data[4] = static_cast<uint8_t>(offMinute);
     data[5] = static_cast<uint8_t>(offSecond);

     for(int i=0;i<6;i++)
     {
        dbg.print(",",data[i]);
     }

    eeprom.writeBytes(ModbusAddr_RL1_StartTime_hh, data, sizeof(data));

    byte data1[6];
    eeprom.readBytes(ModbusAddr_RL1_StartTime_hh, data1, sizeof(data1));

    for(int i=0;i<6;i++)
     {
        dbg.print(",",data1[i]);
     }

    return true;
}
//__________________________________________________________________________________________
bool Update_Relay2_Time_From_Modbus(void)
{
    const uint16_t onHour = modbusMemory[ModbusAddr_RL2_StartTime_hh];
    const uint16_t onMinute = modbusMemory[ModbusAddr_RL2_StartTime_mm];
    const uint16_t onSecond = modbusMemory[ModbusAddr_RL2_StartTime_ss];
    const uint16_t offHour = modbusMemory[ModbusAddr_RL2_EndTime_hh];
    const uint16_t offMinute = modbusMemory[ModbusAddr_RL2_EndTime_mm];
    const uint16_t offSecond = modbusMemory[ModbusAddr_RL2_EndTime_ss];

    const uint32_t onTimeSeconds = static_cast<uint32_t>(onHour) * 3600UL +
                                   static_cast<uint32_t>(onMinute) * 60UL + onSecond;
    const uint32_t offTimeSeconds = static_cast<uint32_t>(offHour) * 3600UL +
                                    static_cast<uint32_t>(offMinute) * 60UL + offSecond;

    RL2_Time.set_on_time(onTimeSeconds);
    RL2_Time.set_off_time(offTimeSeconds);

     uint8_t data[6];

     data[0] = static_cast<uint8_t>(onHour);
     data[1] = static_cast<uint8_t>(onMinute);
     data[2] = static_cast<uint8_t>(onSecond);
     data[3] = static_cast<uint8_t>(offHour);
     data[4] = static_cast<uint8_t>(offMinute);
     data[5] = static_cast<uint8_t>(offSecond);

    eeprom.writeBytes(ModbusAddr_RL2_StartTime_hh, data, sizeof(data));

    return true;
}
//__________________________________________________________________________________________
void Update_RTC_Registers(void)
{
    DateTime current = rtc.now();

    // A zero Unix timestamp is returned when the RTC was not initialized.
    if (current.unixtime() == 0)
    {
        return;
    }

    modbusMemory[ModbusAddr_RTC_Hour] = current.hour();
    modbusMemory[ModbusAddr_RTC_Minute] = current.minute();
    modbusMemory[ModbusAddr_RTC_Second] = current.second();
    modbusMemory[ModbusAddr_RTC_Day] = current.day();
    modbusMemory[ModbusAddr_RTC_Month] = current.month();
    modbusMemory[ModbusAddr_RTC_Year] = current.year() % 100;
}
//__________________________________________________________________________________________
bool Set_RTC_From_Modbus(void)
{
    const uint16_t hour = modbusMemory[ModbusAddr_RTC_Hour];
    const uint16_t minute = modbusMemory[ModbusAddr_RTC_Minute];
    const uint16_t second = modbusMemory[ModbusAddr_RTC_Second];

    if (hour > 23 || minute > 59 || second > 59)
    {
        return false;
    }

    return rtc.setTime(static_cast<uint8_t>(hour),
                       static_cast<uint8_t>(minute),
                       static_cast<uint8_t>(second));
}
//__________________________________________________________________________________________
bool Set_RTC_Date_From_Modbus(void)
{
    const uint16_t day = modbusMemory[ModbusAddr_RTC_Day];
    const uint16_t month = modbusMemory[ModbusAddr_RTC_Month];
    const uint16_t shortYear = modbusMemory[ModbusAddr_RTC_Year];

    if (month < 1 || month > 12 || shortYear > 99)
    {
        return false;
    }

    const uint16_t year = 2000 + shortYear;
    static const uint8_t daysPerMonth[] =
        {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint8_t maximumDay = daysPerMonth[month - 1];
    const bool leapYear = ((year % 4 == 0) && (year % 100 != 0)) ||
                          (year % 400 == 0);

    if (month == 2 && leapYear)
    {
        maximumDay = 29;
    }

    if (day < 1 || day > maximumDay)
    {
        return false;
    }

    const DateTime current = rtc.now();
    if (current.unixtime() == 0)
    {
        return false;
    }

    return rtc.setDateTime(year,
                           static_cast<uint8_t>(month),
                           static_cast<uint8_t>(day),
                           current.hour(),
                           current.minute(),
                           current.second());
}
//__________________________________________________________________________________________
#endif // LowPowerTimerModule
