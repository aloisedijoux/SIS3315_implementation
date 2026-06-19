#pragma once
#include "sis3315-software/libraries_and_includes/sis3315_header/sis3315.h"
#include "sis3315-software/libraries_and_includes/sis_vme_master_class_lib/sis3315_ethernet_access_class.h"
#include "sis3315_class.h"
#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <vector>

// ---------------------------------------------------------------------------
// Acquisition
// ---------------------------------------------------------------------------

enum class AcquisitionMode {
    INTERFACE,  // bank swap by software / address threshold
    NIM,        // bank swap triggered by NIM TI/UI signal
};

struct AcquisitionConfig {
    AcquisitionMode           mode;
    std::vector<unsigned int> channels;
    unsigned int              nof_samples;
    uint32_t                  address_threshold;
    uint32_t                  poll_timeout_us;
    uint32_t                  max_events;
};

AcquisitionConfig ask_acquisition_config();
int setParameter(sis3315_eth* vme_crate, AcquisitionConfig& config);

// ---------------------------------------------------------------------------
// Horloge
// ---------------------------------------------------------------------------

enum class ClockSource {
    INTERNAL,       // sel=0 : internal SI570 oscillator (default 125 MHz)
    EXTERNAL_FPBUS, // sel=2 : external clock via FP-Bus LVDS (frontpanel)
    EXTERNAL_NIM,   // sel=3 : external clock via NIM CL-I input
                    //         (passed through SI5325 multiplier)
};
/*
Complete Clock Configuration
The application sequence is defined by the documentation:
1. Key Reset
2. Write ADC Sample Clock distribution control (source)
3. Source-specific configuration (I2C oscillator / SI5325 multiplier)
+ wait for stabilization
4. Write Bit-Convert divider (AD9508), ratio = 12 for all rates
5. Key ADC Clock DCM/PLL Reset + wait <5 ms
6. (optional) Calibrate tap delay ADC
*/
struct ClockConfig {
    ClockSource source;

    /* --- INTERNAL ---
The SI570 oscillator can be programmed from 10 MHz to 1.4 GHz.
Sampling rate = freq_mhz / 12 (AD9508 divider set to 12).
Preset frequencies available via askClockConfig().
 Value 0 = use raw I2C bytes if provided (advanced use).*/

    double internal_freq_mhz;   

   /*fOUT = fIN * N2 / (N1HS * N1CLK * N3)
Constraints: fIN 10–710 MHz, f3 10–157.5 MHz, fOSC 4.85–5.67 GHz,
fOUT 2 kHz–1.4 GHz
Set bypass_nim_multiplier = true to switch to bypass mode (fOUT = fIN).*/
    bool         bypass_nim_multiplier; // true = bypass SI5325, false = PLL actif
    double       nim_input_freq_mhz;    // fréquence de l'entrée NIM CL-I
    unsigned int nim_n1hs;   // diviseur haute vitesse [4..11]
    unsigned int nim_n1clk1; // diviseur sortie 1      [1,2,4,6,...,220]
    unsigned int nim_n2;     // diviseur feedback       [32,34,36,...,512]
    unsigned int nim_n3;     // diviseur entrée          [1,2,3,...,219]
    unsigned int nim_bw_sel; // bande passante PLL       [0,1,2]

    // ---- EXTERNAL_FPBUS ----

    bool fpbus_is_master;   // true = ce module pilote l'horloge sur le FP-Bus
};

// Displays the 3 available sources and reads the current state of the module
// (returns the currently configured source)
ClockSource   readClockSources(sis3315_eth* vme_crate);

// Interactive input of the clock configuration
ClockConfig   askClockConfig();

// ---------------------------------------------------------------------------
// Trigger
// ---------------------------------------------------------------------------

enum class TriggerSource {
    SOFTWARE,           // KEY_TRIGGER (0x418), software trigger
    EXTERNAL_NIM,       // NIM TI input, synchronous (Acq. Control bit 8)
    FPBUS,              // Front Panel LVDS Bus, Control 1 (Acq. Control bit 4)
    INTERNAL_FIR,       // FIR trigger per individual channel (1–16), asynchronous
    INTERNAL_SUM,       // FIR trigger on sum of a group of 4 channels
    INTERNAL_PILEUP,    // Trigger on pileup detection (Extended Event Cfg)
    INTERNAL_FEEDBACK,  // Internal trigger looped back as global external trigger
                        //   (Acq. Control bit 14 + Internal Trigger Feedback Select)
    COINCIDENCE_LUT,    // Validation by coincidence table (LUT 1 or 2)
};

