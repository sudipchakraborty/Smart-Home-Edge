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
#include "RelayScheduleStorage.h"
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

static void relayScheduleDiagnostic(const String &message)
{
    dbg.println(message);
}

static const RelayScheduleDefinition relayScheduleDefinitions[] = {
    {"Relay 1 ON", ModbusAddr_RL1_StartTime, EEPROM_Addr_RL1_OnTime},
    {"Relay 1 OFF", ModbusAddr_RL1_EndTime, EEPROM_Addr_RL1_OffTime},
    {"Relay 2 ON", ModbusAddr_RL2_StartTime, EEPROM_Addr_RL2_OnTime},
    {"Relay 2 OFF", ModbusAddr_RL2_EndTime, EEPROM_Addr_RL2_OffTime},
};

RelayScheduleStorage relayScheduleStorage(
    eeprom, modbusMemory, relayScheduleDefinitions,
    sizeof(relayScheduleDefinitions) / sizeof(relayScheduleDefinitions[0]),
    relayScheduleDiagnostic);

int countValue = 0;
uint16_t temp[2];
////////////////////////////////////////////////////////////////////////////////////
void LPTM_setup()
{
    st.begin(LED_PIN, 5000);
    rtc.begin();
    dbg.begin(Serial1, 115200, 4);
    dbg.println("system started..");
    rs485.DefaultSetUp();
    dbg.println("system Initialized..");

    SecondTick.set_time(1000);
    SecondTick.start();

    const bool eepromReady = eeprom.begin(21, 22);
    if (eepromReady)
        dbg.println("AT24C32 detected at configured I2C address 0x57");
    else
        dbg.println("EEPROM PROBE ERROR: AT24C32 not detected at configured I2C address 0x57");
    /////////////////////////////
    // Test_Data_Save_To_Modbus();
    // modbusSaveToEEPROM();

    relayScheduleStorage.begin();
    LoadModbusConfigFromEEPROM();
    Update_Time_From_Modbus();
    Update_RTC_Registers();
    
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
        mb.debugPrintModbusFrame(modbusFrame, dbg);

        // Holding-register reads must return the RTC value at request time.
        if (modbusFrame.function == 0x03)
        {
            Update_RTC_Registers();
        }

        char txLen = Modbus_BuildResponse(&modbusFrame,txBuf,sizeof(txBuf));
        if (txLen > 0){
            rs485.send((uint8_t*)txBuf, txLen);
            // dbg.print("Sent Response: ");
            // dbg.printHex((uint8_t*)txBuf, txLen);
            ModbusActionHandler();
        }
    }
 }
//__________________________________________________________________________________________
void ModbusActionHandler(void)
{
    if (!mb.PacketAvailable) return;
     mb.PacketAvailable=false;

         if(modbusFrame.function == 0x06) { // Write Single Register
            switch(modbusFrame.address) 
            {
            case ModbusAddr_Output:
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
                        // No direct way to turn off buzzer if using buzz(duration, frequency)
                        // You can implement a method in Buzzer class to stop buzzing if needed
                        bzr.off();
                        dbg.println("Buzzer OFF");
                    }                
                break;
                ////////////////////////////////////
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                {
                    const RelayScheduleStorage::WriteResult result =
                        relayScheduleStorage.handleModbusWordWrite(modbusFrame.address);
                    if (result == RelayScheduleStorage::WriteResult::Updated)
                        Update_Time_From_Modbus();
                }
                break;
                /////////////////////////////////////
                case ModbusAddr_RTC_Hour:
                case ModbusAddr_RTC_Minute:
                case ModbusAddr_RTC_Second:
                    if (Set_RTC_From_Modbus())
                    {
                        dbg.println("RTC Time Updated: ", rtc.readTimeString());
                    }
                    else
                    {
                        dbg.println("RTC Time Update ERROR");
                        Update_RTC_Registers();
                    }
                break;
                /////////////////////////////////////
                case ModbusAddr_RTC_Day:
                case ModbusAddr_RTC_Month:
                case ModbusAddr_RTC_Year:
                    if (Set_RTC_Date_From_Modbus())
                    {
                        dbg.println("RTC Date Updated: ", rtc.readDateTimeString());
                    }
                    else
                    {
                        dbg.println("RTC Date Update ERROR");
                        Update_RTC_Registers();
                    }
                break;
                /////////////////////////////////////
                default:
                dbg.print("Unhandled Register Address: ");
                break;
            }
        } else {
            dbg.print("Unhandled Modbus Function: ");
            // dbg.println(modbusFrame.function, HEX);
        }   
    }
//__________________________________________________________________________________________
bool modbusSaveToEEPROM(void)
{
    return relayScheduleStorage.persistAll();
} 
//__________________________________________________________________________________________
bool LoadModbusConfigFromEEPROM(void)
{
    return relayScheduleStorage.loadAll();
}
//__________________________________________________________________________________________
void Test_Data_Save_To_Modbus(void)
{
    uint32_t val;

    val=Utils::timeStringToSeconds("17:00:00");
    Utils::storeUint32ToModbus(modbusMemory,ModbusAddr_RL1_StartTime,val);

    val=Utils::timeStringToSeconds("19:30:00");
    Utils::storeUint32ToModbus(modbusMemory,ModbusAddr_RL1_EndTime,val);
    
    val=Utils::timeStringToSeconds("19:35:00");
    Utils::storeUint32ToModbus(modbusMemory,ModbusAddr_RL2_StartTime,val);

    val=Utils::timeStringToSeconds("23:00:00");
    Utils::storeUint32ToModbus(modbusMemory,ModbusAddr_RL2_EndTime,val);
}
//__________________________________________________________________________________________
void Update_Time_From_Modbus(void)
{
    const uint32_t rl1On = Utils::readUint32FromModbus(modbusMemory, ModbusAddr_RL1_StartTime);
    const uint32_t rl1Off = Utils::readUint32FromModbus(modbusMemory, ModbusAddr_RL1_EndTime);
    const uint32_t rl2On = Utils::readUint32FromModbus(modbusMemory, ModbusAddr_RL2_StartTime);
    const uint32_t rl2Off = Utils::readUint32FromModbus(modbusMemory, ModbusAddr_RL2_EndTime);

    if (RelayScheduleStorage::isValid(rl1On)) RL1_Time.set_on_time(rl1On);
    if (RelayScheduleStorage::isValid(rl1Off)) RL1_Time.set_off_time(rl1Off);
    if (RelayScheduleStorage::isValid(rl2On)) RL2_Time.set_on_time(rl2On);
    if (RelayScheduleStorage::isValid(rl2Off)) RL2_Time.set_off_time(rl2Off);
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
