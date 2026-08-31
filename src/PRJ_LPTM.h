#ifndef PRJ_LPTM_H
#define PRJ_LPTM_H
 
#include <Arduino.h>

#define Addr_Broadcast   0
#define EEPROM_LEN       50
#define EEPROM_BASE_ADDR  0x0000
#define EEPROM_MAGIC      0xA5A5

/////Hardware pin definitions
#define RX2_PIN 16
#define TX2_PIN 17
#define DE_RE 33
////////////////
#define LED_TX 32
#define LED_RX 23
#define BZR 19
#define RL1 5
#define RL2 18
#define LED_PIN 2
/////////////////////////
#define ModbusAddr_Output 0
#define ModbusAddr_RL1_StartTime 1  // 2 uint16 to store second value
#define ModbusAddr_RL1_EndTime 3
#define ModbusAddr_RL2_StartTime 5
#define ModbusAddr_RL2_EndTime 7
#define ModbusAddr_RTC_Hour 9
#define ModbusAddr_RTC_Minute 10
#define ModbusAddr_RTC_Second 11
#define ModbusAddr_RTC_Day 12
#define ModbusAddr_RTC_Month 13
#define ModbusAddr_RTC_Year 14
//////////////////////////
#define Modbus_RL1 0x01  
#define Modbus_RL2 0x02
#define Modbus_BZR 0x04
#define Modbus_LED_TX 0x08
#define Modbus_LED_RX 0x10
//////////////////////////
#define EEPROM_Addr_RL1_OnTime 0  
#define EEPROM_Addr_RL1_OffTime 4
#define EEPROM_Addr_RL2_OnTime 8
#define EEPROM_Addr_RL2_OffTime 12
//////////////////////////
enum FSMStates {
    IDLE,
    READING_RTC,
    CHECK_RL1_TIME,
    CHECK_RL2_TIME,
};
//////////////////////////
void LPTM_setup();
void LPTM_loop();
bool modbusSaveToEEPROM(void);
bool LoadModbusConfigFromEEPROM(void);
void displayModbusData(void);
void StateMachine(void);
void ModbusActionHandler(void);
void Modbus_Handler();
void Update_Time_From_Modbus(void);
void Update_RTC_Registers(void);
bool Set_RTC_From_Modbus(void);
bool Set_RTC_Date_From_Modbus(void);
void Test_Data_Save_To_Modbus(void);
///////////////////////////



#endif // PRJ_LPTM_H
