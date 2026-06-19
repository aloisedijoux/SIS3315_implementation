#include "progress.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <iomanip>


// Register arrays indexed by channel (0-based) or group (0-based)


static const uint32_t kPreTrigDelayReg[4] = {
    SIS3315_ADC_CH1_4_PRE_TRIGGER_DELAY_REG,
    SIS3315_ADC_CH5_8_PRE_TRIGGER_DELAY_REG,
    SIS3315_ADC_CH9_12_PRE_TRIGGER_DELAY_REG,
    SIS3315_ADC_CH13_16_PRE_TRIGGER_DELAY_REG,
};

static const uint32_t kGateWindowReg[4] = {
    SIS3315_ADC_CH1_4_TRIGGER_GATE_WINDOW_LENGTH_REG,
    SIS3315_ADC_CH5_8_TRIGGER_GATE_WINDOW_LENGTH_REG,
    SIS3315_ADC_CH9_12_TRIGGER_GATE_WINDOW_LENGTH_REG,
    SIS3315_ADC_CH13_16_TRIGGER_GATE_WINDOW_LENGTH_REG,
};

static const uint32_t kRawDataBufReg[4] = {
    SIS3315_ADC_CH1_4_RAW_DATA_BUFFER_CONFIG_REG,
    SIS3315_ADC_CH5_8_RAW_DATA_BUFFER_CONFIG_REG,
    SIS3315_ADC_CH9_12_RAW_DATA_BUFFER_CONFIG_REG,
    SIS3315_ADC_CH13_16_RAW_DATA_BUFFER_CONFIG_REG,
};

static const uint32_t kDataFormatReg[4] = {
    SIS3315_ADC_CH1_4_DATAFORMAT_CONFIG_REG,
    SIS3315_ADC_CH5_8_DATAFORMAT_CONFIG_REG,
    SIS3315_ADC_CH9_12_DATAFORMAT_CONFIG_REG,
    SIS3315_ADC_CH13_16_DATAFORMAT_CONFIG_REG,
};

static const uint32_t kFirEnergySetupReg[16] = {
    SIS3315_ADC_CH1_FIR_ENERGY_SETUP_REG,
    SIS3315_ADC_CH2_FIR_ENERGY_SETUP_REG,
    SIS3315_ADC_CH3_FIR_ENERGY_SETUP_REG,
    SIS3315_ADC_CH4_FIR_ENERGY_SETUP_REG,
    SIS3315_ADC_CH5_FIR_ENERGY_SETUP_REG,
    SIS3315_ADC_CH6_FIR_ENERGY_SETUP_REG,
    SIS3315_ADC_CH7_FIR_ENERGY_SETUP_REG,
    SIS3315_ADC_CH8_FIR_ENERGY_SETUP_REG,
    SIS3315_ADC_CH9_FIR_ENERGY_SETUP_REG,
    SIS3315_ADC_CH10_FIR_ENERGY_SETUP_REG,
    SIS3315_ADC_CH11_FIR_ENERGY_SETUP_REG,
    SIS3315_ADC_CH12_FIR_ENERGY_SETUP_REG,
    SIS3315_ADC_CH13_FIR_ENERGY_SETUP_REG,
    SIS3315_ADC_CH14_FIR_ENERGY_SETUP_REG,
    SIS3315_ADC_CH15_FIR_ENERGY_SETUP_REG,
    SIS3315_ADC_CH16_FIR_ENERGY_SETUP_REG,
};

static const uint32_t kAverageReg[4] = {
    SIS3315_ADC_CH1_4_AVERAGE_CONFIGURATION_REG,
    SIS3315_ADC_CH5_8_AVERAGE_CONFIGURATION_REG,
    SIS3315_ADC_CH9_12_AVERAGE_CONFIGURATION_REG,
    SIS3315_ADC_CH13_16_AVERAGE_CONFIGURATION_REG,
};

static const uint32_t kFirSetupReg[16] = {
    SIS3315_ADC_CH1_FIR_TRIGGER_SETUP_REG,
    SIS3315_ADC_CH2_FIR_TRIGGER_SETUP_REG,
    SIS3315_ADC_CH3_FIR_TRIGGER_SETUP_REG,
    SIS3315_ADC_CH4_FIR_TRIGGER_SETUP_REG,
    SIS3315_ADC_CH5_FIR_TRIGGER_SETUP_REG,
    SIS3315_ADC_CH6_FIR_TRIGGER_SETUP_REG,
    SIS3315_ADC_CH7_FIR_TRIGGER_SETUP_REG,
    SIS3315_ADC_CH8_FIR_TRIGGER_SETUP_REG,
    SIS3315_ADC_CH9_FIR_TRIGGER_SETUP_REG,
    SIS3315_ADC_CH10_FIR_TRIGGER_SETUP_REG,
    SIS3315_ADC_CH11_FIR_TRIGGER_SETUP_REG,
    SIS3315_ADC_CH12_FIR_TRIGGER_SETUP_REG,
    SIS3315_ADC_CH13_FIR_TRIGGER_SETUP_REG,
    SIS3315_ADC_CH14_FIR_TRIGGER_SETUP_REG,
    SIS3315_ADC_CH15_FIR_TRIGGER_SETUP_REG,
    SIS3315_ADC_CH16_FIR_TRIGGER_SETUP_REG,
};

static const uint32_t kSumFirSetupReg[4] = {
    SIS3315_ADC_CH1_4_SUM_FIR_TRIGGER_SETUP_REG,
    SIS3315_ADC_CH5_8_SUM_FIR_TRIGGER_SETUP_REG,
    SIS3315_ADC_CH9_12_SUM_FIR_TRIGGER_SETUP_REG,
    SIS3315_ADC_CH13_16_SUM_FIR_TRIGGER_SETUP_REG,
};

static const uint32_t kThresholdReg[16] = {
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
};

static const uint32_t kSumThresholdReg[4] = {
    SIS3315_ADC_CH1_4_SUM_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH5_8_SUM_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH9_12_SUM_FIR_TRIGGER_THRESHOLD_REG,
    SIS3315_ADC_CH13_16_SUM_FIR_TRIGGER_THRESHOLD_REG,
};

static const uint32_t kHeThresholdReg[16] = {
    SIS3315_ADC_CH1_FIR_HIGH_ENERGY_THRESHOLD_REG,
    SIS3315_ADC_CH2_FIR_HIGH_ENERGY_THRESHOLD_REG,
    SIS3315_ADC_CH3_FIR_HIGH_ENERGY_THRESHOLD_REG,
    SIS3315_ADC_CH4_FIR_HIGH_ENERGY_THRESHOLD_REG,
    SIS3315_ADC_CH5_FIR_HIGH_ENERGY_THRESHOLD_REG,
    SIS3315_ADC_CH6_FIR_HIGH_ENERGY_THRESHOLD_REG,
    SIS3315_ADC_CH7_FIR_HIGH_ENERGY_THRESHOLD_REG,
    SIS3315_ADC_CH8_FIR_HIGH_ENERGY_THRESHOLD_REG,
    SIS3315_ADC_CH9_FIR_HIGH_ENERGY_THRESHOLD_REG,
    SIS3315_ADC_CH10_FIR_HIGH_ENERGY_THRESHOLD_REG,
    SIS3315_ADC_CH11_FIR_HIGH_ENERGY_THRESHOLD_REG,
    SIS3315_ADC_CH12_FIR_HIGH_ENERGY_THRESHOLD_REG,
    SIS3315_ADC_CH13_FIR_HIGH_ENERGY_THRESHOLD_REG,
    SIS3315_ADC_CH14_FIR_HIGH_ENERGY_THRESHOLD_REG,
    SIS3315_ADC_CH15_FIR_HIGH_ENERGY_THRESHOLD_REG,
    SIS3315_ADC_CH16_FIR_HIGH_ENERGY_THRESHOLD_REG,
};

static const uint32_t kSumHeThresholdReg[4] = {
    SIS3315_ADC_CH1_4_SUM_FIR_HIGH_ENERGY_THRESHOLD_REG,
    SIS3315_ADC_CH5_8_SUM_FIR_HIGH_ENERGY_THRESHOLD_REG,
    SIS3315_ADC_CH9_12_SUM_FIR_HIGH_ENERGY_THRESHOLD_REG,
    SIS3315_ADC_CH13_16_SUM_FIR_HIGH_ENERGY_THRESHOLD_REG,
};

static const uint32_t kEventCfgReg[4] = {
    SIS3315_ADC_CH1_4_EVENT_CONFIG_REG,
    SIS3315_ADC_CH5_8_EVENT_CONFIG_REG,
    SIS3315_ADC_CH9_12_EVENT_CONFIG_REG,
    SIS3315_ADC_CH13_16_EVENT_CONFIG_REG,
};

static const uint32_t kExtEventCfgReg[4] = {
    SIS3315_ADC_CH1_4_EXTENDED_EVENT_CONFIG_REG,
    SIS3315_ADC_CH5_8_EXTENDED_EVENT_CONFIG_REG,
    SIS3315_ADC_CH9_12_EXTENDED_EVENT_CONFIG_REG,
    SIS3315_ADC_CH13_16_EXTENDED_EVENT_CONFIG_REG,
};

static const uint32_t kAddrThresholdReg[4] = {
    SIS3315_ADC_CH1_4_ADDRESS_THRESHOLD_REG,
    SIS3315_ADC_CH5_8_ADDRESS_THRESHOLD_REG,
    SIS3315_ADC_CH9_12_ADDRESS_THRESHOLD_REG,
    SIS3315_ADC_CH13_16_ADDRESS_THRESHOLD_REG,
};

static const uint32_t kTrigDelayReg[4] = {
    SIS3315_ADC_CH1_4_INTERNAL_TRIGGER_DELAY_CONFIG_REG,
    SIS3315_ADC_CH5_8_INTERNAL_TRIGGER_DELAY_CONFIG_REG,
    SIS3315_ADC_CH9_12_INTERNAL_TRIGGER_DELAY_CONFIG_REG,
    SIS3315_ADC_CH13_16_INTERNAL_TRIGGER_DELAY_CONFIG_REG,
};


// Helpers


//   bits [13:0] : even delay, 0-16378
//   bit 15      : additional FIR P+G delay
static uint32_t buildPreTrigDelay(uint32_t delay, bool extra_fir)
{
    return (extra_fir ? (1u << 15) : 0u) | (delay & 0x3FFEu);
}

//   bits [31:16] : raw sample count
//   bits [15:0]  : start index within the window
static uint32_t buildRawDataBuf(uint32_t count, uint32_t start)
{
    return ((count & 0xFFFFu) << 16) | (start & 0xFFFFu);
}

// The same 8-bit configuration is repeated for each channel (positions 0, 8, 16, 24)
//   bit 7 : 1 = 16 bits, 0 = 18 bits natifs
//   bit 5 : Save MAW test buffer
//   bit 4 : Save Start + Max Energy MAW
//   bit 3 : Save 3 MAW trigger values
//   bit 2 : Save accumulators 7 and 8
//   bit 1 : Save Peak High + accumulators 1-6
static uint32_t buildDataFormat(const SamplingConfig& cfg)
{
    uint32_t b = 0;
    if (!cfg.format_18bit)           b |= (1u << 7);
    if (cfg.save_maw_test_buffer)    b |= (1u << 5);
    if (cfg.save_start_max_energy)   b |= (1u << 4);
    if (cfg.save_maw_3values)        b |= (1u << 3);
    if (cfg.save_acc78)              b |= (1u << 2);
    if (cfg.save_acc16_peak)         b |= (1u << 1);
    return b | (b << 8) | (b << 16) | (b << 24);
}

//   bits [31:30] : tau_table  (0–3)
//   bits [29:24] : tau_factor (0–63)
//   bits [23:22] : extra_filter (0=off, 1=×4, 2=×8, 3=×16)
//   bits [21:12] : G gap time  (even, 2-510)
//   bits [11:0]  : P peaking   (even, 2-2046)
static uint32_t buildEnergySetup(unsigned int tau_table, unsigned int tau_factor,
                                  unsigned int extra_filter,
                                  unsigned int gap, unsigned int peaking)
{
    return ((tau_table    & 0x3u)  << 30)
         | ((tau_factor   & 0x3Fu) << 24)
         | ((extra_filter & 0x3u)  << 22)
         | ((gap          & 0x1FEu)<< 12)
         |  (peaking      & 0x7FEu);
}

