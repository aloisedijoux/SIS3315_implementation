#include "SIS3315Module.h"
#include <map>
#include <vector>
#include <algorithm> // pour std::find
#include "/home/aloiselkb/sis3315_implementation/sis3315-software/libraries_and_includes/sis3315_header/sis3315.h"
#define CLOCK_FREQ_MHZ 180.0f
#define CLOCK_DIVIDER_VALUE 12
#define OUTPUT_CLK_MIN 10 //MHz
#define OUTPUT_CLK_MAX 1400 // MHz
static constexpr double SIS3315_MIN_SAMPLING_MSPS   =  1.0;  // MSPS minimum
static constexpr double SIS3315_MAX_SAMPLING_MSPS   = 15.0;  // MSPS maximum (datasheet)
static constexpr int    SIS3315_BIT_TO_CONVERT_RATIO = 12;   // imposé par ADAQ23878
static constexpr double IOB_DELAY_VALUE              = 0x14; // recommandé datasheet

// bits du registre fpbus control (0x58) 
#define FP_BUS_SAMPLE_CLOCK_OUT_MUX_BIT 5 // if 0: onboard programmable oscillator if 1: exrernal clock from nim connector ci
#define FP_BUS_SAMPLE_CLOCK_OUT_ENABLE_BIT 4
#define FP_BUS_STATUS_LINES_OUTPUT_ENABLE_BIT 1
#define FP_BUS_CONTROL_LINES_OUTPUT_ENABLE_BIT 0


#include <iostream>
// VCO things from SI570 datasheet
#define VCO_MIN 4850.0
#define VCO_MAX 5670.0
#include <stdexcept>
SIS3315Module::SIS3315Module(vme_interface_class* crate, unsigned int baseaddress)
    : ADCModule(),
      sis3315_adc(crate, baseaddress)
{
    //parametres supportés doivent toujurs exister donc on les met dasn le constructeur
 
 //les parametres suivants sont ceux pris en charge par le module mais à valeur discrète. 
    supportedParameterValues_[ADCParameterType::SamplingRate] = {
        25.0f, 50.0f, 62.5f, 71.0f, 83.0f, 100.0f,
        104.0f, 114.0f, 119.0f, 125.0f, 139.0f,
        167.0f, 179.0f, 208.0f, 227.0f, 250.0f
    };

     supportedParameterValues_[ADCParameterType::VoltageRange] = {
        3.0f,    // pm1.5 V
        5.0f,    // pm2.5 V
        8.192f,  // pm4.096 V
        10.0f,   // pm5 V
        20.0f    // pm10 V
    };

    supportedParameterValues_[ADCParameterType::InputTermination] = {
        50.0f,   // 50 ohm
        935.0f   // Hi-Z 
    };

     supportedParameterValues_[ADCParameterType::AveragingMode] = {
        0.0f, 4.0f, 8.0f, 16.0f, 32.0f, 64.0f, 128.0f, 256.0f
    };

    supportedParameterValues_[ADCParameterType::ClockSource] = {
        // there are two bits that can be configured in the Sample Clock distribution control” register. 
        // 00 (option 0) is for Onboard programmable Oscillator (Powerup default: 125MHz)
        // 01 (option 1) is unused
        // 10 (option2) is for External Clock from FP-LVDS Bus Connector (Frontpanel)
        // 11 (option 3) is for External Clock from NIM Connector (Frontpanel) (via programmable Clock Multiplier)
        0.0f,  // internal SI570
        2.0f,  // external (LVDS connector)
        3.0f   // external (NIM connector)
    };

    supportedParameterValues_[ADCParameterType::TriggerEnable] = {
        0.0f,  // disabled
        1.0f   // enabled
// TODO:
//         NIM Input TI as Trigger Enable
// NIM Input Contol/Status Reg bit 4
// Feedback Selected Internal Trigger As External Trigger Enable
// Acquisition Contol/Status Reg bit 14
// External trigger enable
// Event conf. Reg bit 3 / 11 / 19 / 27
    };

    //pour les valeurs continues, on utilise des règles de validation avec std::function et des lambdas.
    // ici, supposons qu'on ait une plage ocntinue. On va set un paramètre avec une certaine valeur qui sera ou non  acceptée.

}

