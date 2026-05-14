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

void SIS3316Module::InitSupportedValues() {
    if (is16bit_) {
        // AD9268 — 16 bit
        supportedParameterValues_[ADCParameterType::SamplingRate] = {
            20.0f, 25.0f, 50.0f, 62.5f, 125.0f
        };
    } else {
        // AD9643 — 14 bit
        supportedParameterValues_[ADCParameterType::SamplingRate] = {
            40.0f, 50.0f, 62.5f, 125.0f, 250.0f
        };
    }
    // 125 MHz = valeur par défaut après power-up
    // documentée dans le manuel


supportedParameterValues_[ADCParameterType::VoltageRange] = {
    5.0f,   // 0b00
    2.0f,   // 0b01
    1.9f    // 0b10 ou 0b11
};


supportedParameterValues_[ADCParameterType::InputTermination] = {
    50.0f,    //    50 Ω — bit = 0
    1000.0f   // 1 kΩ  — bit = 1
};


supportedParameterValues_[ADCParameterType::TriggerEnable] = {
    0.0f,  // désactivé — valeur par défaut après reset
    1.0f   // activé
};

supportedParameterValues_[ADCParameterType::ClockSource] = {
    0.0f,  // interne  → Sel1=0, Sel0=0
    1.0f   // externe  → Sel1=1, Sel0=0 (LVDS, défaut externe)
           //          → Sel1=0, Sel0=1 (VXS)
           //          → Sel1=1, Sel0=1 (NIM)
};

if (is16bit_ && firmwareSupportsAveraging_) {
    supportedParameterValues_[ADCParameterType::AveragingMode] = {
        0.0f, 4.0f, 8.0f, 16.0f, 32.0f, 64.0f, 128.0f, 256.0f
    };
} else {
    // Liste vide → IsValueSupported() retourne false
    // pour toute valeur sauf 0 (désactivé)
    supportedParameterValues_[ADCParameterType::AveragingMode] = {
        0.0f
    };
}
}

static uint32_t encodeVoltageRange(float volts) {
    if (volts >= 5.0f) return 0b00;
    if (volts >= 2.0f) return 0b01;
    return                    0b10; // 1.9V
}

