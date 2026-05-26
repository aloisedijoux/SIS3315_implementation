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
#include <vector>
// bits du registre fpbus control (0x58) 
#define FP_BUS_SAMPLE_CLOCK_OUT_MUX_BIT 5 // if 0: onboard programmable oscillator if 1: exrernal clock from nim connector ci
#define FP_BUS_SAMPLE_CLOCK_OUT_ENABLE_BIT 4
#define FP_BUS_STATUS_LINES_OUTPUT_ENABLE_BIT 1
#define FP_BUS_CONTROL_LINES_OUTPUT_ENABLE_BIT 0

#define INPUT_TERMINATION_BIT 4 // 0 for hi Z and 1 for 50 ohm

// bits de threshold
#define ACQ_BIT_ADDR_THRESH_CH1_4   25  // Address Threshold Flag Ch1-4
#define ACQ_BIT_ADDR_THRESH_CH5_8   27  // Address Threshold Flag Ch5-8
#define ACQ_BIT_ADDR_THRESH_CH9_12  29  // Address Threshold Flag Ch9-12
#define ACQ_BIT_ADDR_THRESH_CH13_16 31  // Address Threshold Flag Ch13-16
#define ACQ_BIT_ADDR_THRESH_OR      19  // OR de tous les Address Threshold Flags
#define ACQ_BIT_SAMPLE_LOGIC_ARMED  16  // ADC Sample Logic Armed
#define ACQ_BIT_ARMED_ON_BANK2      17  // ADC Sample Logic Armed on Bank2

#define ACQ_BIT_SAMPLE_LOGIC_ARMED  16  // 1 = sample logic armée
#define ACQ_BIT_ARMED_ON_BANK2      17  // 1 = armée sur Bank2, 0 = Bank1
#define ACQ_BIT_NIM_SWAP_ENABLED    22  // 1 = NIM bank swap logic active

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

unsigned int SIS3315Module::max_events_from_duration(double duration_seconds, double trigger_rate_hz) {} //TODO 

// input termination : SIS3315_HARDWARE_VERSION. bit 4 but only for reading

InputTermination SIS3315Module::ReadInputTermination()
{
    unsigned int hw_version;
    int rc = register_read(SIS3315_HARDWARE_VERSION, &hw_version);
    if (rc) throw std::runtime_error(
        "Lecture Hardware Version échouée (rc=" + std::to_string(rc) + ").");

    return (hw_version & (1 << 4)) ? TERMINATION_50OHM : TERMINATION_HIZ;
}

