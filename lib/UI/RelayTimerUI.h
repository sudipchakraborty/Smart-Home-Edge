#ifndef RELAY_TIMER_UI_H
#define RELAY_TIMER_UI_H

#include <Arduino.h>
#include <WebServer.h>

class RelayTimerUI {
public:
    using SaveCallback = void (*)(uint32_t rl1On, uint32_t rl1Off,
                                  uint32_t rl2On, uint32_t rl2Off);

    explicit RelayTimerUI(uint16_t port = 80);

    void begin(uint32_t rl1On, uint32_t rl1Off,
               uint32_t rl2On, uint32_t rl2Off,
               SaveCallback onSave);
    void handleClient();
    void setTimes(uint32_t rl1On, uint32_t rl1Off,
                  uint32_t rl2On, uint32_t rl2Off);

private:
    WebServer _server;
    SaveCallback _onSave;
    uint32_t _times[4];

    void handleRoot();
    void handleGetTimes();
    void handleSaveTimes();
    static bool parseTime(const String &value, uint32_t &seconds);
    static String formatTime(uint32_t seconds);
};

#endif

