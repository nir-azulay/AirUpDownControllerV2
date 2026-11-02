#!/usr/bin/env python3
"""
Build the Independent Offline Field Updater Package (`ble/dist/`):
1. Compiles `firmware.bin` (`pio run`) and `spiffs.bin` (`pio run -t buildfs`) if `--build` is passed.
2. Copies standalone `firmware.bin` and `spiffs.bin` directly into `ble/dist/` (as well as `ble/dist/payload/`)
   so `ble/dist/` is a 100% self-contained, portable folder.
3. Generates:
   - `ble/dist/OfflineFieldUpdater.html`: Compact 3-step UI with embedded `firmware.bin`, `spiffs.bin`,
     and individual static files, plus `settings.json` downloader, file importer, and 18-key rebuilder.
   - `ble/dist/Open_Field_Updater.command`: Double-clickable launcher that serves the UI on
     `http://127.0.0.1:8765` and proxies `/api/read?ip=...` so "Read Device" bypasses browser `file://`
     CORS restrictions even on older ESP32 firmware versions.
"""

import base64
import json
import os
import re
import shutil
import subprocess
import sys
from datetime import datetime
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parent.parent
DATA_DIR = ROOT_DIR / "data"
FIRMWARE_BIN = ROOT_DIR / ".pio" / "build" / "esp32" / "firmware.bin"
SPIFFS_BIN = ROOT_DIR / ".pio" / "build" / "esp32" / "spiffs.bin"
VERSION_H = ROOT_DIR / "include" / "version.h"
DIST_DIR = ROOT_DIR / "dist"
PAYLOAD_DIR = DIST_DIR / "payload"

STATIC_FILES = [
    "index.html",
    "configurations.html",
    "update.html",
    "fs.html",
    "about.html",
    "profile.jpg",
]


def get_version_short() -> str:
    if VERSION_H.exists():
        content = VERSION_H.read_text(encoding="utf-8", errors="ignore")
        m = re.search(r'#define\s+VERSION_SHORT\s+"([^"]+)"', content)
        if m:
            return m.group(1)
    return "v1.0.unknown"


def build_package(run_pio: bool = False) -> None:
    if run_pio or not FIRMWARE_BIN.exists():
        print("Building ESP32 firmware (`pio run`)...")
        subprocess.run(["pio", "run"], cwd=str(ROOT_DIR), check=True)
    if run_pio or not SPIFFS_BIN.exists():
        print("Building ESP32 SPIFFS image (`pio run -t buildfs`)...")
        subprocess.run(["pio", "run", "-t", "buildfs"], cwd=str(ROOT_DIR), check=True)

    if not FIRMWARE_BIN.exists():
        raise FileNotFoundError(f"Firmware binary not found at {FIRMWARE_BIN}")
    if not SPIFFS_BIN.exists():
        raise FileNotFoundError(f"SPIFFS binary not found at {SPIFFS_BIN}")

    version_str = get_version_short()
    build_time = datetime.now().strftime("%Y-%m-%d %H:%M")
    fw_bytes = FIRMWARE_BIN.read_bytes()
    fw_b64 = base64.b64encode(fw_bytes).decode("ascii")
    spiffs_bytes = SPIFFS_BIN.read_bytes()
    spiffs_b64 = base64.b64encode(spiffs_bytes).decode("ascii")

    if PAYLOAD_DIR.exists():
        shutil.rmtree(PAYLOAD_DIR)
    DIST_DIR.mkdir(parents=True, exist_ok=True)
    PAYLOAD_DIR.mkdir(parents=True, exist_ok=True)

    # Copy standalone firmware.bin and spiffs.bin directly into dist/ AND dist/payload/
    shutil.copy2(FIRMWARE_BIN, DIST_DIR / "firmware.bin")
    shutil.copy2(SPIFFS_BIN, DIST_DIR / "spiffs.bin")
    shutil.copy2(FIRMWARE_BIN, PAYLOAD_DIR / "firmware.bin")
    shutil.copy2(SPIFFS_BIN, PAYLOAD_DIR / "spiffs.bin")

    embedded_files = []
    for fname in STATIC_FILES:
        fpath = DATA_DIR / fname
        if fpath.exists():
            raw = fpath.read_bytes()
            shutil.copy2(fpath, PAYLOAD_DIR / fname)
            embedded_files.append(
                {
                    "name": fname,
                    "size": len(raw),
                    "b64": base64.b64encode(raw).decode("ascii"),
                }
            )

    manifest_js = json.dumps(
        {
            "version": version_str,
            "buildTime": build_time,
            "firmwareSize": len(fw_bytes),
            "firmwareB64": fw_b64,
            "spiffsSize": len(spiffs_bytes),
            "spiffsB64": spiffs_b64,
            "staticFiles": embedded_files,
        }
    )

    html_content = f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Field Updater ({version_str})</title>
