#include "SIS3315Module.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <unistd.h>   // usleep
#include <unordered_map>

/**************************************************************/
//  Tables de registres par paramètre
/**************************************************************/

static const uint32_t kAveragingModeAddrs[] = {
    SIS3315_ADC_CH1_4_AVERAGE_CONFIGURATION_REG,
    SIS3315_ADC_CH5_8_AVERAGE_CONFIGURATION_REG,
    SIS3315_ADC_CH9_12_AVERAGE_CONFIGURATION_REG,
    SIS3315_ADC_CH13_16_AVERAGE_CONFIGURATION_REG,
};

static const uint32_t kClockSourceAddrs[] = {
    SIS3315_SAMPLE_CLOCK_DISTRIBUTION_CONTROL,
};

static const uint32_t kInputTerminationAddrs[] = {
    SIS3315_ADC_CH1_4_ANALOG_CTRL_REG,
    SIS3315_ADC_CH5_8_ANALOG_CTRL_REG,
    SIS3315_ADC_CH9_12_ANALOG_CTRL_REG,
    SIS3315_ADC_CH13_16_ANALOG_CTRL_REG,
};

static const uint32_t kPretriggerAddrs[] = {
    SIS3315_ADC_CH1_4_PRE_TRIGGER_DELAY_REG,
    SIS3315_ADC_CH5_8_PRE_TRIGGER_DELAY_REG,
    SIS3315_ADC_CH9_12_PRE_TRIGGER_DELAY_REG,
    SIS3315_ADC_CH13_16_PRE_TRIGGER_DELAY_REG,
};

static const uint32_t kSampleLengthAddrs[] = {
    SIS3315_ADC_CH1_4_RAW_DATA_BUFFER_CONFIG_REG,
    SIS3315_ADC_CH5_8_RAW_DATA_BUFFER_CONFIG_REG,
    SIS3315_ADC_CH9_12_RAW_DATA_BUFFER_CONFIG_REG,
    SIS3315_ADC_CH13_16_RAW_DATA_BUFFER_CONFIG_REG,
};

static const uint32_t kTIPortsAddrs[] = {
    SIS3315_NIM_INPUT_CONTROL_REG,
};

static const uint32_t kTOPortsAddrs[] = {
    SIS3315_LEMO_OUT_TO_SELECT_REG,
};

static const uint32_t kTriggerEnableAddrs[] = {
    SIS3315_ADC_CH1_4_EVENT_CONFIG_REG,
    SIS3315_ADC_CH5_8_EVENT_CONFIG_REG,
    SIS3315_ADC_CH9_12_EVENT_CONFIG_REG,
    SIS3315_ADC_CH13_16_EVENT_CONFIG_REG,
    SIS3315_ADC_CH1_4_EXTENDED_EVENT_CONFIG_REG,
    SIS3315_ADC_CH5_8_EXTENDED_EVENT_CONFIG_REG,
    SIS3315_ADC_CH9_12_EXTENDED_EVENT_CONFIG_REG,
    SIS3315_ADC_CH13_16_EXTENDED_EVENT_CONFIG_REG,
};

static const uint32_t kTrigThreshAddrs[] = {
    SIS3315_ADC_CH1_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH2_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH3_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH4_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH5_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH6_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH7_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH8_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH9_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH10_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH11_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH12_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH13_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH14_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH15_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH16_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH1_4_SUM_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH5_8_SUM_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH9_12_SUM_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH13_16_SUM_FIR_TRIGGER_THRESHOLD_REG,
};

static const uint32_t kVoltageRangeAddrs[] = {
    SIS3315_HARDWARE_VERSION, // lecture seule
};

/**************************************************************/
//  Registre de paramètres : table de dispatch
/**************************************************************/

