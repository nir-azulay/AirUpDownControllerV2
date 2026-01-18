{
    function onloadPage() 
    {
      fillValues(document.getElementById("txt_values").value);
    }

    function fillValues(values) {
      var aValues = values.split(";");
      document.getElementById("txt_DeviceName").value = aValues[0];
      document.getElementById("txt_wifissid").value = aValues[1];
      document.getElementById("txt_wifipassword").value = aValues[2];
      document.getElementById("txt_RelayCompressorPin").value = aValues[3];
      document.getElementById("txt_RelayFreePin").value = aValues[4];
      document.getElementById("txt_SensorPin").value = aValues[5];
      document.getElementById("txt_MaxSecForStatus").value = aValues[7];
      document.getElementById("txt_SensorValueAt0PSI").value = aValues[8];
      document.getElementById("txt_SensorValueAt50PSI").value = aValues[9];
      document.getElementById("txt_BTPinCode").value = aValues[10];        
      document.getElementById("txt_MessureWait").value = aValues[11];  
      document.getElementById("txt_Precision").value = aValues[12];
      document.getElementById("txt_PressureCalcMethod").value = aValues[13];
      document.getElementById("lbl_version").innerHTML = aValues[14];
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
      BTPinCode: document.getElementById("txt_BTPinCode").value,
      MessureWait: document.getElementById("txt_MessureWait").value,
      PressureCalcMethod: document.getElementById("txt_PressureCalcMethod").value,
      Precision: document.getElementById("txt_Precision").value});
                                      
      var urlwithQS = "/updateSettings?" + params.toString();
      var xhr = new XMLHttpRequest();
      xhr.open("GET", urlwithQS, false);       
      xhr.send();
      console.log('success!');
      alert("settings Successfully updated");
      setTimeout("location.href = '../';", 2000);
    }
}