//   bits [30:28] : averaging mode (0-7)
//   bits [27:16] : pre-trigger delay (12 bits)
//   bits [15:0]  : window length (16 bits)
static uint32_t buildAverageConfig(AverageMode mode, uint32_t pretrig, uint32_t length)
{
    return ((static_cast<uint32_t>(mode) & 0x7u) << 28)
         | ((pretrig & 0xFFFu) << 16)
         |  (length  & 0xFFFFu);
}

//   bits 31-25 : Pulse Length [2..256], bit 24 unused
//   bits 23-13 : Gap Time     [2..510], bit 12 unused
//   bits 11-1  : Peaking Time [2..510], bit 0  unused
static uint32_t buildFirSetup(int peaking, int gap, int pulse)
{
    return ((static_cast<uint32_t>(pulse   & 0x1FE)) << 24)
         | ((static_cast<uint32_t>(gap     & 0x1FE)) << 12)
         |  (static_cast<uint32_t>(peaking & 0x1FE));
}

//   bit 31     : Trigger Enable
//   bit 30     : HE Suppress (requires CFD enabled)
//   bits 29-28 : CFD control
//   bits 27-0  : threshold
static uint32_t buildThreshold(uint32_t thr, CfdMode cfd, bool he_suppress, bool enabled = true)
{
    return (enabled    ? (1u << 31) : 0u)
         | (he_suppress ? (1u << 30) : 0u)
         | ((static_cast<uint32_t>(cfd) & 0x3) << 28)
         | (thr & 0x0FFFFFFFu);
}

//   bit 31     : Trigger on both edges (requires CFD)
//   bits 29-28 : Internal Trigger Stretched Output Pulse mux -> VME FPGA
//                00 = Internal Trigger, 01 = HE Trigger, 10 = Pileup Pulse
//   bits 27-0  : HE threshold
static uint32_t buildHeThreshold(uint32_t he_thr, bool both_edges, uint8_t mux_bits = 0x0)
{
    return (both_edges ? (1u << 31) : 0u)
         | ((static_cast<uint32_t>(mux_bits) & 0x3) << 28)
         | (he_thr & 0x0FFFFFFFu);
}


// SIS3315Manager – construction / destruction


SIS3315Manager::SIS3315Manager(const char* ip_address, const char* local_iface)
    : vme_crate_(nullptr), adc_(nullptr), open_(false)
{
    strncpy(ip_address_, ip_address, sizeof(ip_address_) - 1);
    ip_address_[sizeof(ip_address_) - 1] = '\0';
    strncpy(local_iface_, local_iface, sizeof(local_iface_) - 1);
    local_iface_[sizeof(local_iface_) - 1] = '\0';
}

SIS3315Manager::~SIS3315Manager() {
    if (open_) close();
    delete adc_;
    delete vme_crate_;
}


// open / close


void SIS3315Manager::open() {
    vme_crate_ = new sis3315_eth();
    vme_crate_->set_UdpSocketOptionBufSize(335544432);
    vme_crate_->set_UdpSocketBindMyOwnPort(local_iface_);
    vme_crate_->set_UdpSocketSIS3315_IpAddress(ip_address_);
    vme_crate_->udp_reset_cmd();
    vme_crate_->vme_A32D32_write(SIS3315_INTERFACE_ACCESS_ARBITRATION_CONTROL, 0x80000000);
    vme_crate_->vme_A32D32_write(SIS3315_INTERFACE_ACCESS_ARBITRATION_CONTROL, 0x1);

    int reply = vme_crate_->vmeopen();
    if (reply != 0)
        throw std::runtime_error("Failed to open VME connection");
    open_ = true;

    adc_ = new sis3315_adc(vme_crate_, 0);

    vme_crate_->clear_UdpReceiveBuffer();

    unsigned int data = 0;
    reply = vme_crate_->vme_A32D32_read(SIS3315_MODID, &data);
    if ((data & 0xffff0000) != 0x33150000 || reply)
        throw std::runtime_error("Could not access module");

    std::cout << "VME FPGA firmware version = 0x"
              << std::hex << data << std::dec << std::endl;
}

void SIS3315Manager::close() {
    if (vme_crate_ && open_) {
        vme_crate_->vmeclose();
        open_ = false;
    }
}


// Module information


void SIS3315Manager::readFirmware() {
    static const uint32_t fw_addr[4] = {
        SIS3315_ADC_CH1_4_FIRMWARE_REG,
        SIS3315_ADC_CH5_8_FIRMWARE_REG,
        SIS3315_ADC_CH9_12_FIRMWARE_REG,
        SIS3315_ADC_CH13_16_FIRMWARE_REG,
    };
    static const char fw_text[4][32] = {
        "ADC 1-4  FPGA Firmware",
        "ADC 5-8  FPGA Firmware",
        "ADC 9-12 FPGA Firmware",
        "ADC 13-16 FPGA Firmware",
    };
    unsigned int data = 0;
    for (int i = 0; i < 4; i++) {
        int reply = vme_crate_->vme_A32D32_read(fw_addr[i], &data);
        if (reply)
            std::cerr << "Error reading " << fw_text[i] << std::endl;
        else
            std::cout << fw_text[i] << " version = 0x"
                      << std::hex << data << std::dec << std::endl;
    }
}

void SIS3315Manager::readSerialNumber() {
    unsigned int data = 0;
    if (vme_crate_->vme_A32D32_read(SIS3315_SERIAL_NUMBER_REG, &data))
        std::cerr << "Error reading serial number" << std::endl;
    else
        std::cout << "Serial Number = " << (data & 0xffff) << std::endl;
}

void SIS3315Manager::resetModule() {
    if (vme_crate_->vme_A32D32_write(SIS3315_KEY_RESET, 0) != 0)
        throw std::runtime_error("Reset command failed");
    std::cout << "Module reset OK" << std::endl;
}


// Clock - internal helpers


// Returns 0 if all PLLs are locked, 1 otherwise.
static int checkPllLocked(sis3315_eth* vme_crate) {
    static const uint32_t status_regs[4] = {
        SIS3315_ADC_CH1_4_STATUS_REG,
        SIS3315_ADC_CH5_8_STATUS_REG,
        SIS3315_ADC_CH9_12_STATUS_REG,
        SIS3315_ADC_CH13_16_STATUS_REG,
    };
    static const char* names[4] = {"CH1-4", "CH5-8", "CH9-12", "CH13-16"};
    bool all_locked = true;
    unsigned int data;
    for (int i = 0; i < 4; i++) {
        if (vme_crate->vme_A32D32_read(status_regs[i], &data)) {
            std::cerr << "  Erreur lecture statut ADC " << names[i] << std::endl;
            return 1;
        }
        bool locked = (data >> 20) & 0x1;
        std::cout << "  ADC " << names[i] << " PLL locked = " << (locked ? "OUI" : "NON")
                  << " (status = 0x" << std::hex << data << std::dec << ")" << std::endl;
        if (!locked) all_locked = false;
    }
    return all_locked ? 0 : 1;
}


// Clock - readClockSource (kept for compatibility)


void SIS3315Manager::readClockSource() {
    unsigned int data;
    if (vme_crate_-> vme_A32D32_read(SIS3315_SAMPLE_CLOCK_DISTRIBUTION_CONTROL, &data)) {
        std::cerr << "Error reading clock source" << std::endl;
        return;
    }
    int selection = data & 0x3;// 0x3 because bits 1-0 select the clock source
    std::cout << "Clock source register = 0x" << std::hex << data << std::dec << std::endl;
    std::cout << "Active source (bits 1-0 = " << selection << "): ";
    switch (selection) {
        case 0: std::cout << "SI570 internal oscillator (125 MHz default)" << std::endl; break;
        case 1: std::cout << "Not used" << std::endl; break;
        case 2: std::cout << "External FP-Bus LVDS clock (front panel)" << std::endl; break;
        case 3: std::cout << "External NIM CL-I clock (via SI5325 multiplier)" << std::endl; break;
    }

}

// Clock - readClockSources  (read and display full state)


ClockSource SIS3315Manager::readClockSources() {
    unsigned int data;
    std::cout << "\n=== Sources d'horloge disponibles ===" << std::endl;

    // Source 1 : internal
    std::cout << "\n[1] Oscillateur interne SI570" << std::endl;
    std::cout << "    Programmable de 10 MHz à 1.4 GHz (défaut : 125 MHz)" << std::endl;
    std::cout << "    Taux d'échantillonnage = fréquence / 12" << std::endl;
    std::cout << "    Exemples : 125 MHz -> 10.4 MSPS | 180 MHz -> 15 MSPS (max)" << std::endl;
    std::cout << "    Disponibilité : toujours disponible (embarqué)" << std::endl;

    // Source 2 : FP-Bus
    unsigned int fpbus_reg = 0;
    bool fpbus_ok = false;
    if (!vme_crate_->vme_A32D32_read(SIS3315_FP_LVDS_BUS_CONTROL, &fpbus_reg)) {
        fpbus_ok = true;
        bool clk_out_en = (fpbus_reg >> 4) & 0x1;
        bool ctrl_out   = (fpbus_reg >> 0) & 0x1;
        std::cout << "\n[2] Horloge externe FP-Bus LVDS" << std::endl;
        std::cout << "    Registre FP-Bus Control = 0x" << std::hex << fpbus_reg << std::dec << std::endl;
        std::cout << "    Clock Out Enable (bit 4) = " << clk_out_en
                  << (clk_out_en ? " -> ce module exporte l'horloge sur le bus"
                                 : " -> ce module reçoit l'horloge depuis le bus") << std::endl;
        std::cout << "    Control Lines Output Enable (bit 0) = " << ctrl_out << std::endl;
        std::cout << "    Disponibilité : nécessite une horloge stable sur le connecteur FP-Bus" << std::endl;
    } else {
        std::cout << "\n[2] Horloge externe FP-Bus LVDS" << std::endl;
        std::cout << "    (erreur lecture registre FP-Bus)" << std::endl;
    }

    // Source 3 : NIM via SI5325
    std::cout << "\n[3] Horloge externe NIM CL-I (multiplicateur SI5325)" << std::endl;
    std::cout << "    fIN : 10–710 MHz  |  fOUT = fIN × N2 / (N1HS × N1CLK × N3)" << std::endl;
    std::cout << "    Mode bypass disponible (fOUT = fIN, sans PLL)" << std::endl;
    std::cout << "    Contrainte stabilisation : jusqu'à 1200 ms après config" << std::endl;
    std::cout << "    Disponibilité : nécessite un signal sur l'entrée NIM CL-I" << std::endl;

    // Currently active source
    ClockSource current = ClockSource::INTERNAL;
    if (!vme_crate_->vme_A32D32_read(SIS3315_SAMPLE_CLOCK_DISTRIBUTION_CONTROL, &data)) {
        int sel = data & 0x3;
        std::cout << "\nSource actuellement configurée (bits 1-0 = " << sel << ") : ";
        switch (sel) {
            case 0:
                std::cout << "Oscillateur interne" << std::endl;
                current = ClockSource::INTERNAL;
                break;
            case 2:
                std::cout << "FP-Bus LVDS" << std::endl;
                current = ClockSource::EXTERNAL_FPBUS;
                break;
            case 3:
                std::cout << "NIM CL-I" << std::endl;
                current = ClockSource::EXTERNAL_NIM;
                break;
            default:
                std::cout << "Inconnue (sel=" << sel << ")" << std::endl;
                break;
        }
    }
    return current;
}


// Clock - askClockConfig  (interactive input)


