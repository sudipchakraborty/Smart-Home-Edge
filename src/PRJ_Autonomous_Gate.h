#ifndef PRJ_AUTONOMOUS_GATE_H
#define PRJ_AUTONOMOUS_GATE_H

#include <Arduino.h>

// #define PIN_IR_SENSE  ADC1-CH0
#define PIN_SEN_HOME        34
#define PIN_SEN_TERMINAL    35
#define PIN_ST_LED          2
#define PIN_CALLING_BELL    14

#define PIN_EN              33
#define PIN_L_PWM           27
#define PIN_R_PWM           25 

#define PIN_SW_OUTDOOR      32
#define PIN_SW_INDOOR       23
#define PIN_LED_FAULT       19
#define PIN_RE_DE           18  
#define PIN_BZR             5  

#define Speed_JoG          30
////////////////////////////////

enum{
    FSM_init,
    FSM_Read_Indoor_switch,
    FSM_Door_Opening,
    FSM_Wait_For_Closing,
    FSM_Door_Closing,
    FSM_read_sensor

};
 
typedef struct
{
    // uint8_t configured : 1;
    // uint8_t enabled    : 1;
    // uint8_t running    : 1;
    // uint8_t reserved   : 5;

} GATE_STATUS;

class Autonomous_Gate
{
private:
    void FSM_Handler();

    uint8_t FSM=0;
    uint16_t loop_counter=0;
    // uint8_t channel;

    // uint32_t frequency;
    // uint8_t resolution;

    // uint32_t maxDuty;
    // uint32_t duty;

    // PWM_STATUS status;

    // static uint8_t nextChannel;

public:
    void PRJ_Autonomous_Gate_SetUp();
    void PRJ_Autonomous_Gate_Loop();
    bool Go_Home();
    void Motor_Rotate_Towards_Home(char dutyCycle);
    void Motor_Rotate_Towards_Terminal(char dutyCycle);
    void Motor_Stop();

    // /**
    //  * @brief Construct PWM object
    //  *
    //  * @param gpio GPIO Number
    //  * @param freq PWM Frequency
    //  * @param res Resolution (1-16 bits)
    //  */
    // PWM(uint8_t gpio,
    //     uint32_t freq = 5000,
    //     uint8_t res = 8);

    // /**
    //  * @brief Initialize PWM
    //  */
    // void begin();

    // /**
    //  * @brief Enable PWM output
    //  */
    // void enable();

    // /**
    //  * @brief Disable PWM output
    //  */
    // void disable();

    // /**
    //  * @brief Turn output ON (100%)
    //  */
    // void on();

    // /**
    //  * @brief Turn output OFF
    //  */
    // void off();

    // /**
    //  * @brief Set duty percentage
    //  *
    //  * @param percent 0-100
    //  */
    // void setDuty(uint8_t percent);

    // /**
    //  * @brief Set raw duty
    //  *
    //  * @param raw Raw duty value
    //  */
    // void setDutyRaw(uint32_t raw);

    // /**
    //  * @brief Get duty percentage
    //  */
    // uint8_t getDuty();

    // /**
    //  * @brief Get raw duty
    //  */
    // uint32_t getDutyRaw();

    // /**
    //  * @brief Change frequency
    //  */
    // void setFrequency(uint32_t freq);

    // /**
    //  * @brief Get frequency
    //  */
    // uint32_t getFrequency();

    // /**
    //  * @brief Change resolution
    //  */
    // void setResolution(uint8_t bits);

    // /**
    //  * @brief Get GPIO
    //  */
    // uint8_t getPin();

    // /**
    //  * @brief Get channel
    //  */
    // uint8_t getChannel();

    // /**
    //  * @brief Get status
    //  */
    // PWM_STATUS getStatus();

    // /**
    //  * @brief Test PWM
    //  */
    // void test();

};

#endif