// CFD mode as defined by bits 29-28 of the Trigger Threshold register
enum class CfdMode : uint8_t {
    DISABLED        = 0x0,  // bits 29-28 = 00 or 01 -> CFD disabled
    ZERO_CROSSING   = 0x2,  // bits 29-28 = 10 -> zero-crossing
    FIFTY_PERCENT   = 0x3,  // bits 29-28 = 11 -> 50 %
};

struct TriggerConfig {
    TriggerSource source;

    // ---- INTERNAL_FIR / INTERNAL_PILEUP ----
    int      channel;           // channel number, 1–16

    // ---- INTERNAL_SUM ----
    int      channel_group;     // group of 4 channels, 1–4
                                //   (group 1 = ch1-4, group 2 = ch5-8, ...)

    // ---- COINCIDENCE_LUT ----
    int      lut_index;         // 1 or 2 (two tables available)
    int      coincidence_window;// window length in samples (2, 4, ..., 512)

    // ---- Common FIR parameters (INTERNAL_FIR and INTERNAL_SUM) ----
    // FIR Trigger Setup register :
    //   bits 31-24 : Pulse Length  [2, 4, 6, ..., 256]   (bit 24 not used)
    //   bits 23-12 : Gap Time      [2, 4, 6, ..., 510]   (bit 12 not used)
    //   bits 11-0  : Peaking Time  [2, 4, 6, ..., 510]   (bit 0  not used)
    int      peaking_time;      // samples, even, 2–510
    int      gap_time;          // samples, even, 2–510
    int      pulse_length;      // samples, even, 2–256

    // Trigger Threshold register :
    //   bit 31     : Trigger Enable
    //   bit 30     : High Energy Suppress (requires CFD enabled)
    //   bits 29-28 : CFD control (see CfdMode)
    //   bits 27-0  : threshold (value compared to MAW + 0x800_0000)
    uint32_t threshold;         // 0 to 0x0FFF_FFFF
    CfdMode  cfd_mode;          // CFD mode (DISABLED / ZERO_CROSSING / FIFTY_PERCENT)
    bool     he_suppress;       // suppression if MAW > he_threshold (CFD required)

    // High Energy Trigger Threshold register :
    //   bit 31     : Trigger on both edges (requires CFD enabled)
    //   bits 29-28 : Internal Trigger Stretched Output mux to VME FPGA
    //   bits 27-0  : HE threshold
    uint32_t he_threshold;      // 0 to 0x0FFF_FFFF (ignored if he_suppress = false)
    bool     trigger_on_both_edges; // bit 31 of the HE Threshold register (CFD required)

    // FIR Trigger Setup register, bits 31-24 :
    bool     trigger_out_enabled; // enables trigger output on LEMO T-O

    // Internal Trigger Delay Configuration register :
    //   delay applied after internal trigger generation before transmission
    //   even values : 0, 2, 4, ..., 510 samples
    int      trigger_delay;     // samples, even, 0–510
};

// ---------------------------------------------------------------------------
// Sampling
// ---------------------------------------------------------------------------

// Averaging mode 
// Reduces thermal noise by sqrtN, decreases effective sampling rate = fs/N
enum class AverageMode : uint8_t {
    OFF    = 0,  // no averaging       (feff = fs)
    AVG4   = 1,  // average over 4  -> feff = fs/4,   SNR * sqrt4
    AVG8   = 2,  // average over 8  -> feff = fs/8,   SNR * sqrt8
    AVG16  = 3,  // average over 16 -> feff = fs/16
    AVG32  = 4,  // average over 32 -> feff = fs/32
    AVG64  = 5,  // average over 64 -> feff = fs/64
    AVG128 = 6,  // average over 128
    AVG256 = 7,  // average over 256 -> at 15 MSPS : feff = 58 kSPS (suitable for bolometers)
};

// omplete configuration of the sampling chain.
// The application sequence (setSamplingParameter) follows the order in the documentation :
//   1. Pre-trigger delay      -> 0x1028 (per group)
//   2. Gate window            -> 0x101C (per group)
//   3. Raw data buffer        -> 0x1020 (per group)
//   4. Data format            -> 0x1030 (per group)
//   5. FIR energy             -> 0x10C0–0x40CC (per channel)
//   6. Averaging              -> 0x102C (per group)
//   7. Input invert (Event Config bit 0) -> 0x1010 (per channel)
struct SamplingConfig {

    // ---- Pre-trigger delay ----
    // Allows looking back in time using the internal circular buffer.
    // t(n) = t_trigger − pretrig_delay * Ts + n * Ts
    uint32_t pretrig_delay;      // even clocks, 0–16378 (bits [13:0])
    bool     pretrig_extra_fir;  // bit 15 : adds P+G from MAW trigger filter to the delay

    // ---- Active trigger window ----
    // Duration during which data is saved after the trigger.
    uint32_t gate_window;        // even clocks, 2–65536 (bit 0 unused)