static uint32_t encodeAveragingFactor(float f) {
    if (f <=   0) return 0b000;
    if (f <=   4) return 0b001;
    if (f <=   8) return 0b010;
    if (f <=  16) return 0b011;
    if (f <=  32) return 0b100;
    if (f <=  64) return 0b101;
    if (f <= 128) return 0b110;
    return               0b111; // 256
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


case ADCParameterType::InputTermination: {
    // 50.0f → bit = 0 (termination activée)
    // autre → bit = 1 (termination désactivée)
    bool disable50Ohm = (value != 50.0f);

    int startCh = (channel == -1) ? 0  : channel;
    int endCh   = (channel == -1) ? 15 : channel;

    for (int ch = startCh; ch <= endCh; ++ch) {
        uint32_t reg = hwModule_->ReadRegister(
            SIS3316_REG_GAIN_TERMINATION(ch));

        // Bit "Disable 50 Ohm" — position selon datasheet
        const uint32_t BIT_DISABLE_50OHM = (1u << 2);
        if (disable50Ohm) reg |=  BIT_DISABLE_50OHM;
        else              reg &= ~BIT_DISABLE_50OHM;

        hwModule_->WriteRegister(
            SIS3316_REG_GAIN_TERMINATION(ch), reg);
    }
    break;
}

case ADCParameterType::ClockSource: {
    uint32_t sel;

    if (value == 0.0f) {
        sel = 0b00; // interne
    } else {
        // externe → utiliser le connecteur configuré
        sel = externalClockSel_; // 0b01, 0b10, ou 0b11
    }

    uint32_t reg = hwModule_->ReadRegister(
        SIS3316_REG_CLOCK_MUX);

    // Bits Sel1:Sel0
    reg = (reg & ~0b11u) | (sel & 0b11u);

    hwModule_->WriteRegister(
        SIS3316_REG_CLOCK_MUX, reg);
    break;
}
case ADCParameterType::TriggerEnable: {
    bool enable = (value != 0.0f);

    int startCh = (channel == -1) ? 0  : channel;
    int endCh   = (channel == -1) ? 15 : channel;

    for (int ch = startCh; ch <= endCh; ++ch) {
        uint32_t reg = hwModule_->ReadRegister(
            SIS3316_REG_FIR_TRIGGER_THRESHOLD(ch));

        // Bit 31 uniquement — préserver bits 27:0
        if (enable) reg |=  (1u << 31);
        else        reg &= ~(1u << 31);

        hwModule_->WriteRegister(
            SIS3316_REG_FIR_TRIGGER_THRESHOLD(ch), reg);
    }
    break;
}


case ADCParameterType::AveragingMode: {
    if (!is16bit_ || !firmwareSupportsAveraging_)
        return false; // feature non disponible sur ce hardware

    uint32_t code = encodeAveragingFactor(value);

    uint32_t reg = hwModule_->ReadRegister(
        SIS3316_REG_AVERAGE_CONFIG);

    // Bits [2:0] selon datasheet
    reg = (reg & ~0b111u) | (code & 0b111u);

    hwModule_->WriteRegister(
        SIS3316_REG_AVERAGE_CONFIG, reg);
    break;
}

case ADCParameterType::SampleLength: {
    uint32_t n = static_cast<uint32_t>(value);

    // Forcer valeur paire — bit 0 toujours à 0
    n = n & ~1u;

    if (n <= 65534) {
        hwModule_->WriteRegister(
            SIS3316_REG_RAW_DATA_BUFFER, n);
    } else if (n <= 33554430) {
        // Registre étendu prend priorité si non nul
        hwModule_->WriteRegister(
            SIS3316_REG_EXTENDED_RAW_DATA_BUFFER, n);
    } else {
        return false; // dépassement hardware
    }
    break;
}
case ADCParameterType::VoltageRange: {
    uint32_t gainBits = encodeVoltageRange(value);

    int startCh = (channel == -1) ? 0  : channel;
    int endCh   = (channel == -1) ? 15 : channel;

    for (int ch = startCh; ch <= endCh; ++ch) {
        // Read-modify-write : préserver les autres bits
        uint32_t reg = hwModule_->ReadRegister(
            SIS3316_REG_GAIN_TERMINATION(ch));

        // Bits Gain[1:0] — position selon datasheet
        reg = (reg & ~0b11u) | (gainBits & 0b11u);

        hwModule_->WriteRegister(
            SIS3316_REG_GAIN_TERMINATION(ch), reg);
    }
    break;
}

case ADCParameterType::TriggerThreshold: {
    uint32_t thr = static_cast<uint32_t>(value);

    // Validation : 27 bits maximum
    // 2^27 - 1 = 134217727
    if (thr > 134217727u) return false;

    int startCh = (channel == -1) ? 0  : channel;
    int endCh   = (channel == -1) ? 15 : channel;

    for (int ch = startCh; ch <= endCh; ++ch) {
        uint32_t reg = hwModule_->ReadRegister(
            SIS3316_REG_FIR_TRIGGER_THRESHOLD(ch));

        // Préserver bit 31 (TriggerEnable) et bits 29:28 (CFD)
        // Écrire bits 27:0
        reg = (reg & 0xF000'0000u)
            | (thr & 0x0FFF'FFFFu);

        hwModule_->WriteRegister(
            SIS3316_REG_FIR_TRIGGER_THRESHOLD(ch), reg);
    }
    break;
}
case ADCParameterType::PreTriggerLength: {
    uint32_t n = static_cast<uint32_t>(value);

    // Valeur paire uniquement
    n = n & ~1u;

    // Plafond dépendant du firmware
    if (n > maxPreTrigger_) n = maxPreTrigger_;

    // Registre 14 bits — masque 0x3FFF
    hwModule_->WriteRegister(
        SIS3316_REG_PRE_TRIGGER_DELAY,
        n & 0x3FFF);
    break;
}
case ADCParameterType::SamplingRate: {
    // Oscillateur interne programmable
    // valeur en MHz → fréquence oscillateur
    hwModule_->SetSamplingFrequency(
        static_cast<int>(value));
    break;
}
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


void SIS3316Module::DetectHardwareVersion() {
    uint32_t reg = hwModule_->ReadRegister(
        SIS3316_REG_HARDWARE_VERSION);
    is16bit_ = ((reg & 0xFF) == 0x16); // à adapter selon datasheet
}


void SIS3316Module::DetectFirmwareVersion() {
    uint32_t fw = hwModule_->ReadRegister(
        SIS3316_REG_FIRMWARE_VERSION);
    uint32_t minor = fw & 0xFFFF;
    maxPreTrigger_ = (minor >= 7) ? 16378 : 2042;
}