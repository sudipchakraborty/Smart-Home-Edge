
#ifdef LowPowerTimerModule_V2

#include "PRJ_LPTM_V2.h"
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
#include "RtcDs3231.h"
#include "AppTime.h"
#include "Utils.h"
#include "SimulatedClock.h"
#include "eepromStorage.h"
#include "at24c32N.h"
#include "config.h"
#include "WiFiModule.h"
#include "ClockInternet.h"
#include "RelayTimerUI.h"
#include "BluetoothWiFiProvisioner.h"
#include "WiFiSetupServer.h"
#include "DeviceID.h"
#include "OTAManager.h"
#include "I2CScanner.h"
#include "RelayScheduleStorage.h"

// Service addresses:
// Wi-Fi configuration: http://192.168.4.1
// Relay timer settings: http://192.168.4.1:8080

/////////////////////////////////////////////////////////////////////////////////////////
uint8_t MY_SLAVE_ID = 1;   // change per device   1
RS485 rs485;
char txBuf[255];
uint8_t rxBuf[255];
ModbusASCII mb;
ModbusASCIIFrame modbusFrame;

debug dbg;
StatusBlink st;
RtcDs3231 rtc;
AppTime SecondTick;
 
AppTime RL1_Time;
AppTime RL2_Time;
////////////////////////////
led led_Internet(LED_Internet,0);
led led_WiFi_Tx(led_WF_Tx,0);
led led_WiFi_Rx(led_WF_Rx,0);
led led_Health(LED_Health,0);
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
WiFiModule wifiModule;
ClockInternet internetClock;
RelayTimerUI relayTimerUI(8080);
BluetoothWiFiProvisioner bluetoothProvisioner;
WiFiSetupServer wifiSetupServer;
DeviceID deviceId;
OTAManager otaManager;
bool networkServicesReady = false;

int countValue = 0;
uint16_t temp[2];
void saveRelayTimesFromWeb(uint32_t rl1On, uint32_t rl1Off,
                           uint32_t rl2On, uint32_t rl2Off);