static const std::unordered_map<ADCParameterType, RegisterDescriptor> kParamRegistry = {
    {
        ADCParameterType::AveragingMode,
        {
            .adresses  = kAveragingModeAddrs,
            .addrCount = 4,
            .mask      = 0x00000007,  // bits 2:0
            .shift     = 0,
            .encode    = [](uint32_t v) -> uint32_t { return v & 0x7; },
        }
    },
    {
        ADCParameterType::ClockSource,
        {
            .adresses  = kClockSourceAddrs,
            .addrCount = 1,
            .mask      = 0x00000003,  // bits 1:0
            .shift     = 0,
            .encode    = [](uint32_t v) -> uint32_t { return v & 0x3; },
        }
    },
    {
        ADCParameterType::InputTermination,
                {
        // TODO: The termination is factory-wired (SIS3315_HARDWARE_VERSION bit 4).
        // The only software lever is the DAC offset. This input is therefore
        // read-only; SetParameter() will throw an exception if called.
            .adresses  = kInputTerminationAddrs,
            .addrCount = 4,
            .mask      = 0x00000000,  // aucun bit modifiable
            .shift     = 0,
            .encode    = nullptr,
        }
    },
    {
        ADCParameterType::PreTriggerLength,
        {
            .adresses  = kPretriggerAddrs,
            .addrCount = 4,
            .mask      = 0x00003FFE,  // bits 13:1 (multiple de 2)
            .shift     = 0,
            .encode    = [](uint32_t v) -> uint32_t { return v & 0x3FFE; },
        }
    },
    {
        ADCParameterType::SampleLength,
        {
            .adresses  = kSampleLengthAddrs,
            .addrCount = 4,
            .mask      = 0xFFFF0000,  // bits 31:16
            .shift     = 16,
            .encode    = nullptr,
        }
    },
    {
        ADCParameterType::SamplingRate,
        {
            
            // The sampling rate is set via set_ADC_bit_clock_frequency() which acts on the SI570 (I2C register 0x50) and triggers the DCM Reset (0x438).
            // There is no direct register address to write here; ClockConfiguration() handles this step. 
            // This input exists for validation purposes only.
            
            .adresses  = nullptr,
            .addrCount = 0,
            .mask      = 0,
            .shift     = 0,
            .encode    = nullptr,
        }
    },
    {
        ADCParameterType::TIPort,
        {
            .adresses  = kTIPortsAddrs,
            .addrCount = 1,
            .mask      = 0x00000080,  // bit 7
            .shift     = 7,
            // TODO : encode
            .encode    = nullptr,
        }
    },
    {
        ADCParameterType::TOPort,
        {
            .adresses  = kTOPortsAddrs,
            .addrCount = 1,
            .mask      = 0x40000000,  // bit 30
            .shift     = 30,
            // TODO : encode
            .encode    = nullptr,
        }
    },
    {
        ADCParameterType::TriggerEnable,
        {
            .adresses  = kTriggerEnableAddrs,
            .addrCount = 4,
            .mask      = 0x00000003,  // bits 1:0
            .shift     = 0,
// TODO: encode (distinguish between internal / external / pileup)
            .encode    = nullptr,
        }
    },
    {
        ADCParameterType::TriggerThreshold,
        {
            .adresses  = kTrigThreshAddrs,
            .addrCount = 20,          // 16 canaux + 4 registres SUM
            .mask      = 0x8FFFFFFF,  // bits 27:0 (valeur) + bit 31 (enable)
            .shift     = 0,
            // TODO : encode — combiner valeur seuil (bits 27:0) et trigger enable (bit 31)
            .encode    = nullptr,
        }
    },
    {
        ADCParameterType::VoltageRange,
        {
            .adresses  = kVoltageRangeAddrs,
            .addrCount = 1,
            .mask      = 0x00000007,  // bits 2:0 (Assembly variant)
            .shift     = 0,
            // Lecture seule : SetParameter() doit lever une exception si appelé.
            .encode    = nullptr,
        }
    },
};

/**************************************************************/
//  Constructeur
/**************************************************************/

SIS3315Module::SIS3315Module(vme_interface_class* crate, unsigned int baseaddress)
    : ADCModule()
    , sis3315_adc(crate, baseaddress)
{
    supportedParameterValues_[ADCParameterType::SamplingRate] = {
        25.0f,  50.0f,  62.5f,  71.0f,  83.0f,
       100.0f, 104.0f, 114.0f, 119.0f, 125.0f,
       139.0f, 167.0f, 179.0f, 208.0f, 227.0f, 250.0f
    };

    supportedParameterValues_[ADCParameterType::VoltageRange] = {
        3.0f, 5.0f, 8.192f, 10.0f, 20.0f
    };

    supportedParameterValues_[ADCParameterType::InputTermination] = {
        50.0f, 935.0f
    };

    supportedParameterValues_[ADCParameterType::AveragingMode] = {
        0.0f, 4.0f, 8.0f, 16.0f, 32.0f, 64.0f, 128.0f, 256.0f
    };

    supportedParameterValues_[ADCParameterType::ClockSource] = {
        0.0f, 2.0f, 3.0f
    };

    supportedParameterValues_[ADCParameterType::TriggerEnable] = {
        0.0f, 1.0f
    };
}