// sequence de control flow avec mémoire double banque. on va implémenter les dux modes d'acquisition de données càd interface et nim
// logique : initialisation, boucle (attendre, lire bank1, attendre, lire bank2), disarm
int SIS3315Module::Disarm() {
    int rc = register_write(SIS3315_KEY_DISARM, 0);
    if (rc) throw std::runtime_error("Key Disarm échoué (rc=" + std::to_string(rc) + ").");
}
//TODO: voir su on pa besoin de threads pour faire des pollings simultanés sur les 4 mémoires
bool SIS3315Module::Poll(bool use_nim_mode, bool expect_bank2, unsigned int active_groups_mask, unsigned int timeout) { //config passée dans le control flow
    
//on a un temps total durant lequel oninterroge, en principe il est inifini et cest le timeout

// on a aussi une fréquence à laquelle on interrogecàd que toutes les n secondes, on récupère le status des registres SIS3315_ACQUISITION_CONTROL et n vérifie si l'un des bits d'adresse de seuil est 1. 
//On s'arrete là car cette fonction est utilitaire et sera implémentée dans la boucle ensuite.
// on a aussi le temps écoulé au fur et à mesure depuis le début polling
unsigned int polling_frequency_us = 100;
unsigned int elapsed_time_us = 0;
while (timeout == 0 || elapsed_time_us < timeout) {
    unsigned int acq_control;
    int rc = register_read(SIS3315_ACQUISITION_CONTROL_STATUS, &acq_control);
    if (rc) throw std::runtime_error("Lecture Acquisition Control échouée (rc=" + std::to_string(rc) + ").");
    if (use_nim_mode) {
        //mode nim : bit16=1 et 17=expected_bank2
        bool armed = (acq_control & (1 << ACQ_BIT_SAMPLE_LOGIC_ARMED)) != 0;
        bool bank2_active = (acq_control & (1 << ACQ_BIT_ARMED_ON_BANK2)) != 0;
        if (armed && bank2_active == expect_bank2) {
            std::cout << "[SIS3315] NIM acquisition event detected! (acq_control=0x" << std::hex << acq_control << std::dec << ")\n";
            return true; // événement d'acquisition NIM détecté
        } else {
            //mode interface : tous les groups acrifs levent leur flag
            if ((acq_control & active_groups_mask) == active_groups_mask) 
            return true;
        }
    }
    usleep(polling_frequency_us);
    elapsed_time_us += polling_frequency_us;
}
return false; // timeout atteint sans détecter l'événement d'acquisition
   

}
bool SIS3315Module::checkBankSwap()
{
    // Lire le registre d'état
    unsigned int acq_control = 0;
    int rc = register_read(SIS3315_ACQUISITION_CONTROL_STATUS, &acq_control);
    if (rc) throw std::runtime_error(
        "Lecture Acquisition Control échouée (rc=" + std::to_string(rc) + ").");

    // Le bit 17 dit quel bank est actif :
    //   1 : Bank2 active
    //   0: Bank1 active

    // Extraire la valeur du bit 17
    // On décale le registre de 17 positions vers la droite
    // puis on garde uniquement le dernier bit avec & 1
    unsigned int bit17 = (acq_control >> ACQ_BIT_ARMED_ON_BANK2) & 1;

    if (bit17 == 1)
        return true;   // Bank2 est active
    else
        return false;  // Bank1 est active
}
void SIS3315Module::read_bank_channels(unsigned int bank2_flag, const std::vector<unsigned int>& channels, unsigned int* buffer, DataCallback cb) {
    // vérifier que le channel est valide

    for (unsigned int ch : channels) {
        if (ch > 15)
            throw std::invalid_argument("Numéro de canal invalide: " + std::to_string(ch) + ". Doit être entre 0 et 15.");
        
        // on utilise read_MBLT64_Channel_PreviousBankDataBuffer 
        unsigned int nbofwords = 0;
        int rc = read_MBLT64_Channel_PreviousBankDataBuffer(bank2_flag, ch, &nbofwords, buffer); // buffer que l'utilisateur doit configurer lui meme 
        if (rc) { //bank2flag +1
        std::cerr << "[SIS3315] Avertissement : lecture canal " << ch
                      << " bank" << (bank2_flag + 1)
                      << " échouée (rc=" << rc << "), ignorée\n";
            continue;
        }
        if (nbofwords == 0) {
            throw std::runtime_error("Aucun mot lu pour le canal " + std::to_string(ch) + " bank" + std::to_string(bank2_flag + 1) + ".");
        }

        if (cb) //callback utilisateur 
            cb(bank2_flag+1, ch, buffer, nbofwords); // on retourne à l'utilisateur le numéro de bank, le numéro de canal, le buffer et le nombre de mots lus pour qu'il puisse faire ce qu'il veut avec les données
    }
}
void SIS3315Module::ControlFlowNIM(const AcquisitionConfig& config, DataCallback user_callback, volatile bool* run_flag) {
    /*
    1. Disarm or Reset command
2. Enable “Sample Bank Swap Control with NIM Input T-I/U-I” Logic command.
The Sample Bank Logic will be armed on Bank 1 with the next NIM input signal.
The logic will toggle the active Bank with each following NIM input pulse.
do {
3a. Poll on Sample Logic Armed and Bank 2 flags and wait until Armed on
Bank 2 is valid (acquisition control register: bit 16 = 1 and bit 17 = 1)
4a. read “Previous Bank Sample address registers Ch1 to Ch16”
(bit 24 gives means for checking whether the active
bank was swapped already. It will be cleared if the address corresponds to
Bank1 and will be set if the address corresponds to Bank2,)
5a. read sampled data from Ch1 to Ch16 (Memory Bank 1)
3b. Poll on Sample Logic Armed and Bank 2 flags and wait until Armed on
Bank 1 is valid (acquisition control register: bit 16 = 1 and bit 17 = 0)
4b. read “Previous Bank Sample address registers Ch1 to Ch16”
(bit 24 gives means for checking whether the active
bank was swapped already. It will be cleared if the address corresponds to
Bank1 and will be set if the address corresponds to Bank2,)
5b. read sampled data from Ch1 to Ch16 (Memory Bank 2)
} (run == 1)
6. Disarm command
Respective to 4a/5a and 4b/5b refer to the routine below also:
int sis3315_adc::read_MBLT64_Channel_PreviousBankDataBuffer(.)
in ..\sis3315_class_library\sis3315_class.cpp
Note:
The user must care for proper timing between bank swapping (external NIM signal) and
readout of the not active Bank.
    */

    int event_count = 0;
    unsigned int buffer_size = (config.nof_samples % 2 == 0) ? config.nof_samples : config.nof_samples + 1; //  buffer overflow sinon pour avoir une paire
    std::vector<unsigned int> buffer(buffer_size);
    //1. reset et ddisarm
    resetModule();
    Disarm();
//2. enable nim
    int rc = register_write(SIS3315_KEY_ENABLE_SAMPLE_BANK_SWAP_CONTROL_WITH_NIM_INPUT, 0);

    if (rc) throw std::runtime_error("Enable Sample Bank Swap Control with NIM Input échoué (rc=" + std::to_string(rc) + ").");
    // Vérification : bit 22 doit être à 1

    unsigned int acq_control = 0;
    register_read(SIS3315_ACQUISITION_CONTROL_STATUS, &acq_control);

    if ((acq_control & (1 << ACQ_BIT_NIM_SWAP_ENABLED)) == 0)
        throw std::runtime_error("Échec activation contrôle swap banque avec signal NIM (bit 22 = 0).");
//loop
    while (*run_flag && (config.max_events == 0 || event_count < config.max_events)) {

        // Cycle A : module actif sur bank2 -> lecture bank1
//3a
        if (!Poll(true, true, 0,config.poll_timeout_us)) //nim mode enable, bank2 doit ere actif, pas de mask de groupe actif car en mode nim on sait que les triggers sont sur tous les groupes
            throw std::runtime_error("Polling timeout while waiting for Bank 2 armed.");
//4a et 5a
read_bank_channels(0, config.channels, buffer.data(), user_callback); //bank2flag=0 pour lire la banque non active, càd bank1
++event_count;
if ((config.max_events > 0 && event_count >= config.max_events) || !*run_flag)
    break;

        // Cycle B : module actif sur bank1 -> lecture bank2
            //3b poll Armed on Bank2 (bit16=1, bit17=1)
        if (!Poll(true, false, 0, config.poll_timeout_us)) //nim mode enable, bank1 doit être actif, pas de mask de groupe actif car en mode nim on sait que les triggers sont sur tous les groupes
            throw std::runtime_error("Polling timeout while waiting for Bank 1 armed.");
//4b et 5b
read_bank_channels(1, config.channels, buffer.data(), user_callback); //bank2flag=1 pour lire la banque non active, càd bank2
++event_count;}
//6. disarm
Disarm();
}


