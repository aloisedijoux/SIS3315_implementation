#include "SIS3316Module.h"
#include <mutex>
#include <thread>
#include <atomic>

// "splits data into events" = parse le buffer brut du hardware
// en objets Event individuels, un par trigger
std::vector<Event> SIS3316Module::readData() {
    std::vector<Event> results;
    while (hwModule_->HasData()) {
        auto raw = hwModule_->ReadNextEvent();
        results.push_back(parse(raw)); // conversion SDK vers ADC::Event
    }
    return results;
}

Event reusable;
while (running_) {
    if (hw_->HasData()) {
        reusable = parse(hw_->ReadNext());
        if (cb_) cb_(reusable);
    }
}

// SIS3316Module.cpp
void SIS3316Module::InitSupportedValues() {
    // Valeurs issues de la datasheet SIS3316
    supportedParameterValues_[ADCParameterType::SamplingRate]
        = {62.5f, 125.0f, 250.0f};
    supportedParameterValues_[ADCParameterType::VoltageRange]
        = {2.0f, 5.0f};
    supportedParameterValues_[ADCParameterType::InputTermination]
        = {50.0f, 1'000'000.0f};
    supportedParameterValues_[ADCParameterType::AveragingMode]
        = {0, 2, 4, 8, 16, 32, 64, 128};
}


bool SIS3316Module::SetParameter(ADCParameterType param,
                                  float value,
                                  int channel) {
    // Étape 1 : validation
    if (!IsValueSupported(param, value)) return false;

    // Étape 2 : thread-safety
    std::lock_guard<std::mutex> lock(paramMutex_);

    switch (param) {
        case ADCParameterType::SamplingRate:
            hwModule_->SetSamplingFrequency(
                static_cast<int>(value));
            break;
        case ADCParameterType::VoltageRange:
            hwModule_->SetInputRange(channel, value);
            break;
        default:
            return false;
    }
    return true;
}

// SIS3316Module.cpp
float SIS3316Module::GetParameter(ADCParameterType param,
                                   int channel) const {
    std::lock_guard<std::mutex> lock(paramMutex_);
    int ch = (channel == -1) ? 0 : channel;

    switch (param) {
        case ADCParameterType::SamplingRate:
            return static_cast<float>(
                hwModule_->GetSamplingFrequency()); // lit le registre
        case ADCParameterType::VoltageRange:
            return hwModule_->GetInputRange(ch);
        default:
            return 0.0f;
    }
}