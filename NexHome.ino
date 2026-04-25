// ═══════════════════════════════════════════════════════════════════════════════
//  NexHome ESP32 — Smart Home Dashboard  v2
//  UI matches reference image exactly:
//    ✓ Realistic room illustrations (SVG drawn furniture)
//    ✓ Realistic humidifier SVG with animated mist
//    ✓ Humidifier AUTO ON when humidity < 30%
//    ✓ LEDs toggle via dashboard buttons  (GPIO 26/18/27/25)
//    ✓ Room cards light up with glow + lamp effect on toggle
//    ✓ Circular room icons in footer with ON/OFF colour bars
//    ✓ Live clock, WiFi chip, temp/humidity sensor tiles
// ═══════════════════════════════════════════════════════════════════════════════

#include <WiFi.h>
#include <DHT.h>

// ── WiFi ──────────────────────────────────────────────────────────────────────
const char* ssid     = "ADI";
const char* password = "adi321!!";

// ── DHT11 ─────────────────────────────────────────────────────────────────────
#define DHTPIN   32
#define DHTTYPE  DHT11
DHT dht(DHTPIN, DHTTYPE);

// ── LED GPIO ──────────────────────────────────────────────────────────────────
#define PIN_RED    26   // Living Room
#define PIN_BLUE   18   // Bedroom
#define PIN_YELLOW 27   // Kitchen
#define PIN_GREEN  25   // Dining Room

// ── Global State ──────────────────────────────────────────────────────────────
bool redState = false, blueState = false, yellowState = false, greenState = false;

WiFiServer server(80);