void alertOTAUpdateReceived();
////////////////////////////////////////////////////////////////////////////////////////////////
void LPTM_setup_V2()
{
    st.begin(LED_ST, 1000);
    rtc.begin();
    dbg.begin(Serial1, 115200, 4);
    dbg.println("system started..");

    deviceId.begin();
    String productSerial = deviceId.getEfuseChipIDString();

    bool wifiConnected = false;
    if (!WIFI_PROV_FORCE_RESET) {
        wifiConnected = wifiModule.connectSaved(10000);
    }
    if (wifiConnected) {
        dbg.println("WiFi connected using saved credentials");
        dbg.println("WiFi IP: ", wifiModule.localIP().toString());
        internetClock.syncRtc(rtc, dbg,
                              INTERNET_TIME_GMT_OFFSET_SECONDS,
                              INTERNET_TIME_DAYLIGHT_OFFSET_SECONDS);
        networkServicesReady = true;
    } else {
        dbg.println("Starting Bluetooth WiFi setup");
        bluetoothProvisioner.begin(WIFI_PROV_DEVICE_NAME, WIFI_PROV_POP,
                                   WIFI_PROV_FORCE_RESET);
    }

    if (wifiSetupServer.begin(String(Hotspot_Name), String(Hotspot_password),
                              productSerial)) {
        dbg.println("WiFi setup hotspot: " + wifiSetupServer.accessPointName());
        dbg.println("Factory hotspot password: " + wifiSetupServer.factoryPassword());
        dbg.println("WiFi setup page: http://" + wifiSetupServer.accessPointIP().toString());
    } else {
        dbg.println("WiFi setup hotspot failed to start");
    }

    otaManager.begin(FIRMWARE_VERSION);
    otaManager.setURLs(OTA_VERSION_URL);
    otaManager.setCheckInterval(OTA_CHECK_INTERVAL_MS);
    otaManager.setDebugStream(Serial1);
    otaManager.setUpdateReceivedCallback(alertOTAUpdateReceived);

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
    relayTimerUI.begin(RL1_Time.onTime, RL1_Time.offTime,
                       RL2_Time.onTime, RL2_Time.offTime,
                       saveRelayTimesFromWeb);
    if (wifiModule.isConnected()) {
        dbg.println("Relay timer webpage: http://" + wifiModule.localIP().toString() + ":8080");
    }
    
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

    //  simClock.set(s);
    //  uint32_t startSec = Utils::DT_String_To_Seconds_From_TimePart(s.c_str());
    //  dbg.print("Time In second: ", startSec); 

    SystemTest();
}
//__________________________________________________________________________________________
void LPTM_loop_V2()
{  
    if (!networkServicesReady && wifiModule.isConnected()) {
        networkServicesReady = true;
        internetClock.syncRtc(rtc, dbg,
                              INTERNET_TIME_GMT_OFFSET_SECONDS,
                              INTERNET_TIME_DAYLIGHT_OFFSET_SECONDS);
        dbg.println("Relay timer webpage: http://" + wifiModule.localIP().toString() + ":8080");
    }
    st.blink();
    StateMachine(); 
    Modbus_Handler();
    wifiSetupServer.handleClient();
    relayTimerUI.handleClient();
    otaManager.handle();
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
        
        // âœ… Slave ID filtering (IMPORTANT)
        if (modbusFrame.slaveId != MY_SLAVE_ID && modbusFrame.slaveId != 0)
        {
            dbg.println("Packet not for me, or Not Broadcast...ignored");
            return;
        }
        if (!mb.isModbusPacketHealthy(modbusFrame)) return;
        mb.PacketAvailable=true;
        mb.debugPrintModbusFrame(modbusFrame, dbg);

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

    // Keep the webpage values current when a Modbus client changes the timers.
    relayTimerUI.setTimes(RL1_Time.onTime, RL1_Time.offTime,
                          RL2_Time.onTime, RL2_Time.offTime);
}
//__________________________________________________________________________________________
void saveRelayTimesFromWeb(uint32_t rl1On, uint32_t rl1Off,
                           uint32_t rl2On, uint32_t rl2Off)
{
    Utils::storeUint32ToModbus(modbusMemory, ModbusAddr_RL1_StartTime, rl1On);
    Utils::storeUint32ToModbus(modbusMemory, ModbusAddr_RL1_EndTime, rl1Off);
    Utils::storeUint32ToModbus(modbusMemory, ModbusAddr_RL2_StartTime, rl2On);
    Utils::storeUint32ToModbus(modbusMemory, ModbusAddr_RL2_EndTime, rl2Off);
    const bool saved = modbusSaveToEEPROM();
    Update_Time_From_Modbus();
    if (saved)
        dbg.println("Relay timers updated from webpage");
    else
        dbg.println("Relay timer webpage update ERROR: one or more schedules were not persisted");
}
//__________________________________________________________________________________________
void alertOTAUpdateReceived()
{
    dbg.println("OTA update received - sounding three alert beeps");
    for (uint8_t i = 0; i < 3; ++i) {
        bzr.beep();
        if (i < 2) delay(200);
    }
}
//__________________________________________________________________________________________
void SystemTest(void)
{
    dbg.println("System Test Started...");

#if ENABLE_I2C_SCANNER_TEST
    uint8_t i2cDeviceCount = I2CScanner::scan(Serial1);
    if (i2cDeviceCount == 0) {
        dbg.println("I2C SCAN ERROR - no devices found");
    }

    bool ds3231AddressFound = I2CScanner::devicePresent(0x68);
    bool at24c32AddressFound = false;
    uint8_t detectedEepromAddress = 0;
    for (uint8_t address = 0x50; address <= 0x57; ++address) {
        if (I2CScanner::devicePresent(address)) {
            at24c32AddressFound = true;
            detectedEepromAddress = address;
            break;
        }
    }

    if (ds3231AddressFound) {
        dbg.println("DS3231 detected at I2C address 0x68");
    } else {
        dbg.println("DS3231 ERROR - address 0x68 not detected");
    }

    if (at24c32AddressFound) {
        char addressMessage[48];
        snprintf(addressMessage, sizeof(addressMessage),
                 "AT24C32 detected at I2C address 0x%02X",
                 detectedEepromAddress);
        dbg.println(addressMessage);
        if (detectedEepromAddress != AT24C32_I2C_ADDR) {
            dbg.println("AT24C32 ERROR - detected address does not match configured address");
        }
    } else {
        dbg.println("AT24C32 ERROR - no address found from 0x50 to 0x57");
    }
#endif

    dbg.println("DS3231 RTC test started");

    // Verify DS3231 communication and write/read operation without changing
    // the saved clock time.
    bool rtcCommunicationOk = rtc.isInitialized();
    DateTime savedDateTime = rtc.now();
    DateTime testDateTime(2026, 1, 2, 3, 4, 5);
    bool rtcWriteOk = rtcCommunicationOk && rtc.setDateTime(testDateTime);
    DateTime readDateTime = rtc.now();
    bool rtcReadOk = rtcWriteOk &&
                     (readDateTime.unixtime() >= testDateTime.unixtime()) &&
                     ((readDateTime.unixtime() - testDateTime.unixtime()) <= 1);
    bool rtcRestoreOk = rtcCommunicationOk && rtc.setDateTime(savedDateTime);

    if (rtcCommunicationOk && rtcWriteOk && rtcReadOk && rtcRestoreOk) {
        dbg.println("DS3231 RTC OK - communication/read/write/restore passed");
    } else {
        dbg.println("DS3231 RTC ERROR - system test failed");
        if (!rtcCommunicationOk) dbg.println("RTC initialization or I2C communication failed");
        if (rtcCommunicationOk && !rtcWriteOk) dbg.println("RTC test time write failed");
        if (rtcWriteOk && !rtcReadOk) dbg.println("RTC test time read-back failed");
        if (rtcCommunicationOk && !rtcRestoreOk) dbg.println("RTC original time restore failed");
    }

    // Verify the final physical byte of the onboard AT24C32 (0..4095).
    // Preserve and restore the original value so this test is non-destructive.
    const uint16_t eepromLastAddress = AT24C32_TOTAL_BYTES - 1;
    uint8_t eepromOriginal = 0;
    uint8_t eepromReadBack = 0;
    bool eepromOriginalReadOk = eeprom.readBytes(eepromLastAddress,
                                                  &eepromOriginal, 1);
    bool eepromPattern1Ok = false;
    bool eepromPattern2Ok = false;
    bool eepromRestoreOk = false;

    if (eepromOriginalReadOk) {
        const uint8_t pattern1 = 0x5A;
        const uint8_t pattern2 = 0xA5;

        eepromPattern1Ok = eeprom.writeBytes(eepromLastAddress, &pattern1, 1) &&
                           eeprom.readBytes(eepromLastAddress, &eepromReadBack, 1) &&
                           eepromReadBack == pattern1;

        eepromPattern2Ok = eeprom.writeBytes(eepromLastAddress, &pattern2, 1) &&
                           eeprom.readBytes(eepromLastAddress, &eepromReadBack, 1) &&
                           eepromReadBack == pattern2;

        eepromRestoreOk = eeprom.writeBytes(eepromLastAddress, &eepromOriginal, 1) &&
                          eeprom.readBytes(eepromLastAddress, &eepromReadBack, 1) &&
                          eepromReadBack == eepromOriginal;
    }

    if (eepromOriginalReadOk && eepromPattern1Ok &&
        eepromPattern2Ok && eepromRestoreOk) {
        dbg.println("AT24C32 EEPROM OK - last address 4095 read/write/restore passed");
    } else {
        dbg.println("AT24C32 EEPROM ERROR - last address 4095 test failed");
        if (!eepromOriginalReadOk) dbg.println("EEPROM initial read failed");
        if (eepromOriginalReadOk && !eepromPattern1Ok) dbg.println("EEPROM pattern 0x5A failed");
        if (eepromOriginalReadOk && !eepromPattern2Ok) dbg.println("EEPROM pattern 0xA5 failed");
        if (eepromOriginalReadOk && !eepromRestoreOk) dbg.println("EEPROM original value restore failed");
    }

    ////make all off/////
    rl1.off();
    rl2.off();
    bzr.off();

    ledRx.off();
    ledTx.off();
    led_Health.off(); 
    led_WiFi_Rx.off(); 
    led_WiFi_Tx.off(); 
    led_Internet.off();
    //////////////////////
    rl1.test(500); delay(1000);
    rl2.test(500); delay(1000);
    bzr.beep(); delay(1000);

    ledRx.on(); delay(1000);
    ledTx.on(); delay(1000);
    led_Health.on(); delay(1000);
    led_WiFi_Rx.on(); delay(1000);
    led_WiFi_Tx.on(); delay(1000);
    led_Internet.on(); delay(1000);
 
    ledRx.off(); delay(1000);
    ledTx.off(); delay(1000);
    led_Health.off(); delay(1000);
    led_WiFi_Rx.off(); delay(1000);
    led_WiFi_Tx.off(); delay(1000);
    led_Internet.off(); delay(1000);

    bzr.beep(); delay(1000);
    dbg.println("System Test Completed.");
}
//__________________________________________________________________________________________
#endif // LowPowerTimerModule_V2
