#include <iostream>
#include "/home/aloiselkb/sis3315_implementation/sis3315-software/libraries_and_includes/sis_vme_master_class_lib/sis3315_ethernet_access_class.h"
#include "/home/aloiselkb/sis3315_implementation/sis3315-software/libraries_and_includes/sis3315_header/sis3315.h"

#define CONVERSION_RATE 12.0

// Sélection source horloge (bits 1:0)
#define SIS3315_CLOCK_SEL_INTERNAL      0x0  // Oscillateur interne 125MHz (défaut)
#define SIS3315_CLOCK_SEL_UNUSED        0x1  // inutilisé
#define SIS3315_CLOCK_SEL_FP_LVDS       0x2  // External Clock FP-LVDS Bus
#define SIS3315_CLOCK_SEL_NIM           0x3  // External Clock NIM (via Clock Multiplier)

#define SIS3315_CLOCK_SEL_MASK          0x3  // masque bits 1:0

// AD9508
#define SIS3315_AD9508_SPI_WRITE_CMD        0x900   
#define SIS3315_AD9508_CONVERT_DIV_12       11      
#define SIS3315_AD9508_SPI_REG_VAL(div)     (SIS3315_AD9508_SPI_WRITE_CMD | ((div) - 1))

// IOB tap delay
#define SIS3315_IOB_DELAY_CALIBRATE         0xf00   // déclenche la calibration
#define SIS3315_IOB_DELAY_SET_CMD           0x300   // commande set delay
#define SIS3315_IOB_DELAY_VALUE_DEFAULT     0x14    // valeur universelle, toutes fréquences
#define SIS3315_IOB_DELAY_SET(val)          (SIS3315_IOB_DELAY_SET_CMD | (val))
enum class ClockSource {
    INTERNAL = SIS3315_CLOCK_SEL_INTERNAL,
    EXTERNAL_FPBUS = SIS3315_CLOCK_SEL_FP_LVDS,
    EXTERNAL_NIM = SIS3315_CLOCK_SEL_NIM,
};

struct ClockConfig {
    ClockSource source;
    double internal_freq_mhz; // belongs to  INTENRAL
    bool bypass_nim; // belongs to EXTERNAL_NIM
    double nim_input_freq_mhz; // belongs to EXTERNAL_NIM
    unsigned int nim_n1hs; // belongs to EXTERNAL_NIM
    unsigned int nim_n1clk1; // belongs to EXTERNAL_NIM
    unsigned int nim_n2; // belongs to EXTERNAL_NIM
    unsigned int nim_n3; // belongs to EXTERNAL_NIM
    unsigned int nim_bw_sel; // belongs to EXTERNAL_NIM
    bool fpbus_is_master; // belongs to EXTERNAL_FPBUS
};

