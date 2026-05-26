#pragma once

#include "ADCModule.h"
#include "sis3315-software/libraries_and_includes/sis3315_class_library/sis3315_class.h"
#include "sis3315-software/libraries_and_includes/sis3315_header/sis3315.h"

#include <functional>
#include <vector>
#include <cstdint>

/**************************************************************/
// hardware constants and definitions
/**************************************************************/
static constexpr double SIS3315_MIN_SAMPLING_MSPS    =  1.0;
static constexpr double SIS3315_MAX_SAMPLING_MSPS    = 15.0;
static constexpr int    SIS3315_BIT_TO_CONVERT_RATIO = 12;
static constexpr double IOB_DELAY_VALUE              = 0x14;

static constexpr float  CLOCK_FREQ_MHZ        = 180.0f;
static constexpr int    CLOCK_DIVIDER_VALUE   = 12;
static constexpr int    OUTPUT_CLK_MIN_MHZ    = 10;
static constexpr int    OUTPUT_CLK_MAX_MHZ    = 1400;

// VCO SI570 (MHz)
static constexpr double VCO_MIN = 4850.0;
static constexpr double VCO_MAX = 5670.0;

/**************************************************************/
//  FP-Bus control register bits (0x58)
/**************************************************************/
static constexpr unsigned int FP_BUS_SAMPLE_CLOCK_OUT_MUX_BIT       = 5;
static constexpr unsigned int FP_BUS_SAMPLE_CLOCK_OUT_ENABLE_BIT    = 4;
static constexpr unsigned int FP_BUS_STATUS_LINES_OUTPUT_ENABLE_BIT = 1;
static constexpr unsigned int FP_BUS_CONTROL_LINES_OUTPUT_ENABLE_BIT = 0;

static constexpr unsigned int INPUT_TERMINATION_BIT = 4;

/**************************************************************/
//  Acquisition Control/Status register bits (0x60)
/**************************************************************/
static constexpr unsigned int ACQ_BIT_ADDR_THRESH_CH1_4   = 25;
static constexpr unsigned int ACQ_BIT_ADDR_THRESH_CH5_8   = 27;
static constexpr unsigned int ACQ_BIT_ADDR_THRESH_CH9_12  = 29;
static constexpr unsigned int ACQ_BIT_ADDR_THRESH_CH13_16 = 31;
static constexpr unsigned int ACQ_BIT_ADDR_THRESH_OR      = 19;
static constexpr unsigned int ACQ_BIT_SAMPLE_LOGIC_ARMED  = 16;
static constexpr unsigned int ACQ_BIT_ARMED_ON_BANK2      = 17;
static constexpr unsigned int ACQ_BIT_NIM_SWAP_ENABLED    = 22;

/**************************************************************/
//  enums
/**************************************************************/

enum ClockSource {
    CLOCK_SRC_INTERNAL = 0,
    CLOCK_SRC_FPBUS    = 2,
    CLOCK_SRC_NIM      = 3
};

enum FpBusRole {
    FP_BUS_MASTER,
    FP_BUS_SLAVE
};

enum NimMode {
    NIM_BYPASS,
    NIM_MULTIPLY
};

enum InputTermination {
    TERMINATION_HIZ   = 0,
    TERMINATION_50OHM = 1
};

enum InputRange {
    RANGE_PM25V,
    UNKNOWN_RANGE
};

enum class AcquisitionMode {
    INTERFACE,  // software-triggered bank swap (address threshold)
    NIM         // bank swap triggered by NIM TI/UI signal
};

/**************************************************************/
//  Configuration structures
/**************************************************************/

struct NimClockParams {
    unsigned int bw_sel;    // bandwidth select (0–15)
    unsigned int n1_hs;     // high-speed divider (4–11)
    unsigned int n1_clk1;   // output divider CKOUT1
    unsigned int n1_clk2;   // output divider CKOUT2
    unsigned int n2;        // feedback divider (32–512, pair)
    unsigned int n3;        // input divider (1–219)
    unsigned int clkin_mhz; // input frequency on NIM CI (MHz)
};

struct SIS3315ClockConfig {
    double      frequency_msps;
    double      bit_clock_mhz;
    double      actual_frequency_msps;
    double      actual_precision_ns;
    ClockSource clock_source;
    FpBusRole   fp_bus_role;
    NimMode     nim_mode;
};

struct AcquisitionConfig {
    AcquisitionMode           mode;
    std::vector<unsigned int> channels;         // channels to read (0–15)
    unsigned int              address_threshold; // memory fill threshold
    unsigned int              poll_timeout_us;   // poll timeout (µs), 0 = infinite
    unsigned int              max_events;        // 0 = infinite
    unsigned int              nof_samples;       // samples per trigger
};

/**************************************************************/
//  RegisterDescriptor (for kParamRegistry in the .cpp)
/**************************************************************/

struct RegisterDescriptor {
    const uint32_t*                  adresses;
    uint32_t                         addrCount;
    uint32_t                         mask;
    uint32_t                         shift;
    std::function<uint32_t(uint32_t)> encode;
};

/**************************************************************/
//  User callback type for data readout
/**************************************************************/
using DataCallback = std::function<void(
    unsigned int bank,
    unsigned int channel_no,
    const unsigned int* data,
    unsigned int nbofwords)>;

/**************************************************************/
//  Utilities
/**************************************************************/
template<typename T>
bool inRange(T value, T min, T max) {
    return value >= min && value <= max;
}

/**************************************************************/
//  Main class
/**************************************************************/
class SIS3315Module : public ADCModule, public sis3315_adc {
public:
    SIS3315Module(vme_interface_class* crate, unsigned int baseaddress);

    // --- Overrides ADCModule ---
    int  resetModule() override;
    void InitSupportedValues() override;

    // --- ADC parameters ---
    void  SetParameter(ADCParameterType param, float value, int channelGroup = 0);
    bool  isValueSupported(ADCParameterType param, float value);
    float GetValue();

    // --- Configuration hardware ---
    SIS3315ClockConfig ClockConfiguration(
        SIS::ADC::SIS3315::SampleRate sample_rate,
        ClockSource                   clock_source,
        FpBusRole                     fp_bus_role,
        NimMode                       nim_mode,
        const NimClockParams*         nim_params = nullptr);

    void ConfigureSignal(unsigned int offset_all_channels = 0x8000);

    // --- Hardware readout ---
    InputTermination ReadInputTermination();
    InputRange       ReadInputRange();

    // --- Acquisition control ---
    int  Disarm();
    bool Poll(bool use_nim_mode, bool expect_bank2, unsigned int active_groups_mask, unsigned int timeout_us);
    bool checkBankSwap();
    void read_bank_channels(unsigned int bank2_flag, const std::vector<unsigned int>& channels, unsigned int* buffer, DataCallback cb);

    // --- High-level acquisition flow ---
    void ControlFlow(const AcquisitionConfig& config, DataCallback user_callback, volatile bool* run_flag, bool use_nim_mode);
   
    // --- Utilities ---
    unsigned int max_events_from_duration(double duration_seconds, double trigger_rate_hz);
    bool find_hs_n1(double frequency_mhz, unsigned int& hs_div, unsigned int& n1_div, sis3315_adc* adc);

private:
    void ControlFlowCycles(const AcquisitionConfig& config, DataCallback user_callback, volatile bool* run_flag);
    void ControlFlowNIM(const AcquisitionConfig& config, DataCallback user_callback, volatile bool* run_flag);

};