void SIS3315Module::ControlFlowCycles(const AcquisitionConfig& config, DataCallback user_callback, volatile bool* run_flag)
{
    int event_count = 0;

    // TODO: définir la taille du buffer en fonction du nombre de samples configuré par l'utilisateur
    // dans les registres de configuration de chaque canal (0x1004 pour Ch1-4, etc.)
    // et du nombre de canaux à lire.
    // Taille minimale = nof_samples × (18 bits arrondis à 32 bits)
unsigned int buffer_size = (config.nof_samples % 2 == 0) ? config.nof_samples : config.nof_samples + 1; //  buffer overflow sinon pour avoir une paire
    std::vector<unsigned int> buffer(buffer_size);
    // 1. reset and disarm
    resetModule();
    Disarm();

    // 2. Set address threshold registers.
    // Canaux à lire à chaque bank : liste paramétrable.
    // 1 mémoire par groupe de 4 canaux.
    uint32_t threshold_registers[4] = {
        SIS3315_ADC_CH1_4_ADDRESS_THRESHOLD_REG,
        SIS3315_ADC_CH5_8_ADDRESS_THRESHOLD_REG,
        SIS3315_ADC_CH9_12_ADDRESS_THRESHOLD_REG,
        SIS3315_ADC_CH13_16_ADDRESS_THRESHOLD_REG
    };

    // Bits Address Threshold par groupe dans le registre 0x60
    const unsigned int thresh_bits[4] = {
        (1 << ACQ_BIT_ADDR_THRESH_CH1_4),   
        (1 << ACQ_BIT_ADDR_THRESH_CH5_8),    
        (1 << ACQ_BIT_ADDR_THRESH_CH9_12),   
        (1 << ACQ_BIT_ADDR_THRESH_CH13_16)  
    };

    // déterminer quels groupes sont actifs ou non

    // lambda qui vérifie si un canal est actif
    auto isChannelActive = [&config](unsigned int channel) -> bool {
        for (unsigned int ch : config.channels)
            if (ch == channel)
                return true;
        return false;
    };

    // pour les canaux actifs, écrire la valeur de seuil configurée
    // sinon écrire 0 pour éviter les triggers intempestifs
     unsigned int active_groups_mask = 0;

    for (int group = 0; group < 4; group++) {

        bool active = isChannelActive(group * 4)     || // Pour le groupe 0 : est-ce que le canal 0, 1, 2 ou 3 est dans la liste ?
                      isChannelActive(group * 4 + 1) || // Pour le groupe 1 : est-ce que le canal 4, 5, 6 ou 7 est dans la liste ?
                      isChannelActive(group * 4 + 2) ||
                      isChannelActive(group * 4 + 3);

        unsigned int threshold_value = active ? config.address_threshold : 0; // si le groupe est actif, on met le seuil configuré, sinon 0 pour éviter les triggers sur ce groupe

        int rc = register_write(threshold_registers[group], threshold_value);
        if (rc) throw std::runtime_error(
            "Écriture seuil groupe " + std::to_string(group) +
            " échouée (rc=" + std::to_string(rc) + ").");

        if (active) // Si le groupe est actif, on ajoute son bit au masque
            active_groups_mask |= thresh_bits[group];
    }

    // 3. Disarm active Bank and arm Bank2 command
    // On démarre avec bank2 active en écriture et bank1 libre.
    // On commence à remplir bank2.
    int rc = register_write(SIS3315_KEY_DISARM_AND_ARM_BANK2, 0);

    if (rc)
        throw std::runtime_error("Disarm and Arm Bank2 échoué (rc=" + std::to_string(rc) + ").");

   
   //4. ici on atend le flag address threshold via poll avec tiemout. 
   /*
   3. Disarm active Bank and arm Bank2 command (start with the second bank)
do {
4a. Poll on address threshold flag and wait until valid
(also possible to wait for a defined time)
5a. Disarm active Bank and arm Bank1 command
6a. check if the active Bank is swapped (also possible to wait for defined time,
for example the length of time of one event)
7a. read “Previous Bank Sample address registers Ch1 to Ch16”
(bit 24 will be cleared if the address corresponds to theBank1 and will be set
if the address correspond to Bank2, means for checking whether the active
bank was swapped already)
8a. read sampled data from Ch1 to Ch16 (Memory Bank 2)
4b. Poll on address threshold flag and wait until valid
(also possible to wait for a defined time)
5b. Disarm active Bank and arm Bank2 command
6b. check if the active Bank is swapped
7b. read “Previous Bank Sample address registers Ch1 to Ch16”
8b. read sampled data from Ch1 to Ch16 (Memory Bank 1)
} (run == 1)
9. Disarm command

Respective to 7a/8a and 7b/8b refer to the routine below also:
int sis3315_adc::read_MBLT64_Channel_PreviousBankDataBuffer(.)
in ..\sis3315_class_library\sis3315_class.cpp
Note 5ab / 6ab:
With the command “Disarm active Bank and arm Bank1/2” the logic will disarm the active
Bank to suppress a new start of capturing Hit/Events. The capturing of Hits/Events at this
moment will be continued. The logic waits to arm the Bank1/2 (alternate) until “all channels
are not busy”.
*/
   // Main loop
    // Tant que le flag de contrôle est à true et que le nombre max d'events n'est pas atteint
    while (*run_flag && (config.max_events == 0 || event_count < config.max_events)) {

        /************ Demi-cycle A ************/
        // bank2 vient de se remplir
        // on lit bank2 et le module bascule sur bank1

        // 4a : attente du flag address threshold via poll avec timeout
        if (!Poll(false, false, active_groups_mask, config.poll_timeout_us))
            throw std::runtime_error("Polling timeout reached without detecting address threshold flag.");

        // 5a : disarm bank active + arm bank1
        rc = register_write(SIS3315_KEY_DISARM_AND_ARM_BANK1, 0);

        if (rc)
            throw std::runtime_error("Disarm and Arm Bank1 échoué (rc=" + std::to_string(rc) + ").");

        // 6a : vérifier que bank2 n'est plus active
        // bit 17 du registre 0x60
        if (checkBankSwap())
            throw std::runtime_error("Bank swap check failed after arming Bank1: Bank2 is still active.");

        // 7a : lecture de bank2 -> bank flag = 1
        read_bank_channels(1, config.channels, buffer.data(), user_callback);

        ++event_count;
if ((config.max_events > 0 && event_count >= config.max_events) || !*run_flag)
            break;


        if (!*run_flag) {
            std::cout << "[SIS3315] Run flag set to false. Arrêt du contrôle de flux.\n";
            break;
        }

        /************ Demi-cycle B ************/
        // bank1 vient de se remplir
        // on lit bank1 et le module bascule sur bank2

        // 4b : attente du flag address threshold via poll avec timeout
        if (!Poll(false, false, active_groups_mask, config.poll_timeout_us))
            throw std::runtime_error("Polling timeout reached without detecting address threshold flag.");

        // 5b : disarm bank active + arm bank2
        rc = register_write(SIS3315_KEY_DISARM_AND_ARM_BANK2, 0);

        if (rc)
            throw std::runtime_error("Disarm and Arm Bank2 échoué (rc=" + std::to_string(rc) + ").");

        // 6b : vérifier que bank1 n'est plus active
        // bit 17 du registre 0x60
        if (!checkBankSwap())
            throw std::runtime_error("Bank swap check failed after arming Bank2: Bank1 is still active.");

        // 7b : lecture de bank1 -> bank flag = 0
        read_bank_channels(0, config.channels, buffer.data(), user_callback);

        ++event_count;

        if (config.max_events > 0 && event_count >= config.max_events) {
            std::cout << "[SIS3315] Nombre d'événements maximum atteint (" << event_count << "). Arrêt du contrôle de flux.\n";
            break;
        }

        if (!*run_flag) {
            std::cout << "[SIS3315] Run flag set to false. Arrêt du contrôle de flux.\n";
            break;
        }
    }

    // 9. disarm à la fin du run
    Disarm();
}