<style>
* {{ box-sizing: border-box; }}
body {{
  background: #101318;
  color: #eceff4;
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
  font-size: 14px;
  margin: 0;
  padding: 20px 24px 36px;
}}
.app-shell {{
  max-width: 1340px;
  width: 100%;
  margin: 0 auto;
}}
.dashboard-grid {{
  display: grid;
  grid-template-columns: 460px 1fr;
  gap: 18px;
  align-items: start;
}}
@media (max-width: 960px) {{
  .dashboard-grid {{
    grid-template-columns: 1fr;
  }}
}}
.top-bar {{
  display: flex;
  align-items: center;
  justify-content: space-between;
  background: #181c24;
  border: 1px solid #262c38;
  border-radius: 12px;
  padding: 14px 20px;
  margin-bottom: 16px;
}}
.top-title h1 {{
  font-size: 20px;
  margin: 0 0 3px;
  color: #fff;
}}
.top-title p {{
  margin: 0;
  font-size: 13px;
  color: #9aa5b4;
}}
.ver-pill {{
  background: rgba(226, 148, 37, 0.18);
  color: #e29425;
  border: 1px solid rgba(226, 148, 37, 0.45);
  border-radius: 999px;
  padding: 6px 14px;
  font-size: 13px;
  font-weight: 700;
  white-space: nowrap;
}}
.card {{
  background: #181c24;
  border: 1px solid #262c38;
  border-radius: 12px;
  padding: 18px 20px;
  margin-bottom: 16px;
  box-shadow: 0 4px 14px rgba(0, 0, 0, 0.3);
}}
.step-hdr {{
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 12px;
}}
.step-title {{
  font-size: 14px;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.6px;
  color: #e29425;
  margin: 0;
}}
.step-sub {{
  font-size: 12px;
  color: #8b96a8;
}}
.ap-guide {{
  background: #12161d;
  border-left: 3px solid #00acc1;
  border-radius: 6px;
  padding: 10px 14px;
  margin-bottom: 12px;
  font-size: 13px;
  color: #cfd6e0;
  line-height: 1.5;
}}
.ap-guide strong {{ color: #fff; }}
.ap-guide code {{
  background: #222834;
  color: #e29425;
  padding: 1px 6px;
  border-radius: 4px;
  font-size: 12px;
}}
.row {{
  display: flex;
  gap: 10px;
  align-items: center;
}}
.input-ip {{
  flex: 1;
  height: 40px;
  background: #101318;
  border: 1px solid #323a4a;
  border-radius: 6px;
  color: #fff;
  padding: 0 12px;
  font-size: 15px;
  font-weight: 600;
}}
.chips {{
  display: flex;
  gap: 8px;
  margin-top: 10px;
  flex-wrap: wrap;
}}
.chip {{
  background: #222834;
  color: #b8c2d0;
  border: 1px solid #323a4a;
  border-radius: 6px;
  padding: 6px 10px;
  font-size: 12px;
  font-weight: 600;
  cursor: pointer;
}}
.chip:hover {{
  border-color: #e29425;
  color: #fff;
}}
.device-banner {{
  margin-top: 12px;
  padding: 10px 12px;
  border-radius: 8px;
  background: #101318;
  border: 1px solid #252b37;
  display: flex;
  align-items: center;
  justify-content: space-between;
  font-size: 13px;
  flex-wrap: wrap;
  gap: 6px;
}}
.dot {{
  display: inline-block;
  width: 9px;
  height: 9px;
  border-radius: 50%;
  background: #6c7686;
  margin-right: 7px;
}}
.dot.online {{ background: #2ecc71; box-shadow: 0 0 6px #2ecc71; }}
.btn {{
  background: #00acc1;
  color: #fff;
  border: none;
  border-radius: 7px;
  padding: 10px 14px;
  font-size: 13px;
  font-weight: 700;
  letter-spacing: 0.4px;
  text-transform: uppercase;
  cursor: pointer;
  transition: opacity 0.15s;
  white-space: nowrap;
  text-align: center;
}}
.btn:hover:enabled {{ opacity: 0.9; }}
.btn:disabled {{ background: #2d3442; color: #6c7686; cursor: default; }}
.btn-orange {{ background: #e29425; color: #101318; }}
.btn-green {{ background: #27ae60; color: #fff; }}
.btn-secondary {{ background: #2d3545; color: #dde3ea; }}
.btn-block {{ width: 100%; padding: 13px; font-size: 14px; }}
.form-grid {{
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 12px 14px;
  margin-top: 12px;
}}
@media (max-width: 720px) {{
  .form-grid {{
    grid-template-columns: 1fr 1fr;
  }}
}}
.field {{
  display: flex;
  flex-direction: column;
}}
.field label {{
  font-size: 12px;
  color: #9aa5b4;
  margin-bottom: 4px;
  font-weight: 600;
}}
.field input, .field select {{
  height: 36px;
  background: #101318;
  border: 1px solid #303846;
  border-radius: 6px;
  color: #fff;
  padding: 0 10px;
  font-size: 13px;
}}
.pwd-wrap {{
  display: flex;
  gap: 5px;
}}
.pwd-wrap input {{ flex: 1; min-width: 0; }}
.btn-tiny {{
  background: #283040;
  color: #cfd6e0;
  border: 1px solid #384357;
  border-radius: 6px;
  padding: 0 10px;
  font-size: 12px;
  cursor: pointer;
}}
details.json-preview {{
  margin-top: 14px;
  background: #101318;
  border: 1px solid #252b37;
  border-radius: 8px;
  padding: 8px 12px;
}}
details.json-preview summary {{
  cursor: pointer;
  font-size: 12px;
  font-weight: 600;
  color: #9aa5b4;
}}
details.json-preview pre {{
  margin: 8px 0 2px;
  font-size: 12px;
  color: #88c0d0;
  overflow-x: auto;
}}
.progress-box {{
  background: #181c24;
  border: 1px solid #262c38;
  border-radius: 12px;
  padding: 16px 20px;
  box-shadow: 0 4px 14px rgba(0, 0, 0, 0.3);
}}
.bar-bg {{
  background: #252b37;
  height: 12px;
  border-radius: 6px;
  overflow: hidden;
  margin-bottom: 8px;
}}
.bar-fill {{
  background: linear-gradient(90deg, #00acc1, #e29425);
  width: 0%;
  height: 100%;
  transition: width 0.15s ease;
}}
.status-line {{
  display: flex;
  justify-content: space-between;
  font-size: 13px;
  font-weight: 600;
  color: #d8dee9;
}}
.log-area {{
  margin-top: 10px;
  background: #0b0d11;
  border: 1px solid #222834;
  border-radius: 8px;
  padding: 10px 12px;
  font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
  font-size: 12px;
  color: #a3be8c;
  height: 190px;
  overflow-y: auto;
  white-space: pre-wrap;
}}
.footer {{
  text-align: center;
  font-size: 12px;
  color: #6c7686;
  margin-top: 16px;
}}
</style>
</head>
<body>
<div class="app-shell">
  <div class="top-bar">
    <div class="top-title">
      <h1>ESP32 Offline Field Updater</h1>
      <p>Direct AP &amp; Local Wi-Fi Upgrade Tool (100% Independent)</p>
    </div>
    <span class="ver-pill" id="lblPkgVer">{version_str}</span>
  </div>

  <div class="dashboard-grid">
    <div class="left-col">
      <!-- STEP 1: CONNECT & READ SETTINGS -->
      <div class="card">
        <div class="step-hdr">
          <h2 class="step-title">Step 1 — Connect to Device AP &amp; Read Settings</h2>
          <span class="step-sub">Wi-Fi Hotspot or LAN</span>
        </div>
        <div class="ap-guide">
          1. Connect Wi-Fi to the controller's Hotspot AP (e.g. <strong>esp32-AirUpDownController</strong>, password <code>12345678</code>) or LAN.<br>
          2. Click <strong>Read Device</strong>. <em>(If opened as a raw <code>file://</code> on old firmware without CORS, click <strong>1. Download settings.json</strong> then <strong>2. Load settings.json File</strong>.)</em>
        </div>
        <div class="row">
          <input type="text" id="targetIp" class="input-ip" value="192.168.4.1" />
          <button type="button" class="btn" onclick="readFromDevice()">Read Device</button>
          <button type="button" class="btn btn-secondary" onclick="openWebUI()">Open UI</button>
        </div>
        <div class="row" style="margin-top:8px;">
          <button type="button" class="btn btn-secondary" style="flex:1;" onclick="downloadDeviceSettingsJson()">1. Download settings.json</button>
          <button type="button" class="btn btn-secondary" style="flex:1;" onclick="document.getElementById('fileSettingsInput').click()">2. Load settings.json File</button>
          <input type="file" id="fileSettingsInput" accept=".json,application/json" style="display:none" onchange="onLoadSettingsFile(this)" />
        </div>
        <div class="chips">
          <button type="button" class="chip" onclick="selectIp('192.168.4.1')">AP Hotspot (192.168.4.1)</button>
          <button type="button" class="chip" onclick="selectIp('192.168.10.161')">Raptor (.161)</button>
          <button type="button" class="chip" onclick="selectIp('192.168.10.134')">Landrover (.134)</button>
          <button type="button" class="chip" onclick="selectIp('192.168.10.153')">TestBench (.153)</button>
        </div>
        <div class="device-banner">
          <div><span class="dot" id="connDot"></span><strong id="connName">Not connected yet</strong></div>
          <div id="connVer" style="color:#8b96a8;">Click "Read Device" or load settings.json</div>
        </div>
      </div>

      <!-- STEP 2: UPLOAD FIRMWARE / SPIFFS.BIN -->
      <div class="card">
        <div class="step-hdr">
          <h2 class="step-title">Step 2 — Flash Firmware &amp; SPIFFS Image</h2>
          <span class="step-sub" id="lblFwSize">firmware.bin + spiffs.bin embedded</span>
        </div>
        <button type="button" id="btnFw" class="btn btn-orange btn-block" onclick="uploadFirmwareOnly()">
          Upload Firmware (firmware.bin — {version_str})
        </button>
        <div class="row" style="margin-top:8px;">
          <button type="button" id="btnSpiffsAndSettings" class="btn" style="flex:1;" onclick="uploadSpiffsImage(true)">
            Upload spiffs.bin + Restore settings.json
          </button>
          <button type="button" id="btnSpiffsOnly" class="btn btn-secondary" style="flex:1;" onclick="uploadSpiffsImage(false)">
            Upload spiffs.bin Only
          </button>
        </div>
      </div>

      <!-- PROGRESS & LOG -->
      <div class="progress-box">
        <div class="step-hdr" style="margin-bottom:8px;">
          <h2 class="step-title">Upgrade Progress &amp; Activity Log</h2>
        </div>
        <div class="bar-bg"><div class="bar-fill" id="progBar"></div></div>
        <div class="status-line">
          <span id="progText">Ready</span>
          <span id="progPct">0%</span>
        </div>
        <div class="log-area" id="logBox"></div>
      </div>
    </div>

    <div class="right-col">
      <!-- STEP 3: STATIC FILES + REBUILD SETTINGS.JSON -->
      <div class="card" style="margin-bottom:0;">
        <div class="step-hdr">
          <h2 class="step-title">Step 3 — Static Files &amp; Rebuild settings.json</h2>
          <span class="step-sub">6 Web UI files + 18-key settings.json</span>
        </div>

        <div class="chips" style="margin-top:0; margin-bottom:10px;">
          <span style="font-size:12px; color:#8b96a8; align-self:center;">Presets:</span>
          <button type="button" class="chip" onclick="applyPreset('default')">Default (16/17/34)</button>
          <button type="button" class="chip" onclick="applyPreset('raptor')">Raptor (17/16/32)</button>
          <button type="button" class="chip" onclick="applyPreset('landrover')">Landrover (16/17/34)</button>
        </div>

        <div class="form-grid">
          <div class="field">
            <label>Device Name</label>
            <input type="text" id="cfg_DeviceName" value="esp32-AirUpDownController" oninput="refreshJsonPreview()" />
          </div>
          <div class="field">
            <label>Wi-Fi SSID</label>
            <input type="text" id="cfg_wifissid" value="" placeholder="Home / Vehicle Wi-Fi" oninput="refreshJsonPreview()" />
          </div>
          <div class="field">
            <label>Wi-Fi Password</label>
            <div class="pwd-wrap">
              <input type="password" id="cfg_wifipassword" value="" oninput="refreshJsonPreview()" />
              <button type="button" class="btn-tiny" onclick="togglePwd('cfg_wifipassword', this)">Show</button>
            </div>
          </div>
          <div class="field">
            <label>Hotspot AP Password (min 8)</label>
            <div class="pwd-wrap">
              <input type="password" id="cfg_AccessPointPassword" value="12345678" oninput="refreshJsonPreview()" />
              <button type="button" class="btn-tiny" onclick="togglePwd('cfg_AccessPointPassword', this)">Show</button>
            </div>
          </div>
          <div class="field">
            <label>Relay Compressor Pin</label>
            <input type="number" id="cfg_RelayCompressorPin" value="16" oninput="refreshJsonPreview()" />
          </div>
          <div class="field">
            <label>Relay Free (Deflate) Pin</label>
            <input type="number" id="cfg_RelayFreePin" value="17" oninput="refreshJsonPreview()" />
          </div>
          <div class="field">
            <label>Sensor Pin (ADC1: 32–39)</label>
            <input type="number" id="cfg_SensorPin" value="34" oninput="refreshJsonPreview()" />
          </div>
          <div class="field">
            <label>Calc Method (1=ADC, 2=Volt)</label>
            <select id="cfg_PressureCalcMethod" onchange="refreshJsonPreview()">
              <option value="1">1 — ADC Counts</option>
              <option value="2" selected>2 — Voltage Divider</option>
            </select>
          </div>
          <div class="field">
            <label>Sensor Value @ 0 PSI</label>
            <input type="number" step="0.001" id="cfg_SensorValueAt0PSI" value="0.120" oninput="refreshJsonPreview()" />
          </div>
          <div class="field">
            <label>Sensor Value @ Full Scale</label>
            <input type="number" step="0.001" id="cfg_SensorValueAt50PSI" value="1.460" oninput="refreshJsonPreview()" />
          </div>
          <div class="field">
            <label>Sensor Full Scale (PSI)</label>
            <input type="number" step="0.1" id="cfg_SensorFullScalePSI" value="50" oninput="refreshJsonPreview()" />
          </div>
          <div class="field">
            <label>Precision (PSI)</label>
            <input type="number" step="0.05" id="cfg_Precision" value="0.3" oninput="refreshJsonPreview()" />
          </div>
          <div class="field">
            <label>Max Status Time (sec)</label>
            <input type="number" id="cfg_MaxSecForStatus" value="30" oninput="refreshJsonPreview()" />
          </div>
          <div class="field">
            <label>Measure Wait (sec)</label>
            <input type="number" step="0.1" id="cfg_MessureWait" value="1.0" oninput="refreshJsonPreview()" />
          </div>
          <div class="field">
            <label>Max Pressure (PSI)</label>
            <input type="number" step="0.5" id="cfg_MaxPressurePSI" value="50" oninput="refreshJsonPreview()" />
          </div>
          <div class="field">
            <label>Max Valve Open (sec)</label>
            <input type="number" id="cfg_MaxValveOpenSec" value="120" oninput="refreshJsonPreview()" />
          </div>
          <div class="field">
            <label>BLE Access Code (optional)</label>
            <div class="pwd-wrap">
              <input type="password" id="cfg_AccessCode" value="" placeholder="Unlocked if empty" oninput="refreshJsonPreview()" />
              <button type="button" class="btn-tiny" onclick="togglePwd('cfg_AccessCode', this)">Show</button>
            </div>
          </div>
          <div class="field">
            <label>Simulation Mode</label>
            <select id="cfg_SimulationMode" onchange="refreshJsonPreview()">
              <option value="false" selected>Off (Real Sensor)</option>
              <option value="true">On (Simulation)</option>
            </select>
          </div>
        </div>

        <details class="json-preview" open>
          <summary>Preview Rebuilt settings.json (18 validated keys)</summary>
          <pre id="jsonPreviewBox"></pre>
        </details>

        <button type="button" id="btnStaticAndSettings" class="btn btn-green btn-block" style="margin-top:12px;" onclick="uploadStaticAndRebuiltSettings(true)">
          Upload Static Files + Rebuilt settings.json
        </button>
        <div class="row" style="margin-top:8px;">
          <button type="button" id="btnSettingsOnly" class="btn btn-secondary" style="flex:1;" onclick="uploadRebuiltSettingsOnly()">
            Deploy Rebuilt settings.json Only
          </button>
          <button type="button" id="btnStaticOnly" class="btn btn-secondary" style="flex:1;" onclick="uploadStaticAndRebuiltSettings(false)">
            Upload Static Files Only
          </button>
        </div>
      </div>
    </div>
  </div>

  <div class="footer">&copy; 2026 Nir Azulay — Open-Source DIY Tire Air Pressure System</div>
</div>

<script>
const PACKAGE = {manifest_js};

function b64ToBlob(b64, mime) {{
  const bin = atob(b64);
  const bytes = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; i++) bytes[i] = bin.charCodeAt(i);
  return new Blob([bytes], {{ type: mime || "application/octet-stream" }});
}}

function logMsg(msg) {{
  const box = document.getElementById("logBox");
  const ts = new Date().toLocaleTimeString();
  box.textContent += "[" + ts + "] " + msg + "\\n";
  box.scrollTop = box.scrollHeight;
}}

function setProgress(pct, text) {{
  const clamped = Math.max(0, Math.min(100, Math.round(pct)));
  document.getElementById("progBar").style.width = clamped + "%";
  document.getElementById("progPct").textContent = clamped + "%";
  if (text) document.getElementById("progText").textContent = text;
}}

function getIp() {{
  let ip = document.getElementById("targetIp").value.trim();
  return ip.replace(/^https?:\\/\\//, "").replace(/\\/.*$/, "");
}}

function getBaseUrl() {{
  return "http://" + getIp();
}}

function selectIp(ip) {{
  document.getElementById("targetIp").value = ip;
  readFromDevice();
}}

function openWebUI() {{
  window.open(getBaseUrl() + "/", "_blank");
}}

function downloadDeviceSettingsJson() {{
  const url = getBaseUrl() + "/settings.json";
  logMsg("Opening " + url + " to download settings.json from device...");
  window.open(url, "_blank");
}}

function onLoadSettingsFile(input) {{
  if (!input.files || !input.files[0]) return;
  const file = input.files[0];
  const reader = new FileReader();
  reader.onload = (e) => {{
    try {{
      const obj = JSON.parse(e.target.result);
      applySettingsJsonObject(obj, file.name);
    }} catch (err) {{
      logMsg("ERROR parsing JSON file: " + err.message);
    }}
    input.value = "";
  }};
  reader.readAsText(file);
}}

function applySettingsJsonObject(obj, sourceLabel) {{
  if (!obj || typeof obj !== "object") return false;
  if (obj.DeviceName !== undefined) document.getElementById("cfg_DeviceName").value = obj.DeviceName;
  if (obj.wifissid !== undefined) document.getElementById("cfg_wifissid").value = obj.wifissid;
  if (obj.wifipassword !== undefined) document.getElementById("cfg_wifipassword").value = obj.wifipassword;
  if (obj.RelayCompressorPin !== undefined) document.getElementById("cfg_RelayCompressorPin").value = obj.RelayCompressorPin;
  if (obj.RelayFreePin !== undefined) document.getElementById("cfg_RelayFreePin").value = obj.RelayFreePin;
  if (obj.SensorPin !== undefined) document.getElementById("cfg_SensorPin").value = obj.SensorPin;
  if (obj.MaxSecForStatus !== undefined) document.getElementById("cfg_MaxSecForStatus").value = obj.MaxSecForStatus;
  if (obj.SensorValueAt0PSI !== undefined) document.getElementById("cfg_SensorValueAt0PSI").value = Number(obj.SensorValueAt0PSI).toFixed(3);
  if (obj.SensorValueAt50PSI !== undefined) document.getElementById("cfg_SensorValueAt50PSI").value = Number(obj.SensorValueAt50PSI).toFixed(3);
  if (obj.AccessCode !== undefined) {{
    document.getElementById("cfg_AccessCode").value = obj.AccessCode;
  }} else if (obj.BTPinCode !== undefined && obj.BLESecurityEnabled) {{
    document.getElementById("cfg_AccessCode").value = obj.BTPinCode;
  }}
  if (obj.MessureWait !== undefined) document.getElementById("cfg_MessureWait").value = obj.MessureWait;
  if (obj.Precision !== undefined) document.getElementById("cfg_Precision").value = obj.Precision;
  if (obj.SimulationMode !== undefined) {{
    document.getElementById("cfg_SimulationMode").value = (obj.SimulationMode === true || obj.SimulationMode === 1 || obj.SimulationMode === "1" || obj.SimulationMode === "true") ? "true" : "false";
  }}
  if (obj.PressureCalcMethod !== undefined) {{
    document.getElementById("cfg_PressureCalcMethod").value = String(obj.PressureCalcMethod);
  }} else {{
    // Auto-detect Method 2 if voltage values (< 5.0) are stored
    const v50 = parseFloat(document.getElementById("cfg_SensorValueAt50PSI").value);
    document.getElementById("cfg_PressureCalcMethod").value = (v50 > 0 && v50 <= 5.0) ? "2" : "1";
  }}
  if (obj.MaxPressurePSI !== undefined) document.getElementById("cfg_MaxPressurePSI").value = obj.MaxPressurePSI;
  if (obj.MaxValveOpenSec !== undefined) document.getElementById("cfg_MaxValveOpenSec").value = obj.MaxValveOpenSec;
  if (obj.SensorFullScalePSI !== undefined && parseFloat(obj.SensorFullScalePSI) > 0) {{
    document.getElementById("cfg_SensorFullScalePSI").value = obj.SensorFullScalePSI;
  }}
  if (obj.AccessPointPassword !== undefined && String(obj.AccessPointPassword).length >= 8) {{
    document.getElementById("cfg_AccessPointPassword").value = obj.AccessPointPassword;
  }}

  refreshJsonPreview();
  const rebuilt = buildSettingsObject();
  document.getElementById("connDot").className = "dot online";
  document.getElementById("connName").textContent = rebuilt.DeviceName + " (" + sourceLabel + ")";
  document.getElementById("connVer").textContent = "Pins: " + rebuilt.RelayCompressorPin + "/" + rebuilt.RelayFreePin + "/" + rebuilt.SensorPin + " | 0PSI=" + rebuilt.SensorValueAt0PSI + ", 50PSI=" + rebuilt.SensorValueAt50PSI;
  logMsg("✓ Populated UI and rebuilt clean 18-key settings.json from " + sourceLabel + " (" + rebuilt.DeviceName + ").");
  return true;
}}

function applySettingsValuesString(text, ip, optVer) {{
  const a = text.trim().split(";");
  if (a.length < 12) return false;
  document.getElementById("cfg_DeviceName").value = a[0] || "esp32-AirUpDownController";
  document.getElementById("cfg_wifissid").value = a[1] || "";
  if (a[2]) document.getElementById("cfg_wifipassword").value = a[2];
  document.getElementById("cfg_RelayCompressorPin").value = a[3] || 16;
  document.getElementById("cfg_RelayFreePin").value = a[4] || 17;
  document.getElementById("cfg_SensorPin").value = a[5] || 34;
  document.getElementById("cfg_SimulationMode").value = (a[6] === "1" || a[6] === "true") ? "true" : "false";
  document.getElementById("cfg_MaxSecForStatus").value = a[7] || 30;
  document.getElementById("cfg_SensorValueAt0PSI").value = a[8] || 0.120;
  document.getElementById("cfg_SensorValueAt50PSI").value = a[9] || 1.460;
  if (a[10] && a[10] !== "0" && a[10] !== "1" && a[10] !== "1234") {{
    document.getElementById("cfg_AccessCode").value = a[10];
  }}
  document.getElementById("cfg_MessureWait").value = a[11] || 1.0;
  if (a[12]) document.getElementById("cfg_Precision").value = a[12];
  if (a[13] === "1" || a[13] === "2") {{
    document.getElementById("cfg_PressureCalcMethod").value = a[13];
  }}
  const ver = optVer || a[15] || a[14] || "Unknown";
  if (a[16]) document.getElementById("cfg_MaxPressurePSI").value = a[16];
  if (a[17]) document.getElementById("cfg_MaxValveOpenSec").value = a[17];
  if (a[18] && parseFloat(a[18]) > 0) document.getElementById("cfg_SensorFullScalePSI").value = a[18];
  if (a[19]) document.getElementById("cfg_AccessPointPassword").value = a[19];

  refreshJsonPreview();
  document.getElementById("connDot").className = "dot online";
  document.getElementById("connName").textContent = a[0] + " (" + ip + ")";
  document.getElementById("connVer").textContent = "Firmware: " + ver + " | Pins: " + a[3] + "/" + a[4] + "/" + a[5];
  logMsg("✓ Loaded settings from '" + a[0] + "' (Firmware " + ver + ").");
  return true;
}}

function togglePwd(id, btn) {{
  const el = document.getElementById(id);
  if (el.type === "password") {{
    el.type = "text";
    btn.textContent = "Hide";
  }} else {{
    el.type = "password";
    btn.textContent = "Show";
  }}
}}

function setBusy(busy) {{
  ["btnFw", "btnSpiffsAndSettings", "btnSpiffsOnly", "btnStaticAndSettings", "btnSettingsOnly", "btnStaticOnly"].forEach(id => {{
    const el = document.getElementById(id);
    if (el) el.disabled = busy;
  }});
}}

function applyPreset(name) {{
  if (name === "raptor") {{
    document.getElementById("cfg_DeviceName").value = "Raptor-Air-MNG";
    document.getElementById("cfg_RelayCompressorPin").value = 17;
    document.getElementById("cfg_RelayFreePin").value = 16;
    document.getElementById("cfg_SensorPin").value = 32;
    document.getElementById("cfg_PressureCalcMethod").value = "2";
    document.getElementById("cfg_SensorValueAt0PSI").value = 0.321;
    document.getElementById("cfg_SensorValueAt50PSI").value = 3.140;
    document.getElementById("cfg_Precision").value = 0.3;
    document.getElementById("cfg_MaxSecForStatus").value = 30;
    document.getElementById("cfg_MessureWait").value = 1.0;
  }} else if (name === "landrover") {{
    document.getElementById("cfg_DeviceName").value = "LandroverAirBLE";
    document.getElementById("cfg_RelayCompressorPin").value = 16;
    document.getElementById("cfg_RelayFreePin").value = 17;
    document.getElementById("cfg_SensorPin").value = 34;
    document.getElementById("cfg_PressureCalcMethod").value = "2";
    document.getElementById("cfg_SensorValueAt0PSI").value = 0.120;
    document.getElementById("cfg_SensorValueAt50PSI").value = 1.460;
    document.getElementById("cfg_Precision").value = 0.3;
    document.getElementById("cfg_MaxSecForStatus").value = 30;
    document.getElementById("cfg_MessureWait").value = 1.0;
  }} else {{
    document.getElementById("cfg_DeviceName").value = "esp32-AirUpDownController";
    document.getElementById("cfg_RelayCompressorPin").value = 16;
    document.getElementById("cfg_RelayFreePin").value = 17;
    document.getElementById("cfg_SensorPin").value = 34;
    document.getElementById("cfg_PressureCalcMethod").value = "2";
    document.getElementById("cfg_SensorValueAt0PSI").value = 0.250;
    document.getElementById("cfg_SensorValueAt50PSI").value = 2.250;
    document.getElementById("cfg_Precision").value = 0.3;
    document.getElementById("cfg_MaxSecForStatus").value = 30;
    document.getElementById("cfg_MessureWait").value = 1.0;
  }}
  refreshJsonPreview();
  logMsg("Loaded preset '" + name + "' into settings.json builder.");
}}

function buildSettingsObject() {{
  const apPass = document.getElementById("cfg_AccessPointPassword").value.trim();
  let sensorPin = parseInt(document.getElementById("cfg_SensorPin").value, 10) || 34;
  if (sensorPin < 32 || sensorPin > 39) sensorPin = 34;

  return {{
    DeviceName: document.getElementById("cfg_DeviceName").value.trim() || "esp32-AirUpDownController",
    wifissid: document.getElementById("cfg_wifissid").value.trim(),
    wifipassword: document.getElementById("cfg_wifipassword").value,
    RelayCompressorPin: parseInt(document.getElementById("cfg_RelayCompressorPin").value, 10) || 16,
    RelayFreePin: parseInt(document.getElementById("cfg_RelayFreePin").value, 10) || 17,
    SensorPin: sensorPin,
    MaxSecForStatus: parseInt(document.getElementById("cfg_MaxSecForStatus").value, 10) || 30,
    SensorValueAt0PSI: parseFloat(parseFloat(document.getElementById("cfg_SensorValueAt0PSI").value || 0).toFixed(3)),
    SensorValueAt50PSI: parseFloat(parseFloat(document.getElementById("cfg_SensorValueAt50PSI").value || 0).toFixed(3)),
    AccessCode: document.getElementById("cfg_AccessCode").value.trim(),
    MessureWait: parseFloat(document.getElementById("cfg_MessureWait").value) || 1.0,
    Precision: parseFloat(document.getElementById("cfg_Precision").value) || 0.3,
    SimulationMode: document.getElementById("cfg_SimulationMode").value === "true",
    PressureCalcMethod: parseInt(document.getElementById("cfg_PressureCalcMethod").value, 10) || 2,
    MaxPressurePSI: parseFloat(document.getElementById("cfg_MaxPressurePSI").value) || 50.0,
    MaxValveOpenSec: parseInt(document.getElementById("cfg_MaxValveOpenSec").value, 10) || 120,
    SensorFullScalePSI: parseFloat(document.getElementById("cfg_SensorFullScalePSI").value) || 50.0,
    AccessPointPassword: apPass.length >= 8 ? apPass : "12345678"
  }};
}}

function refreshJsonPreview() {{
  const obj = buildSettingsObject();
  document.getElementById("jsonPreviewBox").textContent = JSON.stringify(obj, null, 2);
}}

async function readFromDevice() {{
  const ip = getIp();
  const baseUrl = "http://" + ip;
  logMsg("Reading settings from " + baseUrl + " ...");

  // 1. Try local helper proxy first (bypasses CORS on all firmware versions)
  try {{
    const proxyResp = await fetch("http://127.0.0.1:8765/api/read?ip=" + encodeURIComponent(ip), {{ cache: "no-store" }});
    if (proxyResp.ok) {{
      const data = await proxyResp.json();
      let loaded = false;
      if (data.settingsValues && data.settingsValues.indexOf(";") >= 0) {{
        loaded = applySettingsValuesString(data.settingsValues, ip);
      }}
      if (data.settingsJson && typeof data.settingsJson === "object") {{
        loaded = applySettingsJsonObject(data.settingsJson, ip + "/settings.json") || loaded;
      }}
      if (loaded) return;
    }}
  }} catch (_) {{}}

  // 2. Try direct fetch of /settings.json and /settingsValues from browser
  try {{
    const jsonResp = await fetch(baseUrl + "/settings.json", {{ cache: "no-store" }});
    if (jsonResp.ok) {{
      const obj = await jsonResp.json();
      if (applySettingsJsonObject(obj, ip + "/settings.json")) {{
        try {{
          const svResp = await fetch(baseUrl + "/settingsValues", {{ cache: "no-store" }});
          if (svResp.ok) applySettingsValuesString(await svResp.text(), ip);
        }} catch (_) {{}}
        return;
      }}
    }}
  }} catch (_) {{}}

  try {{
    const svResp = await fetch(baseUrl + "/settingsValues", {{ cache: "no-store" }});
    if (svResp.ok) {{
      const text = await svResp.text();
      if (applySettingsValuesString(text, ip)) return;
    }}
  }} catch (_) {{}}

  document.getElementById("connDot").className = "dot";
  document.getElementById("connName").textContent = "Browser CORS blocked direct read on " + ip;
  document.getElementById("connVer").textContent = "Click '1. Download settings.json' -> '2. Load settings.json File'";
  logMsg("! Browser blocked direct read from file:// because " + ip + " is on older firmware without CORS headers. Click '1. Download settings.json' then '2. Load settings.json File' (or open via Open_Field_Updater.command).");
}}

async function uploadBlobToDevice(baseUrl, endpoint, fieldName, filename, blob, onProgress) {{
  const formData = new FormData();
  formData.append(fieldName, blob, filename);

  let timer = null;
  let estPct = 3;
  if (onProgress) {{
    onProgress(estPct);
    const stepMs = blob.size > 100000 ? 450 : 80;
    timer = setInterval(() => {{
      if (estPct < 95) {{
        estPct += blob.size > 500000 ? 1.8 : 10;
        onProgress(Math.min(95, estPct));
      }}
    }}, stepMs);
  }}

  try {{
    // Using fetch with mode: "no-cors" and FormData sends a simple POST request
    // WITHOUT triggering an OPTIONS preflight (which older ESP32 firmware rejects with 404).
    await fetch(baseUrl + endpoint, {{
      method: "POST",
      mode: "no-cors",
      body: formData
    }});
    if (onProgress) onProgress(100);
  }} catch (err) {{
    // Note: ESP32 reboots immediately upon finishing /doUpdate, which may reset the final TCP ACK
    if (endpoint === "/doUpdate" && estPct >= 75) {{
      if (onProgress) onProgress(100);
      return;
    }}
    throw err;
  }} finally {{
    if (timer) clearInterval(timer);
  }}
}}

async function uploadFirmwareOnly() {{
  const baseUrl = getBaseUrl();
  setBusy(true);
  try {{
    const wasSimOff = document.getElementById("cfg_SimulationMode").value === "false";
    const connVerText = document.getElementById("connVer").textContent || "";
    const isOldFirmware = connVerText.indexOf("v0.1.") >= 0;

    if (isOldFirmware && wasSimOff) {{
      logMsg("Old v0.1 firmware detected with SimulationMode=Off — temporarily enabling SimulationMode=1 so sensor loop doesn't block OTA...");
      setProgress(5, "Preparing device (enabling non-blocking mode for OTA)...");
      try {{
        await fetch(baseUrl + "/updateSettings?SimulationMode=1", {{ mode: "no-cors" }});
      }} catch (_) {{}}
      await new Promise(r => setTimeout(r, 6500));
    }}

    logMsg("Uploading Firmware " + PACKAGE.version + " (firmware.bin) to " + baseUrl + "/doUpdate ...");
    const fwBlob = b64ToBlob(PACKAGE.firmwareB64, "application/octet-stream");
    await uploadBlobToDevice(baseUrl, "/doUpdate", "update", "firmware.bin", fwBlob, (pct) => {{
      setProgress(pct, "Uploading Firmware (" + PACKAGE.version + ")...");
    }});
    setProgress(100, "Firmware uploaded! Controller is rebooting (~7s)...");
    logMsg("✓ Firmware " + PACKAGE.version + " uploaded! Waiting 7s for reboot...");
    await new Promise(r => setTimeout(r, 7000));

    if (isOldFirmware && wasSimOff) {{
      logMsg("Restoring SimulationMode=Off...");
      try {{
        await fetch(baseUrl + "/updateSettings?SimulationMode=0", {{ mode: "no-cors" }});
      }} catch (_) {{}}
      await new Promise(r => setTimeout(r, 6000));
    }}
    await readFromDevice();
  }} catch (err) {{
    logMsg("ERROR: " + err.message);
    setProgress(0, "Error: " + err.message);
  }} finally {{
    setBusy(false);
  }}
}}

async function uploadSpiffsImage(restoreSettingsAfter) {{
  const baseUrl = getBaseUrl();
  setBusy(true);
  try {{
    const settingsObj = buildSettingsObject();
    logMsg("Uploading full SPIFFS image (spiffs.bin — " + Math.round(PACKAGE.spiffsSize / 1024) + " KB) to " + baseUrl + "/doUpdate ...");
    const spiffsBlob = b64ToBlob(PACKAGE.spiffsB64, "application/octet-stream");
    await uploadBlobToDevice(baseUrl, "/doUpdate", "update", "spiffs.bin", spiffsBlob, (pct) => {{
      setProgress(restoreSettingsAfter ? pct * 0.75 : pct, "Uploading spiffs.bin partition...");
    }});
    logMsg("✓ spiffs.bin uploaded! Waiting 6.5s for controller to reboot...");
    if (restoreSettingsAfter) {{
      setProgress(80, "Controller rebooting after spiffs.bin (waiting 6.5s)...");
      await new Promise(r => setTimeout(r, 6500));
      const jsonStr = JSON.stringify(settingsObj, null, 2);
      const jsonBlob = new Blob([jsonStr], {{ type: "application/json" }});
      setProgress(90, "Redeploying rebuilt settings.json...");
      await uploadBlobToDevice(baseUrl, "/doUpload", "file", "/settings.json", jsonBlob);
      logMsg("✓ Rebuilt settings.json redeployed! Triggering reboot...");
      fetch(baseUrl + "/reboot", {{ mode: "no-cors" }}).catch(() => {{}});
      setProgress(100, "spiffs.bin + rebuilt settings.json deployed!");
      setTimeout(() => readFromDevice(), 5500);
    }} else {{
      setProgress(100, "spiffs.bin uploaded! Controller is rebooting...");
      setTimeout(() => readFromDevice(), 6500);
    }}
  }} catch (err) {{
    logMsg("ERROR: " + err.message);
    setProgress(0, "Error: " + err.message);
  }} finally {{
    setBusy(false);
  }}
}}

async function uploadRebuiltSettingsOnly() {{
  const baseUrl = getBaseUrl();
  setBusy(true);
  try {{
    const settingsObj = buildSettingsObject();
    const jsonStr = JSON.stringify(settingsObj, null, 2);
    const jsonBlob = new Blob([jsonStr], {{ type: "application/json" }});
    logMsg("Uploading rebuilt settings.json (" + jsonStr.length + " bytes) to " + baseUrl + "/doUpload ...");
    setProgress(50, "Uploading rebuilt settings.json...");
    await uploadBlobToDevice(baseUrl, "/doUpload", "file", "/settings.json", jsonBlob);
    logMsg("✓ Rebuilt settings.json uploaded! Triggering reboot to apply...");
    fetch(baseUrl + "/reboot", {{ mode: "no-cors" }}).catch(() => {{}});
    setProgress(100, "settings.json deployed! Controller rebooting (~5s)...");
    setTimeout(() => readFromDevice(), 5500);
  }} catch (err) {{
    logMsg("ERROR: " + err.message);
    setProgress(0, "Error: " + err.message);
  }} finally {{
    setBusy(false);
  }}
}}

async function uploadStaticAndRebuiltSettings(includeSettingsJson) {{
  const baseUrl = getBaseUrl();
  setBusy(true);
  try {{
    const files = PACKAGE.staticFiles;
    const totalSteps = files.length + (includeSettingsJson ? 1 : 0);
    logMsg("Uploading " + files.length + " Web UI static files" + (includeSettingsJson ? " + rebuilt settings.json" : "") + " to " + baseUrl + "...");

    for (let i = 0; i < files.length; i++) {{
      const f = files[i];
      const pct = ((i + 1) / totalSteps) * 100;
      setProgress(pct, "Uploading (" + (i + 1) + "/" + totalSteps + "): " + f.name);
      const blob = b64ToBlob(f.b64, "application/octet-stream");
      await uploadBlobToDevice(baseUrl, "/doUpload", "file", "/" + f.name, blob);
      logMsg("  ✓ Uploaded " + f.name + " (" + f.size + " B)");
      await new Promise(r => setTimeout(r, 300));
    }}

    if (includeSettingsJson) {{
      const settingsObj = buildSettingsObject();
      const jsonStr = JSON.stringify(settingsObj, null, 2);
      const jsonBlob = new Blob([jsonStr], {{ type: "application/json" }});
      setProgress(98, "Uploading rebuilt settings.json...");
      await uploadBlobToDevice(baseUrl, "/doUpload", "file", "/settings.json", jsonBlob);
      logMsg("  ✓ Uploaded rebuilt settings.json (" + jsonStr.length + " B)");
      logMsg("Triggering controller reboot to apply new settings.json...");
      fetch(baseUrl + "/reboot", {{ mode: "no-cors" }}).catch(() => {{}});
      setProgress(100, "All static files + settings.json deployed! Rebooting...");
      setTimeout(() => readFromDevice(), 5500);
    }} else {{
      setProgress(100, "All " + files.length + " Web UI static files deployed!");
      logMsg("✓ All Web UI static files uploaded.");
    }}
  }} catch (err) {{
    logMsg("ERROR: " + err.message);
    setProgress(0, "Error: " + err.message);
  }} finally {{
    setBusy(false);
  }}
}}

document.addEventListener("DOMContentLoaded", () => {{
  document.getElementById("lblFwSize").textContent =
    "firmware.bin (" + (PACKAGE.firmwareSize / 1024).toFixed(0) + " KB) + spiffs.bin (" + (PACKAGE.spiffsSize / 1024).toFixed(0) + " KB)";
  refreshJsonPreview();
  logMsg("Offline Field Updater ready — Firmware " + PACKAGE.version + " + spiffs.bin + " + PACKAGE.staticFiles.length + " Web UI files.");
  readFromDevice();
}});
</script>
</body>
</html>
"""

    html_out = DIST_DIR / "OfflineFieldUpdater.html"
    html_out.write_text(html_content, encoding="utf-8")

    # Create a macOS double-clickable .command launcher that starts a tiny local CORS-free helper & opens the UI
    cmd_launcher = DIST_DIR / "Open_Field_Updater.command"
    cmd_launcher.write_text(
        '''#!/usr/bin/env python3
import http.server
import json
import socketserver
import threading
import urllib.parse
import urllib.request
import webbrowser
from pathlib import Path

DIR = Path(__file__).resolve().parent
PORT = 8765

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(DIR), **kwargs)

    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def do_GET(self):
        if self.path == "/" or self.path.startswith("/?"):
            self.path = "/OfflineFieldUpdater.html"
            return super().do_GET()
        if self.path.startswith("/api/read"):
            qs = urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
            ip = qs.get("ip", ["192.168.4.1"])[0]
            result = {"settingsValues": None, "settingsJson": None}
            try:
                with urllib.request.urlopen(f"http://{ip}/settingsValues", timeout=3.5) as r:
                    result["settingsValues"] = r.read().decode("utf-8", errors="replace").strip()
            except Exception:
                pass
            try:
                with urllib.request.urlopen(f"http://{ip}/settings.json", timeout=3.5) as r:
                    raw_json = r.read().decode("utf-8", errors="replace").strip()
                    result["settingsJson"] = json.loads(raw_json)
            except Exception:
                pass

            if result["settingsValues"] or result["settingsJson"]:
                self.send_response(200)
                self.send_header("Content-Type", "application/json; charset=utf-8")
                self.end_headers()
                self.wfile.write(json.dumps(result).encode("utf-8"))
            else:
                self.send_response(502)
                self.end_headers()
                self.wfile.write(b"Device unreachable")
            return
        return super().do_GET()

    def log_message(self, format, *args):
        pass

if __name__ == "__main__":
    socketserver.TCPServer.allow_reuse_address = True
    try:
        httpd = socketserver.TCPServer(("127.0.0.1", PORT), Handler)
        url = f"http://127.0.0.1:{PORT}/OfflineFieldUpdater.html"
        print(f"Field Updater UI running at {url}")
        threading.Timer(0.4, lambda: webbrowser.open(url)).start()
        httpd.serve_forever()
    except OSError:
        webbrowser.open(f"http://127.0.0.1:{PORT}/OfflineFieldUpdater.html")
''',
        encoding="utf-8",
    )
    os.chmod(cmd_launcher, 0o755)

    print(f"Built Independent Offline Field Updater ({version_str}):")
    print(f"  - {html_out} ({html_out.stat().st_size // 1024} KB)")
    print(f"  - {DIST_DIR / 'firmware.bin'} ({fw_bytes.__len__() // 1024} KB)")
    print(f"  - {DIST_DIR / 'spiffs.bin'} ({spiffs_bytes.__len__() // 1024} KB)")
    print(f"  - {cmd_launcher}")


if __name__ == "__main__":
    run_pio_flag = "--build" in sys.argv
    build_package(run_pio=run_pio_flag)
