/**
 * ╔══════════════════════════════════════════════════════════════╗
 * ║     ADAPTIVE AI IRRIGATION  —  Production v11.0             ║
 * ║     LILYGO T-SIM7600SA-H · 4G Cellular · Weather · Cloud   ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║  v2.0 — Captive portal, weather, local adaptive learning    ║
 * ║  v3.0 — Supabase cloud logging, offline session queue       ║
 * ║  v5.0 — OTA updates, cloud AI zone recommendations          ║
 * ║  v7.0 — TLS, hardware watchdog, POSIX TZ, ADC averaging     ║
 * ║  v8.0 — 18 bug fixes (see below)                            ║
 * ║  v9.0 — QLD/NT timezone support, forward decl cleanup       ║
 * ║  v10.0 — Session queue NVS, manual trigger, normal webserver║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║  v11.0 Changes                                              ║
 * ║  · WiFi STA replaced by LILYGO T-SIM7600SA-H 4G cellular   ║
 * ║  · TinyGSM library for SIM7600 modem (Hologram APN)        ║
 * ║  · Normal-mode local web server removed entirely            ║
 * ║  · Manual triggers via Supabase farms.manual_trigger column ║
 * ║  · Auto-trigger re-enabled (was disabled for testing)       ║
 * ║  · Relay pins: IO14 (Zone 1), IO13 (Zone 2)                ║
 * ║  · IO34 conflict fixed — Sensor 1 moved to IO36 (SVP)      ║
 * ║  · Sensor 2 on IO35 (ADC1_CH7)                             ║
 * ║  · Modem signal strength (RSSI) added to heartbeat         ║
 * ║  · Heartbeat PATCH failure now logged to Serial Monitor     ║
 * ║  · WDT init compatible with Arduino ESP32 core 2.x & 3.x   ║
 * ║  · Global TinyGsmClientSecure replaces per-call allocation  ║
 * ║  · initModem() runs on boot before any Supabase calls       ║
 * ║  · NTP synced via modem network time (NITZ/AT+CCLK)        ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║  v8.0 Bug Fixes (preserved)                                 ║
 * ║  · registerFarm: "now()" → ISO 8601 timestamp               ║
 * ║  · fetchCloudRecommendations: double http.end() fixed        ║
 * ║  · fetchCloudRecommendations + fetchWeather: Static JSON     ║
 * ║  · fetchWeather: ArduinoJson filter 96 B → 256 B            ║
 * ║  · loadConfig: strncpy → strlcpy (null-term fix)            ║
 * ║  · Zone::startedAt: unsigned long → time_t                  ║
 * ║  · lastWatered == 0 treated as never-watered                ║
 * ║  · applyLearning: span == 0 guard                           ║
 * ║  · OTA version compare: atof() → X.Y integer compare        ║
 * ║  · HTTP OTA: WDT suspended during download + flash          ║
 * ║  · sendSessionToCloud: heap String removed, x-api-key gone  ║
 * ║  · serializeJson return checked for truncation              ║
 * ║  · registerFarm: x-api-key header removed                   ║
 * ║  · handleSave: tz validated against known offsets           ║
 * ║  · lastOTACheck set after setup checkHTTPOTA()              ║
 * ║  · syncNTP: 10th-try success no longer falsely reported     ║
 * ╠══════════════════════════════════════════════════════════════╣
 * ║  v10.0 Security & Reliability Fixes (preserved)             ║
 * ║  · isInWateringWindow: NTP grace removed — no time = no    ║
 * ║    water (prevents 2am watering on NTP failure)             ║
 * ║  · cfg_sb_key: 256 → 320 bytes (prevents JWT truncation)   ║
 * ║  · OTA firmware: SHA256 hash verified before flash commit   ║
 * ║  · OTA streaming: chunked with per-chunk WDT reset          ║
 * ║  · Session queue: circular buffer with NVS head pointer     ║
 * ║  · postSession: non-blocking — queues on send failure       ║
 * ║  · applyLearning: saveZone only when target changes         ║
 * ╚══════════════════════════════════════════════════════════════╝
 *
 * Required libraries (Arduino Library Manager):
 *   ArduinoJson  by Benoit Blanchon  (v6.x)
 *   TinyGSM      by Volodymyr Shymanskyy
 *
 * ── BOARD ────────────────────────────────────────────────────────
 *   LILYGO T-SIM7600SA-H (ESP32 + SIM7600 4G modem)
 *   Select "ESP32 Dev Module" in Arduino IDE board manager.
 *
 * ── MODEM WIRING (onboard PCB traces — no external wiring) ──────
 *   MODEM_TX=27  MODEM_RX=26  MODEM_PWRKEY=4
 *   MODEM_FLIGHT=25  MODEM_STATUS=34
 *
 * ── PIN NOTE ─────────────────────────────────────────────────────
 *   SOIL_PINS[0] = IO36 (SVP — ADC1_CH0, input-only, no modem connection)
 *   SOIL_PINS[1] = IO35 (ADC1_CH7, input-only, no modem connection)
 *   MODEM_STATUS = IO34 (PCB trace to SIM7600 only — not used after initModem())
 *   IO34 is NOT connected to any sensor in this build.
 *
 * ── APN ──────────────────────────────────────────────────────────
 *   Hologram APN: "hologram" (no username/password required)
 *
 * ── REQUIRED SUPABASE MIGRATIONS ─────────────────────────────────
 *   Run this SQL in your Supabase SQL editor before first deployment:
 *
 *     ALTER TABLE farms ADD COLUMN IF NOT EXISTS manual_trigger integer DEFAULT 0;
 *     ALTER TABLE farms ADD COLUMN IF NOT EXISTS signal_rssi integer;
 *
 *   manual_trigger: dashboard writes 1 or 2 to fire Zone 1 or 2.
 *                   Firmware clears it back to 0 after actioning.
 *   signal_rssi:    modem signal quality (0–31) written each heartbeat.
 *
 * ── OTA HASH VERIFICATION ────────────────────────────────────────
 *   version.txt must contain two lines:
 *     Line 1: version number, e.g.  11.0
 *     Line 2: SHA256 of firmware.bin, e.g.  SHA256:abcdef01234...
 *   Generate with:  sha256sum firmware.bin
 *   If line 2 is absent the hash check is skipped (backward compat).
 *
 * ── SUPABASE KEY ─────────────────────────────────────────────────
 *   Enter your JWT anon key (eyJhbGci...) via the captive portal.
 *   Do NOT hardcode it here if this file is in a git repo.
 *
 * ── CAPTIVE PORTAL ───────────────────────────────────────────────
 *   On first boot (or after factory reset) the device creates a WiFi
 *   hotspot "FarmIrrigation-Setup" for initial configuration.
 *   Connect and open any webpage to configure Supabase, location, and
 *   weather API key.  WiFi credentials are not required — all data
 *   travels over 4G.
 *
 * ── TLS NOTE ─────────────────────────────────────────────────────
 *   All HTTPS calls use TinyGsmClientSecure (SIM7600 TLS stack).
 *   OTA firmware integrity is protected by SHA256 hash verification.
 *
 * ── 24VAC VALVE WIRING ──────────────────────────────────────────
 *   Power chain: 24VAC plug pack → relay COM/NO → valve → return.
 *   Device powered via onboard USB-C (Raspberry Pi 5.1V 3A adapter).
 */

// ================================================================
//  TINYGSM — must be defined before any TinyGSM include
// ================================================================
#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_RX_BUFFER 1024
#include <TinyGsmClient.h>

#include <WiFi.h>                  // Kept for captive-portal AP mode only
#include "esp_wifi.h"              // esp_read_mac for farm-ID derivation
#include <HTTPClient.h>
#include <WebServer.h>             // Kept for captive-portal setup page
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <time.h>
#include <Update.h>
#include <esp_task_wdt.h>
#include <esp_arduino_version.h>   // Version-safe WDT init (Bug 4)
#include <mbedtls/sha256.h>
#include "esp_mac.h"

// ================================================================
//  HARDWARE CONFIG
// ================================================================

#define NUM_ZONES 2
#define FIRMWARE_VERSION "11.0"

// QUEUE_SIZE must be >= NUM_ZONES (one pending session per zone minimum).
// NVS key scheme assumes at most 8 slots ("sq0"–"sq7").
#define QUEUE_SIZE 8
static_assert(NUM_ZONES <= 8, "QUEUE_SIZE assumes max 8 zones");

// ================================================================
//  MODEM PINS  (LILYGO T-SIM7600SA-H onboard PCB connections)
// ================================================================
#define MODEM_TX     27
#define MODEM_RX     26
#define MODEM_PWRKEY  4
#define MODEM_FLIGHT 25
#define MODEM_STATUS 34   // PCB trace to SIM7600 only — NOT a sensor pin

// ================================================================
//  CREDENTIALS
//  Leave Supabase fields blank — enter via captive portal instead.
//  Hardcoding keys here is a security risk if this file is in a repo.
// ================================================================
const char TEST_SB_URL[] = "https://fahgxylpdvxnqlrggasj.supabase.co";
const char TEST_SB_KEY[] = "";   // Enter via captive portal
const char TEST_APIKEY[] = "c0f41e6c980ea0b421573c0e72092894";

