#ifndef SIS3316MODULE_H
#define SIS3316MODULE_H

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ADCModule.h"
#include "ITransport.h"
#include "drivers/sis3316.h"
#include "drivers/sis3316_class.h"

class SIS3316Module : public ADCModule, public sis3316_adc {
public:
    SIS3316Module(vme_interface_class* crate, unsigned int baseAddress);

    // ADCModule overrides
    bool  SetParameter(ADCParameterType param, float value, int channel = -1) override;
    float GetParameter(ADCParameterType param, int channel = -1) const override;
    bool  SaveToDisk(const std::string& filepath) const override;
    bool  LoadFromDisk(const std::string& filepath) override;
    bool  Connect() override;
    void  Disconnect() override;
    bool  IsConnected() const override;
    int   SoftwareTrigger() override;
    void  StartAcquisition() override;
    void  StartReadoutLoop() override;
    void  StopReadoutLoop() override;
    void  ParseEvent(Event e) override;
    void  DispatchEvent(const Event& e) const override;

    std::vector<Event>  ReadData();
    std::vector<Event*> GetEventPointers();
    void                ReadoutLoop();

    // lit les registres pour adapter le comportement au hardware présent
    void DetectHardwareVersion();
    void DetectFirmwareVersion();

    // sel : 1 = VXS, 2 = LVDS, 3 = NIM
    void SetExternalClockConnector(uint32_t sel);

private:
    void InitSupportedValues();
    bool IsValueSupported(ADCParameterType param, float value) const;

    std::vector<Event>  eventBuffer_;
    std::atomic<bool>   running_{false};
    mutable std::mutex  paramMutex_;
    std::thread         readoutThread_;
    bool                connected_{false};
    bool                is16bit_{true};
    bool                firmwareSupportsAveraging_{false};
    uint32_t            externalClockSel_{0b10}; // LVDS par défaut
    uint32_t            maxPreTrigger_{16378};
};

#endif // SIS3316MODULE_H
