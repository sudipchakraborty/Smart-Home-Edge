#include "RelayTimerUI.h"

namespace {
const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Relay Timer</title><style>
:root{font-family:system-ui,sans-serif;color:#e8eef7;background:#0b1220}
*{box-sizing:border-box}body{margin:0;min-height:100vh;display:grid;place-items:center;padding:20px}
main{width:min(760px,100%)}h1{margin:0 0 6px;font-size:clamp(1.7rem,5vw,2.5rem)}
.sub{margin:0 0 24px;color:#9db0ca}.grid{display:grid;grid-template-columns:repeat(2,1fr);gap:16px}
.card{background:#131e30;border:1px solid #24344e;border-radius:16px;padding:20px;box-shadow:0 12px 32px #0005}
h2{margin:0 0 18px;color:#68d6b1}label{display:block;margin:14px 0 6px;color:#b9c7da}
input{width:100%;padding:12px;border:1px solid #3a4c69;border-radius:9px;background:#0c1524;color:#fff;font-size:1rem;color-scheme:dark}
button{width:100%;margin-top:20px;padding:13px;border:0;border-radius:10px;background:#38b98d;color:#061710;font-weight:700;font-size:1rem;cursor:pointer}
button:disabled{opacity:.6}.msg{height:24px;margin:14px 2px 0;color:#68d6b1;text-align:center}
@media(max-width:600px){.grid{grid-template-columns:1fr}}
</style></head><body><main><h1>Relay Timer Settings</h1>
<p class="sub">Set the daily ON and OFF time for each relay.</p><form id="form"><div class="grid">
<section class="card"><h2>Relay 1</h2><label for="rl1On">ON time</label><input id="rl1On" name="rl1On" type="time" step="1" required>
<label for="rl1Off">OFF time</label><input id="rl1Off" name="rl1Off" type="time" step="1" required></section>
<section class="card"><h2>Relay 2</h2><label for="rl2On">ON time</label><input id="rl2On" name="rl2On" type="time" step="1" required>
<label for="rl2Off">OFF time</label><input id="rl2Off" name="rl2Off" type="time" step="1" required></section>
</div><button id="save" type="submit">Save relay timers</button><div class="msg" id="msg"></div></form></main>
<script>
const fields=['rl1On','rl1Off','rl2On','rl2Off'],msg=document.querySelector('#msg'),btn=document.querySelector('#save');
async function load(){try{const r=await fetch('/api/timers');if(!r.ok)throw 0;const d=await r.json();fields.forEach(k=>document.querySelector('#'+k).value=d[k]);}catch(e){msg.textContent='Unable to load settings';}}
document.querySelector('#form').addEventListener('submit',async e=>{e.preventDefault();btn.disabled=true;msg.textContent='Saving...';try{const r=await fetch('/api/timers',{method:'POST',body:new FormData(e.target)});const d=await r.json();if(!r.ok)throw new Error(d.message||'Save failed');msg.textContent=d.message;fields.forEach(k=>document.querySelector('#'+k).value=d[k]);}catch(e){msg.textContent=e.message;}finally{btn.disabled=false;}});load();
</script></body></html>)HTML";
}

RelayTimerUI::RelayTimerUI(uint16_t port)
    : _server(port), _onSave(nullptr), _times{0, 0, 0, 0} {}

void RelayTimerUI::begin(uint32_t rl1On, uint32_t rl1Off,
                         uint32_t rl2On, uint32_t rl2Off,
                         SaveCallback onSave) {
    setTimes(rl1On, rl1Off, rl2On, rl2Off);
    _onSave = onSave;
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/api/timers", HTTP_GET, [this]() { handleGetTimes(); });
    _server.on("/api/timers", HTTP_POST, [this]() { handleSaveTimes(); });
    _server.onNotFound([this]() { _server.send(404, "application/json", "{\"message\":\"Not found\"}"); });
    _server.begin();
}

void RelayTimerUI::handleClient() { _server.handleClient(); }

void RelayTimerUI::setTimes(uint32_t rl1On, uint32_t rl1Off,
                            uint32_t rl2On, uint32_t rl2Off) {
    _times[0] = rl1On % 86400UL;
    _times[1] = rl1Off % 86400UL;
    _times[2] = rl2On % 86400UL;
    _times[3] = rl2Off % 86400UL;
}

void RelayTimerUI::handleRoot() {
    _server.send_P(200, "text/html", PAGE);
}

void RelayTimerUI::handleGetTimes() {
    String json = "{\"rl1On\":\"" + formatTime(_times[0]) +
                  "\",\"rl1Off\":\"" + formatTime(_times[1]) +
                  "\",\"rl2On\":\"" + formatTime(_times[2]) +
                  "\",\"rl2Off\":\"" + formatTime(_times[3]) + "\"}";
    _server.send(200, "application/json", json);
}

void RelayTimerUI::handleSaveTimes() {
    const char *names[] = {"rl1On", "rl1Off", "rl2On", "rl2Off"};
    uint32_t updated[4];
    for (uint8_t i = 0; i < 4; ++i) {
        if (!_server.hasArg(names[i]) || !parseTime(_server.arg(names[i]), updated[i])) {
            _server.send(400, "application/json", "{\"message\":\"Invalid or missing time\"}");
            return;
        }
    }

    setTimes(updated[0], updated[1], updated[2], updated[3]);
    if (_onSave) _onSave(_times[0], _times[1], _times[2], _times[3]);

    String json = "{\"message\":\"Settings saved\",\"rl1On\":\"" + formatTime(_times[0]) +
                  "\",\"rl1Off\":\"" + formatTime(_times[1]) +
                  "\",\"rl2On\":\"" + formatTime(_times[2]) +
                  "\",\"rl2Off\":\"" + formatTime(_times[3]) + "\"}";
    _server.send(200, "application/json", json);
}

bool RelayTimerUI::parseTime(const String &value, uint32_t &seconds) {
    if (value.length() != 5 && value.length() != 8) return false;
    if (value.charAt(2) != ':' || (value.length() == 8 && value.charAt(5) != ':')) return false;
    for (uint8_t i = 0; i < value.length(); ++i) {
        if (i != 2 && i != 5 && !isDigit(value.charAt(i))) return false;
    }
    uint8_t hour = value.substring(0, 2).toInt();
    uint8_t minute = value.substring(3, 5).toInt();
    uint8_t second = value.length() == 8 ? value.substring(6, 8).toInt() : 0;
    if (hour > 23 || minute > 59 || second > 59) return false;
    seconds = static_cast<uint32_t>(hour) * 3600UL + minute * 60UL + second;
    return true;
}

String RelayTimerUI::formatTime(uint32_t seconds) {
    seconds %= 86400UL;
    char value[9];
    snprintf(value, sizeof(value), "%02lu:%02lu:%02lu",
             seconds / 3600UL, (seconds % 3600UL) / 60UL, seconds % 60UL);
    return String(value);
}