ClockConfig askClockConfig() {
    ClockConfig config = {};
    // here we initialize the boolean which doesnt belong to all sources
    config.bypass_nim = false;
    config.fpbus_is_master = false;

    // choice for the clock source 

    int clock_source_choice;
    std::cout << "Select clock source:" << std::endl;
    std::cout << "1. Internal " << std::endl;
    std::cout << "2. External FP-Bus" << std::endl;
    std::cout << "3. External NIM" << std::endl;
    std::cout << "Choice: ";
    std::cin >> clock_source_choice;

    switch (clock_source_choice) {
        case 1:
        config.source = ClockSource::INTERNAL;
        {
            int internal_freq_choice;
            std::cin >> internal_freq_choice;
            switch (internal_freq_choice) {
                    case 1:  config.internal_freq_mhz = 62.5;  break;
                    case 2:  config.internal_freq_mhz = 125.0; break;
                    case 3:  config.internal_freq_mhz = 150.0; break;
                    case 4:  config.internal_freq_mhz = 180.0; break;
                    default: std::cout << "Enter frequency in MHz (10–1400): ";
                             std::cin >> config.internal_freq_mhz;
                             if (config.internal_freq_mhz < 10 || config.internal_freq_mhz > 1400) {
                                 std::cerr << "Invalid frequency, defaulting to 125 MHz" << std::endl;
                                 config.internal_freq_mhz = 125.0;
                             }
                             break;
            }
        }
        break;
        case 2:
        config.source = ClockSource::EXTERNAL_FPBUS;
        std::cout << "Master? 0=N, 1=Y: ";
        {
            int master_choice;
            std::cin >> master_choice;
            config.fpbus_is_master = (master_choice == 1);
        }
        break;
        case 3:
        config.source = ClockSource::EXTERNAL_NIM;
        std::cout << "bypass mode? 0=N, 1=Y: ";
        {
            int bypass_choice;
            std::cin >> bypass_choice;
            config.bypass_nim = (bypass_choice == 1);
        
        }
        std::cout << "Input frequency in MHz (10-710): ";
        std::cin >> config.nim_input_freq_mhz;
        if (!config.bypass_nim) {
            std::cout << "N1HS [4-11]: ";
            std::cin >> config.nim_n1hs;
            std::cout << "N1CLK1 [1,2,4,...,220]: ";
            std::cin >> config.nim_n1clk1;
            std::cout << "N2 [32,34,...,512]: ";
            std::cin >> config.nim_n2;
            std::cout << "N3 [1,2,3,...,219]: ";
            std::cin >> config.nim_n3;
            std::cout << "bw_sel [0-15]: ";
            std::cin >> config.nim_bw_sel;
            double output_freq = config.nim_input_freq_mhz * static_cast<double>(config.nim_n2) / (static_cast<double>(config.nim_n1hs) * static_cast<double>(config.nim_n1clk1) * static_cast<double>(config.nim_n3));
            std::cout << "Calculated output frequency: " << output_freq << " MHz and in MSPS: " << output_freq / CONVERSION_RATE << std::endl; 
        } else { 
            std::cout << "Bypass mode: output frequency is same as input frequency: " << config.nim_input_freq_mhz << " MHz and in MSPS: " << config.nim_input_freq_mhz / CONVERSION_RATE << std::endl;
        }
        break;
        default:
        std::cerr << "Invalid choice, defaulting to internal 125 MHz" << std::endl;
        config.source = ClockSource::INTERNAL;
        config.internal_freq_mhz = 125.0;
        break;
    }
    return config;
}

sis3315_eth* vme_crate; // global variable for the VME crate access, should be initialized before calling setClockParameter
sis3315_adc* adc_;

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
// here we set the parameters chosen by the user