/**************************************************************/
//  Paramètres ADC
/**************************************************************/

bool SIS3315Module::isValueSupported(ADCParameterType param, float value)
{
    const auto& supported = GetSupportedValues(param);
    return std::find(supported.begin(), supported.end(), value) != supported.end();
}

void SIS3315Module::SetParameter(ADCParameterType param, float value, int channelGroup)
{
    if (!isValueSupported(param, value))
        throw std::invalid_argument(
            "Valeur non supportée pour ce paramètre : " + std::to_string(value));

    auto it = kParamRegistry.find(param);
    if (it == kParamRegistry.end())
        throw std::invalid_argument("Paramètre non trouvé dans kParamRegistry.");

    const RegisterDescriptor& desc = it->second;

    if (desc.encode == nullptr)
        throw std::logic_error(
            "SetParameter() appelé sur un paramètre lecture seule ou non encore implémenté.");

    uint32_t encoded = desc.encode(static_cast<uint32_t>(value));

    // If channelGroup == 0 (default): apply to all groups.
    // Otherwise: apply only to the requested group (index 0-based).
    int first = (channelGroup == 0) ? 0 : channelGroup - 1;
    int last  = (channelGroup == 0) ? static_cast<int>(desc.addrCount) : channelGroup;

    for (int i = first; i < last && i < static_cast<int>(desc.addrCount); ++i) {
        unsigned int current = 0;
        int rc = register_read(desc.adresses[i], &current);
        if (rc) throw std::runtime_error(
            "Lecture registre 0x" + std::to_string(desc.adresses[i]) +
            " échouée (rc=" + std::to_string(rc) + ").");

        current = (current & ~(desc.mask)) | ((encoded << desc.shift) & desc.mask);

        rc = register_write(desc.adresses[i], current);
        if (rc) throw std::runtime_error(
            "Écriture registre 0x" + std::to_string(desc.adresses[i]) +
            " échouée (rc=" + std::to_string(rc) + ").");
    }
}

/**************************************************************/
//  Reset
/**************************************************************/

int SIS3315Module::resetModule()
{
    int rc = register_write(SIS3315_KEY_RESET, 0x0);
    if (rc) throw std::runtime_error(
        "Key Reset échoué (rc=" + std::to_string(rc) + ").");
    return rc;
}

/**************************************************************/
//  Utilitaires
/**************************************************************/

unsigned int SIS3315Module::max_events_from_duration(double duration_seconds,
                                                      double trigger_rate_hz)
{
    // TODO : implémenter
    return 0;
}

/**************************************************************/
//  Lecture hardware
/**************************************************************/

InputTermination SIS3315Module::ReadInputTermination()
{
    unsigned int hw_version = 0;
    int rc = register_read(SIS3315_HARDWARE_VERSION, &hw_version);
    if (rc) throw std::runtime_error(
        "Lecture Hardware Version échouée (rc=" + std::to_string(rc) + ").");

    return (hw_version & (1u << INPUT_TERMINATION_BIT))
           ? TERMINATION_50OHM
           : TERMINATION_HIZ;
}

InputRange SIS3315Module::ReadInputRange()
{
    unsigned int hw_version = 0;
    int rc = register_read(SIS3315_HARDWARE_VERSION, &hw_version);
    if (rc) throw std::runtime_error(
        "Lecture Hardware Version échouée (rc=" + std::to_string(rc) + ").");

    switch (hw_version & 0x7) {
        case 1:
            std::cout << "[SIS3315] Plage d'entrée : ±2.5 V\n";
            return RANGE_PM25V;
        default:
            std::cout << "[SIS3315] Plage d'entrée inconnue (bits 2:0 = "
                      << (hw_version & 0x7) << ")\n";
            return UNKNOWN_RANGE;
    }
}

/**************************************************************/
//  Low-level acquisition control
/**************************************************************/

int SIS3315Module::Disarm()
{
    int rc = register_write(SIS3315_KEY_DISARM, 0);
    if (rc) throw std::runtime_error(
        "Key Disarm échoué (rc=" + std::to_string(rc) + ").");
    return rc;
}

