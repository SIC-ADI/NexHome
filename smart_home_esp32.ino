#include <WiFi.h>
#include <DHT.h>

const char* ssid = "ADI";
const char* password = "adi321!!";

#define DHTPIN 32
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const int Rled = 26;
const int Gled = 25;
const int Yled = 27;
const int Bled = 18;

bool redState   = false;
bool greenState = false;
bool yellowState = false;
bool blueState  = false;

WiFiServer server(80);

// ─── HTML TEMPLATE ────────────────────────────────────────────────────────────
// %TEMP%, %HUM%, %RED%, %GREEN%, %YELLOW%, %BLUE% are replaced at runtime
const char HTML_PAGE[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1.0"/>
<title>NexHome Preview</title>
<style>
*,*::before,*::after{box-sizing:border-box;margin:0;padding:0}
@import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Rajdhani:wght@400;600;700&display=swap');

:root{
  --bg:#0f0f10;--card:#1a1a1d;--card2:#141416;--border:#2a2a30;
  --green:#00ffc3;--text:#e8e8f0;--radius:18px;
}
html,body{height:100%;background:var(--bg);color:var(--text);
  font-family:'Rajdhani',sans-serif;font-size:16px;line-height:1.4}
body::before{content:'';position:fixed;inset:0;
  background-image:linear-gradient(rgba(0,255,195,.025) 1px,transparent 1px),
                   linear-gradient(90deg,rgba(0,255,195,.025) 1px,transparent 1px);
  background-size:40px 40px;pointer-events:none;z-index:0}

.shell{position:relative;z-index:1;display:flex;gap:20px;padding:24px;
  min-height:100vh;max-width:1400px;margin:0 auto;align-items:flex-start}
.left-panel{flex:0 0 38%;display:flex;flex-direction:column;gap:20px}
.right-panel{flex:1;display:flex;flex-direction:column;gap:20px}

.card{background:var(--card);border:1px solid var(--border);border-radius:var(--radius);
  padding:24px;box-shadow:0 4px 32px rgba(0,0,0,.5)}
.card-label{font-size:11px;letter-spacing:3px;text-transform:uppercase;
  color:#ccccdd;margin-bottom:18px;font-family:'Share Tech Mono',monospace;font-weight:600}

.top-bar{display:flex;align-items:center;justify-content:space-between;
  padding:14px 24px;background:var(--card);border:1px solid var(--border);border-radius:var(--radius)}
.logo{font-size:20px;font-weight:700;letter-spacing:2px;color:#00ffc3;text-shadow:0 0 12px #00ffc3}
.time-chip{font-family:'Share Tech Mono',monospace;font-size:12px;color:#bbbbcc;
  background:var(--card2);border:1px solid var(--border);padding:4px 12px;border-radius:30px}

.sensor-display{background:#080810;border:1px solid #00ffc340;border-radius:14px;
  padding:20px 22px;font-family:'Share Tech Mono',monospace;box-shadow:inset 0 0 30px rgba(0,255,195,.05)}
.sensor-row{display:flex;align-items:baseline;gap:10px;padding:8px 0}
.sensor-row+.sensor-row{border-top:1px solid #00ffc315}
.sensor-key{font-size:11px;color:#00ffc380;letter-spacing:2px;min-width:50px}
.sensor-val{font-size:36px;color:#00ffc3;text-shadow:0 0 20px #00ffc3,0 0 40px #00ffc322;line-height:1}
.sensor-unit{font-size:14px;color:#00ffc360}

.bar-wrap{margin-top:10px}
.bar-label{font-size:11px;letter-spacing:2px;color:#bbbbcc;margin-bottom:5px;font-weight:600}
.bar-track{height:6px;background:#1e1e28;border-radius:4px;overflow:hidden}
.bar-fill{height:100%;border-radius:4px}
.bar-fill.temp{background:linear-gradient(90deg,#ff6b35,#ff4455);box-shadow:0 0 8px #ff445588;width:57%}
.bar-fill.hum{background:linear-gradient(90deg,#3d9bff,#00ffc3);box-shadow:0 0 8px #3d9bff88;width:25%}

/* humidifier */
.humidifier-section{text-align:center}
.humi-wrap{position:relative;display:inline-block;margin:0 auto}
.mist-container{position:absolute;bottom:72px;left:50%;transform:translateX(-50%);width:80px;height:80px;pointer-events:none}
.mist{position:absolute;width:12px;height:12px;border-radius:50%;
  background:radial-gradient(circle,rgba(160,220,255,.9) 0%,rgba(100,180,255,0) 70%);opacity:0}
.humidifier.active .mist:nth-child(1){left:30%;animation:mist 2.4s ease-in infinite 0s}
.humidifier.active .mist:nth-child(2){left:50%;animation:mist 2.4s ease-in infinite .5s}
.humidifier.active .mist:nth-child(3){left:70%;animation:mist 2.4s ease-in infinite 1s}
.humidifier.active .mist:nth-child(4){left:40%;animation:mist 2.4s ease-in infinite 1.6s}
.humidifier.active .mist:nth-child(5){left:60%;animation:mist 2.4s ease-in infinite .8s}
@keyframes mist{
  0%{transform:translateY(0) scale(1);opacity:0}
  20%{opacity:.8}80%{opacity:.4}
  100%{transform:translateY(-70px) scale(2.5);opacity:0}
}
.humi-body svg{display:block}
.humidifier.active .humi-body{filter:drop-shadow(0 0 14px rgba(100,200,255,.7))}
.humidifier.inactive .humi-body{filter:brightness(.45) saturate(.2)}
.humi-status{margin-top:14px;font-size:12px;letter-spacing:3px;color:#aaaacc;text-transform:uppercase;font-weight:600}
.humidifier.active .humi-status{color:#3d9bff;text-shadow:0 0 10px #3d9bff}

/* ══ ROOM CARDS ══ */
.rooms-grid{display:grid;grid-template-columns:1fr 1fr;gap:16px}

.room-card{
  background:var(--card2);border:1px solid #333340;border-radius:var(--radius);
  padding:22px 20px 20px;position:relative;overflow:hidden;
  transition:border-color .35s,box-shadow .35s,background .35s;
}

/* colour tokens */
.room-card.r-red   {--rc:#ff4455;--rr:255;--rg:68; --rb:85}
.room-card.r-blue  {--rc:#3d9bff;--rr:61; --rg:155;--rb:255}
.room-card.r-yellow{--rc:#ffd166;--rr:255;--rg:209;--rb:102}
.room-card.r-green {--rc:#06d6a0;--rr:6;  --rg:214;--rb:160}

/* ── ON: vivid glow ── */
.room-card.on{
  border-color:var(--rc);
  background:rgba(var(--rr),var(--rg),var(--rb),.09);
  box-shadow:
    0 0 0 1px var(--rc),
    0 0 30px rgba(var(--rr),var(--rg),var(--rb),.3),
    inset 0 0 60px rgba(var(--rr),var(--rg),var(--rb),.14);
}

/* radial bloom */
.room-card::after{
  content:'';position:absolute;inset:0;border-radius:var(--radius);
  background:radial-gradient(ellipse 80% 60% at 50% 0%,rgba(var(--rr),var(--rg),var(--rb),.3) 0%,transparent 70%);
  opacity:0;transition:opacity .4s;pointer-events:none;
}
.room-card.on::after{opacity:1}

/* ceiling bulb */
.ceiling-light{
  width:16px;height:16px;border-radius:50%;background:var(--rc);
  margin:0 auto 14px;position:relative;opacity:.2;
  transition:opacity .35s,box-shadow .35s;
}
.room-card.on .ceiling-light{
  opacity:1;
  box-shadow:0 0 8px var(--rc),0 0 22px var(--rc),0 0 60px rgba(var(--rr),var(--rg),var(--rb),.55);
}
.ceiling-light::after{
  content:'';position:absolute;top:100%;left:50%;transform:translateX(-50%);
  width:0;height:0;
  border-left:22px solid transparent;border-right:22px solid transparent;border-top:40px solid transparent;
  transition:border-top-color .35s;
}
.room-card.on .ceiling-light::after{border-top-color:rgba(var(--rr),var(--rg),var(--rb),.2)}

.room-name{font-size:17px;font-weight:700;letter-spacing:1px;text-align:center;
  color:#e8e8f0;margin-bottom:5px;position:relative;z-index:1}
.room-name .icon{font-size:24px;display:block;margin-bottom:5px}

/* state label — CLEARLY visible */
.room-state-label{
  font-size:13px;letter-spacing:2px;
  color:#aaaacc;           /* OFF: clear light grey */
  text-align:center;text-transform:uppercase;font-weight:700;
  margin-bottom:18px;position:relative;z-index:1;
  transition:color .3s,text-shadow .3s;
}
.room-card.on .room-state-label{
  color:var(--rc);         /* ON: room's full colour */
  text-shadow:0 0 10px var(--rc);
}

/* toggle */
.switch-wrap{display:flex;justify-content:center;position:relative;z-index:1}
.toggle{position:relative;display:inline-block;width:58px;height:30px}
.toggle input{display:none}
.slider{position:absolute;inset:0;background:#252530;border-radius:34px;cursor:pointer;
  border:1px solid #555566;transition:background .3s,border-color .3s}
.slider::before{content:'';position:absolute;width:22px;height:22px;left:3px;top:3px;
  background:#888899;border-radius:50%;transition:transform .3s,background .3s,box-shadow .3s}
.toggle input:checked+.slider{background:rgba(var(--rr),var(--rg),var(--rb),.2);border-color:var(--rc)}
.toggle input:checked+.slider::before{transform:translateX(28px);background:var(--rc);box-shadow:0 0 12px var(--rc)}

/* ── Active-lights footer ── */
.status-footer{background:var(--card);border:1px solid var(--border);border-radius:var(--radius);padding:16px 22px}
.footer-inner{display:flex;gap:24px;flex-wrap:wrap;align-items:center}
.footer-section-label{font-size:12px;letter-spacing:3px;text-transform:uppercase;
  color:#ddddee;font-family:'Share Tech Mono',monospace;font-weight:700;white-space:nowrap}
.dot-group{display:flex;gap:16px;flex-wrap:wrap}
.dot-item{display:flex;align-items:center;gap:7px}
.dot{width:10px;height:10px;border-radius:50%;background:#2a2a38;border:1px solid #444455;
  transition:background .3s,box-shadow .3s,border-color .3s}
.dot.red-on   {background:#ff4455;border-color:#ff4455;box-shadow:0 0 10px #ff4455}
.dot.blue-on  {background:#3d9bff;border-color:#3d9bff;box-shadow:0 0 10px #3d9bff}
.dot.yellow-on{background:#ffd166;border-color:#ffd166;box-shadow:0 0 10px #ffd166}
.dot.green-on {background:#06d6a0;border-color:#06d6a0;box-shadow:0 0 10px #06d6a0}
/* dot labels — bright white */
.dot-label{font-size:13px;letter-spacing:1.5px;color:#ddddee;text-transform:uppercase;font-weight:600}
.active-count-badge{margin-left:auto;font-family:'Share Tech Mono',monospace;font-size:14px;
  color:#e0e0f0;background:rgba(255,255,255,.06);border:1px solid #444455;
  border-radius:20px;padding:5px 16px;white-space:nowrap}
.active-count-badge span{color:#00ffc3;font-weight:700;text-shadow:0 0 8px #00ffc3}

.page-footer{display:flex;justify-content:space-between;align-items:center;
  font-size:11px;letter-spacing:2px;color:#888899;font-family:'Share Tech Mono',monospace;padding:0 4px}

@media(max-width:768px){
  .shell{flex-direction:column;padding:14px;gap:14px}
  .left-panel,.right-panel{flex:none;width:100%}
  .sensor-val{font-size:28px}
}
@media(max-width:420px){.rooms-grid{grid-template-columns:1fr}}
</style>
</head>
<body>
<div class="shell">

  <!-- LEFT -->
  <div class="left-panel">
    <div class="top-bar">
      <span class="logo">&#9670; NEXHOME</span>
      <span class="time-chip" id="clk">--:--:--</span>
    </div>

    <div class="card">
      <div class="card-label">// Environment Monitor</div>
      <div class="sensor-display">
        <div class="sensor-row">
          <span class="sensor-key">TEMP</span>
          <span class="sensor-val">%TEMP%</span>
          <span class="sensor-unit">&deg;C</span>
        </div>
        <div class="sensor-row">
          <span class="sensor-key">HUM&nbsp;</span>
          <span class="sensor-val">%HUM%</span>
          <span class="sensor-unit">%</span>
        </div>
      </div>
      <div class="bar-wrap" style="margin-top:18px">
        <div class="bar-label">TEMPERATURE</div>
        <div class="bar-track"><div class="bar-fill temp"></div></div>
      </div>
      <div class="bar-wrap" style="margin-top:12px">
        <div class="bar-label">HUMIDITY</div>
        <div class="bar-track"><div class="bar-fill hum"></div></div>
      </div>
    </div>

    <div class="card humidifier-section">
      <div class="card-label">// Humidifier Status</div>
      <div class="humidifier %HUMICLASS%">
        <div class="humi-wrap">
          <div class="mist-container">
            <div class="mist"></div><div class="mist"></div>
            <div class="mist"></div><div class="mist"></div><div class="mist"></div>
          </div>
          <div class="humi-body">
            <svg width="110" height="130" viewBox="0 0 110 130" fill="none">
              <rect x="20" y="50" width="70" height="72" rx="14" fill="#1e2a3a" stroke="#3d9bff44" stroke-width="1.5"/>
              <rect x="32" y="64" width="46" height="28" rx="6" fill="#0a1520" stroke="#3d9bff30"/>
              <rect x="33" y="75" width="44" height="16" rx="5" fill="#1a4a6888"/>
              <rect x="44" y="34" width="22" height="18" rx="6" fill="#253545" stroke="#3d9bff44" stroke-width="1.5"/>
              <rect x="51" y="24" width="8" height="12" rx="4" fill="#2e4055" stroke="#3d9bff55"/>
              <circle cx="37" cy="106" r="6" fill="#0f2030" stroke="#3d9bff55"/>
              <circle cx="55" cy="106" r="6" fill="#3d9bff" opacity=".7"/>
              <circle cx="73" cy="106" r="6" fill="#0f2030" stroke="#3d9bff55"/>
              <line x1="30" y1="96" x2="80" y2="96" stroke="#3d9bff22"/>
              <line x1="30" y1="99" x2="80" y2="99" stroke="#3d9bff22"/>
            </svg>
          </div>
        </div>
        <div class="humi-status">%HUMISTATUS%</div>
      </div>
    </div>

    <div class="page-footer">
      <span>ESP32 · CONNECTED</span><span>LIVE</span>
    </div>
  </div>

  <!-- RIGHT -->
  <div class="right-panel">
    <div class="card-label" style="padding:0 4px">// Rooms &amp; Lighting Control</div>

    <div class="rooms-grid">

      <div class="room-card r-red on" id="card-red">
        <div class="ceiling-light"></div>
        <div class="room-name"><span class="icon">🛋️</span>Living Room</div>
        <div class="room-state-label" id="lbl-red">LIGHT ON</div>
        <div class="switch-wrap">
          <label class="toggle">
            <input type="checkbox" id="sw-red" checked onchange="ledToggle('red',this.checked)">
            <span class="slider"></span>
          </label>
        </div>
      </div>

      <div class="room-card r-blue" id="card-blue">
        <div class="ceiling-light"></div>
        <div class="room-name"><span class="icon">🛏️</span>Bedroom</div>
        <div class="room-state-label" id="lbl-blue">LIGHT OFF</div>
        <div class="switch-wrap">
          <label class="toggle">
            <input type="checkbox" id="sw-blue" onchange="ledToggle('blue',this.checked)">
            <span class="slider"></span>
          </label>
        </div>
      </div>

      <div class="room-card r-yellow" id="card-yellow">
        <div class="ceiling-light"></div>
        <div class="room-name"><span class="icon">🍳</span>Kitchen</div>
        <div class="room-state-label" id="lbl-yellow">LIGHT OFF</div>
        <div class="switch-wrap">
          <label class="toggle">
            <input type="checkbox" id="sw-yellow" onchange="ledToggle('yellow',this.checked)">
            <span class="slider"></span>
          </label>
        </div>
      </div>

      <div class="room-card r-green" id="card-green">
        <div class="ceiling-light"></div>
        <div class="room-name"><span class="icon">🍽️</span>Dining Room</div>
        <div class="room-state-label" id="lbl-green">LIGHT OFF</div>
        <div class="switch-wrap">
          <label class="toggle">
            <input type="checkbox" id="sw-green" onchange="ledToggle('green',this.checked)">
            <span class="slider"></span>
          </label>
        </div>
      </div>

    </div>

    <div class="status-footer">
      <div class="footer-inner">
        <span class="footer-section-label">Active Lights</span>
        <div class="dot-group">
          <div class="dot-item"><span class="dot red-on" id="dot-red"></span><span class="dot-label">Living</span></div>
          <div class="dot-item"><span class="dot" id="dot-blue"></span><span class="dot-label">Bedroom</span></div>
          <div class="dot-item"><span class="dot" id="dot-yellow"></span><span class="dot-label">Kitchen</span></div>
          <div class="dot-item"><span class="dot" id="dot-green"></span><span class="dot-label">Dining</span></div>
        </div>
        <div class="active-count-badge"><span id="activeCount">1</span> / 4 ON</div>
      </div>
    </div>
  </div>

</div>

<script>
(function tick(){
  var d=new Date(),p=function(n){return String(n).padStart(2,'0')};
  document.getElementById('clk').textContent=p(d.getHours())+':'+p(d.getMinutes())+':'+p(d.getSeconds());
  setTimeout(tick,1000);
})();

var DOT={red:'red-on',blue:'blue-on',yellow:'yellow-on',green:'green-on'};

function updateCount(){
  var n=0;
  ['red','blue','yellow','green'].forEach(function(c){
    if(document.getElementById('card-'+c).classList.contains('on')) n++;
  });
  document.getElementById('activeCount').textContent=n;
}

function applyUI(color,isOn){
  var card=document.getElementById('card-'+color);
  var lbl =document.getElementById('lbl-'+color);
  var dot =document.getElementById('dot-'+color);
  if(isOn){
    card.classList.add('on');
    lbl.textContent='LIGHT ON';
    dot.className='dot '+DOT[color];
  }else{
    card.classList.remove('on');
    lbl.textContent='LIGHT OFF';
    dot.className='dot';
  }
  updateCount();
}

function ledToggle(color,isOn){
  applyUI(color,isOn);
  // In preview mode this XHR won't reach an ESP32; in production it will.
  var xhr=new XMLHttpRequest();
  xhr.open('GET','/'+color+'/'+(isOn?'on':'off'),true);
  xhr.timeout=5000;
  xhr.onerror=xhr.ontimeout=function(){
    applyUI(color,!isOn);
    document.getElementById('sw-'+color).checked=!isOn;
  };
  xhr.send();
}

(function(){
  ['red','blue','yellow','green'].forEach(function(c){
    if(document.getElementById('card-'+c).classList.contains('on'))
      document.getElementById('dot-'+c).className='dot '+DOT[c];
  });
  updateCount();
})();
</script>
</body>
</html>
)rawhtml";

// ─── Helpers ──────────────────────────────────────────────────────────────────

String buildPage(float temp, float hum,
                 bool rOn, bool gOn, bool yOn, bool bOn) {

  int activeCount = (int)rOn + (int)gOn + (int)yOn + (int)bOn;

  // humidity bar capped at 100
  int hPct = (int)constrain(hum, 0, 100);
  // temp bar: assume range 0–50°C
  int tPct = (int)constrain(map((long)temp, 0, 50, 0, 100), 0, 100);

  String page = String(HTML_PAGE);

  page.replace("%TEMP%",   String(temp, 1));
  page.replace("%HUM%",    String(hum,  1));
  page.replace("%TPCT%",   String(tPct));
  page.replace("%ACOUNT%", String(activeCount));

  // humidifier
  if (hum < 30) {
    page.replace("%HUMICLASS%",  "active");
    page.replace("%HUMISTATUS%", "SPRAYING — LOW HUMIDITY");
  } else {
    page.replace("%HUMICLASS%",  "inactive");
    page.replace("%HUMISTATUS%", "STANDBY — HUMIDITY OK");
  }

  // room states
  auto roomSubs = [&](String color, bool state) {
    String CLS = color; CLS.toUpperCase();
    page.replace("%" + CLS + "_CLS%", state ? "on" : "");
    page.replace("%" + CLS + "_LBL%", state ? "LIGHT ON" : "LIGHT OFF");
    page.replace("%" + CLS + "_CHK%", state ? "checked" : "");
  };
  roomSubs("RED",    rOn);
  roomSubs("GREEN",  gOn);
  roomSubs("YELLOW", yOn);
  roomSubs("BLUE",   bOn);

  return page;
}

// ─── Setup ────────────────────────────────────────────────────────────────────
void setup() {
  pinMode(Rled, OUTPUT);
  pinMode(Gled, OUTPUT);
  pinMode(Yled, OUTPUT);
  pinMode(Bled, OUTPUT);

  Serial.begin(9600);
  delay(2000);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nConnected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.begin();
  Serial.println("Server started");
}

// ─── Loop ─────────────────────────────────────────────────────────────────────
void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  Serial.println("Client connected");
  String request = "";
  unsigned long t0 = millis();

  // read HTTP request line
  while (client.connected() && millis() - t0 < 2000) {
    if (client.available()) {
      char c = client.read();
      request += c;
      if (request.endsWith("\r\n\r\n")) break;  // headers done
    }
  }

  // Parse first line
  String firstLine = request.substring(0, request.indexOf('\n'));

  // ── LED control ──────────────────────────────────────────────────────────
  if (firstLine.indexOf("GET /red/on")    != -1) { redState    = true;  digitalWrite(Rled, HIGH); }
  if (firstLine.indexOf("GET /red/off")   != -1) { redState    = false; digitalWrite(Rled, LOW);  }
  if (firstLine.indexOf("GET /green/on")  != -1) { greenState  = true;  digitalWrite(Gled, HIGH); }
  if (firstLine.indexOf("GET /green/off") != -1) { greenState  = false; digitalWrite(Gled, LOW);  }
  if (firstLine.indexOf("GET /yellow/on") != -1) { yellowState = true;  digitalWrite(Yled, HIGH); }
  if (firstLine.indexOf("GET /yellow/off")!= -1) { yellowState = false; digitalWrite(Yled, LOW);  }
  if (firstLine.indexOf("GET /blue/on")   != -1) { blueState   = true;  digitalWrite(Bled, HIGH); }
  if (firstLine.indexOf("GET /blue/off")  != -1) { blueState   = false; digitalWrite(Bled, LOW);  }

  // ── Read sensors ─────────────────────────────────────────────────────────
  float temperature = dht.readTemperature();
  float humidity    = dht.readHumidity();

  if (isnan(temperature)) temperature = 0.0;
  if (isnan(humidity))    humidity    = 0.0;

  // For LED-only XHR calls (no HTML needed), send minimal 204
  bool isXHR = (firstLine.indexOf("GET /red")    != -1 ||
                firstLine.indexOf("GET /green")  != -1 ||
                firstLine.indexOf("GET /yellow") != -1 ||
                firstLine.indexOf("GET /blue")   != -1);

  if (isXHR && firstLine.indexOf("GET / ") == -1) {
    client.println("HTTP/1.1 204 No Content");
    client.println("Connection: close");
    client.println();
  } else {
    // Full page
    String html = buildPage(temperature, humidity,
                            redState, greenState, yellowState, blueState);

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html; charset=utf-8");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(html.length());
    client.println();
    client.print(html);
  }

  delay(5);
  client.stop();
  Serial.println("Client disconnected");
}