int setClockParameter(const ClockConfig& config){
    unsigned int data;
    //1. key reset as said in the manual 
    if (vme_crate->vme_A32D32_write(SIS3315_KEY_RESET, 0x0)) {
        return -1;
    }

    //2. source selection
    if (vme_crate->vme_A32D32_read(SIS3315_SAMPLE_CLOCK_DISTRIBUTION_CONTROL, &data)) {
        return -1;
    }
    data = (data & ~SIS3315_CLOCK_SEL_MASK)
         | (static_cast<unsigned int>(config.source) & SIS3315_CLOCK_SEL_MASK);
    if (vme_crate->vme_A32D32_write(SIS3315_SAMPLE_CLOCK_DISTRIBUTION_CONTROL, data)) {
        return -1;
    }

    //3. configuration specific to the source
    switch (config.source) {
        case ClockSource::INTERNAL: {
            unsigned char si570_bytes[6];
            bool use_raw_bytes = false;

            if (config.internal_freq_mhz == 250.0) {
                si570_bytes[0]=0x20; si570_bytes[1]=0xC2; si570_bytes[2]=0xBC;
                si570_bytes[3]=0x33; si570_bytes[4]=0xE4; si570_bytes[5]=0xF2;
                use_raw_bytes = true;
            } else if (config.internal_freq_mhz == 125.0) {
                si570_bytes[0]=0x21; si570_bytes[1]=0xC2; si570_bytes[2]=0xBC;
                si570_bytes[3]=0x33; si570_bytes[4]=0xE4; si570_bytes[5]=0xF2;
                use_raw_bytes = true;
            }

            if (use_raw_bytes) {
                if (adc_->set_frequency(0, si570_bytes)) {
                    return -1;
                }
            } else {
                unsigned int hsdiv, n1div;
                double dummy = 0.0;

        }
        usleep(10000);
        break;

    }
    case ClockSource::EXTERNAL_FPBUS: {
    unsigned int fpbus_reg = 0;
    if (vme_crate->vme_A32D32_read(SIS3315_FP_LVDS_BUS_CONTROL, &fpbus_reg)) {
        return -1;
    }

    if (config.fpbus_is_master) {
        fpbus_reg &= ~(1u << 5);  // MUX = 0 for internal oscillator
        fpbus_reg |=  (1u << 4);  // Clock Out Enable (master only)
        fpbus_reg |=  (1u << 1);  // status lines output enable
        fpbus_reg |=  (1u << 0);  // crl lines output enable
    } else {
        fpbus_reg &= ~(1u << 4);  // Clock Out Disable
        fpbus_reg |=  (1u << 1);  // status lines output enable
        fpbus_reg &= ~(1u << 0);  // crl lines output disable
    }
    if (vme_crate->vme_A32D32_write(SIS3315_FP_LVDS_BUS_CONTROL, fpbus_reg)) {
        return -1;
    }
    break;
}
    case ClockSource::EXTERNAL_NIM: {
        if (config.bypass_nim) {
            if (adc_->bypass_external_clock_multiplier()) {
                return -1;
            }
        } else {
            if (adc_->set_external_clock_multiplier(
                    config.nim_bw_sel,
                    config.nim_n1hs,
                    config.nim_n1clk1,
                    config.nim_n1clk1,  // N1CLK2 = N1CLK1 (symmetrical output)
                    config.nim_n2,
                    config.nim_n3,
                    static_cast<unsigned int>(config.nim_input_freq_mhz))) {
                return -1;
            }
        }
        usleep(1200000); // wait for SI5325 to stabilize
        break;
    }}
    //4. Program the Bit Clock -> Convert Clock divider to 12 for all sampling frequencies.
    if (vme_crate->vme_A32D32_write(SIS3315_CNV_CLK_DIVIDER_SPI_REG, SIS3315_AD9508_SPI_REG_VAL(12))) {
        return -1;
    }

    //5. Issue a Key ADC Clock DCM/PLL Reset command.
    if (vme_crate->vme_A32D32_write(SIS3315_KEY_ADC_CLOCK_DCM_RESET, 0x0)) {
        return -1;
    }
    usleep(5000);

    if (checkPllLocked(vme_crate)) {
        return -1;
    }

    // 6.IOB tap delay
    // CH1-CH4
    if (vme_crate->vme_A32D32_write(SIS3315_ADC_CH1_4_INPUT_TAP_DELAY_REG,
                                    SIS3315_IOB_DELAY_CALIBRATE)) { return 1; }
    usleep(1000);
    if (vme_crate->vme_A32D32_write(SIS3315_ADC_CH1_4_INPUT_TAP_DELAY_REG,
                                    SIS3315_IOB_DELAY_SET(SIS3315_IOB_DELAY_VALUE_DEFAULT))) { return 1; }
    // CH5-CH8
    if (vme_crate->vme_A32D32_write(SIS3315_ADC_CH5_8_INPUT_TAP_DELAY_REG,
                                    SIS3315_IOB_DELAY_CALIBRATE)) { return 1; }
    usleep(1000);
    if (vme_crate->vme_A32D32_write(SIS3315_ADC_CH5_8_INPUT_TAP_DELAY_REG,
                                    SIS3315_IOB_DELAY_SET(SIS3315_IOB_DELAY_VALUE_DEFAULT))) { return 1; }
    // CH9-CH12
    if (vme_crate->vme_A32D32_write(SIS3315_ADC_CH9_12_INPUT_TAP_DELAY_REG,
                                    SIS3315_IOB_DELAY_CALIBRATE)) { return 1; }
    usleep(1000);
    if (vme_crate->vme_A32D32_write(SIS3315_ADC_CH9_12_INPUT_TAP_DELAY_REG,
                                    SIS3315_IOB_DELAY_SET(SIS3315_IOB_DELAY_VALUE_DEFAULT))) { return 1; }
    // CH13-CH16
    if (vme_crate->vme_A32D32_write(SIS3315_ADC_CH13_16_INPUT_TAP_DELAY_REG,
                                    SIS3315_IOB_DELAY_CALIBRATE)) { return 1; }
    usleep(1000);
    if (vme_crate->vme_A32D32_write(SIS3315_ADC_CH13_16_INPUT_TAP_DELAY_REG,
                                    SIS3315_IOB_DELAY_SET(SIS3315_IOB_DELAY_VALUE_DEFAULT))) { return 1; }

    return 0;
    
    }

