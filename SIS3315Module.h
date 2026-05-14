#ifndef SIS3315MODULE_H
#define SIS3315MODULE_H

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ADCModule.h"
#include "ITransport.h"

class SIS3315Module : public ADCModule {
public:
    // Transport injecté — VMETransport, EthernetTransport ou OpticalTransport
    explicit SIS3315Module(std::shared_ptr<ITransport> transport);

    // Raccourci Ethernet : crée un EthernetTransport en interne
    explicit SIS3315Module(const std::string& ip);

    ~SIS3315Module() noexcept override = default;

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

    void DetectHardwareVersion();
    void DetectFirmwareVersion();

private:
    void InitSupportedValues();
    bool IsValueSupported(ADCParameterType param, float value) const;

    std::shared_ptr<ITransport> transport_;
    std::vector<Event>          eventBuffer_;
    std::atomic<bool>           running_{false};
    mutable std::mutex          paramMutex_;
    std::thread                 readoutThread_;
    bool                        connected_{false};
    uint32_t                    maxPreTrigger_{16378};
    float                       currentVoltageRange_{0.0f};
    float                       currentTermination_{0.0f};
};

#endif // SIS3315MODULE_H