bool SIS3315Module::isValueSupported(ADCParameterType param, float value) {
    const auto& supportedValues = GetSupportedValues(param);
    return std::find(supportedValues.begin(), supportedValues.end(), value) != supportedValues.end();
}


void SIS3315Module::SetParameter(ADCParameterType param, float value, int channelGroup=0) {
    //si la valeur est supportée 
    if (isValueSupported(param, value) == true) {
        switch (param) {
            case 
        }
    }
}

int SIS3315Module::resetModule() {
    int rc = register_write(SIS3315_KEY_RESET, 0x0);
    if (rc) throw std::runtime_error("Key Reset échoué (rc=" + std::to_string(rc) + ").");
}

/*
Fonctionnement de l'horgloge : 
la séquence est toujours la même, mais quelques étapes varient selon la source d'holorge chosiie. 
Registre pour la séledction de cloc : SIS3315_SAMPLE_CLOCK_DISTRIBUTION_CONTROL
*/
// trois choix:
// oscillateur interne 
// fp bus:Sur le maître il faut aussi activer la sortie d'horloge vers le FP-Bus (registre 0x58, bit "FP-Bus Sample Clock Out Enable").
// nim externe 

SIS3315ClockConfig SIS3315Module::ClockConfiguration(SIS::ADC::SIS3315::SampleRate sample_rate, ClockSource clock_source, FpBusRole fp_bus_role, NimMode nim_mode, const NimClockParams* nim_params) {
    //0. verifications
    //0.1 vérifier que sample_rate fait bien partie du namespace SIS::ADC::SIS3315::SampleRate
    if (std::find(supportedParameterValues_[ADCParameterType::SamplingRate].begin(), supportedParameterValues_[ADCParameterType::SamplingRate].end(), static_cast<float>(sample_rate)) == supportedParameterValues_[ADCParameterType::SamplingRate].end()) {
        throw std::invalid_argument("Sample rate non supporté.");
    }
    //1 . reset 
    resetModule();
    //2. select clock source 
    // bits 1:0 : 00=SI570 interne, 10=FP-Bus, 11=NIM
    int rc = register_write(SIS3315_SAMPLE_CLOCK_DISTRIBUTION_CONTROL, (unsigned int)clock_source); // conversion de type, si l'utilisateur entre  ClockSource::CLOCK_SRC_INTERNAL, ça écrit 0 dans les bits 1:0 du registre, etc.
    if (rc) throw std::runtime_error("Échec de la sélection de la source d'horloge (rc=" + std::to_string(rc) + ").");
    //3. configurer les étapes suivantes selon la source d'horloge choisie
    switch (clock_source &&  ) {
        case CLOCK_SRC_INTERNAL:
            // configurer l'oscillateur SI570 interne pour obtenir la fréquence d'échantillonnage désirée. La fréquence d'échantillonnage est déterminée par la formule : fout = DCO/(HS_DIV * N1) où DCO est la fréquence du V
            rc = set_ADC_bit_clock_frequency(CLOCK_FREQ_MHZ, nullptr); // la lib fournit par struck fait déjà le travail de programmation du SI570 et de reset de la PLL interne, on n'a plus qu'à lui donner la fréquence d'horloge désirée. La fréquence effectivement programmée peut être récupérée via le second paramètre (pointeur), mais on s'assure d'abord que la configuration est valide avec find_hs_n1.
            if (rc) throw std::runtime_error("Échec de la configuration de l'oscillateur interne (rc=" + std::to_string(rc) + ").");
            break;
        case CLOCK_SRC_FPBUS:
        // Registre 0x58 — FP-Bus control (SIS3315_FP_LVDS_BUS_CONTROL)
        unsigned int fpbus_config;
        // on a une config maitre:
        // - on active la sortie d'horloge vers le FP-Bus (bit 4)
        // - on choisit la source d'horloge (bit 5) : dans ce cas, on veut que ce soit l'oscillateur interne qui soit distribué sur le FP-Bus, donc on met ce bit à 0.
        // - on peut aussi activer les status lines et control lines si besoin (bit 1 et 0), mais ce n'est pas nécessaire pour la distribution de l'horloge.

        if (fp_bus_role == FP_BUS_MASTER) {
            fpbus_config = (1 << FP_BUS_SAMPLE_CLOCK_OUT_ENABLE_BIT) | (1 << FP_BUS_STATUS_LINES_OUTPUT_ENABLE_BIT) | (1 << FP_BUS_CONTROL_LINES_OUTPUT_ENABLE_BIT);
        } else { // esclave : il recoit la clock donc on met le bit 1 à 1 pour activr tous les modules et les bits 4 et 0 à 0 ppour désactiver le pilotage du bus
            fpbus_config = (0 << FP_BUS_CONTROL_LINES_OUTPUT_ENABLE_BIT) | (1 << FP_BUS_STATUS_LINES_OUTPUT_ENABLE_BIT) | (0 << FP_BUS_SAMPLE_CLOCK_OUT_ENABLE_BIT);

        }
        //appliquer la configuration du FP-Bus au registre 0x58
        rc = register_write(SIS3315_FP_LVDS_BUS_CONTROL, fpbus_config);
        if (rc) throw std::runtime_error( "FP-Bus control (0x58) échoué (rc=" + std::to_string(rc) + ").");
        usleep(100000); // attendre 100 ms pour que les changements de configuration prennent effet
        break; //TODO : faire en sorte que si 0 pour bit 5, on applique la logique du internal oscillator et si cest 1, du NIM connector
        case CLOCK_SRC_NIM:
        // 2 cas: soit l'horloge arrive déjà à la bonne frequence, soit elle est trop basse. on a anouté un nimMode
            if (nim_mode == NIM_BYPASS) {
                rc = bypass_external_clock_multiplier();
                if (rc) throw std::runtime_error("Échec du bypass du multiplicateur d'horloge externe (rc=" + std::to_string(rc) + ").");
            } else {
                // mode PLL pour multiplier la frequence d'entrée. on utilise set_external_clock_multiplier()
                // Vérification rapide de la formule avant d'envoyer
            double f_out_check = (double)nim_params->clkin_mhz
                               * nim_params->n2
                               / ((double)nim_params->n1_hs
                                * nim_params->n1_clk1
                                * nim_params->n3);
            rc = set_external_clock_multiplier(nim_params->bw_sel, nim_params->n1_hs, nim_params->n1_clk1, nim_params->n1_clk2, nim_params->n2, nim_params->n3);
                            // Codes d'erreur retournés par set_external_clock_multiplier() :
            //   -2 : clkin_mhz hors plage [10, 250]
            //   -3 : bw_sel > 15
            //   -4 : n1_hs hors [4, 11]
            //   -5 : n1_clk1 invalide (0, impair!=1, ou > 2^20)
            //   -6 : n1_clk2 invalide (0, impair!=1, ou > 2^20)
            //   -7 : n2 hors [32, 512] ou impair
            //   -8 : n3 invalide (0 ou > 2^19)
            }
            break;
        default:
            throw std::invalid_argument("Source d'horloge non supportée.");

        //4. config de la bit clock
        rc = convert_clock_divider_ad9508_setup(CLOCK_DIVIDER_VALUE);
        if (rc) throw std::runtime_error("Échec de la configuration du diviseur d'horloge AD9508 (rc=" + std::to_string(rc) + ").");

        //5. reset dcm pll 
        rc = reset_adc_fpga_sample_clock_PLL();
        if (rc) throw std::runtime_error("Échec du reset de la PLL d'horloge d'échantillonnage du FPGA ADC (rc=" + std::to_string(rc) + ").");

        //6. calibrate and config the adc fpga input logic of the adc data input 
        rc = configure_adc_fpga_iob_delays(IOB_DELAY_VALUE);
        if (rc) throw std::runtime_error("Échec de la configuration des délais IOB du FPGA ADC (rc=" + std::to_string(rc) + ").");

        // results
        double actual_frequency_msps = CLOCK_FREQ_MHZ / CLOCK_DIVIDER_VALUE;
    double actual_precision_ns   = 1000.0 / actual_frequency_msps;

    return SIS3315ClockConfig{
        .frequency_msps = static_cast<double>(sample_rate),
        .bit_clock_mhz = CLOCK_FREQ_MHZ,
        .actual_frequency_msps = actual_frequency_msps,
        .actual_precision_ns = actual_precision_ns,
        .clock_source = clock_source,
        .fp_bus_role = fp_bus_role,
        .nim_mode = nim_mode
    };
        break;
}
// input termination : SIS3315_HARDWARE_VERSION. bit 4
