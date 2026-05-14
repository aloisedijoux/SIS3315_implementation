#include "SIS3315Module.h"
#include "sis3315-software/libraries_and_includes/sis3315_class_library/sis3315_class.h"
#include "EventClass.h"
// pour mutex
#include <mutex>
#include <thread>
#include <atomic>

void SIS3315Module::SetParameter(const std::string& name, int value)
{
    if (name == "threshold")
    {
        register_write(0x0028, value);
    }
}


// Boucle dans un thread dédié — non bloquante pour l'appelant
void SIS3315Module::ReadoutLoop() {
    Event e; // alloué une fois, réutilisé — pas de new à chaque événement

    while (running_.load()) {           // atomic : thread-safe sans mutex
        {
            std::lock_guard<std::mutex> lock(paramMutex_);
            if (hwModule_->DataAvailable()) {
                e = parse(hwModule_->FetchEvent());
            }
        }                               // mutex relâché ici
        DispatchEvent(e);               // callback hors mutex
        std::this_thread::sleep_for(
            std::chrono::microseconds(100)); // ~10kHz max, pas de busy-wait
    }
}

void SIS3315Module::DispatchEvent(const Event& e) const {
    if (cb_) cb_(e); // appel direct, zéro copie
}


// "splits data into events" = parse le buffer brut du hardware
// en objets Event individuels, un par trigger
std::vector<Event> SIS3315Module::readData() {
    std::vector<Event> results;
    while (hwModule_->HasData()) {
        auto raw = hwModule_->ReadNextEvent();
        results.push_back(parse(raw)); // conversion SDK → ADC::Event
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



void SIS3315Module::InitSupportedValues() {
    // Valeurs différentes — datasheet SIS3315
    supportedParameterValues_[ADCParameterType::SamplingRate]
        = {100.0f, 200.0f};          // différent du SIS3316
    supportedParameterValues_[ADCParameterType::VoltageRange]
        = {1.0f, 2.0f, 5.0f};       // différent du SIS3316
    supportedParameterValues_[ADCParameterType::AveragingMode]
        = {0, 4, 16, 64};           // différent du SIS3316
}


bool SIS3315Module::SetParameter(ADCParameterType param,
                                  float value,
                                  int channel) {
    if (!IsValueSupported(param, value)) return false;
    std::lock_guard<std::mutex> lock(paramMutex_);

    switch (param) {
        case ADCParameterType::SamplingRate:
            hwModule_->SetSamplingRate(  // nom différent dans SDK SIS3315
                static_cast<int>(value));
            break;
        case ADCParameterType::VoltageRange:
            hwModule_->SetVoltageRange(channel, value); // nom différent
            break;
        default:
            return false;
    }
    return true;
}