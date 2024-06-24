#include "PDM_Mic.h"
#include "ArduinoBLE.h"
#include "ble_config/ble_config_earable.h"

class PDM_MIC_Service {
public:
    void begin(PDM_Mic * device);
    void update();

    PDM_Mic * device;
private:
    BLEService * pdmConfigService;
    BLEUnsignedCharCharacteristic * pdmGainC;

    void _setup_pdm_mic_service();
};
void change_gain(BLEDevice central, BLECharacteristic characteristic);

extern PDM_MIC_Service pdm_mic_service;