{
    function onloadPage() 
    {
      fillValues(document.getElementById("txt_values").value);
    }

    function fillValues(values) {
      var aValues = values.split(";");
      document.getElementById("txt_DeviceName").value = aValues[0];
      document.getElementById("txt_wifissid").value = aValues[1];
      document.getElementById("txt_wifipassword").value = "";
      document.getElementById("txt_RelayCompressorPin").value = aValues[3];
      document.getElementById("txt_RelayFreePin").value = aValues[4];
      document.getElementById("txt_SensorPin").value = aValues[5];
      document.getElementById("txt_MaxSecForStatus").value = aValues[7];
      document.getElementById("txt_SensorValueAt0PSI").value = aValues[8];
      document.getElementById("txt_SensorValueAt50PSI").value = aValues[9];
      document.getElementById("txt_AccessCode").value = "";
      document.getElementById("chk_ClearAccessCode").checked = false;
      document.getElementById("txt_MessureWait").value = aValues[11];  
      document.getElementById("txt_Precision").value = aValues[12];
      document.getElementById("txt_PressureCalcMethod").value = aValues[13];
      document.getElementById("lbl_version").innerHTML = aValues[15];
      document.getElementById("txt_MaxPressurePSI").value = aValues[16];
      document.getElementById("txt_MaxValveOpenSec").value = aValues[17];
      document.getElementById("txt_SensorFullScalePSI").value = aValues[18];
      document.getElementById("txt_AccessPointPassword").value = aValues[19];
    }

    function saveSettings()
    {
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
      AccessCode: document.getElementById("txt_AccessCode").value,
      ClearAccessCode: document.getElementById("chk_ClearAccessCode").checked ? "true" : "false",
      MessureWait: document.getElementById("txt_MessureWait").value,
      Precision: document.getElementById("txt_Precision").value,
      PressureCalcMethod: document.getElementById("txt_PressureCalcMethod").value,
      MaxPressurePSI: document.getElementById("txt_MaxPressurePSI").value,
      MaxValveOpenSec: document.getElementById("txt_MaxValveOpenSec").value,
      SensorFullScalePSI: document.getElementById("txt_SensorFullScalePSI").value,
      AccessPointPassword: document.getElementById("txt_AccessPointPassword").value});
                                      
      var urlwithQS = "/updateSettings?" + params.toString();
      var xhr = new XMLHttpRequest();
      xhr.open("GET", urlwithQS, false);       
      xhr.send();
      console.log('success!');
      alert("settings Successfully updated");
      setTimeout("location.href = '../';", 2000);
    }
}