ClockConfig askClockConfig() {
    ClockConfig cfg = {};
    cfg.bypass_nim_multiplier = false;
    cfg.fpbus_is_master       = false;

    std::cout << "\n=== Configuration de la source d'horloge ===" << std::endl;
    std::cout << "  1. Oscillateur interne SI570" << std::endl;
    std::cout << "  2. Horloge externe FP-Bus LVDS" << std::endl;
    std::cout << "  3. Horloge externe NIM CL-I (via SI5325)" << std::endl;
    std::cout << "Choix : ";
    int choice;
    std::cin >> choice;

    switch (choice) {

        case 1:
            cfg.source = ClockSource::INTERNAL;
            std::cout << "\nFréquences prédéfinies :" << std::endl;
            std::cout << "  1.  62.5 MHz ->  5.2 MSPS" << std::endl;
            std::cout << "  2. 125.0 MHz -> 10.4 MSPS (défaut)" << std::endl;
            std::cout << "  3. 150.0 MHz -> 12.5 MSPS" << std::endl;
            std::cout << "  4. 180.0 MHz -> 15.0 MSPS (maximum)" << std::endl;
            std::cout << "  5. Fréquence personnalisée (10–1400 MHz)" << std::endl;
            std::cout << "Choix : ";
            {
                int freq_choice;
                std::cin >> freq_choice;
                switch (freq_choice) {
                    case 1:  cfg.internal_freq_mhz = 62.5;  break;
                    case 2:  cfg.internal_freq_mhz = 125.0; break;
                    case 3:  cfg.internal_freq_mhz = 150.0; break;
                    case 4:  cfg.internal_freq_mhz = 180.0; break;
                    default:
                        std::cout << "Fréquence en MHz (10–1400) : ";
                        std::cin >> cfg.internal_freq_mhz;
                        if (cfg.internal_freq_mhz < 10.0 || cfg.internal_freq_mhz > 1400.0) {
                            std::cerr << "Hors plage, 125 MHz par défaut" << std::endl;
                            cfg.internal_freq_mhz = 125.0;
                        }
                        break;
                }
            }
            std::cout << "-> Fréquence SI570 = " << cfg.internal_freq_mhz
                      << " MHz  |  taux = " << cfg.internal_freq_mhz / 12.0
                      << " MSPS" << std::endl;
            break;

        case 2:
            cfg.source = ClockSource::EXTERNAL_FPBUS;
            std::cout << "\nCe module est-il le maître de l'horloge FP-Bus ?" << std::endl;
            std::cout << "(= exporte son horloge sur le bus, un seul module doit l'être)" << std::endl;
            std::cout << "Maître ? (0=non, 1=oui) : ";
            {
                int master;
                std::cin >> master;
                cfg.fpbus_is_master = (master == 1);
            }
            if (!cfg.fpbus_is_master)
                std::cout << "-> Ce module recevra l'horloge depuis le FP-Bus" << std::endl;
            else
                std::cout << "-> Ce module exportera son oscillateur interne sur le FP-Bus" << std::endl;
            break;

        case 3:
            cfg.source = ClockSource::EXTERNAL_NIM;
            std::cout << "\nMode bypass SI5325 ? (fOUT = fIN, pas de PLL) (0=non, 1=oui) : ";
            {
                int bypass;
                std::cin >> bypass;
                cfg.bypass_nim_multiplier = (bypass == 1);
            }
            std::cout << "Fréquence d'entrée NIM CL-I en MHz (10–710) : ";
            std::cin >> cfg.nim_input_freq_mhz;

            if (!cfg.bypass_nim_multiplier) {
                std::cout << "\nParamètres du multiplicateur SI5325" << std::endl;
                std::cout << "  fOUT = fIN × N2 / (N1HS × N1CLK1 × N3)" << std::endl;
                std::cout << "  Exemple pour fIN=10 MHz -> fOUT=250 MHz :" << std::endl;
                std::cout << "    N1HS=5, N1CLK1=4, N2=500, N3=1, bw_sel=0" << std::endl;
                std::cout << "  Exemple pour fIN=50 MHz -> fOUT=125 MHz :" << std::endl;
                std::cout << "    N1HS=5, N1CLK1=8, N2=100, N3=1, bw_sel=1" << std::endl;
                std::cout << "N1HS   [4–11]      : "; std::cin >> cfg.nim_n1hs;
                std::cout << "N1CLK1 [1,2,4,…,220] : "; std::cin >> cfg.nim_n1clk1;
                std::cout << "N2     [32,34,…,512] : "; std::cin >> cfg.nim_n2;
                std::cout << "N3     [1,2,3,…,219] : "; std::cin >> cfg.nim_n3;
                std::cout << "bw_sel [0,1,2]       : "; std::cin >> cfg.nim_bw_sel;

                double fout = cfg.nim_input_freq_mhz
                            * static_cast<double>(cfg.nim_n2)
                            / (static_cast<double>(cfg.nim_n1hs)
                             * static_cast<double>(cfg.nim_n1clk1)
                             * static_cast<double>(cfg.nim_n3));
                std::cout << "-> fOUT calculée = " << fout
                          << " MHz  |  taux = " << fout / 12.0 << " MSPS" << std::endl;
            } else {
                std::cout << "-> Bypass : fOUT = " << cfg.nim_input_freq_mhz
                          << " MHz  |  taux = " << cfg.nim_input_freq_mhz / 12.0
                          << " MSPS" << std::endl;
            }
            break;

        default:
            std::cerr << "Choix invalide, oscillateur interne 125 MHz par défaut" << std::endl;
            cfg.source            = ClockSource::INTERNAL;
            cfg.internal_freq_mhz = 125.0;
            break;
    }

    return cfg;
}




int SIS3315Manager::setClockParameter(const ClockConfig& cfg) {
    unsigned int data;
    std::cout << "\n=== Application de la configuration horloge ===" << std::endl;

    // Step 1: Key Reset (disarms the sampling logic)
    if (vme_crate_->vme_A32D32_write(SIS3315_KEY_RESET, 0)) {
        std::cerr << "Étape 1 – Erreur Key Reset" << std::endl; return 1;
    }
    std::cout << "[1] Key Reset envoyé" << std::endl;

    // Step 2: select the source in ADC Sample Clock distribution control
    uint32_t sel_bits;
    switch (cfg.source) {
        case ClockSource::INTERNAL:       sel_bits = 0; break;
        case ClockSource::EXTERNAL_FPBUS: sel_bits = 2; break;
        case ClockSource::EXTERNAL_NIM:   sel_bits = 3; break;
    }
    if (vme_crate_->vme_A32D32_read(SIS3315_SAMPLE_CLOCK_DISTRIBUTION_CONTROL, &data)) {
        std::cerr << "Étape 2 – Erreur lecture registre horloge" << std::endl; return 1;
    }
    data = (data & ~0x3u) | sel_bits;
    if (vme_crate_->vme_A32D32_write(SIS3315_SAMPLE_CLOCK_DISTRIBUTION_CONTROL, data)) {
        std::cerr << "Étape 2 – Erreur écriture source horloge" << std::endl; return 1;
    }
    std::cout << "[2] Source horloge sélectionnée (bits 1-0 = " << sel_bits << ")" << std::endl;

    // Step 3: source-specific configuration
    switch (cfg.source) {

        case ClockSource::INTERNAL: {
            // SI570 programming via I2C (set_frequency, osc=0)
            // The I2C bytes depend on the target frequency. The SIS library's
            // set_frequency method accepts 6 SI570 configuration bytes.
            //   250 MHz : 0x20 0xC2 0xBC 0x33 0xE4 0xF2
            //   125 MHz : 0x21 0xC2 0xBC 0x33 0xE4 0xF2
            unsigned char si570_bytes[6];
            bool use_raw_bytes = false;

            if (cfg.internal_freq_mhz == 250.0) {
                si570_bytes[0]=0x20; si570_bytes[1]=0xC2; si570_bytes[2]=0xBC;
                si570_bytes[3]=0x33; si570_bytes[4]=0xE4; si570_bytes[5]=0xF2;
                use_raw_bytes = true;
            } else if (cfg.internal_freq_mhz == 125.0) {
                si570_bytes[0]=0x21; si570_bytes[1]=0xC2; si570_bytes[2]=0xBC;
                si570_bytes[3]=0x33; si570_bytes[4]=0xE4; si570_bytes[5]=0xF2;
                use_raw_bytes = true;
            }

            if (use_raw_bytes) {
                if (adc_->set_frequency(0, si570_bytes)) {
                    std::cerr << "Étape 3 – Erreur programmation SI570" << std::endl; return 1;
                }
                std::cout << "[3] SI570 programmé à " << cfg.internal_freq_mhz << " MHz (bytes I2C directs)" << std::endl;
            } else {
                // Use change_frequency_HSdiv_N1div for other frequencies
                // The library computes the dividers from the target frequency
                // via get_SI570_oscillator_hs_div_and_n1_div_values.
                unsigned int hs_div = 0, n1_div = 0;
                double dummy = 0.0;
                // The frequency choice must match SIS3315 constants
                // (e.g. SAMPLERATE_10MSPS, SAMPLERATE_15MSPS, etc.)
                // Here we use frequency_MHz * 12 as the target bit clock frequency.
                // The API expects a frequency constant; we pass the index.
                // In practice the user should use the SAMPLERATE_* constants from the SIS header.
                std::cout << "[3] Fréquence " << cfg.internal_freq_mhz
                          << " MHz – appel change_frequency_HSdiv_N1div requis" << std::endl;
                std::cout << "    (utiliser les constantes SAMPLERATE_* du header SIS3315)" << std::endl;
                // Note: if the change_frequency_HSdiv_N1div method is available,
                //   vme_crate_->get_SI570_oscillator_hs_div_and_n1_div_values(
                //       target_freq_const, &hs_div, &n1_div, &dummy);
                //   vme_crate_->change_frequency_HSdiv_N1div(0, hs_div, n1_div);
            }
            usleep(10000);
            std::cout << "    Attente stabilisation SI570 (10 ms)" << std::endl;
            break;
        }

        case ClockSource::EXTERNAL_FPBUS: {
            unsigned int fpbus_reg = 0;
            if (vme_crate_->vme_A32D32_read(SIS3315_FP_LVDS_BUS_CONTROL, &fpbus_reg)) {
                std::cerr << "Étape 3 – Erreur lecture FP-Bus Control" << std::endl; return 1;
            }
            if (cfg.fpbus_is_master) {
                fpbus_reg |=  (1u << 4);  // Clock Out Enable: exports clock to bus
                fpbus_reg |=  (1u << 0);  // Control Lines Output Enable
                fpbus_reg &= ~(1u << 5);  // Clock Out MUX = 0 -> internal oscillator
            } else {
                fpbus_reg &= ~(1u << 4);  // Clock Out Disable: receives clock from bus
            }
            if (vme_crate_->vme_A32D32_write(SIS3315_FP_LVDS_BUS_CONTROL, fpbus_reg)) {
                std::cerr << "Étape 3 – Erreur écriture FP-Bus Control" << std::endl; return 1;
            }
            std::cout << "[3] FP-Bus configuré en mode "
                      << (cfg.fpbus_is_master ? "maître (export horloge)" : "esclave (réception horloge)")
                      << std::endl;
            std::cout << "    Assurez-vous qu'une horloge stable est présente sur le FP-Bus" << std::endl;
            break;
        }

        case ClockSource::EXTERNAL_NIM: {
            if (cfg.bypass_nim_multiplier) {
                if (adc_->bypass_external_clock_multiplier()) {
                    std::cerr << "Étape 3 – Erreur bypass SI5325" << std::endl; return 1;
                }
                std::cout << "[3] SI5325 en mode bypass (fOUT = fIN = "
                          << cfg.nim_input_freq_mhz << " MHz)" << std::endl;
            } else {
                if (adc_->set_external_clock_multiplier(
                        cfg.nim_bw_sel,
                        cfg.nim_n1hs,
                        cfg.nim_n1clk1,
                        cfg.nim_n1clk1,  // N1CLK2 = N1CLK1 (symmetric output)
                        cfg.nim_n2,
                        cfg.nim_n3,
                        static_cast<unsigned int>(cfg.nim_input_freq_mhz))) {
                    std::cerr << "Étape 3 – Erreur programmation SI5325" << std::endl; return 1;
                }
                double fout = cfg.nim_input_freq_mhz
                            * static_cast<double>(cfg.nim_n2)
                            / (static_cast<double>(cfg.nim_n1hs)
                             * static_cast<double>(cfg.nim_n1clk1)
                             * static_cast<double>(cfg.nim_n3));
                std::cout << "[3] SI5325 configuré : fOUT = " << fout << " MHz" << std::endl;
            }
            std::cout << "    Attente stabilisation SI5325 (1200 ms)" << std::endl;
            usleep(1200000);
            break;
        }
    }

    // Register value = 0x900 | (divider - 1) = 0x900 | 11
    if (vme_crate_->vme_A32D32_write(SIS3315_CNV_CLK_DIVIDER_SPI_REG, 0x900 + 11)) {
        std::cerr << "Étape 4 – Erreur écriture diviseur AD9508" << std::endl; return 1;
    }
    std::cout << "[4] Diviseur AD9508 Bit->Convert = 12" << std::endl;

    // Step 5: Key ADC Clock DCM/PLL Reset + lock verification
    if (vme_crate_->vme_A32D32_write(SIS3315_KEY_ADC_CLOCK_DCM_RESET, 0)) {
        std::cerr << "Étape 5 – Erreur Key ADC Clock DCM/PLL Reset" << std::endl; return 1;
    }
    std::cout << "[5] Key ADC Clock DCM/PLL Reset envoyé" << std::endl;

    if (checkPllLocked(vme_crate_) != 0) {
        std::cerr << "Étape 5 – Erreur : PLL non verrouillé" << std::endl; return 1;
    }
    std::cout << "[5] Tous les PLL verrouillés" << std::endl;
    std::cout << "Configuration horloge appliquée avec succès" << std::endl;
    return 0;
}