bool SIS3315Module::Poll(bool         use_nim_mode,
                         bool         expect_bank2,
                         unsigned int active_groups_mask,
                         unsigned int timeout_us)
{
    constexpr unsigned int POLL_INTERVAL_US = 100;
    unsigned int elapsed = 0;

    while (timeout_us == 0 || elapsed < timeout_us) {
        unsigned int acq = 0;
        int rc = register_read(SIS3315_ACQUISITION_CONTROL_STATUS, &acq);
        if (rc) throw std::runtime_error(
            "Lecture Acquisition Control échouée (rc=" + std::to_string(rc) + ").");

        if (use_nim_mode) {
            // Mode NIM : attendre que le module soit armé ET sur la bonne bank.
            bool armed     = (acq & (1u << ACQ_BIT_SAMPLE_LOGIC_ARMED)) != 0;
            bool on_bank2  = (acq & (1u << ACQ_BIT_ARMED_ON_BANK2))     != 0;
            if (armed && (on_bank2 == expect_bank2)) {
                std::cout << "[SIS3315] NIM event detected (acq=0x"
                          << std::hex << acq << std::dec << ")\n";
                return true;
            }
        } else {
// Interface mode: wait for all active groups to raise their flag.
            if ((acq & active_groups_mask) == active_groups_mask)
                return true;
        }

        usleep(POLL_INTERVAL_US);
        elapsed += POLL_INTERVAL_US;
    }
    return false; // timeout
}

bool SIS3315Module::checkBankSwap()
{
    unsigned int acq = 0;
    int rc = register_read(SIS3315_ACQUISITION_CONTROL_STATUS, &acq);
    if (rc) throw std::runtime_error(
        "Lecture Acquisition Control échouée (rc=" + std::to_string(rc) + ").");

    // Bit 17 = 1 -> Bank2 active ; 0 -> Bank1 active.
    return (acq >> ACQ_BIT_ARMED_ON_BANK2) & 1u;
}

void SIS3315Module::read_bank_channels(unsigned int                     bank2_flag,
                                       const std::vector<unsigned int>& channels,
                                       unsigned int*                    buffer,
                                       DataCallback                     cb)
{
    for (unsigned int ch : channels) {
        if (ch > 15)
            throw std::invalid_argument(
                "Invalid channel: " + std::to_string(ch) + " (must be 0–15).");

        unsigned int nbofwords = 0;
        int rc = read_MBLT64_Channel_PreviousBankDataBuffer(
                     bank2_flag, ch, &nbofwords, buffer);

        if (rc) {
            std::cerr << "[SIS3315] Warning: reading channel " << ch
                      << " bank" << (bank2_flag + 1)
                      << " failed (rc=" << rc << "), ignored\n";
            continue;
        }
        if (nbofwords == 0)
            throw std::runtime_error(
                "No words read — channel " + std::to_string(ch) +
                " bank" + std::to_string(bank2_flag + 1) + ".");

        if (cb)
            cb(bank2_flag + 1, ch, buffer, nbofwords);
    }
}

/**************************************************************/
//  Acquisition flow — NIM mode
/**************************************************************/

