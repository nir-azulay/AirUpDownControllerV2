function onloadPage() {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/settingsValues?t=" + Date.now(), true);
  xhr.onload = function () {
    if (xhr.status === 200) {
      fillValues(xhr.responseText);
    }
  };
  xhr.send();
}

function fillValues(values) {
  var aValues = values.split(";");
  if (aValues.length < 15) return;
  document.getElementById("txt_DeviceName").value = aValues[0] || "";
  document.getElementById("txt_wifissid").value = aValues[1] || "";
  document.getElementById("txt_wifipassword").value = aValues[2] || "";
  document.getElementById("txt_RelayCompressorPin").value = aValues[3] || "16";
  document.getElementById("txt_RelayFreePin").value = aValues[4] || "17";
  document.getElementById("txt_SensorPin").value = aValues[5] || "34";
  document.getElementById("chk_SimulationMode").checked = (aValues[6] === "1" || aValues[6] === "true");
  document.getElementById("txt_MaxSecForStatus").value = aValues[7] || "10";
  document.getElementById("txt_SensorValueAt0PSI").value = aValues[8] || "410.000";
  document.getElementById("txt_SensorValueAt50PSI").value = aValues[9] || "3400.000";
  document.getElementById("txt_AccessCode").value = aValues[10] || "";
  document.getElementById("txt_AccessCode").placeholder = (aValues[10] && aValues[10].length > 0) ? "Access code is set" : "No code set (unlocked)";
  document.getElementById("chk_ClearAccessCode").checked = false;
  document.getElementById("txt_MessureWait").value = aValues[11] || "1.00";
  document.getElementById("txt_Precision").value = aValues[12] || "0.50";
  document.getElementById("txt_PressureCalcMethod").value = aValues[13] || "1";
  document.getElementById("lbl_version").innerHTML = aValues[15] || "-";
  document.getElementById("txt_MaxPressurePSI").value = (aValues[16] !== undefined && aValues[16] !== "") ? aValues[16] : "50.0";
  document.getElementById("txt_MaxValveOpenSec").value = (aValues[17] !== undefined && aValues[17] !== "") ? aValues[17] : "120";
  document.getElementById("txt_SensorFullScalePSI").value = (aValues[18] !== undefined && aValues[18] !== "") ? aValues[18] : "50.0";
  document.getElementById("txt_AccessPointPassword").value = aValues[19] || "12345678";
}

function saveSettings() {
  var clearCode = document.getElementById("chk_ClearAccessCode").checked;
  var codeVal = clearCode ? "" : document.getElementById("txt_AccessCode").value;

  var params = new URLSearchParams({
    DeviceName: document.getElementById("txt_DeviceName").value,
    wifissid: document.getElementById("txt_wifissid").value,
    wifipassword: document.getElementById("txt_wifipassword").value,
    RelayCompressorPin: document.getElementById("txt_RelayCompressorPin").value,
    RelayFreePin: document.getElementById("txt_RelayFreePin").value,
    SensorPin: document.getElementById("txt_SensorPin").value,
    MaxSecForStatus: document.getElementById("txt_MaxSecForStatus").value,
    SensorValueAt0PSI: document.getElementById("txt_SensorValueAt0PSI").value,
    SensorValueAt50PSI: document.getElementById("txt_SensorValueAt50PSI").value,
    AccessCode: codeVal,
    ClearAccessCode: clearCode ? "true" : "false",
    MessureWait: document.getElementById("txt_MessureWait").value,
    Precision: document.getElementById("txt_Precision").value,
    PressureCalcMethod: document.getElementById("txt_PressureCalcMethod").value,
    MaxPressurePSI: document.getElementById("txt_MaxPressurePSI").value,
    MaxValveOpenSec: document.getElementById("txt_MaxValveOpenSec").value,
    SensorFullScalePSI: document.getElementById("txt_SensorFullScalePSI").value,
    AccessPointPassword: document.getElementById("txt_AccessPointPassword").value,
    SimulationMode: document.getElementById("chk_SimulationMode").checked ? "true" : "false"
  });

  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/updateSettings?" + params.toString() + "&t=" + Date.now(), true);
  xhr.onload = function () {
    setTimeout(function () { location.href = "/"; }, 2000);
  };
  xhr.send();
}