// Clock - configureClockInteractive  (interactive input + apply)


int SIS3315Manager::configureClockInteractive() {
    readClockSources();
    ClockConfig cfg = askClockConfig();
    return setClockParameter(cfg);
}


// Clock - testInternalClock  (kept, quick 125 MHz test)


int SIS3315Manager::testInternalClock() {
    std::cout << "\n=== Test horloge interne (125 MHz) ===" << std::endl;
    ClockConfig cfg = {};
    cfg.source            = ClockSource::INTERNAL;
    cfg.internal_freq_mhz = 125.0;
    return setClockParameter(cfg);
}


// Read trigger source state


void SIS3315Manager::readTriggerSources() {
    unsigned int data;
    std::cout << "\n=== Sources de trigger disponibles ===" << std::endl;

    // [1] Software
    std::cout << "\n[1] Trigger logiciel (KEY_TRIGGER 0x418)" << std::endl;
    std::cout << "    -> Toujours disponible via software" << std::endl;

    if (!vme_crate_->vme_A32D32_read(SIS3315_ACQUISITION_CONTROL_STATUS, &data)) {
        bool ext_nim     = (data >>  8) & 0x1;
        bool fpbus       = (data >>  4) & 0x1;
        bool feedback    = (data >> 14) & 0x1;
        std::cout << "\n[2] Trigger externe NIM (entrée TI)" << std::endl;
        std::cout << "    -> " << (ext_nim  ? "ACTIVÉ" : "DÉSACTIVÉ")
                  << " (ACQUISITION_CONTROL bit 8)" << std::endl;
        std::cout << "\n[3] Trigger FP-Bus LVDS (Control 1)" << std::endl;
        std::cout << "    -> " << (fpbus    ? "ACTIVÉ" : "DÉSACTIVÉ")
                  << " (ACQUISITION_CONTROL bit 4)" << std::endl;
        std::cout << "\n[4] Feedback trigger interne comme trigger externe global" << std::endl;
        std::cout << "    -> " << (feedback ? "ACTIVÉ" : "DÉSACTIVÉ")
                  << " (ACQUISITION_CONTROL bit 14)" << std::endl;
    }

    // [5] Internal FIR trigger per channel
    std::cout << "\n[5] Trigger interne FIR par canal :" << std::endl;
    static const char* cfd_str[4] = {"désactivé", "désactivé", "zero-crossing", "50%"};
    for (int ch = 0; ch < 16; ch++) {
        if (vme_crate_->vme_A32D32_read(kThresholdReg[ch], &data)) {
            std::cerr << "    Erreur lecture seuil canal " << ch + 1 << std::endl;
            continue;
        }
        bool     enabled    = (data >> 31) & 0x1;
        bool     he_sup     = (data >> 30) & 0x1;
        int      cfd        = (data >> 28) & 0x3;
        uint32_t thr        = data & 0x0FFFFFFFu;
        std::cout << "    CH" << std::setw(2) << ch + 1
                  << ": " << (enabled ? "ACTIVÉ " : "DÉSACTIVÉ")
                  << "  seuil=0x" << std::hex << std::setw(7) << std::setfill('0') << thr
                  << std::dec << std::setfill(' ')
                  << "  CFD=" << cfd_str[cfd]
                  << (he_sup ? "  [HE-suppress]" : "")
                  << std::endl;
    }

    // [6] SUM trigger per group
    std::cout << "\n[6] Trigger SUM FIR (somme de 4 canaux) :" << std::endl;
    static const char* grp_names[4] = {"CH1-4", "CH5-8", "CH9-12", "CH13-16"};
    for (int g = 0; g < 4; g++) {
        if (vme_crate_->vme_A32D32_read(kSumThresholdReg[g], &data)) {
            std::cerr << "    Erreur lecture seuil SUM groupe " << g + 1 << std::endl;
            continue;
        }
        bool     enabled = (data >> 31) & 0x1;
        uint32_t thr     = data & 0x0FFFFFFFu;
        std::cout << "    Groupe " << grp_names[g]
                  << ": " << (enabled ? "ACTIVÉ " : "DÉSACTIVÉ")
                  << "  seuil=0x" << std::hex << std::setw(7) << std::setfill('0') << thr
                  << std::dec << std::setfill(' ')
                  << std::endl;
    }

    // [7] Pileup trigger (Extended Event Config, bit 0/8/16/24 per channel in group)
    std::cout << "\n[7] Trigger Pileup (Internal Pileup Trigger Enable) :" << std::endl;
    for (int g = 0; g < 4; g++) {
        if (vme_crate_->vme_A32D32_read(kExtEventCfgReg[g], &data)) {
            std::cerr << "    Erreur lecture Extended Event Config groupe " << g + 1 << std::endl;
            continue;
        }
        for (int c = 0; c < 4; c++) {
            int ch        = g * 4 + c;
            bool enabled  = (data >> (c * 8)) & 0x1;
            std::cout << "    CH" << std::setw(2) << ch + 1
                      << ": " << (enabled ? "ACTIVÉ" : "DÉSACTIVÉ") << std::endl;
        }
    }
}


// Internal trigger test (channel 1)


int SIS3315Manager::testInternalTrigger() {
    unsigned int data;
    std::cout << "\n=== Test trigger interne (canal 1) ===" << std::endl;

    if (vme_crate_->vme_A32D32_write(SIS3315_KEY_DISARM, 0)) {
        std::cerr << "Erreur désarmement" << std::endl; return 1;
    }

    if (vme_crate_->vme_A32D32_write(SIS3315_ADC_CH1_4_TRIGGER_GATE_WINDOW_LENGTH_REG, 0x000003FEu)) {
        std::cerr << "Erreur écriture gate window" << std::endl; return 1;
    }
    std::cout << "Trigger gate window = 1024 samples" << std::endl;

    // FIR : peaking=4, gap=4, pulse=10 -> buildFirSetup(4, 4, 10)
    if (vme_crate_->vme_A32D32_write(SIS3315_ADC_CH1_FIR_TRIGGER_SETUP_REG,
                                      buildFirSetup(4, 4, 10))) {
        std::cerr << "Erreur écriture FIR setup" << std::endl; return 1;
    }
    std::cout << "FIR canal 1 : peaking=4 gap=4 pulse=10" << std::endl;

    // Channel 1 trigger threshold, CFD disabled, HE suppress off
    uint32_t thr_val = buildThreshold(500, CfdMode::DISABLED, false, true);
    if (vme_crate_->vme_A32D32_write(SIS3315_ADC_CH1_FIR_TRIGGER_THRESHOLD_REG, thr_val)) {
        std::cerr << "Erreur écriture seuil" << std::endl; return 1;
    }
    std::cout << "Seuil canal 1 = 500" << std::endl;

    // Event Config: enable Internal Trigger Enable (bit 2) for CH1 of group 1
    // and External Trigger Enable (bit 3) to accept KEY_TRIGGER
    if (vme_crate_->vme_A32D32_read(kEventCfgReg[0], &data)) {
        std::cerr << "Erreur lecture Event Config" << std::endl; return 1;
    }
    data |= (1u << 2) | (1u << 3);   // CH1 : internal + external trigger enable
    if (vme_crate_->vme_A32D32_write(kEventCfgReg[0], data)) {
        std::cerr << "Erreur écriture Event Config" << std::endl; return 1;
    }
    std::cout << "Event Config groupe 1 : Internal + External Trigger Enable activés sur CH1" << std::endl;

    if (vme_crate_->vme_A32D32_write(SIS3315_KEY_DISARM_AND_ARM_BANK1, 0)) {
        std::cerr << "Erreur armement" << std::endl; return 1;
    }
    std::cout << "Module armé sur Bank 1" << std::endl;

    if (vme_crate_->vme_A32D32_write(SIS3315_KEY_TRIGGER, 0)) {
        std::cerr << "Erreur envoi trigger" << std::endl; return 1;
    }
    std::cout << "Trigger logiciel envoyé" << std::endl;
    usleep(1000);

    if (vme_crate_->vme_A32D32_read(SIS3315_ACQUISITION_CONTROL_STATUS, &data)) {
        std::cerr << "Erreur lecture statut" << std::endl; return 1;
    }
    std::cout << "Statut acquisition = 0x" << std::hex << data << std::dec << std::endl;
    std::cout << "Armed  = " << ((data >> 17) & 0x1 ? "OUI" : "NON") << std::endl;
    std::cout << "Busy CH1-4 = " << ((data >> 23) & 0x1 ? "OUI" : "NON") << std::endl;

    vme_crate_->vme_A32D32_write(SIS3315_KEY_DISARM, 0);
    std::cout << "Module désarmé\nTest trigger interne terminé" << std::endl;
    return 0;
}


// askTriggerConfig - interactive input


