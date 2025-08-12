#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Shamirah's Galaxy";
const char* password = "password";

// PIR sensor and Buzzer
const int pirPin = 13;        // PIR sensor input pin
const int buzzerPin = 12;     // Buzzer pin

bool systemEnabled = true;
bool motionDetected = false;
String historyLogs = "";

unsigned long lastDetectionTime = 0;
const unsigned long cooldownTime = 10000; // 10 seconds cooldown
const unsigned long buzzDuration = 3000;  // Buzz for 3 seconds

WebServer server(80);

// dashboard HTML
String htmlPage() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <title>Crop Theft Detection</title>
    <style>
      body {
        background-color: #1e1e1e;
        color: #f1f1f1;
        font-family: Arial, sans-serif;
        margin: 0;
        padding: 0;
      }
      header {
        background-color: #2e7d32;
        padding: 20px;
        text-align: center;
      }
      header h1 {
        margin: 0;
        font-size: 28px;
      }
      .container {
        padding: 20px;
      }
      .status {
        margin-bottom: 20px;
        padding: 15px;
        border-radius: 8px;
        background-color: #333;
      }
      .enabled {
        color: #4caf50;
        font-weight: bold;
      }
      .disenabled {
        color: #f44336;
        font-weight: bold;
      }
      .motion-on {
        color: #f44336;
        font-weight: bold;
      }
      .motion-off {
        color: #4caf50;
        font-weight: bold;
      }
      button {
        padding: 12px 25px;
        font-size: 16px;
        border: none;
        border-radius: 5px;
        background-color: #2e7d32;
        color: white;
        cursor: pointer;
      }
      button:hover {
        background-color: #1b5e20;
      }
      .logs {
        background-color: #222;
        padding: 10px;
        border-radius: 5px;
        max-height: 300px;
        overflow-y: scroll;
      }
    </style>
  </head>
  <body>
    <header>
      <h1>Crop Theft Detection</h1>
    </header>
    <div class="container">
      <div class="status">
        <p>System Status: <span class="%SYSTEM_STATUS_CLASS%">%SYSTEM_STATUS%</span></p>
        <p>Motion Status: <span class="%MOTION_STATUS_CLASS%">%MOTION_STATUS%</span></p>
      </div>
      <form action="/toggle" method="POST">
        <button type="submit">%TOGGLE_BUTTON%</button>
      </form>
      <h3>Motion History:</h3>
      <div class="logs">%LOGS%</div>
    </div>
  </body>
  </html>
  )rawliteral";

  html.replace("%SYSTEM_STATUS%", systemEnabled ? "enabled" : "Disabled");
  html.replace("%SYSTEM_STATUS_CLASS%", systemEnabled ? "enabled" : "disabled");
  html.replace("%MOTION_STATUS%", motionDetected ? "Motion Detected" : "No Motion");
  html.replace("%MOTION_STATUS_CLASS%", motionDetected ? "motion-on" : "motion-off");
  html.replace("%TOGGLE_BUTTON%", systemEnabled ? "Disable" : "enable");
  html.replace("%LOGS%", historyLogs);
  return html;
}

// toggle endpoint
void handleToggle() {
  systemEnabled = !systemEnabled;
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// root endpoint
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void setup() {
  Serial.begin(115200);

  pinMode(pirPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected!");
  Serial.print("Ip Address ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/toggle", HTTP_POST, handleToggle);
  server.begin();
  Serial.println("Web server started");

  delay(30000); // Allow PIR to stabilize for 30s
}

void loop() {
  server.handleClient();

  if (!systemEnabled) {
    digitalWrite(buzzerPin, LOW);
    return;
  }

  unsigned long currentMillis = millis();
  int pirState = digitalRead(pirPin);

  if (pirState == HIGH && (currentMillis - lastDetectionTime > cooldownTime)) {
    motionDetected = true;
    digitalWrite(buzzerPin, HIGH);
    delay(buzzDuration);
    digitalWrite(buzzerPin, LOW);

    // Record detection time
    String time = String(currentMillis / 1000) + "s";
    historyLogs = "Motion Detected at: " + time + " sec<br>" + historyLogs;

    lastDetectionTime = currentMillis;
  } else if (pirState == LOW) {
    motionDetected = false;
  }
}