// 4G APN
const char APN[] = "hologram";

// IO36 (SVP/ADC1_CH0) — input-only, no modem connection — safe for ADC
// IO35 (ADC1_CH7)     — input-only, no modem connection — safe for ADC
const uint8_t SOIL_PINS[NUM_ZONES]  = { 36, 35 };   // Bug 1 fix: was { 34, 35 }
const uint8_t RELAY_PINS[NUM_ZONES] = { 14, 13 };   // IN1=IO14 (Zone 1), IN2=IO13 (Zone 2)
const uint8_t RESET_PIN             = 0;
const uint8_t STATUS_LED            = 2;

// ================================================================
//  TUNING
// ================================================================

const int   DRY_THRESHOLD       = 2000;
const float TARGET_MIN          = 0.5f;
const float TARGET_MAX          = 10.0f;
const float TARGET_DEFAULT      = 2.0f;
const float LEARNING_RATE       = 0.12f;
const float RAIN_SKIP_MM        = 4.0f;
const int   ADC_SAMPLES         = 16;

// Production values — ready for real deployment
const int   WATER_HOUR_START    = 0;
const int   WATER_HOUR_END      = 23;
const unsigned long ZONE_COOLDOWN_MS    = 2UL * 60UL * 1000UL;
const unsigned long WEATHER_INTERVAL_MS = 30UL * 60UL * 1000UL;
const unsigned long NTP_INTERVAL_MS     = 6UL * 3600UL * 1000UL;
const unsigned long WDT_TIMEOUT_S       = 60;

// ================================================================
//  CAPTIVE PORTAL
//  AP password derived at runtime from MAC — unique per device
// ================================================================

const char* AP_SSID  = "FarmIrrigation-Setup";
char        AP_PASS[16] = "";
const byte  DNS_PORT = 53;

// ================================================================
//  OTA CONFIG
// ================================================================
const char OTA_VERSION_URL[]  = "https://raw.githubusercontent.com/dannyj17/Irrigation-firmware/main/version.txt";
const char OTA_FIRMWARE_URL[] = "https://raw.githubusercontent.com/dannyj17/Irrigation-firmware/main/firmware.bin";
const unsigned long OTA_CHECK_INTERVAL_MS = 6UL * 3600UL * 1000UL;

// ================================================================
//  RUNTIME CONFIG  (saved to flash via captive portal)
//  v11: WiFi SSID/password removed — device uses 4G for all data
// ================================================================

char  cfg_apikey[48]    = "";
char  cfg_sb_url[128]   = "";
char  cfg_sb_key[320]   = "";   // 320 bytes: safe ceiling for Supabase JWTs
float cfg_lat           = -33.8688f;
float cfg_lon           = 151.2093f;
long  cfg_tz            = 36000;
bool  cfg_ready         = false;

char  farm_id[24]       = "";
char  bearerBuf[344]    = "";   // "Bearer " (7) + key (320) + null + headroom

// ================================================================
//  ZONE STATE
// ================================================================

struct Zone {
  float         target;
  float         waterGiven;
  bool          isRunning;
  unsigned long lastWatered;
  int           preMoisture;
  time_t        startedAt;
};

Zone zones[NUM_ZONES];
int  activeZone = -1;

int lastKnownMoisture[NUM_ZONES] = {0, 0};

// ================================================================
//  GLOBAL STATE
// ================================================================

Preferences  prefs;
WebServer    server(80);   // Captive-portal only
DNSServer    dnsServer;

// TinyGSM modem — Serial1 used for AT commands
TinyGsm modem(Serial1);

// Single global TinyGsmClientSecure — avoids per-call heap allocation (Bug 6)
// All HTTP functions share this client sequentially; http.end() is always
// called before the next http.begin() so there is no concurrent-use risk.
TinyGsmClientSecure gsmClient(modem);

float         forecastRainMM  = 0.0f;
float         forecastTempC   = 20.0f;
bool          weatherReady    = false;
unsigned long lastHeartbeat   = 0;
bool          timeReady       = false;
bool          portalActive    = false;
bool          farmRegistered  = false;

unsigned long lastWeatherFetch = 0;
unsigned long lastNTPSync      = 0;
unsigned long lastStatusPrint  = 0;
unsigned long lastWaterTick    = 0;
unsigned long lastOTACheck     = 0;

struct PendingSession {
  bool   used;
  int    zone;
  int    preMoisture;
  int    postMoisture;
  float  waterGiven;
  float  targetBefore;
  float  targetAfter;
  float  tempC;
  float  rainMM;
  char   learnResult[8];
  time_t startedAt;
  time_t endedAt;
};

PendingSession sessionQueue[QUEUE_SIZE];

// Circular queue pointers persisted to NVS as "sqh" and "sqc".
int queueHead  = 0;
int queueCount = 0;

// ================================================================
//  SETUP PAGE HTML  (captive portal — shown when not configured)
//  v11: WiFi SSID/password fields removed; device uses 4G only.
//  v9:  Added QLD (AEST, no DST) and ACST/NT options.
// ================================================================