TriggerConfig SIS3315Manager::askTriggerConfig() {
    TriggerConfig cfg = {};
    cfg.cfd_mode = CfdMode::DISABLED;

    std::cout << "\n=== Configuration du trigger ===" << std::endl;
    std::cout << "Sources disponibles :" << std::endl;
    std::cout << "  1. Logiciel (KEY_TRIGGER)" << std::endl;
    std::cout << "  2. Externe NIM (entrée TI)" << std::endl;
    std::cout << "  3. FP-Bus LVDS (Control 1)" << std::endl;
    std::cout << "  4. Interne FIR par canal (1-16)" << std::endl;
    std::cout << "  5. Interne SUM (somme groupe de 4 canaux)" << std::endl;
    std::cout << "  6. Interne Pileup (détection pileup par canal)" << std::endl;
    std::cout << "  7. Feedback trigger interne comme trigger externe global" << std::endl;
    std::cout << "  8. Table de coïncidence (LUT)" << std::endl;
    std::cout << "Choix : ";

    int choice;
    std::cin >> choice;

    switch (choice) {
        case 1: cfg.source = TriggerSource::SOFTWARE;      break;
        case 2: cfg.source = TriggerSource::EXTERNAL_NIM;  break;
        case 3: cfg.source = TriggerSource::FPBUS;         break;

        case 4:
            cfg.source = TriggerSource::INTERNAL_FIR;
            std::cout << "Canal (1-16) : ";
            std::cin >> cfg.channel;
            goto ask_fir_params;

        case 5:
            cfg.source = TriggerSource::INTERNAL_SUM;
            std::cout << "Groupe (1=CH1-4, 2=CH5-8, 3=CH9-12, 4=CH13-16) : ";
            std::cin >> cfg.channel_group;
            goto ask_fir_params;

        case 6:
            cfg.source = TriggerSource::INTERNAL_PILEUP;
            std::cout << "Canal (1-16) : ";
            std::cin >> cfg.channel;
            // Pileup trigger requires no additional FIR parameters
            break;

        case 7:
            cfg.source = TriggerSource::INTERNAL_FEEDBACK;
            std::cout << "Ce mode reboucle le trigger interne sélectionné comme trigger" << std::endl;
            std::cout << "externe global (Acq. Control bit 14)." << std::endl;
            std::cout << "Configurer d'abord un trigger INTERNAL_FIR ou INTERNAL_SUM." << std::endl;
            break;

        case 8:
            cfg.source = TriggerSource::COINCIDENCE_LUT;
            std::cout << "Index de la LUT (1 ou 2) : ";
            std::cin >> cfg.lut_index;
            std::cout << "Longueur de fenêtre de coïncidence en samples (pair, 2-512) : ";
            std::cin >> cfg.coincidence_window;
            break;

        default:
            std::cerr << "Choix invalide, trigger logiciel par défaut" << std::endl;
            cfg.source = TriggerSource::SOFTWARE;
            break;
    }
    return cfg;

ask_fir_params:
    std::cout << "Seuil (0 à 0x" << std::hex << 0x0FFFFFFFu << std::dec << ") : ";
    std::cin >> cfg.threshold;
    std::cout << "Peaking time en samples (pair, 2-510) : ";
    std::cin >> cfg.peaking_time;
    std::cout << "Gap time en samples (pair, 2-510) : ";
    std::cin >> cfg.gap_time;
    std::cout << "Pulse length en samples (pair, 2-256) : ";
    std::cin >> cfg.pulse_length;
    std::cout << "Trigger delay en samples (pair, 0-510) : ";
    std::cin >> cfg.trigger_delay;
    {
        int cfd;
        std::cout << "Mode CFD (0=désactivé, 1=zero-crossing, 2=50%) : ";
        std::cin >> cfd;
        cfg.cfd_mode = (cfd == 1) ? CfdMode::ZERO_CROSSING
                     : (cfd == 2) ? CfdMode::FIFTY_PERCENT
                     :              CfdMode::DISABLED;
    }
    if (cfg.cfd_mode != CfdMode::DISABLED) {
        int he;
        std::cout << "High Energy Suppress activé ? (0=non, 1=oui) : ";
        std::cin >> he;
        cfg.he_suppress = (he == 1);
        if (cfg.he_suppress) {
            std::cout << "Seuil High Energy (0 à 0x" << std::hex << 0x0FFFFFFFu << std::dec << ") : ";
            std::cin >> cfg.he_threshold;
            int both;
            std::cout << "Trigger on both edges ? (0=non, 1=oui) : ";
            std::cin >> both;
            cfg.trigger_on_both_edges = (both == 1);
        }
    } else {
        cfg.he_suppress          = false;
        cfg.trigger_on_both_edges = false;
    }
    return cfg;
}


// setTriggerParameter - apply configuration


int SIS3315Manager::setTriggerParameter(const TriggerConfig& cfg) {
    unsigned int data;
    int reply;
    std::cout << "\n=== Application de la configuration trigger ===" << std::endl;

    // Read and mask source bits in Acquisition Control
    reply = vme_crate_->vme_A32D32_read(SIS3315_ACQUISITION_CONTROL_STATUS, &data);
    if (reply) { std::cerr << "Erreur lecture ACQUISITION_CONTROL_STATUS" << std::endl; return 1; }

    // Clear source bits (bit 8 = NIM, bit 4 = FP-Bus, bit 14 = feedback)
    data &= ~((1u << 8) | (1u << 4) | (1u << 14));

    switch (cfg.source) {

        
        case TriggerSource::SOFTWARE:
            reply = vme_crate_->vme_A32D32_write(SIS3315_ACQUISITION_CONTROL_STATUS, data);
            if (reply) { std::cerr << "Erreur écriture ACQUISITION_CONTROL_STATUS" << std::endl; return 1; }
            std::cout << "Trigger logiciel configuré" << std::endl;
            std::cout << "Déclencher avec : vme_A32D32_write(SIS3315_KEY_TRIGGER, 0)" << std::endl;
            break;

        
        case TriggerSource::EXTERNAL_NIM:
            data |= (1u << 8);
            reply = vme_crate_->vme_A32D32_write(SIS3315_ACQUISITION_CONTROL_STATUS, data);
            if (reply) { std::cerr << "Erreur écriture ACQUISITION_CONTROL_STATUS" << std::endl; return 1; }
            std::cout << "Trigger externe NIM activé (Acq. Control bit 8)" << std::endl;
            break;

        
        case TriggerSource::FPBUS:
            data |= (1u << 4);
            reply = vme_crate_->vme_A32D32_write(SIS3315_ACQUISITION_CONTROL_STATUS, data);
            if (reply) { std::cerr << "Erreur écriture ACQUISITION_CONTROL_STATUS" << std::endl; return 1; }
            std::cout << "Trigger FP-Bus LVDS activé (Acq. Control bit 4)" << std::endl;
            break;

        
        case TriggerSource::INTERNAL_FIR: {
            if (cfg.channel < 1 || cfg.channel > 16) {
                std::cerr << "Canal invalide : " << cfg.channel << std::endl; return 1;
            }
            int ch    = cfg.channel - 1;
            int group = ch / 4;
            int pos   = ch % 4;   // position within the group (0-3)

            reply = vme_crate_->vme_A32D32_write(kFirSetupReg[ch],
                        buildFirSetup(cfg.peaking_time, cfg.gap_time, cfg.pulse_length));
            if (reply) { std::cerr << "Erreur écriture FIR Trigger Setup CH" << cfg.channel << std::endl; return 1; }
            std::cout << "FIR CH" << cfg.channel
                      << " : peaking=" << cfg.peaking_time
                      << " gap=" << cfg.gap_time
                      << " pulse=" << cfg.pulse_length << std::endl;

            reply = vme_crate_->vme_A32D32_write(kThresholdReg[ch],
                        buildThreshold(cfg.threshold, cfg.cfd_mode, cfg.he_suppress, true));
            if (reply) { std::cerr << "Erreur écriture Trigger Threshold CH" << cfg.channel << std::endl; return 1; }
            std::cout << "Seuil CH" << cfg.channel << " = 0x"
                      << std::hex << cfg.threshold << std::dec;
            if (cfg.cfd_mode != CfdMode::DISABLED)
                std::cout << " [CFD=" << (cfg.cfd_mode == CfdMode::ZERO_CROSSING ? "zero-crossing" : "50%") << "]";
            if (cfg.he_suppress) std::cout << " [HE-suppress]";
            std::cout << std::endl;

            reply = vme_crate_->vme_A32D32_write(kHeThresholdReg[ch],
                        buildHeThreshold(cfg.he_threshold, cfg.trigger_on_both_edges));
            if (reply) { std::cerr << "Erreur écriture HE Threshold CH" << cfg.channel << std::endl; return 1; }
            if (cfg.he_suppress)
                std::cout << "HE seuil CH" << cfg.channel << " = 0x"
                          << std::hex << cfg.he_threshold << std::dec << std::endl;

            // Register is shared per group, each channel occupies 8 bits:
            //   bits  7-0  = CH1 (pos 0), bits 15-8 = CH2 (pos 1), etc.
            {
                unsigned int delay_reg;
                reply = vme_crate_->vme_A32D32_read(kTrigDelayReg[group], &delay_reg);
                if (reply) { std::cerr << "Erreur lecture Trigger Delay" << std::endl; return 1; }
                uint32_t shift = pos * 8;
                delay_reg &= ~(0xFFu << shift);
                delay_reg |= ((static_cast<uint32_t>(cfg.trigger_delay) & 0xFEu) << shift);
                reply = vme_crate_->vme_A32D32_write(kTrigDelayReg[group], delay_reg);
                if (reply) { std::cerr << "Erreur écriture Trigger Delay" << std::endl; return 1; }
                std::cout << "Trigger delay CH" << cfg.channel << " = " << cfg.trigger_delay << " samples" << std::endl;
            }

            //    enable Internal Trigger Enable for this channel (bit 2 + pos*8),
            //    disable External Trigger Enable for this channel (bit 3 + pos*8)
            {
                unsigned int evt_cfg;
                reply = vme_crate_->vme_A32D32_read(kEventCfgReg[group], &evt_cfg);
                if (reply) { std::cerr << "Erreur lecture Event Config" << std::endl; return 1; }
                uint32_t shift = pos * 8;
                evt_cfg &= ~(1u << (shift + 3));  // External Trigger Enable = 0
                evt_cfg |=  (1u << (shift + 2));  // Internal Trigger Enable = 1
                reply = vme_crate_->vme_A32D32_write(kEventCfgReg[group], evt_cfg);
                if (reply) { std::cerr << "Erreur écriture Event Config" << std::endl; return 1; }
                std::cout << "Event Config CH" << cfg.channel
                          << " : Internal Trigger Enable = 1 (mode asynchrone)" << std::endl;
            }
            break;
        }

        
        case TriggerSource::INTERNAL_SUM: {
            if (cfg.channel_group < 1 || cfg.channel_group > 4) {
                std::cerr << "Groupe invalide : " << cfg.channel_group << std::endl; return 1;
            }
            int g = cfg.channel_group - 1;

            // 1. SUM FIR Trigger Setup
            reply = vme_crate_->vme_A32D32_write(kSumFirSetupReg[g],
                        buildFirSetup(cfg.peaking_time, cfg.gap_time, cfg.pulse_length));
            if (reply) { std::cerr << "Erreur écriture SUM FIR Setup groupe " << cfg.channel_group << std::endl; return 1; }
            std::cout << "SUM FIR groupe " << cfg.channel_group
                      << " : peaking=" << cfg.peaking_time
                      << " gap=" << cfg.gap_time
                      << " pulse=" << cfg.pulse_length << std::endl;

            // 2. SUM Trigger Threshold
            reply = vme_crate_->vme_A32D32_write(kSumThresholdReg[g],
                        buildThreshold(cfg.threshold, cfg.cfd_mode, cfg.he_suppress, true));
            if (reply) { std::cerr << "Erreur écriture SUM Threshold groupe " << cfg.channel_group << std::endl; return 1; }
            std::cout << "SUM seuil groupe " << cfg.channel_group
                      << " = 0x" << std::hex << cfg.threshold << std::dec << std::endl;

            // 3. SUM HE Threshold
            reply = vme_crate_->vme_A32D32_write(kSumHeThresholdReg[g],
                        buildHeThreshold(cfg.he_threshold, cfg.trigger_on_both_edges));
            if (reply) { std::cerr << "Erreur écriture SUM HE Threshold" << std::endl; return 1; }

            // 4. Event Configuration: enable Internal SUM Trigger Enable (bit 1)
            //    for all 4 channels in the group
            {
                unsigned int evt_cfg;
                reply = vme_crate_->vme_A32D32_read(kEventCfgReg[g], &evt_cfg);
                if (reply) { std::cerr << "Erreur lecture Event Config" << std::endl; return 1; }
                for (int pos = 0; pos < 4; pos++) {
                    uint32_t shift = pos * 8;
                    evt_cfg &= ~(1u << (shift + 3));  // External Trigger Enable = 0
                    evt_cfg &= ~(1u << (shift + 2));  // Internal Trigger Enable = 0
                    evt_cfg |=  (1u << (shift + 1));  // Internal SUM Trigger Enable = 1
                }
                reply = vme_crate_->vme_A32D32_write(kEventCfgReg[g], evt_cfg);
                if (reply) { std::cerr << "Erreur écriture Event Config" << std::endl; return 1; }
                std::cout << "Event Config groupe " << cfg.channel_group
                          << " : SUM Trigger Enable = 1 pour les 4 canaux" << std::endl;
            }
            break;
        }

        
        case TriggerSource::INTERNAL_PILEUP: {
            if (cfg.channel < 1 || cfg.channel > 16) {
                std::cerr << "Canal invalide : " << cfg.channel << std::endl; return 1;
            }
            int ch    = cfg.channel - 1;
            int group = ch / 4;
            int pos   = ch % 4;

            unsigned int ext_cfg;
            reply = vme_crate_->vme_A32D32_read(kExtEventCfgReg[group], &ext_cfg);
            if (reply) { std::cerr << "Erreur lecture Extended Event Config" << std::endl; return 1; }
            uint32_t shift = pos * 8;
            ext_cfg |= (1u << shift);  // Pileup Trigger Enable = 1
            reply = vme_crate_->vme_A32D32_write(kExtEventCfgReg[group], ext_cfg);
            if (reply) { std::cerr << "Erreur écriture Extended Event Config" << std::endl; return 1; }

            // Event Config: disable Internal and External Trigger Enable for this channel
            unsigned int evt_cfg;
            reply = vme_crate_->vme_A32D32_read(kEventCfgReg[group], &evt_cfg);
            if (reply) { std::cerr << "Erreur lecture Event Config" << std::endl; return 1; }
            evt_cfg &= ~(1u << (shift + 3));  // External Trigger Enable = 0
            evt_cfg &= ~(1u << (shift + 2));  // Internal Trigger Enable = 0
            reply = vme_crate_->vme_A32D32_write(kEventCfgReg[group], evt_cfg);
            if (reply) { std::cerr << "Erreur écriture Event Config" << std::endl; return 1; }

            std::cout << "Pileup Trigger Enable activé sur CH" << cfg.channel << std::endl;
            break;
        }

        
        case TriggerSource::INTERNAL_FEEDBACK:
            // Enable feedback (bit 14) - the source (channel or SUM) must be
            // configured separately via INTERNAL_FIR or INTERNAL_SUM,
            // and selected in the Internal Trigger Feedback Select register (0x7C)
            data |= (1u << 14);
            reply = vme_crate_->vme_A32D32_write(SIS3315_ACQUISITION_CONTROL_STATUS, data);
            if (reply) { std::cerr << "Erreur écriture ACQUISITION_CONTROL_STATUS" << std::endl; return 1; }
            std::cout << "Feedback trigger interne activé (Acq. Control bit 14)" << std::endl;
            std::cout << "Sélectionner la source dans SIS3315_INTERNAL_TRIGGER_FEEDBACK_SELECT_REG (0x7C)" << std::endl;
            break;

        
        case TriggerSource::COINCIDENCE_LUT:
            //   SIS3315_TRIGGER_COINCIDENCE_LOOKUP_TABLE_CONTROL_REG  (0x64)
            //   SIS3315_TRIGGER_COINCIDENCE_LOOKUP_TABLE_ADDRESS_REG  (0x68)
            //   SIS3315_TRIGGER_COINCIDENCE_LOOKUP_TABLE_DATA_REG     (0x6C)
            // The validation output can be routed to T-O (LUT 1) or U-O (LUT 2)
            // or used as feedback (LUT 1 only).
            std::cout << "Coïncidence LUT " << cfg.lut_index << " sélectionnée" << std::endl;
            std::cout << "Initialiser la table via :" << std::endl;
            std::cout << "  SIS3315_TRIGGER_COINCIDENCE_LOOKUP_TABLE_CONTROL_REG (0x64)" << std::endl;
            std::cout << "  SIS3315_TRIGGER_COINCIDENCE_LOOKUP_TABLE_ADDRESS_REG (0x68)" << std::endl;
            std::cout << "  SIS3315_TRIGGER_COINCIDENCE_LOOKUP_TABLE_DATA_REG    (0x6C)" << std::endl;
            std::cout << "Fenêtre de coïncidence : " << cfg.coincidence_window << " samples" << std::endl;
            break;
    }

    // Final write of Acquisition Control register (if not already written in the case)
    if (cfg.source != TriggerSource::EXTERNAL_NIM &&
        cfg.source != TriggerSource::FPBUS        &&
        cfg.source != TriggerSource::INTERNAL_FEEDBACK) {
        reply = vme_crate_->vme_A32D32_write(SIS3315_ACQUISITION_CONTROL_STATUS, data);
        if (reply) { std::cerr << "Erreur écriture ACQUISITION_CONTROL_STATUS" << std::endl; return 1; }
    }

    std::cout << "Configuration trigger appliquée" << std::endl;
    return 0;
}


