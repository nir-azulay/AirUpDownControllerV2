#!/usr/bin/env python3
"""
Build the Offline Field Updater Package (`ble/dist/`):
1. Compiles the latest ESP32 firmware (`pio run`) if `--build` is passed or `firmware.bin` is missing.
2. Embeds `.pio/build/esp32/firmware.bin` and the 6 self-contained Web UI static files
   (`index.html`, `configurations.html`, `update.html`, `fs.html`, `about.html`, `profile.jpg`)
   into a single, compact, zero-dependency HTML application:
     - `ble/dist/OfflineFieldUpdater.html`
     - `ble/dist/Open_Field_Updater.command` (double-click launcher for macOS)
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
        print("Building ESP32 firmware with PlatformIO (`pio run`)...")
        subprocess.run(["pio", "run"], cwd=str(ROOT_DIR), check=True)

    if not FIRMWARE_BIN.exists():
        raise FileNotFoundError(f"Firmware binary not found at {FIRMWARE_BIN}")

    version_str = get_version_short()
    build_time = datetime.now().strftime("%Y-%m-%d %H:%M")
    fw_bytes = FIRMWARE_BIN.read_bytes()
    fw_b64 = base64.b64encode(fw_bytes).decode("ascii")

    if PAYLOAD_DIR.exists():
        shutil.rmtree(PAYLOAD_DIR)
    DIST_DIR.mkdir(parents=True, exist_ok=True)
    PAYLOAD_DIR.mkdir(parents=True, exist_ok=True)

    shutil.copy2(FIRMWARE_BIN, PAYLOAD_DIR / "firmware.bin")

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
  font-size: 13px;
  margin: 0;
  padding: 16px 12px 32px;
}}
.app-shell {{
  max-width: 520px;
  margin: 0 auto;
}}
.top-bar {{
  display: flex;
  align-items: center;
  justify-content: space-between;
  background: #181c24;
  border: 1px solid #262c38;
  border-radius: 10px;
  padding: 12px 16px;
  margin-bottom: 12px;
}}
.top-title h1 {{
  font-size: 17px;
  margin: 0 0 2px;
  color: #fff;
}}
.top-title p {{
  margin: 0;
  font-size: 11px;
  color: #9aa5b4;
}}
.ver-pill {{
  background: rgba(226, 148, 37, 0.18);
  color: #e29425;
  border: 1px solid rgba(226, 148, 37, 0.45);
  border-radius: 999px;
  padding: 4px 10px;
  font-size: 11px;
  font-weight: 700;
  white-space: nowrap;
}}
.card {{
  background: #181c24;
  border: 1px solid #262c38;
  border-radius: 10px;
  padding: 14px 16px;
  margin-bottom: 12px;
  box-shadow: 0 4px 14px rgba(0, 0, 0, 0.3);
}}
.step-hdr {{
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 10px;
}}
.step-title {{
  font-size: 13px;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  color: #e29425;
  margin: 0;
}}
.step-sub {{
  font-size: 11px;
  color: #8b96a8;
}}
.ap-guide {{
  background: #12161d;
  border-left: 3px solid #00acc1;
  border-radius: 6px;
  padding: 8px 12px;
  margin-bottom: 10px;
  font-size: 12px;
  color: #cfd6e0;
  line-height: 1.45;
}}
.ap-guide strong {{ color: #fff; }}
.ap-guide code {{
  background: #222834;
  color: #e29425;
  padding: 1px 5px;
  border-radius: 4px;
  font-size: 11px;
}}
.row {{
  display: flex;
  gap: 8px;
  align-items: center;
}}
.input-ip {{
  flex: 1;
  height: 36px;
  background: #101318;
  border: 1px solid #323a4a;
  border-radius: 6px;
  color: #fff;
  padding: 0 10px;
  font-size: 14px;
  font-weight: 600;
}}
.chips {{
  display: flex;
  gap: 6px;
  margin-top: 8px;
  flex-wrap: wrap;
}}
.chip {{
  background: #222834;
  color: #b8c2d0;
  border: 1px solid #323a4a;
  border-radius: 5px;
  padding: 4px 8px;
  font-size: 11px;
  font-weight: 600;
  cursor: pointer;
}}
.chip:hover {{
  border-color: #e29425;
  color: #fff;
}}
.device-banner {{
  margin-top: 10px;
  padding: 8px 10px;
  border-radius: 6px;
  background: #101318;
  border: 1px solid #252b37;
  display: flex;
  align-items: center;
  justify-content: space-between;
  font-size: 12px;
}}
.dot {{
  display: inline-block;
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: #6c7686;
  margin-right: 6px;
}}
.dot.online {{ background: #2ecc71; box-shadow: 0 0 6px #2ecc71; }}
.btn {{
  background: #00acc1;
  color: #fff;
  border: none;
  border-radius: 6px;
  padding: 9px 14px;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.4px;
  text-transform: uppercase;
  cursor: pointer;
  transition: opacity 0.15s;
  white-space: nowrap;
}}
.btn:hover:enabled {{ opacity: 0.9; }}
.btn:disabled {{ background: #2d3442; color: #6c7686; cursor: default; }}
.btn-orange {{ background: #e29425; color: #101318; }}
.btn-green {{ background: #27ae60; color: #fff; }}
.btn-secondary {{ background: #2d3545; color: #dde3ea; }}
.btn-block {{ width: 100%; padding: 11px; font-size: 13px; }}
.form-grid {{
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 8px 10px;
  margin-top: 10px;
}}
.field {{
  display: flex;
  flex-direction: column;
}}
.field.full {{
  grid-column: span 2;
}}
.field label {{
  font-size: 11px;
  color: #9aa5b4;
  margin-bottom: 3px;
  font-weight: 600;
}}
.field input, .field select {{
  height: 32px;
  background: #101318;
  border: 1px solid #303846;
  border-radius: 5px;
  color: #fff;
  padding: 0 8px;
  font-size: 12px;
}}
.pwd-wrap {{
  display: flex;
  gap: 4px;
}}
.pwd-wrap input {{ flex: 1; }}
.btn-tiny {{
  background: #283040;
  color: #cfd6e0;
  border: 1px solid #384357;
  border-radius: 5px;
  padding: 0 8px;
  font-size: 11px;
  cursor: pointer;
}}
details.json-preview {{
  margin-top: 10px;
  background: #101318;
  border: 1px solid #252b37;
  border-radius: 6px;
  padding: 6px 10px;
}}
details.json-preview summary {{
  cursor: pointer;
  font-size: 11px;
  font-weight: 600;
  color: #9aa5b4;
}}
details.json-preview pre {{
  margin: 8px 0 2px;
  font-size: 11px;
  color: #88c0d0;
  overflow-x: auto;
}}
.progress-box {{
  background: #101318;
  border: 1px solid #262c38;
  border-radius: 8px;
  padding: 10px 12px;
}}
.bar-bg {{
  background: #252b37;
  height: 10px;
  border-radius: 5px;
  overflow: hidden;
  margin-bottom: 6px;
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
  font-size: 12px;
  font-weight: 600;
  color: #d8dee9;
}}
.log-area {{
  margin-top: 8px;
  background: #0b0d11;
  border: 1px solid #222834;
  border-radius: 6px;
  padding: 8px 10px;
  font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
  font-size: 11px;
  color: #a3be8c;
  max-height: 130px;
  overflow-y: auto;
  white-space: pre-wrap;
}}
.footer {{
  text-align: center;
  font-size: 11px;
  color: #6c7686;
  margin-top: 12px;
}}
</style>
</head>
<body>
<div class="app-shell">
  <div class="top-bar">
    <div class="top-title">
      <h1>ESP32 Offline Field Updater</h1>
      <p>Direct AP &amp; Local Wi-Fi Upgrade Tool (No Internet Needed)</p>
    </div>
    <span class="ver-pill" id="lblPkgVer">{version_str}</span>
  </div>

  <!-- STEP 1: CONNECT VIA AP -->
  <div class="card">
    <div class="step-hdr">
      <h2 class="step-title">Step 1 — Connect to Device AP</h2>
      <span class="step-sub">Wi-Fi Hotspot or LAN</span>
    </div>
    <div class="ap-guide">
      1. On your laptop/phone, join the controller's Wi-Fi AP (e.g. <strong>esp32-AirUpDownController</strong> or vehicle name, password <code>12345678</code>).<br>
      2. Keep IP as <code>192.168.4.1</code> (or pick a LAN IP) and click <strong>Read Device</strong>.
    </div>
    <div class="row">
      <input type="text" id="targetIp" class="input-ip" value="192.168.4.1" />
      <button type="button" class="btn" onclick="readFromDevice()">Read Device</button>
      <button type="button" class="btn btn-secondary" onclick="openWebUI()">Open UI</button>
    </div>
    <div class="chips">
      <button type="button" class="chip" onclick="selectIp('192.168.4.1')">AP Hotspot (192.168.4.1)</button>
      <button type="button" class="chip" onclick="selectIp('192.168.10.161')">Raptor (.161)</button>
      <button type="button" class="chip" onclick="selectIp('192.168.10.134')">Landrover (.134)</button>
      <button type="button" class="chip" onclick="selectIp('192.168.10.153')">TestBench (.153)</button>
    </div>
    <div class="device-banner">
      <div><span class="dot" id="connDot"></span><strong id="connName">Not connected yet</strong></div>
      <div id="connVer" style="color:#8b96a8;">Click "Read Device" to load current config</div>
    </div>
  </div>

  <!-- STEP 2: UPLOAD FIRMWARE -->
  <div class="card">
    <div class="step-hdr">
      <h2 class="step-title">Step 2 — Firmware Upgrade</h2>
      <span class="step-sub" id="lblFwSize">1.66 MB embedded</span>
    </div>
    <button type="button" id="btnFw" class="btn btn-orange btn-block" onclick="uploadFirmwareOnly()">
      Upload Firmware ({version_str})
    </button>
  </div>

  <!-- STEP 3: STATIC FILES + REBUILD SETTINGS.JSON -->
  <div class="card">
    <div class="step-hdr">
      <h2 class="step-title">Step 3 — Static Files &amp; Rebuild settings.json</h2>
      <span class="step-sub">6 Web UI files + settings.json</span>
    </div>

    <div class="chips" style="margin-top:0; margin-bottom:8px;">
      <span style="font-size:11px; color:#8b96a8; align-self:center;">Load Preset:</span>
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

    <details class="json-preview">
      <summary>Preview Rebuilt settings.json (18 validated keys)</summary>
      <pre id="jsonPreviewBox"></pre>
    </details>

    <button type="button" id="btnStaticAndSettings" class="btn btn-green btn-block" style="margin-top:10px;" onclick="uploadStaticAndRebuiltSettings(true)">
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

  <!-- PROGRESS & LOG -->
  <div class="progress-box">
    <div class="bar-bg"><div class="bar-fill" id="progBar"></div></div>
    <div class="status-line">
      <span id="progText">Ready</span>
      <span id="progPct">0%</span>
    </div>
    <div class="log-area" id="logBox"></div>
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

function getBaseUrl() {{
  let ip = document.getElementById("targetIp").value.trim();
  ip = ip.replace(/^https?:\\/\\//, "").replace(/\\/.*$/, "");
  return "http://" + ip;
}}

function selectIp(ip) {{
  document.getElementById("targetIp").value = ip;
  readFromDevice();
}}

function openWebUI() {{
  window.open(getBaseUrl() + "/", "_blank");
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
  ["btnFw", "btnStaticAndSettings", "btnSettingsOnly", "btnStaticOnly"].forEach(id => {{
    document.getElementById(id).disabled = busy;
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
  const ip = document.getElementById("targetIp").value.trim().replace(/^https?:\\/\\//, "").replace(/\\/.*$/, "");
  const baseUrl = "http://" + ip;
  logMsg("Reading current settings from " + baseUrl + " ...");
  let text = "";
  try {{
    const resp = await fetch(baseUrl + "/settingsValues", {{ cache: "no-store" }});
    text = (await resp.text()).trim();
  }} catch (directErr) {{
    try {{
      const proxyResp = await fetch("http://127.0.0.1:8765/api/read?ip=" + encodeURIComponent(ip), {{ cache: "no-store" }});
      if (proxyResp.ok) {{
        text = (await proxyResp.text()).trim();
      }}
    }} catch (_) {{}}
  }}

  if (text && text.indexOf(";") >= 0) {{
    const a = text.split(";");
    if (a.length >= 13) {{
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
      document.getElementById("cfg_Precision").value = a[12] || 0.3;
      if (a[13] === "1" || a[13] === "2") {{
        document.getElementById("cfg_PressureCalcMethod").value = a[13];
      }}
      const ver = a[15] || a[14] || "Unknown";
      if (a[16]) document.getElementById("cfg_MaxPressurePSI").value = a[16];
      if (a[17]) document.getElementById("cfg_MaxValveOpenSec").value = a[17];
      if (a[18] && parseFloat(a[18]) > 0) document.getElementById("cfg_SensorFullScalePSI").value = a[18];
      if (a[19]) document.getElementById("cfg_AccessPointPassword").value = a[19];

      document.getElementById("connDot").className = "dot online";
      document.getElementById("connName").textContent = a[0] + " (" + ip + ")";
      document.getElementById("connVer").textContent = "Firmware: " + ver + " | Pins: " + a[3] + "/" + a[4] + "/" + a[5];
      refreshJsonPreview();
      logMsg("✓ Loaded settings from '" + a[0] + "' (Firmware " + ver + ").");
      return;
    }}
  }}

  document.getElementById("connDot").className = "dot";
  document.getElementById("connName").textContent = "Ready to Upload to " + ip;
  document.getElementById("connVer").textContent = "Select a preset or edit fields below, then click Upload";
  logMsg("! Could not read /settingsValues from " + ip + " (if on older firmware without CORS, launch via Open_Field_Updater.command). Direct Firmware & Static uploads work from any browser.");
}}

function uploadBlobToDevice(baseUrl, endpoint, fieldName, filename, blob, onProgress) {{
  return new Promise((resolve, reject) => {{
    const xhr = new XMLHttpRequest();
    const formData = new FormData();
    formData.append(fieldName, blob, filename);

    let maxPct = 0;
    xhr.upload.addEventListener("progress", (evt) => {{
      if (evt.lengthComputable && onProgress) {{
        maxPct = (evt.loaded / evt.total) * 100;
        onProgress(maxPct);
      }}
    }});
    xhr.addEventListener("load", () => {{
      if (xhr.status >= 200 && xhr.status < 400) {{
        resolve();
      }} else {{
        reject(new Error("HTTP " + xhr.status));
      }}
    }});
    xhr.addEventListener("error", () => {{
      if (endpoint === "/doUpload" || maxPct >= 95) {{
        resolve();
      }} else {{
        reject(new Error("Upload interrupted at " + Math.round(maxPct) + "%"));
      }}
    }});
    xhr.open("POST", baseUrl + endpoint);
    xhr.send(formData);
  }});
}}

async function uploadFirmwareOnly() {{
  const baseUrl = getBaseUrl();
  setBusy(true);
  try {{
    logMsg("Uploading Firmware " + PACKAGE.version + " to " + baseUrl + "/doUpdate ...");
    const fwBlob = b64ToBlob(PACKAGE.firmwareB64, "application/octet-stream");
    await uploadBlobToDevice(baseUrl, "/doUpdate", "update", "firmware.bin", fwBlob, (pct) => {{
      setProgress(pct, "Uploading Firmware (" + PACKAGE.version + ")...");
    }});
    setProgress(100, "Firmware uploaded! Controller is rebooting (~6s)...");
    logMsg("✓ Firmware " + PACKAGE.version + " uploaded! Waiting 6s for reboot...");
    setTimeout(() => readFromDevice(), 6500);
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
  document.getElementById("lblFwSize").textContent = (PACKAGE.firmwareSize / 1024).toFixed(0) + " KB embedded";
  refreshJsonPreview();
  logMsg("Offline Field Updater ready — Firmware " + PACKAGE.version + " + " + PACKAGE.staticFiles.length + " Web UI files.");
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
        super().end_headers()

    def do_GET(self):
        if self.path == "/" or self.path.startswith("/?"):
            self.path = "/OfflineFieldUpdater.html"
            return super().do_GET()
        if self.path.startswith("/api/read"):
            qs = urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
            ip = qs.get("ip", ["192.168.4.1"])[0]
            try:
                with urllib.request.urlopen(f"http://{ip}/settingsValues", timeout=4) as r:
                    data = r.read()
                self.send_response(200)
                self.send_header("Content-Type", "text/plain; charset=utf-8")
                self.end_headers()
                self.wfile.write(data)
            except Exception as e:
                self.send_response(502)
                self.end_headers()
                self.wfile.write(str(e).encode("utf-8"))
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

    print(f"Built Offline Field Updater ({version_str}):")
    print(f"  - {html_out} ({html_out.stat().st_size // 1024} KB)")
    print(f"  - {cmd_launcher}")


if __name__ == "__main__":
    run_pio_flag = "--build" in sys.argv
    build_package(run_pio=run_pio_flag)
