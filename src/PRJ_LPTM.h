#ifndef PRJ_LPTM_H
#define PRJ_LPTM_H
 
#include <Arduino.h>

#define Addr_Broadcast   0
#define EEPROM_LEN       50
#define EEPROM_BASE_ADDR  0x0000
#define EEPROM_MAGIC      0xA5A5
#define LPTM_EEPROM_I2C_ADDR 0x50

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


///////////////////////////////////////////////////////
#define ModbusAddr_Output   0b0000000000000000
////Hardware pin definitions
#define Modbus_RL1          0b0000000000000001
#define Modbus_RL2          0b0000000000000010
#define Modbus_BZR          0b0000000000000100
#define Modbus_LED_TX       0b0000000000001000
#define Modbus_LED_RX       0b0000000000010000
#define Modbus_LED_STATUS   0b0000000000100000
//////////////////////////////////////////////////////
#define ModbusAddr_RL1_StartTime_hh      1  // hh
#define ModbusAddr_RL1_StartTime_mm      2  // mm
#define ModbusAddr_RL1_StartTime_ss      3  // ss

#define ModbusAddr_RL1_EndTime_hh        4  // hh
#define ModbusAddr_RL1_EndTime_mm        5  // mm
#define ModbusAddr_RL1_EndTime_ss        6  // ss

#define ModbusAddr_RL1_Time_Update       7  // 
/////////////////////////////////////////////////////
#define ModbusAddr_RL2_StartTime_hh      8  // hh
#define ModbusAddr_RL2_StartTime_mm      9  // mm
#define ModbusAddr_RL2_StartTime_ss      10 // ss

#define ModbusAddr_RL2_EndTime_hh        11 // hh
#define ModbusAddr_RL2_EndTime_mm        12 // mm
#define ModbusAddr_RL2_EndTime_ss        13 // ss

#define ModbusAddr_RL2_Time_Update       14 // 
/////////////////////////////////////////////////////
// RTC related modbus addresses for updating the RTC time and date from Modbus registers
#define ModbusAddr_RTC_Hour              15
#define ModbusAddr_RTC_Minute            16
#define ModbusAddr_RTC_Second            17
#define ModbusAddr_RTC_Day               18
#define ModbusAddr_RTC_Month             19
#define ModbusAddr_RTC_Year              20

#define ModbusAddr_RTC_Update            21
//////////////////////////////////////////////////////
// this below are used to store the RTC values in holding registers for read-only access
#define ModbusAddr_RTC_Hour_Reg          22
#define ModbusAddr_RTC_Minute_Reg        23
#define ModbusAddr_RTC_Second_Reg        24
#define ModbusAddr_RTC_Day_Reg           25
#define ModbusAddr_RTC_Month_Reg         26
#define ModbusAddr_RTC_Year_Reg          27

#define ModbusAddr_Beep                  28
/////////////////////////////////////////////////////
#define EEPROM_Addr_RL1_OnTime      ModbusAddr_RL1_StartTime_hh 
#define EEPROM_Addr_RL1_OffTime     ModbusAddr_RL1_EndTime_hh
#define EEPROM_Addr_RL2_OnTime      ModbusAddr_RL2_StartTime_hh
#define EEPROM_Addr_RL2_OffTime     ModbusAddr_RL2_EndTime_hh
/////////////////////////////////////////////////////
enum FSMStates {
    IDLE,
    READING_RTC,
    CHECK_RL1_TIME,
    CHECK_RL2_TIME,
};
//////////////////////////
void LPTM_setup();
void LPTM_loop();
uint8_t Discover_I2C_Devices(void);
bool modbusSaveToEEPROM(void);
bool LoadModbusConfigFromEEPROM(void);
void displayModbusData(void);
void StateMachine(void);
void ModbusActionHandler(void);
void Modbus_Handler();
void Update_Time_From_Modbus(void);
bool Update_Relay1_Time_From_Modbus(void);
bool Update_Relay2_Time_From_Modbus(void);
void Update_RTC_Registers(void);
bool Set_RTC_From_Modbus(void);
bool Set_RTC_Date_From_Modbus(void);
void Test_Data_Save_To_Modbus(void);
///////////////////////////



#endif // PRJ_LPTM_H