// ═══════════════════════════════════════════════════════════════════════════════
//  HTML  (PROGMEM)
// ═══════════════════════════════════════════════════════════════════════════════
const char HTML_PAGE[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1.0"/>
<title>NexHome</title>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700;800&display=swap" rel="stylesheet"/>
<style>
*,*::before,*::after{box-sizing:border-box;margin:0;padding:0}
:root{
  --bg:#0b0b12;--card:#12121e;--card2:#0e0e1a;--border:#1c1c2e;
  --text:#e4e4f0;--sub:#7070a0;
  --red:#ff3d5a;--blue:#3d9fff;--yellow:#ffcc44;--green:#00d48a;
  --green-acc:#00ffc3;--radius:18px;
  --rr:255;--rg:61;--rb:90;   /* defaults — overridden per room */
}
html,body{height:100%;background:var(--bg);color:var(--text);
  font-family:'Inter',sans-serif;font-size:14px;line-height:1.4}

/* subtle dot-grid */
body::before{content:'';position:fixed;inset:0;
  background:radial-gradient(rgba(0,255,195,.10) 1px,transparent 1px);
  background-size:30px 30px;pointer-events:none;z-index:0}

/* ── SHELL ── */
.shell{position:relative;z-index:1;
  display:grid;
  grid-template-areas:"topbar topbar"
                       "left   right"
                       "footer footer";
  grid-template-columns:380px 1fr;
  grid-template-rows:auto 1fr auto;
  gap:14px;padding:18px;
  min-height:100vh;max-width:1440px;margin:0 auto}

/* ── TOP BAR ── */
.top-bar{grid-area:topbar;display:flex;align-items:center;gap:14px;
  background:var(--card);border:1px solid var(--border);border-radius:var(--radius);
  padding:12px 22px}
.logo{display:flex;align-items:center;gap:10px;text-decoration:none}
.logo-box{width:38px;height:38px;border-radius:12px;
  background:linear-gradient(135deg,#2266ff,#00ffc3);
  display:flex;align-items:center;justify-content:center;font-size:18px;
  box-shadow:0 0 18px rgba(0,255,195,.35)}
.logo-text{font-size:22px;font-weight:800;letter-spacing:2px;color:#fff}
.logo-text span{color:var(--green-acc)}
.top-right{margin-left:auto;display:flex;align-items:center;gap:10px}
.chip{display:flex;align-items:center;gap:7px;border-radius:30px;
  padding:7px 16px;font-size:12px;font-weight:600;border:1px solid}
.chip-wifi{background:#0d1f0d;border-color:#1a3a1a;color:#c0f0c0}
.chip-time{background:var(--card2);border-color:var(--border);color:var(--text);
  font-variant-numeric:tabular-nums;font-size:13px}
.wifi-led{width:8px;height:8px;border-radius:50%;background:#00d48a;
  box-shadow:0 0 8px #00d48a;animation:blink 2s infinite}
@keyframes blink{0%,100%{opacity:1}50%{opacity:.4}}

/* ── CARDS ── */
.left-col{grid-area:left;display:flex;flex-direction:column;gap:14px}
.right-col{grid-area:right;display:flex;flex-direction:column;gap:14px}
.card{background:var(--card);border:1px solid var(--border);
  border-radius:var(--radius);padding:20px}
.card-hdr{display:flex;align-items:center;gap:9px;margin-bottom:18px}
.card-ico{width:32px;height:32px;border-radius:10px;
  display:flex;align-items:center;justify-content:center;font-size:16px}
.card-ico.g{background:rgba(0,212,138,.14);border:1px solid rgba(0,212,138,.3)}
.card-ttl{font-size:14px;font-weight:700;color:#d0d0e8}

/* ── SENSOR TILES ── */
.sensor-row{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:18px}
.stile{background:var(--card2);border:1px solid var(--border);border-radius:14px;
  padding:18px 14px;text-align:center}
.stile .ico{font-size:34px;display:block;margin-bottom:6px}
.stile .lbl{font-size:10px;font-weight:700;letter-spacing:2px;color:var(--sub);
  text-transform:uppercase;margin-bottom:6px}
.stile .val{font-size:30px;font-weight:800;line-height:1}
.stile .unit{font-size:15px;font-weight:500}
.stile.temp .val{color:#ff4466;text-shadow:0 0 20px rgba(255,68,102,.45)}
.stile.hum  .val{color:#3d9fff;text-shadow:0 0 20px rgba(61,159,255,.45)}

/* ── BARS ── */
.brow{display:flex;justify-content:space-between;align-items:center;margin-bottom:5px}
.blbl{font-size:10px;font-weight:700;letter-spacing:1.5px;text-transform:uppercase;color:var(--sub)}
.bval{font-size:10px;font-weight:700}
.btrack{height:5px;background:#181828;border-radius:4px;overflow:hidden;margin-bottom:12px}
.bfill{height:100%;border-radius:4px;transition:width .6s}
.bfill.t{background:linear-gradient(90deg,#ff6030,#ff3d5a);box-shadow:0 0 6px rgba(255,61,90,.5)}
.bfill.h{background:linear-gradient(90deg,#3d9fff,#00ffc3);box-shadow:0 0 6px rgba(61,159,255,.5)}

/* ── HUMIDIFIER ── */
.humi-wrap{display:flex;align-items:center;gap:20px}
.humi-left{position:relative;flex:0 0 100px;height:120px;
  display:flex;align-items:flex-end;justify-content:center}
.mist-zone{position:absolute;bottom:100px;left:50%;transform:translateX(-50%);
  width:70px;height:70px;pointer-events:none}
.mp{position:absolute;border-radius:50%;opacity:0;
  background:radial-gradient(circle,rgba(180,230,255,.95) 0%,transparent 70%)}
/* mist only when .humi-on */
.humi-on .mp:nth-child(1){width:12px;height:12px;left:15%;animation:mp 2.2s ease-in infinite .0s}
.humi-on .mp:nth-child(2){width:9px; height:9px; left:42%;animation:mp 2.2s ease-in infinite .5s}
.humi-on .mp:nth-child(3){width:12px;height:12px;left:65%;animation:mp 2.2s ease-in infinite 1.0s}
.humi-on .mp:nth-child(4){width:8px; height:8px; left:30%;animation:mp 2.2s ease-in infinite 1.6s}
@keyframes mp{
  0%  {transform:translateY(0) scale(1);opacity:0}
  15% {opacity:.9}
  80% {opacity:.35}
  100%{transform:translateY(-62px) scale(2.4);opacity:0}
}
.humi-svg-wrap{position:relative;z-index:1;transition:.3s}
.humi-on  .humi-svg-wrap{filter:drop-shadow(0 0 18px rgba(100,200,255,.8))}
.humi-off .humi-svg-wrap{filter:brightness(.38) saturate(.15)}
.humi-right .hl{font-size:14px;font-weight:700;color:var(--green-acc);margin-bottom:5px}
.humi-off  .humi-right .hl{color:var(--sub)}
.humi-right .hd{font-size:12px;color:var(--sub);line-height:1.6;margin-bottom:14px}
.tsm{position:relative;width:50px;height:26px;flex-shrink:0}
.tsm input{display:none}
.slsm{position:absolute;inset:0;border-radius:26px;cursor:pointer;
  background:#181828;border:1px solid #30304a;transition:.3s}
.slsm::before{content:'';position:absolute;width:20px;height:20px;top:2px;left:2px;
  border-radius:50%;background:#50507a;transition:.3s}
.tsm input:checked+.slsm{background:rgba(0,212,138,.28);border-color:var(--green-acc)}
.tsm input:checked+.slsm::before{transform:translateX(24px);
  background:var(--green-acc);box-shadow:0 0 10px var(--green-acc)}
.row-c{display:flex;align-items:center;gap:10px}
.tlbl{font-size:12px;font-weight:600;color:var(--sub)}

/* ── ROOMS ── */
.sec-ttl{font-size:13px;font-weight:700;letter-spacing:1px;color:#c8c8e0;
  display:flex;align-items:center;gap:9px}
.sec-ttl::before{content:'';width:4px;height:18px;border-radius:2px;
  background:var(--green-acc);box-shadow:0 0 10px var(--green-acc)}
.rooms-grid{display:grid;grid-template-columns:1fr 1fr;gap:14px}

/* room card base */
.rc{background:var(--card);border:1px solid var(--border);
  border-radius:var(--radius);padding:0 0 16px;
  position:relative;overflow:hidden;cursor:pointer;
  transition:border-color .35s,box-shadow .35s,background .35s}

/* colour tokens per room */
.rc.r-red   {--rc:#ff3d5a;--rr:255;--rg:61; --rb:90 }
.rc.r-blue  {--rc:#3d9fff;--rr:61; --rg:159;--rb:255}
.rc.r-yellow{--rc:#ffcc44;--rr:255;--rg:204;--rb:68 }
.rc.r-green {--rc:#00d48a;--rr:0;  --rg:212;--rb:138}

/* ON state */
.rc.on{
  border-color:var(--rc);
  background:rgba(var(--rr),var(--rg),var(--rb),.06);
  box-shadow:0 0 0 1px rgba(var(--rr),var(--rg),var(--rb),.5),
             0 0 50px rgba(var(--rr),var(--rg),var(--rb),.22),
             inset 0 0 70px rgba(var(--rr),var(--rg),var(--rb),.07)}

/* corner dot */
.cdot{position:absolute;top:14px;right:14px;width:10px;height:10px;
  border-radius:50%;background:rgba(var(--rr),var(--rg),var(--rb),.18);
  border:1px solid rgba(var(--rr),var(--rg),var(--rb),.4);transition:.3s;z-index:2}
.rc.on .cdot{background:var(--rc);border-color:var(--rc);
  box-shadow:0 0 8px var(--rc),0 0 22px rgba(var(--rr),var(--rg),var(--rb),.55)}

/* ── ROOM ILLUSTRATION ── */
.room-scene{width:100%;height:155px;position:relative;overflow:hidden;
  background:rgba(var(--rr),var(--rg),var(--rb),.04);
  border-bottom:1px solid rgba(var(--rr),var(--rg),var(--rb),.12);
  transition:.35s}
.rc.on .room-scene{
  background:rgba(var(--rr),var(--rg),var(--rb),.13);
  border-bottom-color:rgba(var(--rr),var(--rg),var(--rb),.3)}

/* pendant */
.pend{position:absolute;top:0;left:50%;transform:translateX(-50%)}
.pw{stroke:rgba(var(--rr),var(--rg),var(--rb),.35);stroke-width:2;transition:.3s}
.ps{fill:rgba(var(--rr),var(--rg),var(--rb),.2);stroke:rgba(var(--rr),var(--rg),var(--rb),.5);transition:.3s}
.rc.on .pw{stroke:rgba(var(--rr),var(--rg),var(--rb),.95)}
.rc.on .ps{fill:var(--rc);stroke:var(--rc);filter:drop-shadow(0 0 14px var(--rc))}

/* light cone */
.cone{position:absolute;top:44px;left:50%;transform:translateX(-50%);
  width:0;height:0;
  border-left:65px solid transparent;border-right:65px solid transparent;
  border-top:100px solid rgba(var(--rr),var(--rg),var(--rb),0);
  transition:.4s;pointer-events:none}
.rc.on .cone{border-top-color:rgba(var(--rr),var(--rg),var(--rb),.20)}

/* floor-glow */
.fglow{position:absolute;bottom:0;left:50%;transform:translateX(-50%);
  width:100%;height:40px;
  background:radial-gradient(ellipse 70% 100% at 50% 100%,rgba(var(--rr),var(--rg),var(--rb),0) 0%,transparent 100%);
  transition:.4s;pointer-events:none}
.rc.on .fglow{background:radial-gradient(ellipse 70% 100% at 50% 100%,rgba(var(--rr),var(--rg),var(--rb),.28) 0%,transparent 100%)}

/* furniture SVGs — always present, brightness controlled */
.furn{position:absolute;bottom:0;left:0;width:100%;transition:.4s;
  filter:saturate(.25) brightness(.45)}
.rc.on .furn{filter:saturate(1) brightness(1)}

/* room info */
.rinfo{
  padding:16px;
  display:flex;
  justify-content:space-between;
  align-items:center;
}
.rc{
  padding-bottom:20px;
}
.rname{font-size:15px;font-weight:700;margin-bottom:3px}
.rstate{font-size:10px;font-weight:700;letter-spacing:2px;text-transform:uppercase;
  color:var(--sub);margin-bottom:12px;transition:.3s}
.rc.on .rstate{color:var(--rc);text-shadow:0 0 10px var(--rc)}

/* toggle */
.tgl{position:relative;width:54px;height:28px}
.tgl input{display:none}
.sl{position:absolute;inset:0;border-radius:28px;cursor:pointer;
  background:#181828;border:1px solid #30304a;transition:.3s}
.sl::before{content:'';position:absolute;width:20px;height:20px;top:3px;left:3px;
  border-radius:50%;background:#50507a;transition:.3s}
.tgl input:checked+.sl{background:rgba(var(--rr),var(--rg),var(--rb),.22);border-color:var(--rc)}
.tgl input:checked+.sl::before{transform:translateX(26px);
  background:var(--rc);box-shadow:0 0 12px var(--rc)}

/* ── FOOTER ── */
.footer-bar{grid-area:footer;
  background:var(--card);border:1px solid var(--border);border-radius:var(--radius);
  padding:16px 24px}
.fi{display:flex;align-items:center;gap:18px;flex-wrap:wrap}
.fttl{font-size:13px;font-weight:700;letter-spacing:1px;color:#d0d0e8;
  white-space:nowrap;display:flex;align-items:center;gap:8px}
.frooms{display:flex;gap:22px;flex:1;flex-wrap:wrap}

/* circular room item */
.fri{display:flex;flex-direction:column;align-items:flex-start;gap:5px;min-width:90px}
.fri-circle{width:52px;height:52px;border-radius:50%;border:2px solid var(--border);
  background:var(--card2);display:flex;align-items:center;justify-content:center;
  font-size:22px;transition:.35s;position:relative}
.fri.on .fri-circle{border-color:var(--rc);
  background:rgba(var(--rr),var(--rg),var(--rb),.15);
  box-shadow:0 0 16px rgba(var(--rr),var(--rg),var(--rb),.35)}
/* per-room tokens on footer items */
.fri.r-red   {--rc:#ff3d5a;--rr:255;--rg:61; --rb:90 }
.fri.r-blue  {--rc:#3d9fff;--rr:61; --rg:159;--rb:255}
.fri.r-yellow{--rc:#ffcc44;--rr:255;--rg:204;--rb:68 }
.fri.r-green {--rc:#00d48a;--rr:0;  --rg:212;--rb:138}
.fri-name{font-size:11px;font-weight:600;color:var(--sub)}
.fri-state{font-size:11px;font-weight:800;letter-spacing:1.5px;
  text-transform:uppercase;color:var(--sub);transition:.3s}
.fri.on .fri-state{color:var(--rc)}
.fri-bar{width:52px;height:3px;background:#181828;border-radius:2px;overflow:hidden}
.fri-bf{height:100%;width:0;background:var(--rc);border-radius:2px;transition:.35s}
.fri.on .fri-bf{width:100%}
.badge{margin-left:auto;background:rgba(0,255,195,.1);
  border:1px solid rgba(0,255,195,.35);border-radius:22px;
  padding:7px 20px;font-size:14px;font-weight:800;white-space:nowrap}
.badge span{color:var(--green-acc);text-shadow:0 0 10px var(--green-acc)}

/* responsive */
@media(max-width:960px){
  .shell{grid-template-areas:"topbar""left""right""footer";
    grid-template-columns:1fr}
}
@media(max-width:480px){.rooms-grid{grid-template-columns:1fr}}
</style>
</head>
<body>
<div class="shell">

<!-- ═══ TOP BAR ═══ -->
<div class="top-bar">
  <div class="logo">
    <div class="logo-box">🏠</div>
    <span class="logo-text">NEX<span>HOME</span></span>
  </div>
  <div class="top-right">
    <div class="chip chip-wifi">
      <span>📶</span><div class="wifi-led"></div><span>ESP32 Connected</span>
    </div>
    <div class="chip chip-time">
      <span>🕐</span><span id="clk">--:--:--</span>
    </div>
  </div>
</div>

<!-- ═══ LEFT COLUMN ═══ -->
<div class="left-col">

  <!-- Environment Monitor -->
  <div class="card">
    <div class="card-hdr">
      <div class="card-ico g">🌿</div>
      <span class="card-ttl">Environment Monitor</span>
    </div>
    <div class="sensor-row">
      <div class="stile temp">
        <span class="ico">🌡️</span>
        <div class="lbl">Temperature</div>
        <div class="val">%TEMP%<span class="unit">°C</span></div>
      </div>
      <div class="stile hum">
        <span class="ico">💧</span>
        <div class="lbl">Humidity</div>
        <div class="val">%HUM%<span class="unit">%</span></div>
      </div>
    </div>
    <div class="brow">
      <span class="blbl">Temperature</span>
      <span class="bval" style="color:#ff4466">%TPCT%%</span>
    </div>
    <div class="btrack"><div class="bfill t" style="width:%TPCT%%"></div></div>
    <div class="brow">
      <span class="blbl">Humidity</span>
      <span class="bval" style="color:#3d9fff">%HUM%%</span>
    </div>
    <div class="btrack"><div class="bfill h" style="width:%HUM%%"></div></div>
  </div>

  <!-- Humidifier -->
  <div class="card">
    <div class="card-hdr">
      <div class="card-ico g">💧</div>
      <span class="card-ttl">Humidifier Status</span>
    </div>
    <div class="humi-wrap">
      <!-- illustration -->
      <div class="humi-left %HUMICLASS%" id="humiLeft">
        <div class="mist-zone" id="mistZone">
          <div class="mp"></div><div class="mp"></div>
          <div class="mp"></div><div class="mp"></div>
        </div>
        <div class="humi-svg-wrap">
          <!-- realistic humidifier SVG -->
          <svg width="90" height="110" viewBox="0 0 90 110" fill="none" xmlns="http://www.w3.org/2000/svg">
            <!-- body -->
            <ellipse cx="45" cy="85" rx="34" ry="20" fill="#1a2535" stroke="#3d9bff33" stroke-width="1"/>
            <path d="M11 68 Q11 48 45 44 Q79 48 79 68 L79 85 Q79 105 45 105 Q11 105 11 85 Z"
                  fill="url(#hbody)" stroke="#3d9bff44" stroke-width="1.5"/>
            <!-- neck -->
            <rect x="31" y="30" width="28" height="18" rx="8" fill="#1e2e42" stroke="#3d9bff44" stroke-width="1.5"/>
            <!-- nozzle -->
            <rect x="37" y="18" width="16" height="14" rx="6" fill="#253545" stroke="#4db0ff66" stroke-width="1.5"/>
            <!-- opening hole -->
            <ellipse cx="45" cy="18" rx="5" ry="3" fill="#0a1520"/>
            <!-- panel window -->
            <rect x="22" y="56" width="46" height="26" rx="7" fill="#0a1825" stroke="#3d9bff30" stroke-width="1"/>
            <rect x="24" y="58" width="42" height="22" rx="6" fill="#08141e"/>
            <!-- water level bar -->
            <rect x="26" y="67" width="38" height="10" rx="4" fill="#0e2234"/>
            <rect x="26" y="67" width="26" height="10" rx="4" fill="url(#wlevel)" opacity=".85"/>
            <!-- buttons row -->
            <circle cx="32" cy="94" r="5.5" fill="#0d1f30" stroke="#3d9bff44" stroke-width="1"/>
            <circle cx="45" cy="94" r="5.5" fill="#3d9bff" opacity=".85"/>
            <circle cx="58" cy="94" r="5.5" fill="#0d1f30" stroke="#3d9bff44" stroke-width="1"/>
            <!-- button inner glow -->
            <circle cx="45" cy="94" r="3" fill="#80d0ff" opacity=".5"/>
            <!-- separator lines -->
            <line x1="18" y1="86" x2="72" y2="86" stroke="#3d9bff18" stroke-width="1"/>
            <!-- gradients -->
            <defs>
              <linearGradient id="hbody" x1="0" y1="0" x2="0" y2="1">
                <stop offset="0%" stop-color="#253a52"/>
                <stop offset="100%" stop-color="#162030"/>
              </linearGradient>
              <linearGradient id="wlevel" x1="0" y1="0" x2="1" y2="0">
                <stop offset="0%" stop-color="#3d9bff"/>
                <stop offset="100%" stop-color="#00ffc3"/>
              </linearGradient>
            </defs>
          </svg>
        </div>
      </div>
      <!-- info -->
      <div class="humi-right" id="humiRight">
        <div class="hl" id="humiLbl">%HUMILABEL%</div>
        <div class="hd">Humidifier turns ON<br>when humidity &lt; 30%</div>
        <div class="row-c">
          <label class="tsm">
            <input type="checkbox" id="sw-humi" %HUMICHK% onchange="humiToggle(this.checked)">
            <span class="slsm"></span>
          </label>
          <span class="tlbl">Auto Mode</span>
        </div>
      </div>
    </div>
  </div>

</div><!-- /left-col -->

<!-- ═══ RIGHT COLUMN ═══ -->
<div class="right-col">
  <div class="sec-ttl">Rooms &amp; Lighting Control</div>
  <div class="rooms-grid">

    <!-- ── LIVING ROOM (Red) ── -->
    <div class="rc r-red %RED_CLS%" id="card-red" onclick="cardClick('red')">
      <div class="cdot"></div>
      <div class="room-scene">
        <!-- pendant lamp -->
        <svg class="pend" width="100" height="55" viewBox="0 0 100 55">
          <line class="pw" x1="50" y1="0" x2="50" y2="22"/>
          <path class="ps" d="M30 30 Q50 42 70 30 L65 22 Q50 16 35 22 Z"/>
        </svg>
        <div class="cone"></div>
        <div class="fglow"></div>
        <!-- Living room SVG furniture -->
        <svg class="furn" viewBox="0 0 320 120" xmlns="http://www.w3.org/2000/svg">
          <!-- floor -->
          <rect x="0" y="108" width="320" height="12" fill="#1a0a14" opacity=".8"/>
          <!-- sofa body -->
          <rect x="60" y="72" width="160" height="36" rx="10" fill="#c0304a"/>
          <!-- sofa back -->
          <rect x="60" y="50" width="160" height="28" rx="8" fill="#d03858"/>
          <!-- sofa armrests -->
          <rect x="52" y="60" width="20" height="44" rx="8" fill="#b02840"/>
          <rect x="228" y="60" width="20" height="44" rx="8" fill="#b02840"/>
          <!-- sofa cushions -->
          <rect x="72" y="55" width="60" height="50" rx="6" fill="#c83050"/>
          <rect x="140" y="55" width="60" height="50" rx="6" fill="#c83050"/>
          <!-- cushion lines -->
          <line x1="102" y1="60" x2="102" y2="100" stroke="#e04060" stroke-width="1" opacity=".5"/>
          <line x1="170" y1="60" x2="170" y2="100" stroke="#e04060" stroke-width="1" opacity=".5"/>
          <!-- pillow left -->
          <rect x="78" y="60" width="38" height="22" rx="5" fill="#e05070" opacity=".8"/>
          <!-- pillow right -->
          <rect x="146" y="60" width="38" height="22" rx="5" fill="#e05070" opacity=".8"/>
          <!-- plant left -->
          <rect x="18" y="90" width="14" height="18" rx="3" fill="#8B4513"/>
          <ellipse cx="25" cy="85" rx="16" ry="18" fill="#1a6a1a"/>
          <ellipse cx="18" cy="78" rx="10" ry="12" fill="#228822"/>
          <ellipse cx="32" cy="79" rx="9" ry="11" fill="#1a7a1a"/>
          <!-- side table -->
          <rect x="240" y="82" width="36" height="4" rx="2" fill="#5a3a2a"/>
          <rect x="245" y="86" width="4" height="22" rx="2" fill="#4a2a1a"/>
          <rect x="267" y="86" width="4" height="22" rx="2" fill="#4a2a1a"/>
          <!-- plant right small -->
          <rect x="248" y="70" width="10" height="14" rx="2" fill="#8B4513"/>
          <ellipse cx="253" cy="66" rx="10" ry="10" fill="#1a7a2a"/>
        </svg>
      </div>
      <div class="rinfo">
        <div class="rname">Living Room</div>
        <div class="rstate" id="lbl-red">%RED_LBL%</div>
        <label class="tgl" onclick="event.stopPropagation()">
          <input type="checkbox" id="sw-red" %RED_CHK% onchange="ledToggle('red',this.checked)">
          <span class="sl"></span>
        </label>
      </div>
    </div>

    <!-- ── BEDROOM (Blue) ── -->
    <div class="rc r-blue %BLUE_CLS%" id="card-blue" onclick="cardClick('blue')">
      <div class="cdot"></div>
      <div class="room-scene">
        <svg class="pend" width="100" height="55" viewBox="0 0 100 55">
          <line class="pw" x1="50" y1="0" x2="50" y2="22"/>
          <path class="ps" d="M30 30 Q50 42 70 30 L65 22 Q50 16 35 22 Z"/>
        </svg>
        <div class="cone"></div>
        <div class="fglow"></div>
        <!-- Bedroom SVG furniture -->
        <svg class="furn" viewBox="0 0 320 120" xmlns="http://www.w3.org/2000/svg">
          <rect x="0" y="108" width="320" height="12" fill="#050a18" opacity=".8"/>
          <!-- bed frame -->
          <rect x="70" y="60" width="180" height="55" rx="6" fill="#1a2a4a"/>
          <!-- headboard -->
          <rect x="70" y="38" width="180" height="26" rx="8" fill="#223460"/>
          <!-- mattress -->
          <rect x="76" y="62" width="168" height="48" rx="5" fill="#e8eaf6"/>
          <!-- duvet -->
          <rect x="76" y="75" width="168" height="35" rx="5" fill="#3d62aa"/>
          <!-- duvet stripes -->
          <rect x="76" y="82" width="168" height="4" rx="2" fill="#4d72ba" opacity=".6"/>
          <rect x="76" y="92" width="168" height="4" rx="2" fill="#4d72ba" opacity=".6"/>
          <!-- pillows -->
          <rect x="82" y="62" width="60" height="18" rx="6" fill="#f0f4ff"/>
          <rect x="178" y="62" width="60" height="18" rx="6" fill="#f0f4ff"/>
          <!-- pillow shading -->
          <rect x="82" y="62" width="60" height="4" rx="3" fill="#d8dcf0" opacity=".5"/>
          <rect x="178" y="62" width="60" height="4" rx="3" fill="#d8dcf0" opacity=".5"/>
          <!-- nightstand left -->
          <rect x="22" y="78" width="40" height="30" rx="5" fill="#1a2a3a"/>
          <rect x="26" y="82" width="32" height="20" rx="3" fill="#152030"/>
          <circle cx="42" cy="92" r="3" fill="#3d9bff" opacity=".6"/>
          <!-- lamp on nightstand -->
          <rect x="35" y="60" width="6" height="18" rx="3" fill="#2a3a4a"/>
          <path d="M25 60 Q41 52 57 60 L54 65 Q41 58 28 65 Z" fill="#4d7acc" opacity=".9"/>
          <!-- nightstand right -->
          <rect x="258" y="78" width="40" height="30" rx="5" fill="#1a2a3a"/>
          <!-- plant -->
          <rect x="268" y="65" width="8" height="16" rx="2" fill="#2a3a2a"/>
          <ellipse cx="272" cy="62" rx="10" ry="10" fill="#1a6a3a"/>
          <ellipse cx="266" cy="58" rx="7" ry="8" fill="#228844"/>
          <!-- pencils in cup right nightstand -->
          <rect x="272" y="58" width="3" height="14" rx="1" fill="#ff8833"/>
          <rect x="276" y="56" width="3" height="16" rx="1" fill="#3355ff"/>
        </svg>
      </div>
      <div class="rinfo">
        <div class="rname">Bedroom</div>
        <div class="rstate" id="lbl-blue">%BLUE_LBL%</div>
        <label class="tgl" onclick="event.stopPropagation()">
          <input type="checkbox" id="sw-blue" %BLUE_CHK% onchange="ledToggle('blue',this.checked)">
          <span class="sl"></span>
        </label>
      </div>
    </div>

    <!-- ── KITCHEN (Yellow) ── -->
    <div class="rc r-yellow %YELLOW_CLS%" id="card-yellow" onclick="cardClick('yellow')">
      <div class="cdot"></div>
      <div class="room-scene">
        <svg class="pend" width="100" height="55" viewBox="0 0 100 55">
          <line class="pw" x1="50" y1="0" x2="50" y2="22"/>
          <path class="ps" d="M30 30 Q50 42 70 30 L65 22 Q50 16 35 22 Z"/>
        </svg>
        <div class="cone"></div>
        <div class="fglow"></div>
        <!-- Kitchen SVG furniture -->
        <svg class="furn" viewBox="0 0 320 120" xmlns="http://www.w3.org/2000/svg">
          <rect x="0" y="108" width="320" height="12" fill="#180e00" opacity=".8"/>
          <!-- counter top -->
          <rect x="10" y="66" width="240" height="8" rx="3" fill="#555560"/>
          <!-- cabinet lower left -->
          <rect x="10" y="74" width="70" height="36" rx="4" fill="#3a3a45"/>
          <line x1="45" y1="74" x2="45" y2="110" stroke="#50505a" stroke-width="1"/>
          <circle cx="38" cy="92" r="3" fill="#888890"/>
          <circle cx="52" cy="92" r="3" fill="#888890"/>
          <!-- microwave -->
          <rect x="88" y="68" width="70" height="42" rx="5" fill="#2a2a35"/>
          <rect x="90" y="70" width="50" height="36" rx="3" fill="#181822"/>
          <rect x="145" y="70" width="11" height="36" rx="2" fill="#333340"/>
          <!-- microwave display -->
          <rect x="92" y="76" width="46" height="14" rx="2" fill="#0a1a0a"/>
          <rect x="94" y="78" width="22" height="10" rx="1" fill="#002200"/>
          <!-- microwave buttons -->
          <circle cx="150" cy="78" r="3" fill="#555560"/>
          <circle cx="150" cy="86" r="3" fill="#555560"/>
          <circle cx="150" cy="94" r="3" fill="#cc4400"/>
          <!-- refrigerator right -->
          <rect x="168" y="30" width="78" height="80" rx="6" fill="#2e2e3a"/>
          <rect x="170" y="32" width="74" height="37" rx="4" fill="#252530"/>
          <rect x="170" y="71" width="74" height="37" rx="4" fill="#252530"/>
          <rect x="232" y="48" width="5" height="8" rx="2" fill="#888890"/>
          <rect x="232" y="80" width="5" height="8" rx="2" fill="#888890"/>
          <!-- plant on counter -->
          <rect x="52" y="52" width="10" height="16" rx="3" fill="#4a3010"/>
          <ellipse cx="57" cy="48" rx="12" ry="12" fill="#2a6a2a"/>
          <ellipse cx="50" cy="44" rx="8" ry="9" fill="#338833"/>
          <ellipse cx="65" cy="45" rx="7" ry="8" fill="#2a7a2a"/>
          <!-- bottles on counter -->
          <rect x="26" y="52" width="7" height="16" rx="3" fill="#1a6a3a"/>
          <rect x="34" y="54" width="6" height="14" rx="3" fill="#8B3a00"/>
        </svg>
      </div>
      <div class="rinfo">
        <div class="rname">Kitchen</div>
        <div class="rstate" id="lbl-yellow">%YELLOW_LBL%</div>
        <label class="tgl" onclick="event.stopPropagation()">
          <input type="checkbox" id="sw-yellow" %YELLOW_CHK% onchange="ledToggle('yellow',this.checked)">
          <span class="sl"></span>
        </label>
      </div>
    </div>

    <!-- ── DINING ROOM (Green) ── -->
    <div class="rc r-green %GREEN_CLS%" id="card-green" onclick="cardClick('green')">
      <div class="cdot"></div>
      <div class="room-scene">
        <svg class="pend" width="100" height="55" viewBox="0 0 100 55">
          <line class="pw" x1="50" y1="0" x2="50" y2="22"/>
          <path class="ps" d="M30 30 Q50 42 70 30 L65 22 Q50 16 35 22 Z"/>
        </svg>
        <div class="cone"></div>
        <div class="fglow"></div>
        <!-- Dining room SVG furniture -->
        <svg class="furn" viewBox="0 0 320 120" xmlns="http://www.w3.org/2000/svg">
          <rect x="0" y="108" width="320" height="12" fill="#051408" opacity=".8"/>
          <!-- dining table top -->
          <rect x="85" y="68" width="150" height="10" rx="4" fill="#7a5a3a"/>
          <!-- table leg left -->
          <rect x="90" y="78" width="6" height="30" rx="3" fill="#6a4a2a"/>
          <!-- table leg right -->
          <rect x="224" y="78" width="6" height="30" rx="3" fill="#6a4a2a"/>
          <!-- chair left (facing right) -->
          <rect x="44" y="72" width="30" height="26" rx="5" fill="#5a4020"/>
          <rect x="44" y="52" width="30" height="24" rx="5" fill="#6a5030"/>
          <rect x="46" y="98" width="6" height="10" rx="3" fill="#4a3010"/>
          <rect x="62" y="98" width="6" height="10" rx="3" fill="#4a3010"/>
          <!-- chair right (facing left) -->
          <rect x="246" y="72" width="30" height="26" rx="5" fill="#5a4020"/>
          <rect x="246" y="52" width="30" height="24" rx="5" fill="#6a5030"/>
          <rect x="248" y="98" width="6" height="10" rx="3" fill="#4a3010"/>
          <rect x="264" y="98" width="6" height="10" rx="3" fill="#4a3010"/>
          <!-- plate on table -->
          <ellipse cx="160" cy="73" rx="18" ry="5" fill="#e8e8e0" opacity=".7"/>
          <ellipse cx="160" cy="73" rx="12" ry="3.5" fill="#d0d0c8" opacity=".5"/>
          <!-- plant in corner -->
          <rect x="270" y="80" width="12" height="28" rx="4" fill="#4a3010"/>
          <ellipse cx="276" cy="74" rx="18" ry="20" fill="#1a6a2a"/>
          <ellipse cx="266" cy="68" rx="12" ry="14" fill="#228833"/>
          <ellipse cx="286" cy="70" rx="11" ry="13" fill="#1a7a2a"/>
          <!-- picture frame on wall -->
          <rect x="16" y="30" width="44" height="34" rx="4" fill="#2a2a30" stroke="#3a3a40" stroke-width="1"/>
          <rect x="19" y="33" width="38" height="28" rx="2" fill="#181820"/>
          <ellipse cx="38" cy="47" rx="12" ry="8" fill="#1a1a28" opacity=".8"/>
        </svg>
      </div>
      <div class="rinfo">
        <div class="rname">Dining Room</div>
        <div class="rstate" id="lbl-green">%GREEN_LBL%</div>
        <label class="tgl" onclick="event.stopPropagation()">
          <input type="checkbox" id="sw-green" %GREEN_CHK% onchange="ledToggle('green',this.checked)">
          <span class="sl"></span>
        </label>
      </div>
    </div>

  </div><!-- /rooms-grid -->
</div><!-- /right-col -->

<!-- ═══ FOOTER ═══ -->
<div class="footer-bar">
  <div class="fi">
    <div class="fttl">☀️ Active Lights Overview</div>
    <div class="frooms">

      <div class="fri r-red" id="fr-red">
        <div class="fri-circle">🛋️</div>
        <div class="fri-name">Living Room</div>
        <div class="fri-state" id="frs-red">OFF</div>
        <div class="fri-bar"><div class="fri-bf"></div></div>
      </div>

      <div class="fri r-blue" id="fr-blue">
        <div class="fri-circle">🛏️</div>
        <div class="fri-name">Bedroom</div>
        <div class="fri-state" id="frs-blue">OFF</div>
        <div class="fri-bar"><div class="fri-bf"></div></div>
      </div>

      <div class="fri r-yellow" id="fr-yellow">
        <div class="fri-circle">🍳</div>
        <div class="fri-name">Kitchen</div>
        <div class="fri-state" id="frs-yellow">OFF</div>
        <div class="fri-bar"><div class="fri-bf"></div></div>
      </div>

      <div class="fri r-green" id="fr-green">
        <div class="fri-circle">🍽️</div>
        <div class="fri-name">Dining Room</div>
        <div class="fri-state" id="frs-green">OFF</div>
        <div class="fri-bar"><div class="fri-bf"></div></div>
      </div>

    </div>
    <div class="badge"><span id="cnt">0</span> / 4 ON</div>
  </div>
</div>

</div><!-- /shell -->

<script>
/* ── Clock ─────────────────────────────────────────── */
(function tick(){
  var d=new Date(),z=function(n){return String(n).padStart(2,'0')};
  document.getElementById('clk').textContent=
    z(d.getHours())+':'+z(d.getMinutes())+':'+z(d.getSeconds());
  setTimeout(tick,1000);
})();

/* ── Count active rooms ─────────────────────────────── */
function updateCount(){
  var n=0;
  ['red','blue','yellow','green'].forEach(function(c){
    if(document.getElementById('card-'+c).classList.contains('on')) n++;
  });
  document.getElementById('cnt').textContent=n;
}

/* ── Apply UI for a room ────────────────────────────── */
function applyRoom(color,on){
  var card=document.getElementById('card-'+color);
  var lbl =document.getElementById('lbl-'+color);
  var fri =document.getElementById('fr-'+color);
  var frs =document.getElementById('frs-'+color);
  if(on){
    card.classList.add('on');
    lbl.textContent='LIGHT ON';
    fri.classList.add('on');
    frs.textContent='ON';
  }else{
    card.classList.remove('on');
    lbl.textContent='LIGHT OFF';
    fri.classList.remove('on');
    frs.textContent='OFF';
  }
  updateCount();
}

/* ── Card area click ────────────────────────────────── */
function cardClick(color){
  var sw=document.getElementById('sw-'+color);
  sw.checked=!sw.checked;
  ledToggle(color,sw.checked);
}

/* ── LED toggle → XHR to ESP32 ─────────────────────── */
function ledToggle(color,on){
  var x=new XMLHttpRequest();
  x.open('GET','/'+color+'/'+(on?'on':'off'),true);

  x.onload = function(){
    if(x.status === 204){
      applyRoom(color,on);
    }
  };

  x.onerror = function(){
    alert("ESP32 not responding");
  };

  x.send();
}

/* ── Humidifier toggle ──────────────────────────────── */
function humiToggle(on){
  var l=document.getElementById('humiLeft');
  var lbl=document.getElementById('humiLbl');
  l.className='humi-left '+(on?'humi-on':'humi-off');
  document.getElementById('mistZone').style.display=on?'':'none';
  lbl.textContent=on?'Auto Mode':'Standby';
  lbl.style.color=on?'':'var(--sub)';
  var x=new XMLHttpRequest();
  x.open('GET','/humi/'+(on?'on':'off'),true);
  x.timeout=5000;x.send();
}

/* ── Init from server-rendered state ───────────────── */
(function init(){
  ['red','blue','yellow','green'].forEach(function(c){
    if(document.getElementById('card-'+c).classList.contains('on')){
      document.getElementById('fr-'+c).classList.add('on');
      document.getElementById('frs-'+c).textContent='ON';
    }
  });
  // humidifier init
  var chk=document.getElementById('sw-humi').checked;
  var l=document.getElementById('humiLeft');
  l.className='humi-left '+(chk?'humi-on':'humi-off');
  if(!chk) document.getElementById('mistZone').style.display='none';
  updateCount();
})();
</script>
</body>
</html>
)rawhtml";

// ═══════════════════════════════════════════════════════════════════════════════
//  buildPage()
// ═══════════════════════════════════════════════════════════════════════════════
String buildPage(float temp, float hum) {
  // Temperature bar: 0–50 °C → 0–100 %
  int tPct = (int)constrain(map((long)temp, 0, 50, 0, 100), 0, 100);

  // ✅ Humidifier logic: AUTO ON when hum < 30
  bool humiOn = (hum < 30.0f);

  String page = String(HTML_PAGE);

  page.replace("%TEMP%",      String(temp, 1));
  page.replace("%HUM%",       String(hum,  0));
  page.replace("%TPCT%",      String(tPct));
  page.replace("%HUMILABEL%", humiOn ? "Auto Mode" : "Standby");
  page.replace("%HUMICHK%",   humiOn ? "checked"   : "");
  page.replace("%HUMICLASS%", humiOn ? "humi-on"   : "humi-off");

  // Helper: per-room substitutions
  auto roomSub = [&](const String& col, bool on) {
    String U = col; 
    U.toUpperCase();
    page.replace("%RED_CLS%",    redState    ? "on" : "");
page.replace("%RED_LBL%",    redState    ? "LIGHT ON" : "LIGHT OFF");
page.replace("%RED_CHK%",    redState    ? "checked" : "");

page.replace("%BLUE_CLS%",   blueState   ? "on" : "");
page.replace("%BLUE_LBL%",   blueState   ? "LIGHT ON" : "LIGHT OFF");
page.replace("%BLUE_CHK%",   blueState   ? "checked" : "");

page.replace("%YELLOW_CLS%", yellowState ? "on" : "");
page.replace("%YELLOW_LBL%", yellowState ? "LIGHT ON" : "LIGHT OFF");
page.replace("%YELLOW_CHK%", yellowState ? "checked" : "");

page.replace("%GREEN_CLS%",  greenState  ? "on" : "");
page.replace("%GREEN_LBL%",  greenState  ? "LIGHT ON" : "LIGHT OFF");
page.replace("%GREEN_CHK%",  greenState  ? "checked" : "");
  };

  roomSub("RED",    redState);
  roomSub("BLUE",   blueState);
  roomSub("YELLOW", yellowState);
  roomSub("GREEN",  greenState);

  return page;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);

  pinMode(PIN_RED,    OUTPUT);
  pinMode(PIN_BLUE,   OUTPUT);
  pinMode(PIN_YELLOW, OUTPUT);
  pinMode(PIN_GREEN,  OUTPUT);

  // LEDs off on boot
digitalWrite(PIN_RED,    redState    ? LOW : HIGH);
digitalWrite(PIN_BLUE,   blueState   ? LOW : HIGH);
digitalWrite(PIN_YELLOW, yellowState ? LOW : HIGH);
digitalWrite(PIN_GREEN,  greenState  ? LOW : HIGH);

  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nConnected!");
  Serial.print("Dashboard: http://");
  Serial.println(WiFi.localIP());

  server.begin();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  LOOP
// ═══════════════════════════════════════════════════════════════════════════════
void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  Serial.println("Client connected");
  String req = "";
  unsigned long t0 = millis();

  while (client.connected() && millis() - t0 < 2000) {
    if (client.available()) {
      char c = client.read();
      req += c;
      if (req.endsWith("\r\n\r\n")) break;
    }
  }

  String line = req.substring(0, req.indexOf('\n'));
  Serial.println("REQ: " + line);

  // ── LED Commands ─────────────────────────────────────────────────────────
  if (line.indexOf("/red/on")     != -1) { redState    = true;  digitalWrite(PIN_RED,    HIGH); }
  if (line.indexOf("/red/off")    != -1) { redState    = false; digitalWrite(PIN_RED,    LOW);  }
  if (line.indexOf("/blue/on")    != -1) { blueState   = true;  digitalWrite(PIN_BLUE,   HIGH); }
  if (line.indexOf("/blue/off")   != -1) { blueState   = false; digitalWrite(PIN_BLUE,   LOW);  }
  if (line.indexOf("/yellow/on")  != -1) { yellowState = true;  digitalWrite(PIN_YELLOW, HIGH); }
  if (line.indexOf("/yellow/off") != -1) { yellowState = false; digitalWrite(PIN_YELLOW, LOW);  }
  if (line.indexOf("/green/on")   != -1) { greenState  = true;  digitalWrite(PIN_GREEN,  HIGH); }
  if (line.indexOf("/green/off")  != -1) { greenState  = false; digitalWrite(PIN_GREEN,  LOW);  }
  // /humi/ endpoints: humidifier is auto-controlled by firmware; no GPIO needed unless relay added

  // ── Read DHT ─────────────────────────────────────────────────────────────
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();
  if (isnan(temp)) temp = 0.0f;
  if (isnan(hum))  hum  = 0.0f;

  // ── Response ─────────────────────────────────────────────────────────────
  bool isApiCall = (
    line.indexOf("/red")    != -1 ||
    line.indexOf("/blue")   != -1 ||
    line.indexOf("/yellow") != -1 ||
    line.indexOf("/green")  != -1 ||
    line.indexOf("/humi")   != -1
  );

  if (isApiCall && line.indexOf("GET / ") == -1) {
    // XHR toggle — no body needed
    client.println("HTTP/1.1 204 No Content");
    client.println("Connection: close");
    client.println();
  } else {
    // Full page
    String html = buildPage(temp, hum);
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
  Serial.println("Client disconnected\n");
}