// setAcquisitionParameter


int SIS3315Manager::setAcquisitionParameter(AcquisitionConfig& config) {
    return setParameter(vme_crate_, config);
}


// ask_acquisition_config / setParameter  (functionally unchanged)


AcquisitionConfig ask_acquisition_config() {
    AcquisitionConfig config;

    std::cout << "\n=== Configuration acquisition ===" << std::endl;
    std::cout << "\nMode d'acquisition :" << std::endl;
    std::cout << "  1. Software / Address Threshold" << std::endl;
    std::cout << "  2. NIM (bank swap déclenché par signal NIM TI/UI)" << std::endl;
    std::cout << "Choix : ";
    int mode_choice;
    std::cin >> mode_choice;
    config.mode = (mode_choice == 2) ? AcquisitionMode::NIM : AcquisitionMode::INTERFACE;

    std::cout << "\nCanaux à lire (0-15, terminer par -1) : ";
    config.channels.clear();
    int ch;
    while (std::cin >> ch && ch != -1) {
        if (ch >= 0 && ch <= 15)
            config.channels.push_back(static_cast<unsigned int>(ch));
        else
            std::cerr << "  Canal " << ch << " ignoré (hors plage 0-15)" << std::endl;
    }
    if (config.channels.empty()) {
        std::cout << "  Aucun canal sélectionné, tous les canaux activés par défaut" << std::endl;
        for (unsigned int i = 0; i < 16; i++)
            config.channels.push_back(i);
    }

    std::cout << "\nNombre de samples par déclenchement (ex: 1024) : ";
    std::cin >> config.nof_samples;
    if (config.nof_samples == 0) {
        std::cout << "  Valeur invalide, 1024 par défaut" << std::endl;
        config.nof_samples = 1024;
    }

    if (config.mode == AcquisitionMode::INTERFACE) {
        std::cout << "\nSeuil de remplissage mémoire (address threshold, ex: 512) : ";
        std::cin >> config.address_threshold;
    } else {
        config.address_threshold = 0;
    }

    std::cout << "\nTimeout du poll en µs (ex: 1000000) : ";
    std::cin >> config.poll_timeout_us;
    if (config.poll_timeout_us == 0) {
        std::cout << "  Valeur invalide, 1000000 µs par défaut" << std::endl;
        config.poll_timeout_us = 1000000;
    }

    std::cout << "\nNombre max d'événements (0 = infini) : ";
    std::cin >> config.max_events;

    return config;
}


// askSamplingConfig - interactive input


SamplingConfig askSamplingConfig() {
    SamplingConfig cfg = {};
    cfg.format_18bit   = true;
    cfg.average_mode   = AverageMode::OFF;

    std::cout << "\n=== Configuration de l'échantillonnage ===" << std::endl;

    // Pre-trigger delay
    std::cout << "\n[1] Délai pré-trigger en clocks pairs (0–16378) : ";
    std::cin >> cfg.pretrig_delay;
    cfg.pretrig_delay &= 0x3FFEu;
    {
        int extra;
        std::cout << "    Additional FIR P+G delay ? (0=non, 1=oui) : ";
        std::cin >> extra;
        cfg.pretrig_extra_fir = (extra == 1);
    }
    std::cout << "-> pretrig = " << cfg.pretrig_delay << " clocks"
              << (cfg.pretrig_extra_fir ? " + P+G" : "") << std::endl;

    // Gate window
    std::cout << "\n[2] Fenêtre de déclenchement en clocks pairs (2–65536) : ";
    std::cin >> cfg.gate_window;
    cfg.gate_window &= 0xFFFEu;
    if (cfg.gate_window < 2) cfg.gate_window = 2;
    std::cout << "-> gate window = " << cfg.gate_window << " clocks" << std::endl;

    // Raw data buffer
    std::cout << "\n[3] Nombre d'échantillons bruts à sauvegarder (0 = désactivé) : ";
    std::cin >> cfg.raw_sample_count;
    if (cfg.raw_sample_count > 0) {
        std::cout << "    Index de début dans la fenêtre (0 = depuis le début) : ";
        std::cin >> cfg.raw_start_index;
    }
    std::cout << "-> raw data : " << cfg.raw_sample_count << " samples"
              << (cfg.raw_sample_count > 0 ? " (start=" + std::to_string(cfg.raw_start_index) + ")" : " (désactivé)")
              << std::endl;

    // Data format
    std::cout << "\n[4] Format de données" << std::endl;
    {
        int bit;
        std::cout << "    18 bits (natif ADAQ23878) ? (0=16 bits, 1=18 bits) [recommandé : 1] : ";
        std::cin >> bit;
        cfg.format_18bit = (bit == 1);
    }
    {
        int v;
        std::cout << "    Sauvegarder Peak High + accumulateurs 1–6 ? (0/1) : ";
        std::cin >> v; cfg.save_acc16_peak = (v == 1);
        std::cout << "    Sauvegarder accumulateurs 7 et 8 ? (0/1) : ";
        std::cin >> v; cfg.save_acc78 = (v == 1);
        std::cout << "    Sauvegarder 3 valeurs MAW trigger ? (0/1) : ";
        std::cin >> v; cfg.save_maw_3values = (v == 1);
        std::cout << "    Sauvegarder Start + Max Energy MAW ? (0/1) : ";
        std::cin >> v; cfg.save_start_max_energy = (v == 1);
        std::cout << "    Sauvegarder buffer MAW test ? (0/1) : ";
        std::cin >> v; cfg.save_maw_test_buffer = (v == 1);
    }

    // Input inversion
    std::cout << "\n[5] Canaux à inverser (signaux négatifs)" << std::endl;
    std::cout << "    Masque hexadécimal 16 bits (bit i = canal i+1), 0x0 = aucun : 0x";
    std::cin >> std::hex >> cfg.input_invert_mask >> std::dec;
    if (cfg.input_invert_mask)
        std::cout << "-> canaux inversés : masque 0x" << std::hex << cfg.input_invert_mask << std::dec << std::endl;

    // FIR energy
    std::cout << "\n[6] Filtre FIR énergie (second trapèze pour mesure d'énergie)" << std::endl;
    {
        int en;
        std::cout << "    Activer le filtre FIR énergie ? (0=non, 1=oui) : ";
        std::cin >> en;
        cfg.energy_fir_enabled = (en == 1);
    }
    if (cfg.energy_fir_enabled) {
        std::cout << "    Peaking time P en clocks pairs (2–2046) : ";
        std::cin >> cfg.energy_peaking;
        std::cout << "    Gap time G en clocks pairs (2–510) : ";
        std::cin >> cfg.energy_gap;
        std::cout << "    Tau table [0–3] (correction déclin) : ";
        std::cin >> cfg.energy_tau_table;
        std::cout << "    Tau factor [0–63] : ";
        std::cin >> cfg.energy_tau_factor;
        std::cout << "    Extra filter (0=off, 1=×4, 2=×8, 3=×16) : ";
        std::cin >> cfg.energy_extra_filter;
        double feff = (cfg.energy_extra_filter == 0) ? 1.0
                    : (cfg.energy_extra_filter == 1) ? 4.0
                    : (cfg.energy_extra_filter == 2) ? 8.0 : 16.0;
        std::cout << "-> FIR énergie P=" << cfg.energy_peaking
                  << " G=" << cfg.energy_gap
                  << " tau_table=" << cfg.energy_tau_table
                  << " tau_factor=" << cfg.energy_tau_factor
                  << " extra=×" << (int)feff << std::endl;
    }

    // Averaging mode
    std::cout << "\n[7] Mode moyenne (réduit le bruit, diminue feff)" << std::endl;
    std::cout << "    0=off  1=×4  2=×8  3=×16  4=×32  5=×64  6=×128  7=×256" << std::endl;
    std::cout << "    Choix : ";
    {
        int m;
        std::cin >> m;
        if (m < 0 || m > 7) m = 0;
        cfg.average_mode = static_cast<AverageMode>(m);
    }
    if (cfg.average_mode != AverageMode::OFF) {
        std::cout << "    Pré-trigger delay pour la voie moyennée (12 bits) : ";
        std::cin >> cfg.average_pretrig_delay;
        std::cout << "    Longueur de la fenêtre de moyenne (16 bits) : ";
        std::cin >> cfg.average_sample_length;
        int n = 1 << (static_cast<int>(cfg.average_mode) + 1);  // 4, 8, 16, …
        std::cout << "-> moyennage N=" << n << " -> SNR × √" << n
                  << "  |  feff = fs/" << n << std::endl;
    }

    return cfg;
}


