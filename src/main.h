void readCurrentPressure();

void SyncWithBlueToothDevice();

void OperateAir();

void StopAir();

double calculateDelatFactor();

void printProgress(size_t prg, size_t sz);

void listDir(fs::FS &fs, const char *dirname, uint8_t levels);

void saveSettingFile();

void printSettingsFile();

void loadSettingsfile();

void InflateAir();

void DeflateAir();

void SendSettingsToMobile();

void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);

String getSettingsJSON();