    // ---- Raw data buffer ----
    // bits [31:16] = number of raw samples to save
    // bits [15:0]  = start index in the window
    uint32_t raw_sample_count;   // 0–65535 (0 = disabled)
    uint32_t raw_start_index;    // 0–65535

    // ---- Data format ----
    // Same configuration applied to all 4 channels of each group.
    // Bit 7 per channel = 0 -> 18 bits native (ADAQ23878), 1 -> 16 bits truncated
    bool     format_18bit;            // true = 18 bits (default), false = 16 bits
    bool     save_maw_test_buffer;    // bit 5 : MAW test buffer (debug FIR)
    bool     save_start_max_energy;   // bit 4 : Start + Max Energy MAW values
    bool     save_maw_3values;        // bit 3 : 3 MAW trigger values
    bool     save_acc78;              // bit 2 : accumulators 7 and 8
    bool     save_acc16_peak;         // bit 1 : Peak High + accumulators 1–6

    // ---- Input inversion (Event Config bit 0 per channel) ----
    // To be used when the physical signal is negative (falling pulse).
    // Bit i (0-based) = 1 -> channel i+1 inverted before the MAW filter.
    uint16_t input_invert_mask;

    // ---- Energy FIR filter  ----
    // Second trapezoidal filter: measures the energy (integral) of the signal.
    // Parameters applied identically to all channels.
    // fOUT = fIN * N2 / (N1HS * N1CLK * N3) (if extra_filter active, divide first)
    bool         energy_fir_enabled;   // false = does not touch the energy FIR registers
    unsigned int energy_tau_table;     // 0–3 : exponential decay correction table
    unsigned int energy_tau_factor;    // 0–63 : exponential decay correction factor
    unsigned int energy_extra_filter;  // 0=off, 1=*4, 2=*8, 3=*16 (oversampling)
    unsigned int energy_gap;           // clocks pairs, 2–510 (energy trapezoid G)
    unsigned int energy_peaking;       // clocks pairs, 2–2046 (energy trapezoid P)

    // ---- Averaging mode ----
    AverageMode  average_mode;            // OFF...AVG256
    uint32_t     average_pretrig_delay;   // pre-trigger delay for the averaged channel (12 bits)
    uint32_t     average_sample_length;   // length of the averaging window (16 bits)
};

SamplingConfig askSamplingConfig();

// ---------------------------------------------------------------------------
// Manager
// ---------------------------------------------------------------------------

class SIS3315Manager {
public:
    SIS3315Manager(const char* ip_address, const char* local_iface = "");
    ~SIS3315Manager();

    // Opens the VME connection to the module (UDP socket, etc.)
    void open();

    // Reads and displays the firmware versions (VME FPGA + 4 ADC FPGAs)
    void readFirmware();

    // Reads and displays the serial number
    void readSerialNumber();

    // Sends a reset to the module
    void resetModule();

    // Reads and displays the current clock source (old helper, retained)
    void readClockSource();

    // Configures and verifies the internal clock (SI570 125 MHz) — quick test
    int testInternalClock();

    // Displays available sources and the current state, returns the active source
    ClockSource readClockSources();

    // Interactive input + full application of the clock configuration
    int configureClockInteractive();

    // Applies an already constructed ClockConfig
    int setClockParameter(const ClockConfig& cfg);

    // Reads and displays the state of all trigger sources
    void readTriggerSources();

    // Tests internal FIR trigger on channel 1
    int testInternalTrigger();

    // Interactive input for trigger configuration
    static TriggerConfig askTriggerConfig();

    // Applies the trigger configuration to the module
    int setTriggerParameter(const TriggerConfig& cfg);

    // Applies the acquisition configuration to the module
    int setAcquisitionParameter(AcquisitionConfig& config);

    // Reads and displays the current sampling configuration
    void readSamplingConfig();

    // Interactive input + full application of the sampling configuration
    int configureSamplingInteractive();

    // Applies an already constructed SamplingConfig
    int setSamplingParameter(const SamplingConfig& cfg);

    // Noise acquisition: arms bank1, fires a software trigger,
    // waits for filling, swaps, reads the 16 channels and displays stats + excerpts
    int acquireNoise(unsigned int nof_samples);

    // Closes the VME connection
    void close();

private:
    sis3315_eth* vme_crate_;
    sis3315_adc* adc_;
    char ip_address_[64];
    char local_iface_[64];
    bool open_;
};

// Saves / loads the complete configuration to/from a text file.
// Returns true if the operation was successful.
bool saveConfigs(const std::string& path,
                 const ClockConfig& clk,
                 const TriggerConfig& trig,
                 const AcquisitionConfig& acq);

bool loadConfigs(const std::string& path,
                 ClockConfig& clk,
                 TriggerConfig& trig,
                 AcquisitionConfig& acq);