void SIS3315Module::ControlFlowNIM(const AcquisitionConfig& config,
                                   DataCallback             user_callback,
                                   volatile bool*           run_flag)
{
    /* Datasheet sequence (NIM bank swap): 
        1. Reset + Disarm 
        2. Enable bank swap NIM 
        Loop : 
        3a. Armed poll on Bank2 
        4a/5a. Read Bank1 (bank2_flag=0) 
        3b. Armed poll on Bank1 
        4b/5b. Read Bank2 (bank2_flag=1) 
        6. Disarm 
    */

    int event_count = 0;
    unsigned int buf_size = (config.nof_samples % 2 == 0)
                            ? config.nof_samples
                            : config.nof_samples + 1;
    std::vector<unsigned int> buffer(buf_size);

    // 1.
    resetModule();
    Disarm();

    // 2.
    int rc = register_write(SIS3315_KEY_ENABLE_SAMPLE_BANK_SWAP_CONTROL_WITH_NIM_INPUT, 0);
    if (rc) throw std::runtime_error(
        "Enable NIM bank swap échoué (rc=" + std::to_string(rc) + ").");

    unsigned int acq = 0;
    register_read(SIS3315_ACQUISITION_CONTROL_STATUS, &acq);
    if (!(acq & (1u << ACQ_BIT_NIM_SWAP_ENABLED)))
        throw std::runtime_error("Bit 22 (NIM swap) non levé après activation.");

    while (*run_flag && (config.max_events == 0 || event_count < config.max_events)) {

        // Demi-cycle A : module armé sur Bank2 -> lire Bank1
        if (!Poll(true, true, 0, config.poll_timeout_us))
            throw std::runtime_error("Timeout poll en attendant Bank2 armé.");

        read_bank_channels(0, config.channels, buffer.data(), user_callback);
        ++event_count;

        if ((config.max_events > 0 && event_count >= static_cast<int>(config.max_events))
            || !*run_flag)
            break;

        // Demi-cycle B : module armé sur Bank1 -> lire Bank2
        if (!Poll(true, false, 0, config.poll_timeout_us))
            throw std::runtime_error("Timeout poll en attendant Bank1 armé.");

        read_bank_channels(1, config.channels, buffer.data(), user_callback);
        ++event_count;
    }

    Disarm();
}

/**************************************************************/
//  Flux d'acquisition — mode interface (address threshold)
/**************************************************************/

void SIS3315Module::ControlFlowCycles(const AcquisitionConfig& config,
                                      DataCallback             user_callback,
                                      volatile bool*           run_flag)
{
    /* Datasheet sequence (software bank swap): 
        1. Reset + Disarm 
        2. Write address thresholds 
        3. Disarm + Arm Bank2 
        Loop : 
        4a. Poll threshold flag 
        5a. Disarm + Arm Bank1 
        6a. Check swap 
        7a/8a. Read Bank2 (bank2_flag=1) 
        4b. Poll threshold flag 
        5b. Disarm + Arm Bank2 
        6b. Check swap 
        7b/8b. Read Bank1 (bank2_flag=0) 
        9. Disarm 
    */

    int event_count = 0;
    unsigned int buf_size = (config.nof_samples % 2 == 0)
                            ? config.nof_samples
                            : config.nof_samples + 1;
    std::vector<unsigned int> buffer(buf_size);

    // 1.
    resetModule();
    Disarm();

    // 2. Seuils d'adresse
    static const uint32_t kThreshRegs[4] = {
        SIS3315_ADC_CH1_4_ADDRESS_THRESHOLD_REG,
        SIS3315_ADC_CH5_8_ADDRESS_THRESHOLD_REG,
        SIS3315_ADC_CH9_12_ADDRESS_THRESHOLD_REG,
        SIS3315_ADC_CH13_16_ADDRESS_THRESHOLD_REG,
    };
    static const unsigned int kThreshBits[4] = {
        1u << ACQ_BIT_ADDR_THRESH_CH1_4,
        1u << ACQ_BIT_ADDR_THRESH_CH5_8,
        1u << ACQ_BIT_ADDR_THRESH_CH9_12,
        1u << ACQ_BIT_ADDR_THRESH_CH13_16,
    };

    auto isChannelActive = [&](unsigned int ch) -> bool {
        return std::find(config.channels.begin(), config.channels.end(), ch)
               != config.channels.end();
    };

    unsigned int active_groups_mask = 0;
    for (int g = 0; g < 4; ++g) {
        bool active = isChannelActive(g * 4)     || isChannelActive(g * 4 + 1) ||
                      isChannelActive(g * 4 + 2) || isChannelActive(g * 4 + 3);

        uint32_t threshold = active ? config.address_threshold : 0;
        int rc = register_write(kThreshRegs[g], threshold);
        if (rc) throw std::runtime_error(
            "Écriture seuil groupe " + std::to_string(g) +
            " échouée (rc=" + std::to_string(rc) + ").");

        if (active)
            active_groups_mask |= kThreshBits[g];
    }

    // 3.
    int rc = register_write(SIS3315_KEY_DISARM_AND_ARM_BANK2, 0);
    if (rc) throw std::runtime_error("Disarm + Arm Bank2 échoué (rc=" + std::to_string(rc) + ").");

    while (*run_flag && (config.max_events == 0 || event_count < static_cast<int>(config.max_events))) {

        /*** Demi-cycle A : Bank2 vient de se remplir -> lire Bank2, basculer sur Bank1 ***/

        // 4a.
        if (!Poll(false, false, active_groups_mask, config.poll_timeout_us))
            throw std::runtime_error("Timeout poll address threshold (demi-cycle A).");

        // 5a.
        rc = register_write(SIS3315_KEY_DISARM_AND_ARM_BANK1, 0);
        if (rc) throw std::runtime_error("Disarm + Arm Bank1 échoué (rc=" + std::to_string(rc) + ").");

        // 6a. Après armement de Bank1, le bit 17 doit être 0.
        if (checkBankSwap())
            throw std::runtime_error("Swap check : Bank2 encore active après armement Bank1.");

        // 7a/8a. Lire Bank2 (bank2_flag=1).
        read_bank_channels(1, config.channels, buffer.data(), user_callback);
        ++event_count;

        if ((config.max_events > 0 && event_count >= static_cast<int>(config.max_events)) || !*run_flag)
            break;

        /*** Demi-cycle B : Bank1 vient de se remplir -> lire Bank1, basculer sur Bank2 ***/

        // 4b.
        if (!Poll(false, false, active_groups_mask, config.poll_timeout_us))
            throw std::runtime_error("Timeout poll address threshold (demi-cycle B).");

        // 5b.
        rc = register_write(SIS3315_KEY_DISARM_AND_ARM_BANK2, 0);
        if (rc) throw std::runtime_error("Disarm + Arm Bank2 échoué (rc=" + std::to_string(rc) + ").");

        // 6b. Après armement de Bank2, le bit 17 doit être 1.
        if (!checkBankSwap())
            throw std::runtime_error("Swap check : Bank1 encore active après armement Bank2.");

        // 7b/8b. Lire Bank1 (bank2_flag=0).
        read_bank_channels(0, config.channels, buffer.data(), user_callback);
        ++event_count;
    }

    // 9.
    Disarm();
}