// readSamplingConfig - read and display current configuration


void SIS3315Manager::readSamplingConfig() {
    unsigned int data;
    static const char* grp_names[4] = {"CH1-4", "CH5-8", "CH9-12", "CH13-16"};
    static const int   avg_n[8]     = {1, 4, 8, 16, 32, 64, 128, 256};

    std::cout << "\n=== Configuration d'échantillonnage courante ===" << std::endl;

    for (int g = 0; g < 4; g++) {
        std::cout << "\n--- Groupe " << grp_names[g] << " ---" << std::endl;

        // Pre-trigger delay
        if (!vme_crate_->vme_A32D32_read(kPreTrigDelayReg[g], &data)) {
            uint32_t delay    = data & 0x3FFEu;
            bool     extra    = (data >> 15) & 0x1;
            std::cout << "  Pre-trigger delay  = " << delay << " clocks"
                      << (extra ? " + FIR P+G" : "") << std::endl;
        }

        // Gate window
        if (!vme_crate_->vme_A32D32_read(kGateWindowReg[g], &data)) {
            std::cout << "  Gate window        = " << (data & 0xFFFEu) << " clocks" << std::endl;
        }

        // Raw data buffer
        if (!vme_crate_->vme_A32D32_read(kRawDataBufReg[g], &data)) {
            uint32_t count = (data >> 16) & 0xFFFF;
            uint32_t start =  data        & 0xFFFF;
            std::cout << "  Raw data buffer    = " << count
                      << " samples (start=" << start << ")" << std::endl;
        }

        // Data format
        if (!vme_crate_->vme_A32D32_read(kDataFormatReg[g], &data)) {
            uint32_t b     = data & 0xFFu;  // CH1 bits (same for all channels)
            bool     b18   = !((b >> 7) & 0x1);
            std::cout << "  Data format        = " << (b18 ? "18" : "16") << " bits";
            if (b & (1u << 1)) std::cout << "  [acc1-6+peak]";
            if (b & (1u << 2)) std::cout << "  [acc7-8]";
            if (b & (1u << 3)) std::cout << "  [maw-3val]";
            if (b & (1u << 4)) std::cout << "  [start+maxE]";
            if (b & (1u << 5)) std::cout << "  [maw-test]";
            std::cout << std::endl;
        }

        // Average config
        if (!vme_crate_->vme_A32D32_read(kAverageReg[g], &data)) {
            int mode  = (data >> 28) & 0x7;
            int ptd   = (data >> 16) & 0xFFF;
            int slen  =  data        & 0xFFFF;
            std::cout << "  Averaging          = ×" << avg_n[mode]
                      << "  pretrig=" << ptd << "  length=" << slen << std::endl;
        }
    }

    // FIR Energy Setup (CH1 only, representative)
    if (!vme_crate_->vme_A32D32_read(kFirEnergySetupReg[0], &data)) {
        unsigned int tau_table     = (data >> 30) & 0x3;
        unsigned int tau_factor    = (data >> 24) & 0x3F;
        unsigned int extra_filter  = (data >> 22) & 0x3;
        unsigned int gap           = (data >> 12) & 0x1FE;
        unsigned int peaking       =  data        & 0x7FE;
        static const int filt_n[4] = {1, 4, 8, 16};
        std::cout << "\nFIR énergie (CH1) : P=" << peaking
                  << " G=" << gap
                  << " tau_table=" << tau_table
                  << " tau_factor=" << tau_factor
                  << " extra=×" << filt_n[extra_filter] << std::endl;
    }
}


// setSamplingParameter - apply configuration


int SIS3315Manager::setSamplingParameter(const SamplingConfig& cfg) {
    unsigned int data;
    int reply;
    static const char* grp_names[4] = {"CH1-4", "CH5-8", "CH9-12", "CH13-16"};

    std::cout << "\n=== Application de la configuration d'échantillonnage ===" << std::endl;

    for (int g = 0; g < 4; g++) {

        // [1] Pre-trigger delay
        reply = vme_crate_->vme_A32D32_write(kPreTrigDelayReg[g],
                    buildPreTrigDelay(cfg.pretrig_delay, cfg.pretrig_extra_fir));
        if (reply) {
            std::cerr << "Erreur écriture Pre-Trigger Delay groupe " << grp_names[g] << std::endl;
            return 1;
        }

        // [2] Gate window
        reply = vme_crate_->vme_A32D32_write(kGateWindowReg[g],
                    cfg.gate_window & 0xFFFEu);
        if (reply) {
            std::cerr << "Erreur écriture Gate Window groupe " << grp_names[g] << std::endl;
            return 1;
        }

        // [3] Raw data buffer
        reply = vme_crate_->vme_A32D32_write(kRawDataBufReg[g],
                    buildRawDataBuf(cfg.raw_sample_count, cfg.raw_start_index));
        if (reply) {
            std::cerr << "Erreur écriture Raw Data Buffer groupe " << grp_names[g] << std::endl;
            return 1;
        }

        // [4] Data format
        reply = vme_crate_->vme_A32D32_write(kDataFormatReg[g],
                    buildDataFormat(cfg));
        if (reply) {
            std::cerr << "Erreur écriture Data Format groupe " << grp_names[g] << std::endl;
            return 1;
        }

        // [5] Average config
        reply = vme_crate_->vme_A32D32_write(kAverageReg[g],
                    buildAverageConfig(cfg.average_mode,
                                       cfg.average_pretrig_delay,
                                       cfg.average_sample_length));
        if (reply) {
            std::cerr << "Erreur écriture Average Config groupe " << grp_names[g] << std::endl;
            return 1;
        }

        // [6] Input invert (Event Config bit 0 per channel)
        reply = vme_crate_->vme_A32D32_read(kEventCfgReg[g], &data);
        if (reply) {
            std::cerr << "Erreur lecture Event Config groupe " << grp_names[g] << std::endl;
            return 1;
        }
        for (int pos = 0; pos < 4; pos++) {
            int ch     = g * 4 + pos;
            uint32_t shift = static_cast<uint32_t>(pos) * 8;
            if ((cfg.input_invert_mask >> ch) & 0x1)
                data |=  (1u << shift);  // bit 0 of channel field = input invert
            else
                data &= ~(1u << shift);
        }
        reply = vme_crate_->vme_A32D32_write(kEventCfgReg[g], data);
        if (reply) {
            std::cerr << "Erreur écriture Event Config groupe " << grp_names[g] << std::endl;
            return 1;
        }

        std::cout << "Groupe " << grp_names[g]
                  << " : pretrig=" << cfg.pretrig_delay
                  << " gate=" << cfg.gate_window
                  << " raw=" << cfg.raw_sample_count
                  << " fmt=" << (cfg.format_18bit ? "18b" : "16b")
                  << " avg=" << (cfg.average_mode == AverageMode::OFF ? "off"
                              : ("×" + std::to_string(1 << (static_cast<int>(cfg.average_mode) + 1))))
                  << std::endl;
    }

    // [7] FIR energy - per channel if enabled
    if (cfg.energy_fir_enabled) {
        uint32_t energy_val = buildEnergySetup(cfg.energy_tau_table,
                                               cfg.energy_tau_factor,
                                               cfg.energy_extra_filter,
                                               cfg.energy_gap,
                                               cfg.energy_peaking);
        for (int ch = 0; ch < 16; ch++) {
            reply = vme_crate_->vme_A32D32_write(kFirEnergySetupReg[ch], energy_val);
            if (reply) {
                std::cerr << "Erreur écriture FIR Energy Setup CH" << ch + 1 << std::endl;
                return 1;
            }
        }
        std::cout << "FIR énergie (16 canaux) : P=" << cfg.energy_peaking
                  << " G=" << cfg.energy_gap
                  << " tau_table=" << cfg.energy_tau_table
                  << " tau_factor=" << cfg.energy_tau_factor
                  << " extra=×" << (cfg.energy_extra_filter == 0 ? 1
                                  : cfg.energy_extra_filter == 1 ? 4
                                  : cfg.energy_extra_filter == 2 ? 8 : 16)
                  << std::endl;
    }

    std::cout << "Configuration échantillonnage appliquée" << std::endl;
    return 0;
}


// configureSamplingInteractive - interactive input + apply


int SIS3315Manager::configureSamplingInteractive() {
    readSamplingConfig();
    SamplingConfig cfg = askSamplingConfig();
    return setSamplingParameter(cfg);
}


// acquireNoise - single-shot acquisition, software trigger, 16 channels


int SIS3315Manager::acquireNoise(unsigned int nof_samples) {

    nof_samples = (nof_samples & ~1u);   // must be even
    if (nof_samples < 2) nof_samples = 2;

    std::cout << "\n=== Acquisition bruit – " << nof_samples
              << " samples/canal ===" << std::endl;

    // Configure all 4 groups: gate window, raw buffer, minimal format,
    // address threshold = 1 event, External Trigger Enable on all channels.
    for (int g = 0; g < 4; g++) {
    // Gate window
    vme_crate_->vme_A32D32_write(kGateWindowReg[g], (nof_samples - 2u) & 0xFFFEu);

    // Raw data buffer - count in [31:16], start=0 in [15:0]
    vme_crate_->vme_A32D32_write(kRawDataBufReg[g],
                                 buildRawDataBuf(nof_samples, 0));

    // Data format - 18 bits, nothing else
    vme_crate_->vme_A32D32_write(kDataFormatReg[g], 0x00000000u);

    // Address threshold - bit 31 suppresses subsequent events (only 1 event wanted)
    vme_crate_->vme_A32D32_write(kAddrThresholdReg[g], 0x80000000u | (nof_samples + 2u - 1u));

    // Event Config: External Trigger Enable ONLY, reset everything else
    // bit pos*8+3 = External Trigger Enable for each channel in the group
    uint32_t evt_cfg = 0;
    for (int pos = 0; pos < 4; pos++)
        evt_cfg |= (1u << (pos * 8 + 3));
    vme_crate_->vme_A32D32_write(kEventCfgReg[g], evt_cfg);  // overwrite, do not read
}

    // KEY_TRIGGER (0x418) goes through the "External Trigger Function" - bit 8 required.
    // setTriggerParameter(SOFTWARE) clears it -> restore it here for the single shot.
    {
        unsigned int acq_ctrl = 0;
        vme_crate_->vme_A32D32_read(SIS3315_ACQUISITION_CONTROL_STATUS, &acq_ctrl);
        vme_crate_->vme_A32D32_write(SIS3315_ACQUISITION_CONTROL_STATUS,
                                      (acq_ctrl & 0x0000FFFFu) | (1u << 8));
    }

    // Arm bank 1
    if (vme_crate_->vme_A32D32_write(SIS3315_KEY_DISARM_AND_ARM_BANK1, 0)) {
        std::cerr << "Erreur armement bank 1" << std::endl; return 1;
    }
    std::cout << "Bank 1 armé" << std::endl;

    // Software trigger
    if (vme_crate_->vme_A32D32_write(SIS3315_KEY_TRIGGER, 0)) {
        std::cerr << "Erreur envoi trigger" << std::endl; return 1;
    }
    std::cout << "Trigger logiciel envoyé" << std::endl;

    // Wait until address threshold is reached (bit 19 of ACQ_CTRL_STATUS)
    // Timeout = nof_samples / (fs_min = 5.2 MSPS) * 10 = margin * 10
    unsigned int acq_status = 0;
    const uint32_t timeout_us = std::max(500000u,
                                         (nof_samples * 1000u / 5u) * 10u);
    for (uint32_t waited = 0; waited < timeout_us; waited += 100) {
        vme_crate_->vme_A32D32_read(SIS3315_ACQUISITION_CONTROL_STATUS,
                                    &acq_status);
        if (acq_status & (1u << 19)) break;
        usleep(100);
        if (waited + 100 >= timeout_us) {
            std::cerr << "Timeout attente bank 1 (status=0x"
                      << std::hex << acq_status << std::dec << ")" << std::endl;
            vme_crate_->vme_A32D32_write(SIS3315_KEY_DISARM, 0);
            return 1;
        }
    }
    std::cout << "Bank 1 plein (status=0x" << std::hex << acq_status
              << std::dec << ")" << std::endl;

    // Atomic swap: arm bank 2, release bank 1 for reading
    vme_crate_->vme_A32D32_write(SIS3315_KEY_DISARM_AND_ARM_BANK2, 0);

    // Read 16 channels from bank 1 (bank2_read_flag=0)
    // Minimal header = 2 words (word0: CH_ID|ts_hi, word1: ts_lo)
    const uint32_t HEADER = 3u;
    const uint32_t MAX_LWORDS = 0x100000u;   // 4 MiB, enough for 1024 samples
    std::vector<unsigned int> buf(MAX_LWORDS);

    std::cout << "\n"
              << std::left  << std::setw(6)  << "CH"
              << std::right << std::setw(7)  << "mots"
              << std::setw(9)  << "min"
              << std::setw(9)  << "max"
              << std::setw(11) << "moyenne"
              << std::setw(8)  << "range"
              << std::endl;
    std::cout << std::string(50, '-') << std::endl;

    for (int ch = 0; ch < 16; ch++) {
        unsigned int got = 0;
        int rc = adc_->read_DMA_Channel_PreviousBankDataBuffer(
            0,          // bank2_read_flag=0 -> bank 1 = previous bank
            (unsigned int)ch,
            MAX_LWORDS,
            &got,
            buf.data());

        if (rc != 0 || got <= HEADER) {
            std::cerr << "CH" << std::setw(2) << ch + 1
                      << " : erreur DMA (rc=0x" << std::hex << rc
                      << std::dec << "  got=" << got << ")" << std::endl;
            continue;
        }

        unsigned int* s    = buf.data() + HEADER;
        unsigned int  n    = got - HEADER;
        uint64_t      sum  = 0;
        uint32_t      minv = 0x3FFFFu, maxv = 0;
        for (unsigned int i = 0; i < n; i++) {
            uint32_t v = s[i] & 0x3FFFFu;
            sum += v;
            if (v < minv) minv = v;
            if (v > maxv) maxv = v;
        }
        double mean = (double)sum / n;

        std::cout << std::left  << "CH" << std::setw(3) << ch + 1
                  << std::right << std::setw(6)  << got
                  << std::setw(9)  << minv
                  << std::setw(9)  << maxv
                  << std::fixed << std::setprecision(1)
                  << std::setw(11) << mean
                  << std::setw(8)  << (maxv - minv)
                  << std::endl;

        // Print first 20 samples for the first 4 channels
        if (ch < 4) {
            std::cout << "     samples :";
            unsigned int disp = std::min(n, 20u);
            for (unsigned int i = 0; i < disp; i++)
                std::cout << " " << (s[i] & 0x3FFFFu);
            std::cout << std::endl;
        }
    }

    vme_crate_->vme_A32D32_write(SIS3315_KEY_DISARM, 0);
    std::cout << "\nDésarmé." << std::endl;
    return 0;
}

