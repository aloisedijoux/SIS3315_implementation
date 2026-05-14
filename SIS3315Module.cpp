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


// Fonction helper locale au .cpp — pas dans le .h
// Traduit le facteur → code 3 bits
static uint32_t encodeAveragingFactor(float factor) {
    if (factor <=   0) return 0b000;
    if (factor <=   4) return 0b001;
    if (factor <=   8) return 0b010;
    if (factor <=  16) return 0b011;
    if (factor <=  32) return 0b100;
    if (factor <=  64) return 0b101;
    if (factor <= 128) return 0b110;
    return                     0b111; // 256
}


static uint32_t encodeClockSource(float value, bool useNIM = false) {
    if (value == 0.0f) return 0b00; // interne — Sel1=0, Sel0=0
    if (useNIM)        return 0b11; // externe NIM  — Sel1=1, Sel0=1
    return                   0b10; // externe LVDS — Sel1=1, Sel0=0
}


void SIS3315Module::InitSupportedValues() {
    // Valeurs différentes — datasheet SIS3315

supportedParameterValues_[ADCParameterType::SamplingRate] = {
    10.4f,   // 125 MHz oscillateur → 125/12 ≈ 10.4 MSPS
    15.0f    // 180 MHz oscillateur → maximum hardware
};

supportedParameterValues_[ADCParameterType::VoltageRange] = {
    3.0f,     // ±1.5 V
    5.0f,     // ±2.5 V
    8.192f,   // ±4.096 V
    10.0f,    // ±5 V
    20.0f     // ±10 V
};

supportedParameterValues_[ADCParameterType::InputTermination] = {
    50.0f,    // 50 Ω
    935.0f    // Hi-Z ≥ 935 Ω — valeur minimale documentée
};


supportedParameterValues_[ADCParameterType::AveragingMode] = {
    0.0f,    // 000 → désactivé
    4.0f,    // 001 → 4 samples
    8.0f,    // 010 → 8 samples
    16.0f,   // 011 → 16 samples
    32.0f,   // 100 → 32 samples
    64.0f,   // 101 → 64 samples
    128.0f,  // 110 → 128 samples
    256.0f   // 111 → 256 samples
};


supportedParameterValues_[ADCParameterType::ClockSource] = {
    0.0f,  // interne
    1.0f   // externe
};

supportedParameterValues_[ADCParameterType::TriggerEnable] = {
    0.0f,  // désactivé
    1.0f   // activé
};
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
        case ADCParameterType::SamplingRate: {
    // Conversion MSPS → fréquence oscillateur SI570
    // f_oscillateur = value * 12 (MHz)
    uint32_t oscFreqMHz = static_cast<uint32_t>(value * 12.0f);
    hwModule_->SetOscillatorFrequency(oscFreqMHz);
    break;
}

case ADCParameterType::ClockSource: {
    uint32_t sel = encodeClockSource(value);
    //                               ↑
    //    false par défaut → connecteur LVDS
    //    passer true si connecteur NIM utilisé

    uint32_t reg = hwModule_->ReadRegister(
        SIS3315_REG_CLOCK_MUX);

    // Effacer bits Sel1:Sel0, écrire la nouvelle sélection
    reg = (reg & ~0b11u) | (sel & 0b11u);

    hwModule_->WriteRegister(
        SIS3315_REG_CLOCK_MUX, reg);
    break;
}
case ADCParameterType::AveragingMode: {
    uint32_t code = encodeAveragingFactor(value);

    uint32_t reg = hwModule_->ReadRegister(
        SIS3315_REG_AVERAGE_CONFIG);

    // Effacer bits 30:28, écrire le nouveau code
    reg = (reg & ~(0b111u << 28)) | (code << 28);

    hwModule_->WriteRegister(
        SIS3315_REG_AVERAGE_CONFIG, reg);
    break;
}
case ADCParameterType::SampleLength: {
    uint32_t n = static_cast<uint32_t>(value);

    // Contrainte 1 : valeurs paires uniquement
    if (n % 2 != 0) {
        n = n & ~1u; // arrondi au pair inférieur
    }

    if (n <= 65534) {
        // Registre standard 16 bits
        hwModule_->WriteRegister(
            SIS3315_REG_RAW_DATA_BUFFER,
            n);
    } else if (n <= 33554430) {
        // Registre étendu 25 bits — prend priorité
        hwModule_->WriteRegister(
            SIS3315_REG_EXTENDED_RAW_DATA_BUFFER,
            n);
    } else {
        return false; // dépassement hardware
    }
    break;
}

case ADCParameterType::TriggerEnable: {
    bool enable = (value != 0.0f);

    int startCh = (channel == -1) ? 0 : channel;
    int endCh   = (channel == -1) ? 15 : channel;

    for (int ch = startCh; ch <= endCh; ++ch) {
        uint32_t reg = hwModule_->ReadRegister(
            SIS3315_REG_TRIGGER_THRESHOLD(ch));

        // Bit 31 = TriggerEnable
        if (enable) reg |=  (1u << 31); // mettre bit 31 à 1
        else        reg &= ~(1u << 31); // mettre bit 31 à 0

        hwModule_->WriteRegister(
            SIS3315_REG_TRIGGER_THRESHOLD(ch), reg);
    }
    break;
}
case ADCParameterType::PreTriggerLength: {
    uint32_t n = static_cast<uint32_t>(value);

    // Contrainte 1 : valeurs paires
    n = n & ~1u;

    // Contrainte 2 : plafond hardware 16378
    if (n > 16378) {
        n = 16378; // plafonné selon datasheet
    }

    // Contrainte 3 : registre 14 bits → masque 0x3FFF
    hwModule_->WriteRegister(
        SIS3315_REG_PRE_TRIGGER_DELAY,
        n & 0x3FFF);
    break;
}

case ADCParameterType::TriggerThreshold: {
    uint32_t thr = static_cast<uint32_t>(value);

    // Validation : 28 bits maximum
    if (thr > 0x0FFF'FFFF) return false;

    int startCh = (channel == -1) ? 0 : channel;
    int endCh   = (channel == -1) ? 15 : channel;

    for (int ch = startCh; ch <= endCh; ++ch) {
        // Lire la valeur actuelle pour préserver bits 31:28
        uint32_t reg = hwModule_->ReadRegister(
            SIS3315_REG_TRIGGER_THRESHOLD(ch));

        // Effacer bits 27:0, écrire le nouveau seuil
        reg = (reg & 0xF000'0000) | (thr & 0x0FFF'FFFF);

        hwModule_->WriteRegister(
            SIS3315_REG_TRIGGER_THRESHOLD(ch), reg);
    }
    break;
}
case ADCParameterType::VoltageRange: {
    // Pas d'écriture registre possible — hardware fixé en usine
    // On valide seulement que la valeur est dans la liste supportée
    // IsValueSupported() s'en charge avant d'arriver ici
    // On stocke pour référence interne
    currentVoltageRange_ = value;
    break;
    // IMPORTANT : commenter clairement pourquoi pas de WriteRegister()
    // pour que le prochain développeur comprenne
}

case ADCParameterType::InputTermination: {
    // Pas d'écriture registre — hardware fixé en usine
    // Validation uniquement via IsValueSupported()
    currentTermination_ = value;
    break;
}
        default:
            return false;
    }
    return true;
}


supportedParameterValues_[ADCParameterType::VoltageRange] = {
    3.0f,     // ±1.5 V
    5.0f,     // ±2.5 V
    8.192f,   // ±4.096 V
    10.0f,    // ±5 V
    20.0f     // ±10 V
};
 
// SIS3315Module.cpp — InitSupportedValues()
supportedParameterValues_[ADCParameterType::InputTermination] = {
    50.0f,    // 50 Ω
    935.0f    // Hi-Z ≥ 935 Ω — valeur minimale documentée
};

// Dans SetParameter() — SIS3315Module.cpp
case ADCParameterType::InputTermination: {
    // Pas d'écriture registre — hardware fixé en usine
    // Validation uniquement via IsValueSupported()
    currentTermination_ = value;
    break;
}