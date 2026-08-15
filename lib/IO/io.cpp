#include "IO.h"

/**
 * @brief Construct a new IO object
 */
IO::IO(uint8_t ioPin,
       IO_MODE ioMode,
       IO_LOGIC logic)
{
    pin = ioPin;
    mode = ioMode;

    status.configured = 0;
    status.output = (ioMode == IO_OUTPUT);
    status.logic = logic;
    status.state = 0;

    begin();
}

/**
 * @brief Initialize GPIO
 */
void IO::begin()
{
    switch(mode)
    {
        case IO_INPUT:
            pinMode(pin, INPUT);
            break;

        case IO_INPUT_PULLUP:
            pinMode(pin, INPUT_PULLUP);
            break;

#if defined(ESP32)
        case IO_INPUT_PULLDOWN:
            pinMode(pin, INPUT_PULLDOWN);
            break;
#endif

        case IO_OUTPUT:
            pinMode(pin, OUTPUT);
            off();
            break;
    }

    status.configured = 1;
}

/**
 * @brief Write output state
 */
void IO::write(bool state)
{
    if(!status.output)
        return;

    status.state = state;

    bool level;

    if(status.logic == ACTIVE_HIGH)
        level = state;
    else
        level = !state;

    digitalWrite(pin, level);
}

/**
 * @brief Turn output ON
 */
void IO::on()
{
    write(true);
}

/**
 * @brief Turn output OFF
 */
void IO::off()
{
    write(false);
}

/**
 * @brief Toggle output
 */
void IO::toggle()
{
    write(!status.state);
}

/**
 * @brief Read input/output state
 */
bool IO::read()
{
    bool value;

    if(status.output)
    {
        value = status.state;
    }
    else
    {
        value = digitalRead(pin);

        if(status.logic == ACTIVE_LOW)
            value = !value;
    }

    return value;
}
//__________________________________________________________________________________________________________
/**
 * @brief Read switch_check times and update only when all readings agree
 */
bool IO::steady_read()
{
    char count_true = 0;
    char count_false = 0;

    while(1)
    {
        delay(10);

        if(read()==true)
        {
            count_true++;
            count_false = 0;
            if(count_true>=switch_check)
            {         
                return true;
            }
        }
        else
        {     
            if(read()==false)
            {
                count_false++;
                count_true = 0;
                if(count_false>=switch_check)
                {
                    return false;
                }
            }
        }     
    }
}
//_________________________________________________________________________________________________________
/**
 * @brief Check ON state
 */
bool IO::isON()
{
    return read();
}

/**
 * @brief Check OFF state
 */
bool IO::isOFF()
{
    return !read();
}

/**
 * @brief Toggle output with 500ms delay
 */
void IO::test()
{
    toggle();
    delay(500);
}

/**
 * @brief Generate pulse
 */
void IO::pulse(uint32_t pulseTime)
{
    on();
    delay(pulseTime);
    off();
}

/**
 * @brief Get GPIO Number
 */
uint8_t IO::getPin()
{
    return pin;
}

/**
 * @brief Get IO Mode
 */
IO_MODE IO::getMode()
{
    return mode;
}

/**
 * @brief Get Logic
 */
IO_LOGIC IO::getLogic()
{
    return (IO_LOGIC)status.logic;
}

/**
 * @brief Get Status Register
 */
IO_STATUS IO::getStatus()
{
    return status;
}