//deux modes combinés
void SIS3315Module::ControlFlow(const AcquisitionConfig& config, DataCallback user_callback, volatile bool* run_flag, bool use_nim_mode) {
    
 
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
InputRange SIS3315Module::ReadInputRange()
{
    unsigned int hw_version = 0;
    int rc = register_read(SIS3315_HARDWARE_VERSION, &hw_version);
    if (rc) throw std::runtime_error(
        "Lecture Hardware Version échouée (rc=" + std::to_string(rc) + ").");

    unsigned int bits2_0 = hw_version & 0x7; //section 5.7 datasheet

    switch (bits2_0) {
    case 1:
        std::cout << "[SIS3315] Plage d'entrée : ±2.5V (5V total)\n";
        return RANGE_PM25V;
    default:
        std::cout << "[SIS3315] Plage d'entrée : inconnue (bits 2:0 = "
                  << bits2_0 << ")\n";
        return UNKNOWN_RANGE;
    }
}
// offsets methods 

void SIS3315Module::ConfigureSignal(unsigned int offset_all_channels = 0x8000)
{
    // 1. Info hardware — lecture seule, pour que le physicien sache
    //    quel câble utiliser et si son signal rentre dans la plage
    ReadInputTermination();
    ReadInputRange();

    // 2. Offset DAC — même valeur sur tous les canaux
    //    0x8000 = centré sur 0V (défaut recommandé)
    //    Le physicien peut modifier canal par canal après cet appel :
    //    module.adc_dac_offset_ch_array[0] = 0x6667;
    for (unsigned int ch = 0; ch < 16; ch++)
        this->adc_dac_offset_ch_array[ch] = offset_all_channels;

    // 3. Envoyer les offsets au hardware
    int rc = write_all_adc_dac_offsets();
    if (rc) throw std::runtime_error(
        "write_all_adc_dac_offsets() échoué (rc=" + std::to_string(rc) + ").");
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