/**************************************************************/
//  Flux d'acquisition — point d'entrée public
/**************************************************************/

void SIS3315Module::ControlFlow(const AcquisitionConfig& config,
                                DataCallback             user_callback,
                                volatile bool*           run_flag,
                                bool                     use_nim_mode)
{
    if (config.channels.empty())
        throw std::invalid_argument("La liste de canaux ne peut pas être vide.");
    if (config.nof_samples == 0)
        throw std::invalid_argument("nof_samples doit être > 0.");
    if (run_flag == nullptr)
        throw std::invalid_argument("run_flag ne peut pas être nullptr.");

    if (use_nim_mode)
        ControlFlowNIM(config, user_callback, run_flag);
    else
        ControlFlowCycles(config, user_callback, run_flag);
}

/**************************************************************/
//  Configuration du signal d'entrée (offset DAC)
/**************************************************************/

void SIS3315Module::ConfigureSignal(unsigned int offset_all_channels)
{
    ReadInputTermination();
    ReadInputRange();

    for (unsigned int ch = 0; ch < 16; ++ch)
        this->adc_dac_offset_ch_array[ch] = offset_all_channels;

    int rc = write_all_adc_dac_offsets();
    if (rc) throw std::runtime_error(
        "write_all_adc_dac_offsets() échoué (rc=" + std::to_string(rc) + ").");
}

/**************************************************************/
//  Configuration de l'horloge
/**************************************************************/

