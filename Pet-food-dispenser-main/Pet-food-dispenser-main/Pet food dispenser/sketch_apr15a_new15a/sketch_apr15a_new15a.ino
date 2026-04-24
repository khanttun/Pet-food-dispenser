#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// --- CONFIGURATION ---
const char* ssid = "kent";
const char* password = "12345678";

Servo myServo;
WebServer server(80);
LiquidCrystal_I2C display(0x27, 16, 2);

const int servoPin = 18;
unsigned long lastFeedTime = 0;
int lastDuration = 0;
bool isFeeding = false;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Smart Pet Food Dispenser</title>
  <style>
* { box-sizing: border-box; }
html, body { margin: 0; min-height: 100%; }
body { background: #8c5a46; color: #fff7ef; font-family: Arial, Helvetica, sans-serif; }
.app-shell {
  position: relative; z-index: 2;
  display: grid;
  grid-template-columns: minmax(320px, 1fr) 320px;
  gap: 20px; min-height: 100%; padding: 24px; box-sizing: border-box;
  background: radial-gradient(circle at 10% 10%, rgba(255,226,185,0.2), transparent 35%),
              radial-gradient(circle at 90% 85%, rgba(255,163,117,0.15), transparent 30%);
}
.dashboard { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; align-content: start; }
.dashboard h1 { grid-column: 1 / -1; margin: 0; color: #fff7ef; }
.subtitle { grid-column: 1 / -1; margin: -4px 0 8px; color: #ffe7d6; }
.card {
  background: linear-gradient(165deg, rgba(66,42,29,0.78), rgba(42,23,15,0.72));
  border: 1px solid rgba(255,218,186,0.3); border-radius: 18px; padding: 20px;
  color: #fff8f2; backdrop-filter: blur(4px);
  box-shadow: 0 10px 28px rgba(0,0,0,0.3);
  transition: transform 0.25s ease, box-shadow 0.25s ease;
}
.card:hover { transform: translateY(-2px); box-shadow: 0 14px 32px rgba(0,0,0,0.35); }
.card h2 { margin-top: 0; }
.timer-value { font-size: 2.1rem; font-weight: bold; letter-spacing: 2px; color: #ffd9b8; }
.timer-chips { display: flex; gap: 8px; margin-top: 14px; flex-wrap: wrap; }
.chip { font-size: 0.8rem; padding: 5px 10px; border-radius: 999px; background: rgba(255,226,197,0.16); border: 1px solid rgba(255,226,197,0.25); color: #ffe8d4; }
#statusChip { background: rgba(183,255,214,0.16); border-color: rgba(183,255,214,0.34); }
.controls-card label { display: block; margin: 12px 0 8px; }
#feedBtn {
  border: none; border-radius: 10px;
  background: linear-gradient(135deg, #f8ae33, #ffcf74);
  color: #41250d; padding: 12px 14px; font-size: 1rem; font-weight: bold;
  cursor: pointer; display: inline-flex; align-items: center; gap: 8px;
  box-shadow: 0 5px 12px rgba(248,174,51,0.3);
  transition: transform 0.15s ease, box-shadow 0.15s ease;
}
#feedBtn:hover { box-shadow: 0 9px 18px rgba(248,174,51,0.4); transform: translateY(-1px); }
#feedBtn:active { transform: translateY(1px); }
#feedBtn:disabled { opacity: 0.5; cursor: not-allowed; transform: none; }
.btn-dot { width: 10px; height: 10px; border-radius: 50%; background: #5f3a00; }
#durationRange { width: 100%; accent-color: #f8ae33; }
.duration-text { margin-bottom: 0; font-weight: 600; color: #ffe1c8; }
.history-panel ul { list-style: none; margin: 0; padding: 0; max-height: 70vh; overflow-y: auto; }
.history-panel li { padding: 10px 0; border-bottom: 1px dashed rgba(255,255,255,0.2); color: #ffe8d4; }
.history-entry { animation: slideIn 0.35s ease; }
.background-corgi { position: fixed; inset: 0; z-index: 1; opacity: 0.45; pointer-events: none; }
.container { width: 100%; height: 100%; display: flex; justify-content: center; align-items: center; }

@keyframes slideIn { from { opacity: 0; transform: translateX(12px); } to { opacity: 1; transform: translateX(0); } }
@keyframes chipPulse { 0% { box-shadow: 0 0 0 rgba(183,255,214,0.5); } 50% { box-shadow: 0 0 0 8px rgba(183,255,214,0); } 100% { box-shadow: 0 0 0 rgba(183,255,214,0); } }
.pulse { animation: chipPulse 1.2s ease; }

@keyframes eye-blink { 0%,20%,100% { transform: scaleY(1); } 10% { transform: scaleY(0.1); } }
@keyframes tail-wag { 0%,20%,40%,60%,100% { transform: rotate(-25deg); } 10%,30%,50%,70% { transform: rotate(0deg); } }
@keyframes tongue-stick {
  0%,20% { transform: scaleY(0.1) translateY(-20px); }
  30% { transform: scaleY(0.5) translateY(0px); }
  40%,60%,80% { transform: scaleY(1) translateY(0px) rotate(0deg); }
  50%,90% { transform: scaleY(0.8) translateY(0px) rotate(15deg); }
  70% { transform: scaleY(0.8) translateY(0px) rotate(-15deg); }
  100% { transform: scaleY(0.1) translateY(-20px) rotate(0deg); }
}
@keyframes ear-shake-right { 0%,30%,100% { transform: rotate(70deg); } 10%,40% { transform: rotate(80deg); } }
@keyframes ear-shake-left  { 0%,30%,100% { transform: rotate(-70deg); } 10%,40% { transform: rotate(-80deg); } }
@keyframes body-shake { 0%,33%,66%,100% { transform: translateY(0px); } 16%,50%,83% { transform: translateY(2%); } }
@keyframes paw-press { 0%,33%,66%,100% { transform: scaleY(1) scaleX(1); } 16%,50%,83% { transform: scaleY(0.9) scaleX(1.05) translateY(10%); } }
@keyframes neck-shake { 0%,33%,66%,100% { top: 9%; } 16%,50%,83% { top: 11%; } }
@keyframes head-shake { 0%,33%,66%,100% { top: 6%; } 16%,50%,83% { top: 8%; } }
@keyframes mouth-shake { 0%,33%,66%,100% { bottom: 0%; } 16%,50%,83% { bottom: 2%; } }

.corgi { height: 60vmin; width: 84vmin; position: relative; }
.corgi div { position: absolute; }
.corgi .ear { background-color: #F09F2E; height: 30%; width: 55%; top: 5%; z-index: 3; }
.corgi .ear--r { right: 75%; border-bottom-left-radius: 100% 90%; border-top-left-radius: 10%; transform-origin: 80% center; animation: ear-shake-right 2s infinite; }
.corgi .ear--l { left: 63%; background-color: #D27537; border-bottom-right-radius: 100% 90%; border-top-right-radius: 10%; transform-origin: 20% center; animation: ear-shake-left 2s infinite; }
.corgi .head { top: 6%; right: 10%; height: 40%; width: 30%; z-index: 3; animation: head-shake 2s infinite; }
.corgi .face { background-color: #F09F2E; border-radius: 50%; overflow: hidden; height: 100%; width: 100%; z-index: 4; }
.corgi .eye { background-color: #3E3954; height: 6%; width: 6%; position: absolute; z-index: 6; border-radius: 50%; animation: eye-blink 2s infinite; }
.corgi .eye--left { left: 40%; top: 43%; }
.corgi .eye--right { right: 13%; top: 41%; }
.corgi .face__white { background-color: #FFFFFF; width: 45%; height: 77%; top: -15%; left: 29%; transform: rotate(-25deg); }
.corgi .face__orange { background-color: #F09F2E; content: " "; position: absolute; width: 110%; height: 110%; display: block; border-radius: 100%; }
.corgi .face__orange--l { right: 65%; }
.corgi .face__orange--r { left: 65%; }
.corgi .face__curve { background-color: #F09F2E; width: 30%; height: 20%; right: -12%; bottom: 42%; overflow: hidden; }
.corgi .face__curve:after { content: ""; background-color: #8C5A46; position: absolute; width: 69%; height: 82%; border-radius: 0% 100%; top: -32%; right: -13%; }
.corgi .mouth { bottom: 0%; width: 100%; height: 50%; left: 28%; z-index: 5; animation: mouth-shake 2s infinite; }
.corgi .nose { height: 36%; width: 27%; top: 0%; background-color: #3E3954; z-index: 1; right: 0%; border-bottom-right-radius: 50% 100%; border-bottom-left-radius: 50% 100%; }
.corgi .nose:after { content: ""; width: 100%; height: 30%; display: block; border-top-right-radius: 50% 100%; border-top-left-radius: 50% 100%; background-color: #3E3954; position: absolute; top: -25%; }
.corgi .mouth__left { background-color: #FFFFFF; width: 50%; height: 55%; }
.corgi .mouth__left--round { background-color: #F09F2E; width: 100%; height: 100%; border-radius: 100%; left: -50%; top: -50%; }
.corgi .mouth__left--sharp { background-color: #F09F2E; width: 35%; height: 50%; bottom: 0px; left: -20%; transform: skewX(50deg); }
.corgi .lowerjaw { background-color: #FFFFFF; width: 100%; height: 80%; border-radius: 50%/100%; border-top-left-radius: 0; border-top-right-radius: 0; bottom: -9%; }
.corgi .lips { z-index: 2; height: 25%; width: 35%; top: 19%; right: 2%; }
.corgi .lips:before, .corgi .lips:after { content: ""; display: block; background: #FFFFFF; width: 39%; height: 100%; border-color: #3E3954; border-width: 3px; border-style: solid; border-bottom-right-radius: 50%; border-bottom-left-radius: 50%; border-top-left-radius: 40%; border-top-right-radius: 20%; border-top-color: transparent; position: absolute; }
.corgi .lips:before { z-index: 1; }
.corgi .lips:after { transform: rotateY(180deg); left: initial; right: 9%; }
.corgi .tongue { width: 15%; height: 60%; background-color: #F15F55; right: 14%; top: 35%; border-bottom-right-radius: 50% 50%; border-bottom-left-radius: 50% 50%; transform-origin: 50% 0%; animation: tongue-stick 2s infinite; }
.corgi .snout { background-color: #FFFFFF; right: 0%; top: 0%; width: 50%; height: 36%; border-top-right-radius: 35% 75%; }
.corgi .neck__back { height: 50%; width: 20%; transform: skewX(-20deg); background-color: #F09F2E; z-index: 2; right: 24%; top: 9%; animation: neck-shake 2s infinite; }
.corgi .neck__front { height: 50%; width: 20%; right: 11%; top: 20%; background-color: #F09F2E; z-index: 2; transform: skewX(2deg); }
.corgi .body { height: 44%; width: 77%; background-color: #F09F2E; right: 10.5%; bottom: 12%; border-top-left-radius: 20% 50%; border-bottom-left-radius: 20% 50%; border-top-right-radius: 20% 60%; border-bottom-right-radius: 20% 40%; z-index: 2; overflow: hidden; animation: body-shake 2s infinite; }
.corgi .body__chest { background-color: #FFFFFF; height: 87%; width: 29%; right: 5%; bottom: -3%; border-top-left-radius: 50% 40%; border-top-right-radius: 50% 40%; }
.corgi .foot { height: 35%; width: 9.5%; bottom: 0; }
.corgi .foot__left { z-index: 3; background-color: #F09F2E; }
.corgi .foot__left:after { background-color: #FFFFFF; }
.corgi .foot__left:before { background-color: #F09F2E; }
.corgi .foot__right { z-index: 1; background-color: #D27537; }
.corgi .foot__right:after { background-color: #B6D8EF; }
.corgi .foot__right:before { background-color: #D27537; }
.corgi .foot__back:before { transform: skewX(-10deg); right: -25%; }
.corgi .foot__front:before { transform: skewX(10deg); right: 25%; }
.corgi .foot__1 { right: 37%; }
.corgi .foot__2 { right: 15%; }
.corgi .foot__2:before { transform: skewX(-10deg); right: -25%; }
.corgi .foot__3 { left: 12.65%; }
.corgi .foot__4 { left: 31%; }
.corgi .foot:before { content: ""; position: absolute; height: 100%; width: 100%; display: block; }
.corgi .foot:after { content: ""; position: absolute; bottom: 0; left: 0; width: 125%; height: 18%; border-top-left-radius: 50% 100%; border-top-right-radius: 50% 100%; animation: paw-press 2s infinite; }
.corgi .tail { width: 26%; height: 13%; background-color: #D27537; border-radius: 50% / 100%; bottom: 40%; left: 1%; transform-origin: 80% center; animation: tail-wag 2s infinite; }

@media (max-width: 900px) {
  body { overflow: auto; }
  .app-shell { grid-template-columns: 1fr; }
  .dashboard { grid-template-columns: 1fr; }
}
  </style>
</head>
<body>
  <div class="app-shell">
    <main class="dashboard">
      <h1>Smart Pet Food Dispenser</h1>
      <p class="subtitle">Monitor feeding activity and trigger feeding instantly.</p>

      <section class="card timer-card">
        <h2>Last Feeding</h2>
        <p id="lastFeedLabel">No feeding yet</p>
        <div class="timer-value" id="elapsedTime">00:00:00</div>
        <div class="timer-chips">
          <span class="chip">Auto-refresh: 1s</span>
          <span class="chip" id="statusChip">Status: Waiting</span>
        </div>
      </section>

      <section class="card controls-card">
        <h2>Feed Control</h2>
        <button id="feedBtn" type="button">
          <span class="btn-dot"></span>
          Feed Pet Now
        </button>
        <label for="durationRange">Dispenser open duration</label>
        <input id="durationRange" type="range" min="1" max="30" value="5">
        <p class="duration-text"><span id="durationValue">5</span> seconds</p>
      </section>
    </main>

    <aside class="card history-panel">
      <h2>Feeding History</h2>
      <ul id="historyList">
        <li>No feedings recorded yet.</li>
      </ul>
    </aside>
  </div>

  <div class="container background-corgi" aria-hidden="true">
    <div class="corgi">
      <div class="head">
        <div class="ear ear--r"></div>
        <div class="ear ear--l"></div>
        <div class="eye eye--left"></div>
        <div class="eye eye--right"></div>
        <div class="face">
          <div class="face__white">
            <div class="face__orange face__orange--l"></div>
            <div class="face__orange face__orange--r"></div>
          </div>
        </div>
        <div class="face__curve"></div>
        <div class="mouth">
          <div class="nose"></div>
          <div class="mouth__left">
            <div class="mouth__left--round"></div>
            <div class="mouth__left--sharp"></div>
          </div>
          <div class="lowerjaw">
            <div class="lips"></div>
            <div class="tongue"></div>
          </div>
          <div class="snout"></div>
        </div>
      </div>
      <div class="neck__back"></div>
      <div class="neck__front"></div>
      <div class="body">
        <div class="body__chest"></div>
      </div>
      <div class="foot foot__left foot__front foot__1"></div>
      <div class="foot foot__right foot__front foot__2"></div>
      <div class="foot foot__left foot__back foot__3"></div>
      <div class="foot foot__right foot__back foot__4"></div>
      <div class="tail"></div>
    </div>
  </div>

  <script>
    const feedBtn       = document.getElementById("feedBtn");
    const durationRange = document.getElementById("durationRange");
    const durationValue = document.getElementById("durationValue");
    const elapsedTime   = document.getElementById("elapsedTime");
    const lastFeedLabel = document.getElementById("lastFeedLabel");
    const historyList   = document.getElementById("historyList");
    const statusChip    = document.getElementById("statusChip");

    let lastFeedingTimestamp = null;

    function formatTime(date) {
      return date.toLocaleTimeString([], { hour: "2-digit", minute: "2-digit", second: "2-digit" });
    }

    function renderElapsed() {
      if (!lastFeedingTimestamp) { elapsedTime.textContent = "00:00:00"; return; }
      const total   = Math.floor((Date.now() - lastFeedingTimestamp) / 1000);
      const hours   = String(Math.floor(total / 3600)).padStart(2, "0");
      const minutes = String(Math.floor((total % 3600) / 60)).padStart(2, "0");
      const seconds = String(total % 60).padStart(2, "0");
      elapsedTime.textContent = `${hours}:${minutes}:${seconds}`;
    }

    function addHistoryEntry(text) {
      if (historyList.children.length === 1 && historyList.children[0].textContent.includes("No feedings")) {
        historyList.innerHTML = "";
      }
      const li = document.createElement("li");
      li.textContent = text;
      li.classList.add("history-entry");
      historyList.prepend(li);
    }

    durationRange.addEventListener("input", () => {
      durationValue.textContent = durationRange.value;
    });

    feedBtn.addEventListener("click", () => {
      const dur = durationRange.value;
      feedBtn.disabled = true;
      feedBtn.textContent = "Feeding...";

      fetch(`/trigger-feed?dur=${dur}`)
        .then(response => {
          if (response.ok) {
            const now = new Date();
            lastFeedingTimestamp = now.getTime();
            lastFeedLabel.textContent = `Last fed at ${formatTime(now)}`;
            addHistoryEntry(`Fed at ${formatTime(now)} for ${dur}s`);
            statusChip.textContent = `Status: Dispensing ${dur}s`;
            statusChip.classList.add("pulse");
            renderElapsed();

            setTimeout(() => {
              statusChip.textContent = "Status: Waiting";
              statusChip.classList.remove("pulse");
              feedBtn.disabled = false;
              feedBtn.innerHTML = '<span class="btn-dot"></span> Feed Pet Now';
            }, Number(dur) * 1000);

          } else {
            alert("ESP32 is busy! Please wait.");
            feedBtn.disabled = false;
            feedBtn.innerHTML = '<span class="btn-dot"></span> Feed Pet Now';
          }
        })
        .catch(err => {
          console.error("Error:", err);
          alert("Could not connect to PawFeeder.");
          feedBtn.disabled = false;
          feedBtn.innerHTML = '<span class="btn-dot"></span> Feed Pet Now';
        });
    });

    setInterval(renderElapsed, 1000);
  </script>
</body>
</html>
)rawliteral";

// ---- LCD HELPER ----
void updateOLED(String status, String info) {
  display.clear();
  display.setCursor(0, 0);
  display.print("PawFeeder");
  display.setCursor(0, 1);
  String line2 = status + " " + info;
  if (line2.length() > 16) line2 = line2.substring(0, 16);
  display.print(line2);
}

// ---- HTTP HANDLERS ----
void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleFeed() {
  if (isFeeding) {
    server.send(503, "text/plain", "Busy");
    return;
  }
  if (server.hasArg("dur")) {
    lastDuration = server.arg("dur").toInt();
    if (lastDuration < 1)  lastDuration = 1;
    if (lastDuration > 30) lastDuration = 30;

    isFeeding = true;
    myServo.attach(servoPin);
    delay(100);
    myServo.write(180);      // 180 = open position
    lastFeedTime = millis();
    updateOLED("FEEDING", String(lastDuration) + "s");
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing dur parameter");
  }
}

// ---- SETUP ----
void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  display.init();
  display.backlight();
  updateOLED("Starting", "WiFi...");

  myServo.attach(servoPin);
  myServo.write(0);         // 0 = closed/home position
  delay(500);
  myServo.detach();

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.println(WiFi.localIP());
  display.clear();
  display.setCursor(0, 0);
  display.print("IP Address:");
  display.setCursor(0, 1);
  display.print(WiFi.localIP().toString());

  server.on("/", handleRoot);
  server.on("/trigger-feed", handleFeed);
  server.begin();
}

// ---- MAIN LOOP ----
void loop() {
  server.handleClient();

  if (isFeeding && (millis() - lastFeedTime >= (unsigned long)lastDuration * 1000)) {
    myServo.write(0);       // 0 = return to closed/home position
    delay(500);
    myServo.detach();
    isFeeding = false;
    lastFeedTime = 0;
    updateOLED("Ready", "Waiting...");
  }
}