const char SETUP_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Farm Irrigation Setup</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { font-family: -apple-system, sans-serif; background: #f0f4f0; min-height: 100vh; padding: 20px; }
  .card { background: white; border-radius: 16px; padding: 28px 24px; max-width: 420px; margin: 0 auto; box-shadow: 0 4px 24px rgba(0,0,0,0.10); }
  .logo { text-align: center; margin-bottom: 24px; }
  .logo-icon { font-size: 48px; }
  h1 { font-size: 22px; font-weight: 700; color: #1a2e1a; text-align: center; margin-bottom: 4px; }
  .sub { color: #6b7c6b; font-size: 14px; text-align: center; margin-bottom: 28px; }
  .section { font-size: 11px; font-weight: 600; color: #4a7c4a; text-transform: uppercase; letter-spacing: 0.08em; margin: 20px 0 10px; border-top: 1px solid #eef2ee; padding-top: 16px; }
  label { display: block; font-size: 14px; color: #2d3d2d; margin-bottom: 6px; font-weight: 500; }
  input, select { width: 100%; padding: 12px 14px; border: 1.5px solid #d0ddd0; border-radius: 10px; font-size: 15px; color: #1a2e1a; background: #f8faf8; outline: none; transition: border-color 0.2s; margin-bottom: 14px; }
  input:focus, select:focus { border-color: #4a7c4a; background: white; }
  .hint { font-size: 12px; color: #8a9e8a; margin-top: -10px; margin-bottom: 14px; line-height: 1.4; }
  button { width: 100%; padding: 15px; background: #2d6a2d; color: white; border: none; border-radius: 12px; font-size: 16px; font-weight: 600; cursor: pointer; margin-top: 8px; }
  button:active { background: #1a4a1a; }
  .opt { color: #8a9e8a; font-weight: 400; font-size: 12px; }
  .success { display: none; text-align: center; padding: 20px; }
  .success-icon { font-size: 56px; margin-bottom: 12px; }
  .success h2 { color: #2d6a2d; font-size: 20px; margin-bottom: 8px; }
  .success p { color: #6b7c6b; font-size: 14px; line-height: 1.6; }
  .info-box { background:#e8f4e8; border-radius:10px; padding:12px 14px; font-size:13px; color:#2d4a2d; margin-bottom:18px; }
</style>
</head>
<body>
<div class="card">
  <div id="form-view">
    <div class="logo"><div class="logo-icon">&#128167;</div></div>
    <h1>Irrigation Setup</h1>
    <p class="sub">Configure your 4G irrigation system</p>
    <div class="info-box">&#128246; This device connects via 4G cellular (Hologram). No WiFi credentials needed.</div>

    <div class="section" style="border-top:none;padding-top:0">Location</div>
    <label>Timezone</label>
    <select id="tz">
      <option value="36000">AEST/DST &mdash; NSW, VIC, TAS, ACT (UTC+10, DST aware)</option>
      <option value="36001">AEST &mdash; QLD (UTC+10, no DST)</option>
      <option value="34200">ACST/DST &mdash; SA (UTC+9:30, DST aware)</option>
      <option value="34201">ACST &mdash; NT (UTC+9:30, no DST)</option>
      <option value="28800">AWST &mdash; WA (UTC+8, no DST)</option>
      <option value="0">UTC</option>
    </select>
    <label>Latitude <span class="opt">(optional)</span></label>
    <input type="number" id="lat" placeholder="-33.8688" step="0.0001">
    <label>Longitude <span class="opt">(optional)</span></label>
    <input type="number" id="lon" placeholder="151.2093" step="0.0001">

    <div class="section">Weather API <span class="opt">(optional &mdash; free at openweathermap.org)</span></div>
    <label>OpenWeatherMap Key</label>
    <input type="text" id="apikey" placeholder="Leave blank to skip" autocomplete="off" autocorrect="off" autocapitalize="none">

    <div class="section">Cloud Logging <span class="opt">(optional &mdash; free at supabase.com)</span></div>
    <p class="hint" style="margin-bottom:12px">Stores watering history so your system learns even if the device is replaced.</p>
    <label>Supabase Project URL</label>
    <input type="text" id="sb_url" placeholder="https://xxxx.supabase.co" autocomplete="off" autocorrect="off" autocapitalize="none">
    <label>Supabase Anon Key</label>
    <input type="text" id="sb_key" placeholder="eyJh..." autocomplete="off" autocorrect="off" autocapitalize="none">
    <p class="hint">Find these in your Supabase project &rarr; Settings &rarr; API</p>

    <button onclick="save()">Save &amp; Start</button>
  </div>

  <div class="success" id="success-view">
    <div class="success-icon">&#9989;</div>
    <h2>All done!</h2>
    <p>Your irrigation system is starting up over 4G.<br><br>This page will close in a moment. The system will begin monitoring your soil within 60 seconds.</p>
  </div>
</div>

<script>
function save() {
  var tz     = document.getElementById('tz').value;
  var lat    = document.getElementById('lat').value   || '-33.8688';
  var lon    = document.getElementById('lon').value   || '151.2093';
  var apikey = document.getElementById('apikey').value.trim();
  var sb_url = document.getElementById('sb_url').value.trim();
  var sb_key = document.getElementById('sb_key').value.trim();

  var btn = document.querySelector('button');
  btn.textContent = 'Saving...';
  btn.disabled = true;

  fetch('/save', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: 'tz='    + encodeURIComponent(tz) +
          '&lat='   + encodeURIComponent(lat) +
          '&lon='   + encodeURIComponent(lon) +
          '&key='   + encodeURIComponent(apikey) +
          '&sb_url='+ encodeURIComponent(sb_url) +
          '&sb_key='+ encodeURIComponent(sb_key)
  })
  .then(r => r.text())
  .then(() => {
    document.getElementById('form-view').style.display = 'none';
    document.getElementById('success-view').style.display = 'block';
  })
  .catch(() => { btn.textContent = 'Save & Start'; btn.disabled = false; });
}
</script>
</body>
</html>
)rawhtml";

// ================================================================
//  FORWARD DECLARATIONS
// ================================================================
void loadConfig();
void loadZones();
void saveZone(int idx);
void startCaptivePortal();
void handleRoot();
void handleSave();
void initModem();
void syncNTP();
void fetchWeather();
void registerFarm();
void sendHeartbeat();
void checkSupabaseSchema();
void fetchCloudRecommendations();
void pollManualTrigger();
void flushSessionQueue();
void flushOneSession();
void runIrrigation();
void startZone(int idx);
void stopZone(int idx);
const char* applyLearning(int idx, int post);
void postSession(int zone, int preMoist, int postMoist, float waterGiven, float targetBefore, float targetAfter, float tempC, float rainMM, const char* learnResult);
bool sendSessionToCloud(int zone, int preMoist, int postMoist, float waterGiven, float targetBefore, float targetAfter, float tempC, float rainMM, const char* learnResult, time_t startTs, time_t endTs);
void queueSession(int zone, int preMoist, int postMoist, float waterGiven, float targetBefore, float targetAfter, float tempC, float rainMM, const char* learnResult, time_t startTs, time_t endTs);
void loadSessionQueueFromNVS();
void saveQueueSlotNVS(int i);
void clearQueueSlotNVS(int i);
int  readMoisture(int idx);
bool isInWateringWindow();
void printStatus();
void checkHTTPOTA();
const char* getPosixTZ(long tzOffset);
bool isNewerVersion(const char* remoteVer, const char* localVer);

// ================================================================
//  SETUP
// ================================================================

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println(F("\n=== Adaptive AI Irrigation  v11.0  [LILYGO T-SIM7600SA-H] ===\n"));

  // Derive farm ID from WiFi MAC (reads eFuse — no WiFi stack needed)
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  snprintf(farm_id, sizeof(farm_id), "%02x:%02x:%02x:%02x:%02x:%02x",
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print(F("[Device] Farm ID (MAC): ")); Serial.println(farm_id);

  // Derive AP password from last 4 MAC hex chars (unique per device)
  String apPassStr = String(farm_id).substring(String(farm_id).length() - 5);
  apPassStr.replace(":", "");
  apPassStr = "irrig-" + apPassStr;
  apPassStr.toCharArray(AP_PASS, sizeof(AP_PASS));
  Serial.print(F("[Device] Portal/PIN password: ")); Serial.println(AP_PASS);

  pinMode(RESET_PIN, INPUT_PULLUP);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);

  for (int i = 0; i < NUM_ZONES; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], HIGH);   // Relays off (active LOW)
    pinMode(SOIL_PINS[i], INPUT);
  }

  memset(sessionQueue, 0, sizeof(sessionQueue));
  prefs.begin("irrig3", false);
  loadSessionQueueFromNVS();

  if (digitalRead(RESET_PIN) == LOW) {
    Serial.println(F("[Config] BOOT held — hold 3s to factory reset..."));
    // WDT not yet armed — delay here is safe
    delay(3000);
    if (digitalRead(RESET_PIN) == LOW) {
      prefs.clear();
      Serial.println(F("[Config] Wiped. Restarting..."));
      delay(500);
      ESP.restart();
    }
  }

  loadConfig();
  loadZones();

  if (!cfg_ready) {
    startCaptivePortal();
  } else {
    // initModem() called before WDT is armed — blocking waits inside are safe.
    initModem();
    syncNTP();
    fetchWeather();
    registerFarm();
    checkSupabaseSchema();
    fetchCloudRecommendations();
    // Blocking boot flush — called before WDT is armed, so safe.
    flushSessionQueue();
    checkHTTPOTA();
    lastOTACheck = millis();
  }

  // ── WDT init — compatible with Arduino ESP32 core 2.x and 3.x (Bug 4) ─
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms     = WDT_TIMEOUT_S * 1000,
    .idle_core_mask = (1 << 0),
    .trigger_panic  = true
  };
  esp_task_wdt_reconfigure(&wdt_config);
  esp_task_wdt_add(NULL);
#else
  esp_task_wdt_init(WDT_TIMEOUT_S, true);
  esp_task_wdt_add(NULL);
#endif

  // Prime lastKnownMoisture so printStatus() shows real values immediately
  for (int i = 0; i < NUM_ZONES; i++) {
    readMoisture(i);
  }

  Serial.println(F("\n[System] Ready.\n"));
}

// ================================================================
//  MAIN LOOP
// ================================================================

void loop() {
  // Captive-portal mode: serve setup page, blink LED
  if (portalActive) {
    dnsServer.processNextRequest();
    server.handleClient();
    static unsigned long lb = 0;
    if (millis() - lb > 300) { digitalWrite(STATUS_LED, !digitalRead(STATUS_LED)); lb = millis(); }
    esp_task_wdt_reset();
    return;
  }

  unsigned long now = millis();

  // ── 4G connectivity watchdog ────────────────────────────────────
  static unsigned long lastModemCheck = 0;
  if (now - lastModemCheck > 30000UL) {
    lastModemCheck = now;
    if (!modem.isGprsConnected()) {
      Serial.println(F("[Modem] GPRS lost — reconnecting..."));
      if (modem.gprsConnect(APN, "", "")) {
        Serial.println(F("[Modem] Reconnected"));
      } else {
        Serial.println(F("[Modem] Reconnect failed — running offline"));
      }
    }
  }

  if (now - lastHeartbeat > 60000UL)               { lastHeartbeat = now; sendHeartbeat(); }
  if (now - lastWeatherFetch > WEATHER_INTERVAL_MS)   fetchWeather();
  if (now - lastNTPSync      > NTP_INTERVAL_MS)       syncNTP();
  if (now - lastStatusPrint  > 3000)               { printStatus(); lastStatusPrint = now; }

  if (now - lastOTACheck > OTA_CHECK_INTERVAL_MS) {
    checkHTTPOTA();
    lastOTACheck = now;
  }

  // Flush one queued session per 5 s — non-blocking, safe under WDT
  static unsigned long lastFlushTick = 0;
  if (now - lastFlushTick > 5000UL) {
    lastFlushTick = now;
    flushOneSession();
  }

  // Poll Supabase for dashboard manual triggers every 5 s (Bug 2)
  static unsigned long lastTriggerPoll = 0;
  if (now - lastTriggerPoll > 60000UL) {
    lastTriggerPoll = now;
    pollManualTrigger();
  }

  runIrrigation();
  esp_task_wdt_reset();
  digitalWrite(STATUS_LED, HIGH);
}

// ================================================================
//  MODEM INIT  (LILYGO T-SIM7600SA-H)
//  Called from setup() before WDT is armed — blocking waits are safe.
// ================================================================

void initModem() {
  Serial.println(F("[Modem] Initialising SIM7600..."));

  pinMode(MODEM_PWRKEY,  OUTPUT);
  pinMode(MODEM_FLIGHT,  OUTPUT);
  pinMode(MODEM_STATUS,  INPUT);

  // Disengage flight mode
  digitalWrite(MODEM_FLIGHT, LOW);

  // Start UART to modem
  Serial1.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(100);

  // Power on modem if status pin indicates it is off
  if (digitalRead(MODEM_STATUS) == LOW) {
    Serial.println(F("[Modem] Powering on..."));
    digitalWrite(MODEM_PWRKEY, HIGH);
    delay(500);
    digitalWrite(MODEM_PWRKEY, LOW);
    delay(5000);   // Allow SIM7600 boot time — WDT not armed, safe
  }

  Serial.print(F("[Modem] Sending AT..."));
  modem.restart();
  Serial.println(F(" OK"));

  String info = modem.getModemInfo();
  Serial.print(F("[Modem] Info: ")); Serial.println(info);

  Serial.print(F("[Modem] Waiting for network..."));
  if (!modem.waitForNetwork(60000L)) {
    Serial.println(F(" FAILED — no carrier"));
    return;
  }
  Serial.println(F(" OK"));

  Serial.print(F("[Modem] Connecting APN: ")); Serial.print(APN); Serial.print(F("..."));
  if (!modem.gprsConnect(APN, "", "")) {
    Serial.println(F(" FAILED"));
    return;
  }
  Serial.println(F(" OK"));

  Serial.print(F("[Modem] IP:     ")); Serial.println(modem.localIP());
  Serial.print(F("[Modem] Signal: ")); Serial.println(modem.getSignalQuality());
}

// ================================================================
//  CAPTIVE PORTAL  (WiFi AP — for initial device configuration)
// ================================================================

void startCaptivePortal() {
  portalActive = true;
  Serial.println(F("[Portal] Starting setup hotspot"));
  Serial.print(F("[Portal] SSID: "));     Serial.println(AP_SSID);
  Serial.print(F("[Portal] Password: ")); Serial.println(AP_PASS);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  delay(500);

  Serial.print(F("[Portal] IP: ")); Serial.println(WiFi.softAPIP());

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  server.on("/",     HTTP_GET,  handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.onNotFound(handleRoot);
  server.begin();
  Serial.println(F("[Portal] Web server started"));
}

void handleRoot() {
  server.send_P(200, "text/html", SETUP_HTML);
}

void handleSave() {
  server.arg("key").toCharArray(cfg_apikey,    sizeof(cfg_apikey));
  server.arg("sb_url").toCharArray(cfg_sb_url, sizeof(cfg_sb_url));
  server.arg("sb_key").toCharArray(cfg_sb_key, sizeof(cfg_sb_key));

  float parsedLat = server.arg("lat").toFloat();
  float parsedLon = server.arg("lon").toFloat();
  cfg_lat = (parsedLat >= -90.0f  && parsedLat <= 90.0f)  ? parsedLat : -33.8688f;
  cfg_lon = (parsedLon >= -180.0f && parsedLon <= 180.0f) ? parsedLon : 151.2093f;

  // Validate timezone against all known portal values (including QLD/NT entries)
  long parsedTz = server.arg("tz").toInt();
  const long validTz[] = { 36000, 36001, 34200, 34201, 28800, 0 };
  cfg_tz = 36000;
  for (int ti = 0; ti < 6; ti++) {
    if (parsedTz == validTz[ti]) { cfg_tz = parsedTz; break; }
  }

  prefs.putString("apikey", cfg_apikey);
  prefs.putString("sb_url", cfg_sb_url);
  prefs.putString("sb_key", cfg_sb_key);
  prefs.putFloat("lat",     cfg_lat);
  prefs.putFloat("lon",     cfg_lon);
  prefs.putLong("tz",       cfg_tz);
  prefs.putBool("configured", true);

  server.send(200, "text/plain", "OK");
  Serial.println(F("[Portal] Config saved — rebooting..."));
  delay(1500);
  ESP.restart();
}

// ================================================================
//  CONFIG LOAD
// ================================================================

void loadConfig() {
  cfg_ready = prefs.getBool("configured", false);
  if (!cfg_ready) { Serial.println(F("[Config] No saved config")); return; }

  prefs.getString("apikey", cfg_apikey,  sizeof(cfg_apikey));
  prefs.getString("sb_url", cfg_sb_url,  sizeof(cfg_sb_url));
  prefs.getString("sb_key", cfg_sb_key,  sizeof(cfg_sb_key));
  cfg_lat = prefs.getFloat("lat", -33.8688f);
  cfg_lon = prefs.getFloat("lon", 151.2093f);
  cfg_tz  = prefs.getLong("tz",   36000);

  // Bounds-check loaded values. Flash corruption or a missing key can
  // return 0 from getFloat()/getLong(), silently breaking NTP and weather.
  if (cfg_lat < -90.0f || cfg_lat > 90.0f) {
    cfg_lat = -33.8688f;
    Serial.println(F("[Config] WARNING: cfg_lat out of bounds — reset to default"));
  }
  if (cfg_lon < -180.0f || cfg_lon > 180.0f) {
    cfg_lon = 151.2093f;
    Serial.println(F("[Config] WARNING: cfg_lon out of bounds — reset to default"));
  }
  const long validTz[] = { 36000, 36001, 34200, 34201, 28800, 0 };
  bool tzOk = false;
  for (int ti = 0; ti < 6; ti++) {
    if (cfg_tz == validTz[ti]) { tzOk = true; break; }
  }
  if (!tzOk) {
    cfg_tz = 36000;
    Serial.println(F("[Config] WARNING: cfg_tz invalid — reset to AEST"));
  }

  if (!cfg_sb_url[0]  && TEST_SB_URL[0])  strlcpy(cfg_sb_url,  TEST_SB_URL, sizeof(cfg_sb_url));
  if (!cfg_sb_key[0]  && TEST_SB_KEY[0])  strlcpy(cfg_sb_key,  TEST_SB_KEY, sizeof(cfg_sb_key));
  if (!cfg_apikey[0]  && TEST_APIKEY[0])  strlcpy(cfg_apikey,  TEST_APIKEY, sizeof(cfg_apikey));

  snprintf(bearerBuf, sizeof(bearerBuf), "Bearer %s", cfg_sb_key);

  Serial.print(F("[Config] Supabase: ")); Serial.println(cfg_sb_url[0] ? F("set") : F("not set"));
  Serial.print(F("[Config] Weather:  ")); Serial.println(cfg_apikey[0] ? F("set") : F("not set"));
}

// ================================================================
//  TIMEZONE — POSIX TZ string from stored offset
//
//  v9 adds QLD (36001) and NT (34201) as distinct no-DST entries.
//  The portal uses fictional offset values (+1) to distinguish them
//  from DST-aware states that share the same UTC offset.
// ================================================================

const char* getPosixTZ(long tzOffset) {
  switch (tzOffset) {
    case 36000: return "AEST-10AEDT,M10.1.0,M4.1.0/3";   // NSW/VIC/TAS/ACT
    case 36001: return "AEST-10";                           // QLD — no DST
    case 34200: return "ACST-9:30ACDT,M10.1.0,M4.1.0/3"; // SA
    case 34201: return "ACST-9:30";                         // NT — no DST
    case 28800: return "AWST-8";                            // WA
    case 0:     return "UTC0";
    default:    return "AEST-10AEDT,M10.1.0,M4.1.0/3";
  }
}

// Cloud AI — minimum good sessions before overriding local target
const int AI_MIN_CONFIDENCE = 3;

// ================================================================
//  CLOUD — FETCH AI RECOMMENDATIONS
// ================================================================

void fetchCloudRecommendations() {
  if (!cfg_sb_url[0] || !cfg_sb_key[0]) return;
  if (!modem.isGprsConnected()) return;

  Serial.println(F("[AI] Fetching cloud recommendations..."));

  for (int i = 0; i < NUM_ZONES; i++) {
    StaticJsonDocument<128> reqDoc;
    reqDoc["p_farm_id"]    = farm_id;
    reqDoc["p_zone_index"] = i;
    char body[128];
    serializeJson(reqDoc, body, sizeof(body));

    char url[200];
    snprintf(url, sizeof(url), "%s/rest/v1/rpc/get_zone_recommendation", cfg_sb_url);

    HTTPClient http;
    http.begin(gsmClient, url);
    http.addHeader("Content-Type",  "application/json");
    http.addHeader("Authorization", bearerBuf);
    http.addHeader("apikey",        cfg_sb_key);
    http.setTimeout(8000);

    int code = http.POST(body);

    if (code == 200) {
      StaticJsonDocument<256> resDoc;
      DeserializationError err = deserializeJson(resDoc, http.getStream());
      http.end();  // Always end here — no second http.end() below

      if (!err) {
        JsonVariant row = resDoc.is<JsonArray>() ? resDoc[0].as<JsonVariant>() : resDoc.as<JsonVariant>();

        int   confidence  = row["confidence"]         | 0;
        float recommended = row["recommended_target"] | 0.0f;
        int   analyzed    = row["sessions_analyzed"]  | 0;

        Serial.print(F("[AI] Zone ")); Serial.print(i + 1);
        Serial.print(F(" | sessions: "));  Serial.print(analyzed);
        Serial.print(F(" | confidence: ")); Serial.print(confidence);
        Serial.print(F(" | recommended: ")); Serial.println(recommended, 2);

        if (confidence >= AI_MIN_CONFIDENCE && recommended > 0.0f) {
          float old = zones[i].target;
          zones[i].target = constrain(recommended, TARGET_MIN, TARGET_MAX);
          saveZone(i);
          Serial.print(F("[AI] Zone ")); Serial.print(i + 1);
          Serial.print(F(" target updated: "));
          Serial.print(old, 2); Serial.print(F(" -> "));
          Serial.println(zones[i].target, 2);
        } else {
          Serial.print(F("[AI] Zone ")); Serial.print(i + 1);
          Serial.print(F(" keeping local target (need "));
          Serial.print(AI_MIN_CONFIDENCE - confidence);
          Serial.println(F(" more good sessions)"));
        }
      } else {
        Serial.print(F("[AI] Zone ")); Serial.print(i + 1);
        Serial.println(F(" — parse error"));
      }
    } else {
      http.end();
      Serial.print(F("[AI] Zone ")); Serial.print(i + 1);
      Serial.print(F(" fetch failed: ")); Serial.println(code);
    }
  }
}

// ================================================================
//  CLOUD — HEARTBEAT  (includes 4G signal quality)
//  Bug 3 fix: HTTP response code checked; failure logged to Serial.
// ================================================================

void sendHeartbeat() {
  if (!cfg_sb_url[0] || !cfg_sb_key[0]) return;
  if (!modem.isGprsConnected()) return;

  char tsBuf[32];
  time_t now_ts = time(nullptr);
  strftime(tsBuf, sizeof(tsBuf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now_ts));

  int rssi = modem.getSignalQuality();

  char body[80];
  snprintf(body, sizeof(body),
    "{\"last_seen\":\"%s\",\"signal_rssi\":%d}", tsBuf, rssi);

  char url[192];
  snprintf(url, sizeof(url), "%s/rest/v1/farms?farm_id=eq.%s", cfg_sb_url, farm_id);

  HTTPClient http;
  http.begin(gsmClient, url);
  http.addHeader("Content-Type",  "application/json");
  http.addHeader("Authorization", bearerBuf);
  http.addHeader("apikey",        cfg_sb_key);
  http.addHeader("Prefer",        "return=minimal");
  http.setTimeout(8000);

  int code = http.sendRequest("PATCH", body);
  http.end();

  // Log failures so they are visible in Serial Monitor (Bug 3)
  if (code != 200 && code != 204) {
    Serial.print(F("[Heartbeat] PATCH failed: ")); Serial.println(code);
  }
}

// ================================================================
//  CLOUD — REGISTER FARM
// ================================================================

void registerFarm() {
  if (!cfg_sb_url[0] || !cfg_sb_key[0]) return;
  if (!modem.isGprsConnected()) return;

  Serial.print(F("[Cloud] Registering farm..."));

  // QLD (36001) and NT (34201) use fictional +1 offsets internally.
  // Normalise back to the real UTC offset before writing to Supabase.
  long storedTz = cfg_tz;
  if (storedTz == 36001) storedTz = 36000;
  if (storedTz == 34201) storedTz = 34200;

  StaticJsonDocument<256> doc;
  doc["farm_id"]         = farm_id;
  doc["lat"]             = cfg_lat;
  doc["lon"]             = cfg_lon;
  doc["timezone_offset"] = storedTz;
  doc["firmware_ver"]    = FIRMWARE_VERSION;
  time_t now_ts = time(nullptr);
  char tsBuf[32];
  strftime(tsBuf, sizeof(tsBuf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now_ts));
  doc["last_seen"] = tsBuf;

  char body[256];
  size_t len = serializeJson(doc, body, sizeof(body));
  if (len >= sizeof(body)) {
    Serial.println(F("[Cloud] registerFarm: JSON truncated"));
  }

  char url[192];
  snprintf(url, sizeof(url), "%s/rest/v1/farms", cfg_sb_url);

  HTTPClient http;
  http.begin(gsmClient, url);
  http.setTimeout(8000);
  http.addHeader("Content-Type",  "application/json");
  http.addHeader("Authorization", bearerBuf);
  http.addHeader("apikey",        cfg_sb_key);
  http.addHeader("Prefer",        "resolution=merge-duplicates");

  int code = http.POST(body);

  if (code == 200 || code == 201 || code == 409) {
    farmRegistered = true;
    Serial.println(F(" OK"));
  } else {
    Serial.print(F(" HTTP ")); Serial.println(code);
  }
  http.end();
}

// ================================================================
//  CLOUD — SUPABASE SCHEMA VALIDATION
// ================================================================

void checkSupabaseSchema() {
  if (!cfg_sb_url[0] || !cfg_sb_key[0]) return;
  if (!modem.isGprsConnected()) return;

  char url[200];
  snprintf(url, sizeof(url), "%s/rest/v1/sessions?limit=0", cfg_sb_url);

  HTTPClient http;
  http.begin(gsmClient, url);
  http.addHeader("Authorization", bearerBuf);
  http.addHeader("apikey",        cfg_sb_key);
  http.setTimeout(8000);

  int code = http.GET();
  http.end();

  if (code == 200 || code == 206) {
    Serial.println(F("[Cloud] sessions table reachable — schema OK"));
  } else {
    Serial.println(F("[Cloud] WARNING: sessions table unreachable — check RLS policies"));
  }
}

// ================================================================
//  CLOUD — MANUAL TRIGGER POLLING  (Bug 2)
//
//  Polls the Supabase farms table for a dashboard-written manual_trigger
//  value every 5 s from loop(). If 1 or 2, starts the corresponding zone
//  and immediately clears the column back to 0.
//
//  Prerequisite SQL (run once in Supabase SQL editor):
//    ALTER TABLE farms ADD COLUMN IF NOT EXISTS manual_trigger integer DEFAULT 0;
// ================================================================

void pollManualTrigger() {
  if (activeZone >= 0)               return;   // Don't interrupt active irrigation
  if (!cfg_sb_url[0] || !cfg_sb_key[0]) return;
  if (!modem.isGprsConnected())      return;

  char url[220];
  snprintf(url, sizeof(url),
    "%s/rest/v1/farms?farm_id=eq.%s&select=manual_trigger",
    cfg_sb_url, farm_id);

  HTTPClient http;
  http.begin(gsmClient, url);
  http.addHeader("Authorization", bearerBuf);
  http.addHeader("apikey",        cfg_sb_key);
  http.setTimeout(5000);

  int code      = http.GET();
  int triggerVal = 0;

  if (code == 200) {
    StaticJsonDocument<64> doc;
    DeserializationError err = deserializeJson(doc, http.getStream());
    if (!err && doc.is<JsonArray>() && doc.size() > 0) {
      triggerVal = doc[0]["manual_trigger"] | 0;
    }
  }
  http.end();

  if (triggerVal < 1 || triggerVal > NUM_ZONES) return;   // 0 or out-of-range — nothing to do

  int zoneIdx = triggerVal - 1;   // Convert 1-based dashboard value to 0-based index
  startZone(zoneIdx);
  activeZone    = zoneIdx;
  lastWaterTick = millis();
  Serial.print(F("[Trigger] Manual trigger received for Zone "));
  Serial.println(zoneIdx + 1);

  // Clear the trigger column in Supabase so it won't re-fire on next poll
  char patchUrl[192];
  snprintf(patchUrl, sizeof(patchUrl),
    "%s/rest/v1/farms?farm_id=eq.%s", cfg_sb_url, farm_id);

  HTTPClient patchHttp;
  patchHttp.begin(gsmClient, patchUrl);
  patchHttp.addHeader("Content-Type",  "application/json");
  patchHttp.addHeader("Authorization", bearerBuf);
  patchHttp.addHeader("apikey",        cfg_sb_key);
  patchHttp.addHeader("Prefer",        "return=minimal");
  patchHttp.setTimeout(5000);
  patchHttp.sendRequest("PATCH", "{\"manual_trigger\":0}");
  patchHttp.end();
}

// ================================================================
//  CLOUD — POST SESSION
// ================================================================

void postSession(int zone, int preMoist, int postMoist,
                 float waterGiven, float targetBefore, float targetAfter,
                 float tempC, float rainMM, const char* learnResult) {

  if (!cfg_sb_url[0] || !cfg_sb_key[0]) return;

  time_t now_ts = time(nullptr);

  if (!modem.isGprsConnected()) {
    queueSession(zone, preMoist, postMoist, waterGiven,
                 targetBefore, targetAfter, tempC, rainMM, learnResult,
                 zones[zone].startedAt, now_ts);
    return;
  }

  bool ok = sendSessionToCloud(zone, preMoist, postMoist, waterGiven,
                               targetBefore, targetAfter, tempC, rainMM, learnResult,
                               zones[zone].startedAt, now_ts);
  if (!ok) {
    queueSession(zone, preMoist, postMoist, waterGiven,
                 targetBefore, targetAfter, tempC, rainMM, learnResult,
                 zones[zone].startedAt, now_ts);
  }
}

bool sendSessionToCloud(int zone, int preMoist, int postMoist,
                        float waterGiven, float targetBefore, float targetAfter,
                        float tempC, float rainMM, const char* learnResult,
                        time_t startTs, time_t endTs) {

  StaticJsonDocument<512> doc;
  doc["farm_id"]       = farm_id;
  doc["zone"]          = zone;
  doc["pre_moisture"]  = preMoist;
  doc["post_moisture"] = postMoist;
  doc["water_given"]   = waterGiven;
  doc["target_before"] = targetBefore;
  doc["target_after"]  = targetAfter;
  doc["temp_c"]        = tempC;
  doc["rain_mm"]       = rainMM;
  doc["learn_result"]  = learnResult;
  doc["dry_threshold"] = DRY_THRESHOLD;

  char ts[32];
  struct tm* t;
  t = gmtime(&startTs);
  strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", t);
  doc["started_at"] = ts;
  t = gmtime(&endTs);
  strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", t);
  doc["ended_at"] = ts;

  char body[512];
  size_t len = serializeJson(doc, body, sizeof(body));
  if (len >= sizeof(body)) {
    Serial.println(F("[Cloud] sendSession: JSON truncated"));
  }

  char url[192];
  snprintf(url, sizeof(url), "%s/rest/v1/sessions", cfg_sb_url);

  HTTPClient http;
  http.begin(gsmClient, url);
  http.addHeader("Content-Type",  "application/json");
  http.addHeader("Authorization", bearerBuf);
  http.addHeader("apikey",        cfg_sb_key);
  http.addHeader("Prefer",        "return=minimal");
  http.setTimeout(8000);

  int code = http.POST(body);
  http.end();

  bool ok = (code == 201 || code == 200);
  Serial.print(F("[Cloud] Session POST: "));
  if (ok) { Serial.println(F("OK")); } else { Serial.println(code); }
  return ok;
}

// ================================================================
//  SESSION QUEUE — CIRCULAR BUFFER
//
//  Physical slots "sq0"–"sq7" are mapped via queueHead + logical index.
//  Head pointer ("sqh") and count ("sqc") are persisted to NVS.
//
//  Write ordering for power-loss safety:
//    Enqueue: write blob → update sqh/sqc
//    Dequeue: update sqh/sqc → remove blob
// ================================================================

void queueSession(int zone, int preMoist, int postMoist,
                  float waterGiven, float targetBefore, float targetAfter,
                  float tempC, float rainMM, const char* learnResult,
                  time_t startTs, time_t endTs) {

  int slot;
  if (queueCount < QUEUE_SIZE) {
    slot = (queueHead + queueCount) % QUEUE_SIZE;
    queueCount++;
  } else {
    Serial.println(F("[Cloud] Queue full — overwriting oldest session"));
    slot = queueHead;
    queueHead = (queueHead + 1) % QUEUE_SIZE;
  }

  sessionQueue[slot].used         = true;
  sessionQueue[slot].zone         = zone;
  sessionQueue[slot].preMoisture  = preMoist;
  sessionQueue[slot].postMoisture = postMoist;
  sessionQueue[slot].waterGiven   = waterGiven;
  sessionQueue[slot].targetBefore = targetBefore;
  sessionQueue[slot].targetAfter  = targetAfter;
  sessionQueue[slot].tempC        = tempC;
  sessionQueue[slot].rainMM       = rainMM;
  sessionQueue[slot].startedAt    = startTs;
  sessionQueue[slot].endedAt      = endTs;
  strlcpy(sessionQueue[slot].learnResult, learnResult, sizeof(sessionQueue[slot].learnResult));

  saveQueueSlotNVS(slot);
  prefs.putInt("sqh", queueHead);
  prefs.putInt("sqc", queueCount);

  Serial.print(F("[Cloud] Queued offline session (slot ")); Serial.print(slot); Serial.println(F(")"));
}

void flushOneSession() {
  if (queueCount == 0) return;
  if (!cfg_sb_url[0] || !cfg_sb_key[0]) return;
  if (!modem.isGprsConnected()) return;

  int slot = queueHead;

  if (!sessionQueue[slot].used) {
    Serial.println(F("[Cloud] Stale queue slot — discarding"));
    clearQueueSlotNVS(slot);
    queueHead = (queueHead + 1) % QUEUE_SIZE;
    queueCount--;
    prefs.putInt("sqh", queueHead);
    prefs.putInt("sqc", queueCount);
    return;
  }

  Serial.print(F("[Cloud] Flushing queued session (slot ")); Serial.print(slot); Serial.println(F(")"));

  bool ok = sendSessionToCloud(
    sessionQueue[slot].zone,         sessionQueue[slot].preMoisture,
    sessionQueue[slot].postMoisture,  sessionQueue[slot].waterGiven,
    sessionQueue[slot].targetBefore,  sessionQueue[slot].targetAfter,
    sessionQueue[slot].tempC,         sessionQueue[slot].rainMM,
    sessionQueue[slot].learnResult,
    sessionQueue[slot].startedAt,     sessionQueue[slot].endedAt
  );

  if (ok) {
    queueHead = (queueHead + 1) % QUEUE_SIZE;
    queueCount--;
    prefs.putInt("sqh", queueHead);
    prefs.putInt("sqc", queueCount);
    sessionQueue[slot].used = false;
    clearQueueSlotNVS(slot);
  }
  // On failure, leave in place — next call retries
}

void flushSessionQueue() {
  if (!cfg_sb_url[0] || !cfg_sb_key[0]) return;
  if (!modem.isGprsConnected()) return;

  while (queueCount > 0) {
    int slot = queueHead;

    if (!sessionQueue[slot].used) {
      clearQueueSlotNVS(slot);
      queueHead = (queueHead + 1) % QUEUE_SIZE;
      queueCount--;
      prefs.putInt("sqh", queueHead);
      prefs.putInt("sqc", queueCount);
      continue;
    }

    Serial.print(F("[Cloud] Flushing queued session (slot ")); Serial.print(slot); Serial.println(F(")"));

    bool ok = sendSessionToCloud(
      sessionQueue[slot].zone,         sessionQueue[slot].preMoisture,
      sessionQueue[slot].postMoisture,  sessionQueue[slot].waterGiven,
      sessionQueue[slot].targetBefore,  sessionQueue[slot].targetAfter,
      sessionQueue[slot].tempC,         sessionQueue[slot].rainMM,
      sessionQueue[slot].learnResult,
      sessionQueue[slot].startedAt,     sessionQueue[slot].endedAt
    );

    if (ok) {
      queueHead = (queueHead + 1) % QUEUE_SIZE;
      queueCount--;
      prefs.putInt("sqh", queueHead);
      prefs.putInt("sqc", queueCount);
      sessionQueue[slot].used = false;
      clearQueueSlotNVS(slot);
    } else {
      break;   // Stop on first failure; loop's flushOneSession() will retry
    }
  }
}

// ================================================================
//  SESSION QUEUE NVS PERSISTENCE
// ================================================================

void saveQueueSlotNVS(int i) {
  char key[4];
  snprintf(key, sizeof(key), "sq%d", i);
  if (sessionQueue[i].used) {
    prefs.putBytes(key, &sessionQueue[i], sizeof(PendingSession));
  } else {
    prefs.remove(key);
  }
}

void clearQueueSlotNVS(int i) {
  char key[4];
  snprintf(key, sizeof(key), "sq%d", i);
  prefs.remove(key);
}

void loadSessionQueueFromNVS() {
  queueHead  = prefs.getInt("sqh", 0);
  queueCount = prefs.getInt("sqc", 0);

  if (queueHead  < 0 || queueHead  >= QUEUE_SIZE) queueHead  = 0;
  if (queueCount < 0 || queueCount >  QUEUE_SIZE) queueCount = 0;

  int restored = 0;
  for (int li = 0; li < queueCount; li++) {
    int pi = (queueHead + li) % QUEUE_SIZE;
    char key[4];
    snprintf(key, sizeof(key), "sq%d", pi);
    size_t sz = prefs.getBytesLength(key);
    if (sz == sizeof(PendingSession)) {
      prefs.getBytes(key, &sessionQueue[pi], sizeof(PendingSession));
      if (sessionQueue[pi].used) { restored++; continue; }
    }
    Serial.print(F("[Cloud] NVS queue truncated at logical slot ")); Serial.println(li);
    queueCount = li;
    prefs.putInt("sqc", queueCount);
    break;
  }

  if (restored > 0) {
    Serial.print(F("[Cloud] Restored "));
    Serial.print(restored);
    Serial.println(F(" queued session(s) from NVS"));
  }
}

// ================================================================
//  IRRIGATION STATE MACHINE
//  Auto-trigger re-enabled from v11 — all commented-out start code
//  and the testing return removed.
// ================================================================

void runIrrigation() {
  unsigned long now = millis();

  // ── Active zone: track water given, stop when target reached ───
  if (activeZone >= 0) {
    if (now - lastWaterTick >= 1000) {
      lastWaterTick = now;
      zones[activeZone].waterGiven += 0.05f;

      Serial.print(F("  Zone ")); Serial.print(activeZone + 1);
      Serial.print(F(" | given: ")); Serial.print(zones[activeZone].waterGiven, 2);
      Serial.print(F(" / target: ")); Serial.println(zones[activeZone].target, 2);
    }

    if (zones[activeZone].waterGiven >= zones[activeZone].target) {
      stopZone(activeZone);
      activeZone = -1;
    }
    return;
  }

  // ── No active zone: check watering window and rain forecast ────
  if (!isInWateringWindow()) return;
  if (forecastRainMM >= RAIN_SKIP_MM) return;

  // Find the driest eligible zone
  int driest = -1, driestReading = 0;
  for (int i = 0; i < NUM_ZONES; i++) {
    int  m  = readMoisture(i);
    // lastWatered == 0 means never watered — allow immediately (no boot cooldown)
    bool ok = (zones[i].lastWatered == 0 || (now - zones[i].lastWatered > ZONE_COOLDOWN_MS));
    if (m > DRY_THRESHOLD && ok && m > driestReading) {
      driestReading = m; driest = i;
    }
  }

  if (driest >= 0) {
    startZone(driest);
    activeZone    = driest;
    lastWaterTick = millis();
  }
}

// ================================================================
//  ZONE CONTROL
// ================================================================

void startZone(int idx) {
  zones[idx].waterGiven  = 0;
  zones[idx].isRunning   = true;
  zones[idx].preMoisture = readMoisture(idx);
  zones[idx].startedAt   = time(nullptr);
  digitalWrite(RELAY_PINS[idx], LOW);   // Active LOW relay

  Serial.print(F("\n[Zone ")); Serial.print(idx + 1);
  Serial.print(F("] OPEN  | moisture: ")); Serial.print(zones[idx].preMoisture);
  Serial.print(F(" | target: ")); Serial.println(zones[idx].target, 2);
}

void stopZone(int idx) {
  float targetBefore = zones[idx].target;
  int   postMoisture = readMoisture(idx);

  digitalWrite(RELAY_PINS[idx], HIGH);
  zones[idx].isRunning   = false;
  zones[idx].lastWatered = millis();

  Serial.print(F("[Zone ")); Serial.print(idx + 1);
  Serial.print(F("] CLOSE | post-moisture: ")); Serial.println(postMoisture);

  const char* learnResult = applyLearning(idx, postMoisture);

  postSession(idx,
    zones[idx].preMoisture, postMoisture,
    zones[idx].waterGiven,
    targetBefore, zones[idx].target,
    forecastTempC, forecastRainMM,
    learnResult);
}

// ================================================================
//  ADAPTIVE LEARNING
// ================================================================

const char* applyLearning(int idx, int post) {
  float span  = (float)(4095 - DRY_THRESHOLD);
  if (span <= 0.0f) span = 1.0f;   // Guard: prevents div-by-zero
  float error = constrain((float)(post - DRY_THRESHOLD) / span, -1.0f, 1.0f);
  float delta = 0.0f;
  const char* result = "good";

  if (post > DRY_THRESHOLD) {
    delta  = LEARNING_RATE * (1.0f + error);
    result = "dry";
    Serial.println(F("  [Learn] Still DRY → target ↑"));
  } else if (error < -0.15f) {
    delta  = LEARNING_RATE * error;
    result = "over";
    Serial.println(F("  [Learn] Over-watered → target ↓"));
  } else {
    Serial.println(F("  [Learn] Good moisture — no change"));
  }

  if (delta != 0.0f) {
    // Apply temperature factor to delta only — not to the full target.
    // Multiplying the full target causes unbounded drift in heatwaves.
    float tf = constrain(1.0f + (forecastTempC - 20.0f) * 0.005f, 0.85f, 1.20f);
    zones[idx].target += delta * tf;
    zones[idx].target = constrain(zones[idx].target, TARGET_MIN, TARGET_MAX);
    // Only write to NVS when the target actually changed
    saveZone(idx);
    Serial.print(F("  [Learn] Zone ")); Serial.print(idx + 1);
    Serial.print(F(" new target: ")); Serial.print(zones[idx].target, 3);
    Serial.print(F("  (temp: ")); Serial.print(tf, 3); Serial.println(F(")"));
  }

  return result;
}

// ================================================================
//  SENSOR READ
// ================================================================

int readMoisture(int idx) {
  long sum = 0;
  for (int s = 0; s < ADC_SAMPLES; s++) {
    sum += analogRead(SOIL_PINS[idx]);
    delayMicroseconds(100);
  }
  int result = (int)(sum / ADC_SAMPLES);
  lastKnownMoisture[idx] = result;
  return result;
}

// ================================================================
//  PERSISTENT STORAGE
// ================================================================

void loadZones() {
  for (int i = 0; i < NUM_ZONES; i++) {
    char key[4];
    snprintf(key, sizeof(key), "t%d", i);
    zones[i].target      = prefs.getFloat(key, TARGET_DEFAULT);
    zones[i].waterGiven  = 0;
    zones[i].isRunning   = false;
    zones[i].lastWatered = 0;
    zones[i].preMoisture = 0;
    zones[i].startedAt   = 0;

    // Sanity check: corrupt NVS can return 0, keeping the valve open indefinitely
    if (zones[i].target < TARGET_MIN || zones[i].target > TARGET_MAX) {
      zones[i].target = TARGET_DEFAULT;
      Serial.print(F("[Config] WARNING: Zone ")); Serial.print(i + 1);
      Serial.println(F(" target out of bounds — reset to default"));
    }

    Serial.print(F("Loaded Target ")); Serial.print(i + 1);
    Serial.print(F(": ")); Serial.println(zones[i].target);
  }
}

void saveZone(int idx) {
  char key[4];
  snprintf(key, sizeof(key), "t%d", idx);
  prefs.putFloat(key, zones[idx].target);
  Serial.print(F("  [Save] Zone ")); Serial.print(idx + 1);
  Serial.print(F(" target: ")); Serial.println(zones[idx].target, 3);
}

// ================================================================
//  NTP — Sync via SIM7600 modem network time (NITZ / AT+CCLK)
//
//  TinyGSM's getNetworkTime() reads AT+CCLK, which returns the
//  modem's local clock (set by NITZ from the carrier network).
//  The returned 'tzOff' is the UTC offset in hours (e.g. 10.0 for
//  AEST).  We interpret the raw tm fields as UTC-equivalent by
//  temporarily switching TZ to UTC0 before calling mktime(), then
//  subtract tzOff to obtain the true UTC epoch, and finally restore
//  the configured TZ so localtime() works correctly everywhere else.
// ================================================================

void syncNTP() {
  lastNTPSync = millis();
  if (!modem.isGprsConnected()) return;

  setenv("TZ", getPosixTZ(cfg_tz), 1);
  tzset();

  Serial.print(F("[NTP]  Syncing via modem..."));

  int   year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
  float tzOff = 0.0f;
  bool  got   = false;

  // Retry up to 10 × 500ms = 5s — safe while WDT is armed (resets each iter)
  for (int i = 0; i < 10 && !got; i++) {
    got = modem.getNetworkTime(&year, &month, &day, &hour, &minute, &second, &tzOff);
    if (!got) { delay(500); esp_task_wdt_reset(); }
  }

  if (!got || year < 2024) {
    Serial.println(F("\n[NTP]  Failed — watering disabled until time is known"));
    return;
  }

  // Build tm from modem local time
  struct tm t  = {};
  t.tm_year    = year - 1900;
  t.tm_mon     = month - 1;
  t.tm_mday    = day;
  t.tm_hour    = hour;
  t.tm_min     = minute;
  t.tm_sec     = second;
  t.tm_isdst   = 0;

  // Use UTC0 so mktime() treats the modem local values as-is (no conversion)
  setenv("TZ", "UTC0", 1); tzset();
  time_t localEpoch = mktime(&t);
  // Restore configured TZ for all subsequent localtime() calls
  setenv("TZ", getPosixTZ(cfg_tz), 1); tzset();

  // UTC = modem-local − UTC-offset
  time_t utcEpoch = localEpoch - (time_t)(tzOff * 3600.0f);
  struct timeval tv = { .tv_sec = utcEpoch };
  settimeofday(&tv, NULL);
  timeReady = true;

  char buf[32];
  struct tm lt;
  localtime_r(&utcEpoch, &lt);
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &lt);
  Serial.print(F("\n[NTP]  ")); Serial.println(buf);
}

// Returns true only if local time is confirmed and within the watering window.
// If NTP has never synced, returns false — no watering without confirmed time.
bool isInWateringWindow() {
  if (!timeReady) return false;
  struct tm t;
  if (!getLocalTime(&t)) return false;
  return (t.tm_hour >= WATER_HOUR_START && t.tm_hour < WATER_HOUR_END);
}

// ================================================================
//  WEATHER  (routes through 4G modem)
// ================================================================

void fetchWeather() {
  lastWeatherFetch = millis();
  if (!modem.isGprsConnected() || strlen(cfg_apikey) < 10) return;

  char url[256];
  snprintf(url, sizeof(url),
    "https://api.openweathermap.org/data/2.5/forecast"
    "?lat=%.4f&lon=%.4f&cnt=4&units=metric&appid=%s",
    cfg_lat, cfg_lon, cfg_apikey);

  HTTPClient http;
  http.begin(gsmClient, url);
  http.setTimeout(8000);
  int code = http.GET();
  if (code != 200) { http.end(); return; }

  // Filter reduces ~3 KB JSON to ~512 B
  StaticJsonDocument<256> filter;
  filter["cnt"] = true;
  filter["list"][0]["rain"]["3h"] = true;
  filter["list"][0]["main"]["temp"] = true;

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, http.getStream(),
                                             DeserializationOption::Filter(filter));
  http.end();
  if (err) return;

  forecastRainMM = 0.0f;
  int cnt = doc["cnt"] | 4;
  for (int i = 0; i < cnt; i++) {
    forecastRainMM += doc["list"][i]["rain"]["3h"] | 0.0f;
    if (i == 0) forecastTempC = doc["list"][i]["main"]["temp"] | 20.0f;
  }
  weatherReady = true;
  Serial.print(F("[Weather] Rain: ")); Serial.print(forecastRainMM, 1);
  Serial.print(F("mm | Temp: ")); Serial.print(forecastTempC, 1); Serial.println(F("°C"));
}

// ================================================================
//  OTA — HTTP OTA (remote update from GitHub, over 4G)
//
//  ArduinoOTA (LAN-based) removed in v11 — device has no WiFi STA.
//  HTTP OTA still works over cellular using the global gsmClient.
//
//  Bug 5 fix: esp_task_wdt_reset() called immediately after every
//  esp_task_wdt_add(NULL) in failure paths to prevent a spurious
//  WDT trip on core 3.x after re-adding a previously deleted task.
//
//  version.txt format (two lines):
//    11.0
//    SHA256:abcdef0123456789...  (64 hex chars, sha256sum of firmware.bin)
// ================================================================

void checkHTTPOTA() {
  if (!modem.isGprsConnected()) return;
  if (!OTA_VERSION_URL[0] || strstr(OTA_VERSION_URL, "YOUR_GITHUB")) return;

  Serial.print(F("[OTA] Checking for update..."));

  // ── Step 1: Fetch version.txt ─────────────────────────────────
  HTTPClient http;
  http.begin(gsmClient, OTA_VERSION_URL);
  http.setTimeout(8000);
  int code = http.GET();

  if (code != 200) {
    Serial.print(F(" HTTP ")); Serial.println(code);
    http.end();
    return;
  }

  Client* verStream = http.getStreamPtr();
  String versionLine = verStream->readStringUntil('\n');
  versionLine.trim();
  String hashLine    = verStream->readStringUntil('\n');
  hashLine.trim();
  http.end();

  char expectedHash[65] = "";
  if (hashLine.startsWith("SHA256:")) {
    hashLine.substring(7).toCharArray(expectedHash, sizeof(expectedHash));
  }

  Serial.print(F(" remote=v")); Serial.print(versionLine);
  Serial.print(F(" local=v")); Serial.println(FIRMWARE_VERSION);

  if (expectedHash[0]) {
    Serial.print(F("[OTA] Expected SHA256: ")); Serial.println(expectedHash);
  } else {
    Serial.println(F("[OTA] WARNING: No SHA256 in version.txt — integrity check skipped"));
  }

  if (!isNewerVersion(versionLine.c_str(), FIRMWARE_VERSION)) {
    Serial.println(F("[OTA] Already up to date"));
    return;
  }

  Serial.println(F("[OTA] New version found — downloading..."));

  // Close all relays and clear zone state before flashing
  for (int i = 0; i < NUM_ZONES; i++) {
    digitalWrite(RELAY_PINS[i], HIGH);
    zones[i].isRunning = false;
  }
  activeZone = -1;

  // Suspend WDT — per-chunk reset below replaces it
  esp_task_wdt_delete(NULL);

  // ── Step 2: Download firmware binary ─────────────────────────
  // gsmClient is reused here — http.end() above closed the previous connection
  HTTPClient updateHttp;
  updateHttp.begin(gsmClient, OTA_FIRMWARE_URL);
  updateHttp.setTimeout(30000);
  int updateCode = updateHttp.GET();

  if (updateCode != 200) {
    Serial.print(F("[OTA] Download failed: ")); Serial.println(updateCode);
    updateHttp.end();
    esp_task_wdt_add(NULL);
    esp_task_wdt_reset();   // Bug 5: prevent spurious trip after re-add
    return;
  }

  int    contentLength = updateHttp.getSize();
  size_t updateSize    = (contentLength > 0) ? (size_t)contentLength : UPDATE_SIZE_UNKNOWN;

  if (!Update.begin(updateSize)) {
    Serial.println(F("[OTA] Not enough flash space"));
    updateHttp.end();
    esp_task_wdt_add(NULL);
    esp_task_wdt_reset();   // Bug 5
    return;
  }

  Client* stream = updateHttp.getStreamPtr();

  // ── Step 3: Stream in 1 KB chunks, computing SHA256 in-flight ─
  mbedtls_sha256_context sha_ctx;
  mbedtls_sha256_init(&sha_ctx);
  mbedtls_sha256_starts(&sha_ctx, 0);   // 0 = SHA256 (not SHA224)

  static uint8_t chunk[1024];
  size_t written = 0;
  bool   writeOk = true;

  while (contentLength <= 0 || written < (size_t)contentLength) {
    size_t toRead = (contentLength > 0)
      ? min((size_t)sizeof(chunk), (size_t)contentLength - written)
      : sizeof(chunk);

    int n = stream->readBytes(chunk, toRead);
    if (n <= 0) {
      if (contentLength > 0 && written < (size_t)contentLength) {
        Serial.println(F("[OTA] Stream ended early — aborting"));
        writeOk = false;
      }
      break;
    }

    mbedtls_sha256_update(&sha_ctx, chunk, (size_t)n);

    if (Update.write(chunk, (size_t)n) != (size_t)n) {
      Serial.println(F("[OTA] Flash write error — aborting"));
      writeOk = false;
      break;
    }

    written += (size_t)n;
    esp_task_wdt_reset();   // Reset WDT every chunk — safe for slow cellular
  }

  updateHttp.end();

  uint8_t digest[32];
  mbedtls_sha256_finish(&sha_ctx, digest);
  mbedtls_sha256_free(&sha_ctx);

  char computedHash[65];
  for (int i = 0; i < 32; i++) {
    sprintf(computedHash + i * 2, "%02x", digest[i]);
  }
  computedHash[64] = '\0';

  // ── Step 4: Abort on stream/write error ───────────────────────
  if (!writeOk) {
    Update.abort();
    esp_task_wdt_add(NULL);
    esp_task_wdt_reset();   // Bug 5
    return;
  }

  // ── Step 5: Verify SHA256 before committing to flash ──────────
  if (expectedHash[0]) {
    if (strcmp(computedHash, expectedHash) != 0) {
      Serial.println(F("[OTA] SHA256 MISMATCH — refusing to flash"));
      Serial.print(F("[OTA] Expected: ")); Serial.println(expectedHash);
      Serial.print(F("[OTA] Computed: ")); Serial.println(computedHash);
      Update.abort();
      esp_task_wdt_add(NULL);
      esp_task_wdt_reset();   // Bug 5
      return;
    }
    Serial.println(F("[OTA] SHA256 verified OK"));
  }

  // ── Step 6: Commit ────────────────────────────────────────────
  bool success = Update.end(true);
  if (success && (contentLength <= 0 || written == (size_t)contentLength)) {
    Serial.println(F("[OTA] Flash successful — rebooting"));
    delay(1000);
    ESP.restart();
  } else {
    Serial.println(F("[OTA] Flash commit failed"));
    Update.printError(Serial);
    esp_task_wdt_add(NULL);
    esp_task_wdt_reset();   // Bug 5
  }
}

// ================================================================
//  STATUS PRINT
// ================================================================

void printStatus() {
  Serial.println(F("──────────────────────────────────────────"));
  for (int i = 0; i < NUM_ZONES; i++) {
    int m = lastKnownMoisture[i];
    Serial.print(F("Zone ")); Serial.print(i + 1);
    Serial.print(F("  moisture=")); Serial.print(m);
    Serial.print(m > DRY_THRESHOLD ? F(" (DRY)  ") : F(" (wet)  "));
    Serial.print(F("target=")); Serial.print(zones[i].target, 2);
    Serial.print(F("  ")); Serial.println(zones[i].isRunning ? F("RUNNING") : F("idle"));
  }
  Serial.print(F("Window: "));
  Serial.print(isInWateringWindow() ? F("OPEN") : F("closed"));
  Serial.print(F(" (")); Serial.print(WATER_HOUR_START);
  Serial.print(F(":00–")); Serial.print(WATER_HOUR_END); Serial.print(F(":00)"));
  if (!timeReady) Serial.print(F("  |  time: NOT SYNCED (watering disabled)"));
  if (forecastRainMM >= RAIN_SKIP_MM) {
    Serial.print(F("  |  RAIN SKIP (")); Serial.print(forecastRainMM, 1); Serial.print(F("mm)"));
  }
  if (!weatherReady) Serial.print(F("  |  weather: offline"));
  Serial.print(F("  |  4G: "));
  if (modem.isGprsConnected()) {
    Serial.print(F("connected (RSSI ")); Serial.print(modem.getSignalQuality()); Serial.print(F(")"));
  } else {
    Serial.print(F("offline"));
  }
  Serial.print(F("  |  cloud: ")); Serial.println(farmRegistered ? F("connected") : F("offline"));
}

// ================================================================
//  VERSION COMPARE
//  Returns true if remoteVer is strictly newer than localVer.
//  Parses X.Y as two integers so "9.10" correctly beats "9.9".
// ================================================================

bool isNewerVersion(const char* remoteVer, const char* localVer) {
  int rMaj = 0, rMin = 0, lMaj = 0, lMin = 0;
  sscanf(remoteVer, "%d.%d", &rMaj, &rMin);
  sscanf(localVer,  "%d.%d", &lMaj, &lMin);
  if (rMaj != lMaj) return rMaj > lMaj;
  return rMin > lMin;
}