SIS3315ClockConfig SIS3315Module::ClockConfiguration(
    SIS::ADC::SIS3315::SampleRate sample_rate,
    ClockSource                   clock_source,
    FpBusRole                     fp_bus_role,
    NimMode                       nim_mode,
    const NimClockParams*         nim_params)
{
    // 0. Valider la fréquence
    const auto& rates = supportedParameterValues_[ADCParameterType::SamplingRate];
    if (std::find(rates.begin(), rates.end(), static_cast<float>(sample_rate)) == rates.end())
        throw std::invalid_argument("Fréquence d'échantillonnage non supportée.");

    // 1. Reset
    resetModule();

    // 2. Sélectionner la source d'horloge (bits 1:0 du registre de distribution)
    int rc = register_write(SIS3315_SAMPLE_CLOCK_DISTRIBUTION_CONTROL,
                            static_cast<unsigned int>(clock_source));
    if (rc) throw std::runtime_error(
        "Sélection source d'horloge échouée (rc=" + std::to_string(rc) + ").");

    // 3. Configuration spécifique à la source
    switch (clock_source) {

        case CLOCK_SRC_INTERNAL: {
            // set_ADC_bit_clock_frequency() programme le SI570 via I2C,
            // puis effectue en interne le DCM Reset (0x438) et l'attente 5 ms.
            rc = set_ADC_bit_clock_frequency(CLOCK_FREQ_MHZ, nullptr);
            if (rc) throw std::runtime_error(
                "Configuration oscillateur interne échouée (rc=" + std::to_string(rc) + ").");
            break;
        }

        case CLOCK_SRC_FPBUS: {
            // Registre 0x58 — FP-Bus LVDS control
            unsigned int fpbus_cfg = 0;
            if (fp_bus_role == FP_BUS_MASTER) {
                // Maître : distribuer l'horloge interne sur le FP-Bus
                fpbus_cfg = (1u << FP_BUS_SAMPLE_CLOCK_OUT_ENABLE_BIT)
                          | (1u << FP_BUS_STATUS_LINES_OUTPUT_ENABLE_BIT)
                          | (1u << FP_BUS_CONTROL_LINES_OUTPUT_ENABLE_BIT);
                // Bit 5 = 0 : la source de l'horloge FP-Bus est l'oscillateur interne.
            } else {
                // Esclave : recevoir l'horloge, ne pas en émettre
                fpbus_cfg = (1u << FP_BUS_STATUS_LINES_OUTPUT_ENABLE_BIT);
            }
            rc = register_write(SIS3315_FP_LVDS_BUS_CONTROL, fpbus_cfg);
            if (rc) throw std::runtime_error(
                "FP-Bus control échoué (rc=" + std::to_string(rc) + ").");
            usleep(100000); // 100 ms : stabilisation de l'horloge
            break;
        }

        case CLOCK_SRC_NIM: {
            if (nim_mode == NIM_BYPASS) {
                // SI5325 en bypass — horloge NIM transmise telle quelle
                rc = bypass_external_clock_multiplier();
                if (rc) throw std::runtime_error(
                    "Bypass multiplicateur NIM échoué (rc=" + std::to_string(rc) + ").");
            } else {
                // Mode PLL — multiplier la fréquence d'entrée
                if (nim_params == nullptr)
                    throw std::invalid_argument(
                        "nim_params requis pour NIM_MULTIPLY.");

                rc = set_external_clock_multiplier(
                    nim_params->bw_sel,
                    nim_params->n1_hs,
                    nim_params->n1_clk1,
                    nim_params->n1_clk2,
                    nim_params->n2,
                    nim_params->n3);
                if (rc) throw std::runtime_error(
                    "Programmation SI5325 PLL échouée (rc=" + std::to_string(rc) + ").");
            }
            break;
        }

        default:
            throw std::invalid_argument("Source d'horloge non supportée.");
    }

    // 4. Configurer le diviseur d'horloge AD9508
    rc = convert_clock_divider_ad9508_setup(CLOCK_DIVIDER_VALUE);
    if (rc) throw std::runtime_error(
        "Configuration diviseur AD9508 échouée (rc=" + std::to_string(rc) + ").");

    // 5. Reset PLL FPGA
    rc = reset_adc_fpga_sample_clock_PLL();
    if (rc) throw std::runtime_error(
        "Reset PLL FPGA échoué (rc=" + std::to_string(rc) + ").");

    // 6. Calibration des délais IOB
    rc = configure_adc_fpga_iob_delays(static_cast<unsigned int>(IOB_DELAY_VALUE));
    if (rc) throw std::runtime_error(
        "Configuration délais IOB échouée (rc=" + std::to_string(rc) + ").");

    // Résultats
    double actual_msps = static_cast<double>(CLOCK_FREQ_MHZ) / CLOCK_DIVIDER_VALUE;
    double precision_ns = 1000.0 / actual_msps;

    return SIS3315ClockConfig{
        .frequency_msps       = static_cast<double>(sample_rate),
        .bit_clock_mhz        = CLOCK_FREQ_MHZ,
        .actual_frequency_msps = actual_msps,
        .actual_precision_ns  = precision_ns,
        .clock_source         = clock_source,
        .fp_bus_role          = fp_bus_role,
        .nim_mode             = nim_mode,
    };
}