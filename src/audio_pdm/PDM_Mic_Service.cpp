#include "PDM_Mic_Service.h"

void PDM_MIC_Service::begin(PDM_Mic * device) {
    this->device = device;

    _setup_pdm_mic_service();
}

void PDM_MIC_Service::update() {
    return;
}

void PDM_MIC_Service::_setup_pdm_mic_service() {
    pdmConfigService = new BLEService(pdmMicConfigService);
    pdmGainC = new BLEUnsignedCharCharacteristic(pdmMicGainCharUuid, BLEWrite| BLERead);

    BLE.setAdvertisedService(*pdmConfigService);
    pdmConfigService->addCharacteristic(*pdmGainC);

    BLE.addService(*pdmConfigService);
    pdmGainC->setValue(device->getGain());

}
