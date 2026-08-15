#ifndef IO_H
#define IO_H

#include <Arduino.h>

// Number of consecutive matching samples required to change switch state.
#define switch_check 10

/**
 * @brief IO Direction
 */
typedef enum
{
    IO_INPUT = 0,
    IO_INPUT_PULLUP,
    IO_INPUT_PULLDOWN,
    IO_OUTPUT

} IO_MODE;

/**
 * @brief IO Active Logic
 */
typedef enum
{
    ACTIVE_LOW = 0,
    ACTIVE_HIGH

} IO_LOGIC;

/**
 * @brief IO Status Register
 */
typedef struct
{
    uint8_t configured : 1;
    uint8_t output     : 1;
    uint8_t state      : 1;
    uint8_t logic      : 1;
    uint8_t reserved   : 4;

} IO_STATUS;

class IO
{
private:

    uint8_t pin;
    IO_MODE mode;
    bool steadyState = false;

    IO_STATUS status;

public:

    /**
     * @brief Construct a new IO object
     *
     * @param ioPin GPIO Number
     * @param ioMode Input/Output mode
     * @param logic Active High / Active Low
     */
    IO(uint8_t ioPin,
       IO_MODE ioMode,
       IO_LOGIC logic = ACTIVE_HIGH);

    /**
     * @brief Initialize GPIO
     */
    void begin();

    /**
     * @brief Turn ON output
     */
    void on();

    /**
     * @brief Turn OFF output
     */
    void off();

    /**
     * @brief Toggle output
     */
    void toggle();

    /**
     * @brief Write output state
     *
     * @param state true = ON
     */
    void write(bool state);

    /**
     * @brief Read IO
     *
     * @return true Logical ON
     */
    bool read();

    /**
     * @brief Read IO switch_check times and accept only a steady result
     *
     * @return New state when every sample agrees, otherwise previous stable state
     */
    bool steady_read();

    /**
     * @brief Check logical ON state
     */
    bool isON();

    /**
     * @brief Check logical OFF state
     */
    bool isOFF();

    /**
     * @brief Toggle once with 500ms delay
     */
    void test();

    /**
     * @brief Generate pulse
     *
     * @param pulseTime Pulse width in ms
     */
    void pulse(uint32_t pulseTime);

    /**
     * @brief Get GPIO Number
     */
    uint8_t getPin();

    /**
     * @brief Get IO Mode
     */
    IO_MODE getMode();

    /**
     * @brief Get Logic
     */
    IO_LOGIC getLogic();

    /**
     * @brief Get Status Register
     */
    IO_STATUS getStatus();

};

#endif