int setParameter(sis3315_eth* vme_crate, AcquisitionConfig& config) {
    std::cout << "\n=== Validation et application de la configuration ===" << std::endl;

    if (config.channels.empty()) {
        std::cerr << "Erreur : aucun canal sélectionné" << std::endl; return 1;
    }
    if (config.nof_samples == 0) {
        std::cerr << "Erreur : nof_samples doit être > 0" << std::endl; return 1;
    }
    if (config.nof_samples % 2 != 0) {
        config.nof_samples++;
        std::cout << "nof_samples arrondi à " << config.nof_samples << std::endl;
    }

    static const uint32_t buffer_config_regs[4] = {
        SIS3315_ADC_CH1_4_RAW_DATA_BUFFER_CONFIG_REG,
        SIS3315_ADC_CH5_8_RAW_DATA_BUFFER_CONFIG_REG,
        SIS3315_ADC_CH9_12_RAW_DATA_BUFFER_CONFIG_REG,
        SIS3315_ADC_CH13_16_RAW_DATA_BUFFER_CONFIG_REG,
    };
    static const uint32_t threshold_regs[4] = {
        SIS3315_ADC_CH1_4_ADDRESS_THRESHOLD_REG,
        SIS3315_ADC_CH5_8_ADDRESS_THRESHOLD_REG,
        SIS3315_ADC_CH9_12_ADDRESS_THRESHOLD_REG,
        SIS3315_ADC_CH13_16_ADDRESS_THRESHOLD_REG,
    };

    auto isActive = [&](unsigned int c) {
        for (unsigned int x : config.channels)
            if (x == c) return true;
        return false;
    };

    for (int g = 0; g < 4; g++) {
        bool active = isActive(g*4) || isActive(g*4+1) || isActive(g*4+2) || isActive(g*4+3);
        if (!active) continue;
        uint32_t buf_val = ((config.nof_samples & 0xFFFFu) << 16) | 0u;
        if (vme_crate->vme_A32D32_write(buffer_config_regs[g], buf_val)) {
            std::cerr << "Erreur écriture buffer config groupe " << g << std::endl; return 1;
        }
        std::cout << "Groupe " << g << " : buffer = " << config.nof_samples << " samples" << std::endl;
    }

    if (config.mode == AcquisitionMode::INTERFACE) {
        for (int g = 0; g < 4; g++) {
            bool active = isActive(g*4) || isActive(g*4+1) || isActive(g*4+2) || isActive(g*4+3);
            uint32_t thr_val = active ? config.address_threshold : 0;
            if (vme_crate->vme_A32D32_write(threshold_regs[g], thr_val)) {
                std::cerr << "Erreur écriture address threshold groupe " << g << std::endl; return 1;
            }
        }
        std::cout << "Address threshold = " << config.address_threshold << std::endl;
    }

    std::cout << "\n--- Résumé configuration ---" << std::endl;
    std::cout << "Mode       : " << (config.mode == AcquisitionMode::NIM ? "NIM" : "Software/Threshold") << std::endl;
    std::cout << "Canaux     : ";
    for (unsigned int c : config.channels) std::cout << c << " ";
    std::cout << std::endl;
    std::cout << "Samples    : " << config.nof_samples << std::endl;
    std::cout << "Max events : " << (config.max_events == 0 ? "infini" : std::to_string(config.max_events)) << std::endl;
    std::cout << "Timeout    : " << config.poll_timeout_us << " µs" << std::endl;
    std::cout << "Configuration acquisition appliquée" << std::endl;
    return 0;
}


// saveConfigs / loadConfigs


bool saveConfigs(const std::string& path,
                 const ClockConfig& clk,
                 const TriggerConfig& trig,
                 const AcquisitionConfig& acq)
{
    std::ofstream f(path);
    if (!f) return false;

    // --- Clock ---
    f << "clk_source "         << static_cast<int>(clk.source)          << "\n";
    f << "clk_freq "           << clk.internal_freq_mhz                  << "\n";
    f << "clk_bypass_nim "     << clk.bypass_nim_multiplier               << "\n";
    f << "clk_nim_freq "       << clk.nim_input_freq_mhz                  << "\n";
    f << "clk_nim_n1hs "       << clk.nim_n1hs                            << "\n";
    f << "clk_nim_n1clk1 "     << clk.nim_n1clk1                          << "\n";
    f << "clk_nim_n2 "         << clk.nim_n2                              << "\n";
    f << "clk_nim_n3 "         << clk.nim_n3                              << "\n";
    f << "clk_nim_bw "         << clk.nim_bw_sel                          << "\n";
    f << "clk_fpbus_master "   << clk.fpbus_is_master                     << "\n";

    // --- Trigger ---
    f << "trig_source "        << static_cast<int>(trig.source)          << "\n";
    f << "trig_channel "       << trig.channel                            << "\n";
    f << "trig_group "         << trig.channel_group                      << "\n";
    f << "trig_lut "           << trig.lut_index                          << "\n";
    f << "trig_coinc "         << trig.coincidence_window                 << "\n";
    f << "trig_peaking "       << trig.peaking_time                       << "\n";
    f << "trig_gap "           << trig.gap_time                           << "\n";
    f << "trig_pulse "         << trig.pulse_length                       << "\n";
    f << "trig_threshold "     << trig.threshold                          << "\n";
    f << "trig_cfd "           << static_cast<int>(trig.cfd_mode)        << "\n";
    f << "trig_he_suppress "   << trig.he_suppress                        << "\n";
    f << "trig_he_threshold "  << trig.he_threshold                       << "\n";
    f << "trig_both_edges "    << trig.trigger_on_both_edges               << "\n";
    f << "trig_out "           << trig.trigger_out_enabled                 << "\n";
    f << "trig_delay "         << trig.trigger_delay                      << "\n";

    // --- Acquisition ---
    f << "acq_mode "           << static_cast<int>(acq.mode)             << "\n";
    f << "acq_nof_samples "    << acq.nof_samples                         << "\n";
    f << "acq_addr_threshold " << acq.address_threshold                   << "\n";
    f << "acq_poll_timeout "   << acq.poll_timeout_us                     << "\n";
    f << "acq_max_events "     << acq.max_events                          << "\n";
    f << "acq_channels "       << acq.channels.size();
    for (unsigned int c : acq.channels) f << " " << c;
    f << "\n";

    return f.good();
}

bool loadConfigs(const std::string& path,
                 ClockConfig& clk,
                 TriggerConfig& trig,
                 AcquisitionConfig& acq)
{
    std::ifstream f(path);
    if (!f) return false;

    std::string key;
    while (f >> key) {
        if      (key == "clk_source")        { int v; f>>v; clk.source = static_cast<ClockSource>(v); }
        else if (key == "clk_freq")          { f >> clk.internal_freq_mhz; }
        else if (key == "clk_bypass_nim")    { f >> clk.bypass_nim_multiplier; }
        else if (key == "clk_nim_freq")      { f >> clk.nim_input_freq_mhz; }
        else if (key == "clk_nim_n1hs")      { f >> clk.nim_n1hs; }
        else if (key == "clk_nim_n1clk1")    { f >> clk.nim_n1clk1; }
        else if (key == "clk_nim_n2")        { f >> clk.nim_n2; }
        else if (key == "clk_nim_n3")        { f >> clk.nim_n3; }
        else if (key == "clk_nim_bw")        { f >> clk.nim_bw_sel; }
        else if (key == "clk_fpbus_master")  { f >> clk.fpbus_is_master; }
        else if (key == "trig_source")       { int v; f>>v; trig.source = static_cast<TriggerSource>(v); }
        else if (key == "trig_channel")      { f >> trig.channel; }
        else if (key == "trig_group")        { f >> trig.channel_group; }
        else if (key == "trig_lut")          { f >> trig.lut_index; }
        else if (key == "trig_coinc")        { f >> trig.coincidence_window; }
        else if (key == "trig_peaking")      { f >> trig.peaking_time; }
        else if (key == "trig_gap")          { f >> trig.gap_time; }
        else if (key == "trig_pulse")        { f >> trig.pulse_length; }
        else if (key == "trig_threshold")    { f >> trig.threshold; }
        else if (key == "trig_cfd")          { int v; f>>v; trig.cfd_mode = static_cast<CfdMode>(v); }
        else if (key == "trig_he_suppress")  { f >> trig.he_suppress; }
        else if (key == "trig_he_threshold") { f >> trig.he_threshold; }
        else if (key == "trig_both_edges")   { f >> trig.trigger_on_both_edges; }
        else if (key == "trig_out")          { f >> trig.trigger_out_enabled; }
        else if (key == "trig_delay")        { f >> trig.trigger_delay; }
        else if (key == "acq_mode")          { int v; f>>v; acq.mode = static_cast<AcquisitionMode>(v); }
        else if (key == "acq_nof_samples")   { f >> acq.nof_samples; }
        else if (key == "acq_addr_threshold"){ f >> acq.address_threshold; }
        else if (key == "acq_poll_timeout")  { f >> acq.poll_timeout_us; }
        else if (key == "acq_max_events")    { f >> acq.max_events; }
        else if (key == "acq_channels") {
            size_t n; f >> n;
            acq.channels.resize(n);
            for (size_t i = 0; i < n; i++) f >> acq.channels[i];
        }
    }
    return f.eof() && !f.bad();
}
