#include "ADCModule.h"
#include "sis3315-software/libraries_and_includes/sis3315_class_library/sis3315_class.h"
#include <functional>
// pour chaque parametre ADCParameterType, on crée une struct const uint32 contenant les registres quicorrespondent à l'enum 

static const uint32_t kAveragingModeAddrs[] = {
    
SIS3315_ADC_CH1_4_AVERAGE_CONFIGURATION_REG,						 // only SIS3316-16bit
SIS3315_ADC_CH5_8_AVERAGE_CONFIGURATION_REG	,					 // only SIS3316-16bit
SIS3315_ADC_CH9_12_AVERAGE_CONFIGURATION_REG,					 // only SIS3316-16bit
SIS3315_ADC_CH13_16_AVERAGE_CONFIGURATION_REG,					 // only SIS3316-16bit

};

static const uint32_t kClockSourceAddrs[] = {
    SIS3315_SAMPLE_CLOCK_DISTRIBUTION_CONTROL,    	    			      /* read/write; D32 */
SIS3315_ADC_CLK_OSC_I2C_REG,	// TODO: voir ce qu'on peut déjà faire avec les fonctions qui implémentent ce registre, fournies par struck, càd set_frequency,set_ADC_bit_clock_frequency,change_frequency_HSdiv_N1div et get_frequency			     	    			      /* read/write; D32 */

};

static const uint32_t kIputTerminationAddrs[] = {
    SIS3315_HARDWARE_VERSION, // soudure en usine, pas de registre logiciel. Uniquement lisible.
    // seul ajustement logiciel possible est le décalage d'offset via les reg suivant
    SIS3315_ADC_CH1_4_ANALOG_CTRL_REG, // TODO: see write_all_gain_termination_values
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
    SIS3315_ADC_CH5_8_RAW_DATA_BUFFER_CONFIG_REG , 						  
SIS3315_ADC_CH9_12_RAW_DATA_BUFFER_CONFIG_REG, 						  
SIS3315_ADC_CH13_16_RAW_DATA_BUFFER_CONFIG_REG,		

};

static const uint32_t kTIPortsAddrs[] = {
    SIS3315_NIM_INPUT_CONTROL_REG,

};

static const uint32_t kTOPortsAddrs[] = {
    SIS3315_LEMO_OUT_TO_SELECT_REG, // TODO: see write_nim_output_selection_values
};

static const uint32_t kTriggerEnableAddrs[] = {
    SIS3315_ADC_CH1_4_EVENT_CONFIG_REG,
    SIS3315_ADC_CH5_8_EVENT_CONFIG_REG  ,								  
SIS3315_ADC_CH9_12_EVENT_CONFIG_REG, 								  
SIS3315_ADC_CH13_16_EVENT_CONFIG_REG,

//pileup trigger (bit 0 de chaque canal)
SIS3315_ADC_CH1_4_EXTENDED_EVENT_CONFIG_REG,
SIS3315_ADC_CH5_8_EXTENDED_EVENT_CONFIG_REG  		 ,				  
SIS3315_ADC_CH9_12_EXTENDED_EVENT_CONFIG_REG ,						  
SIS3315_ADC_CH13_16_EXTENDED_EVENT_CONFIG_REG,
};

static const uint32_t kTrigThreshAddrs[] = {

    // TODO: see internal_trigger_generation_setup
    SIS3315_ADC_CH1_FIR_TRIGGER_THRESHOLD_REG,  // 0x1044
    SIS3315_ADC_CH2_FIR_TRIGGER_THRESHOLD_REG,  // 0x1054
    SIS3315_ADC_CH3_FIR_TRIGGER_THRESHOLD_REG,  // 0x1064
    SIS3315_ADC_CH4_FIR_TRIGGER_THRESHOLD_REG,  // 0x1074
    SIS3315_ADC_CH5_FIR_TRIGGER_THRESHOLD_REG,  // 0x2044
    SIS3315_ADC_CH6_FIR_TRIGGER_THRESHOLD_REG	,					 
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
SIS3315_ADC_CH5_8_SUM_FIR_TRIGGER_THRESHOLD_REG	,				 
SIS3315_ADC_CH9_12_SUM_FIR_TRIGGER_THRESHOLD_REG,				 
SIS3315_ADC_CH13_16_SUM_FIR_TRIGGER_THRESHOLD_REG,
};

static const uint32_t kVoltageRangeAddrs[] = {
    SIS3315_HARDWARE_VERSION,
};

// la questino qui se pose est l'écriture d'un registre depuis une valeur entrée par setParameter, correspondant à un parametre. 
// on relie ces différents élements grace à une map. elle contiendra un parametre et un rgisterdescriptor, càd une sorte de table qui indique comment configurer chaque bit selon un format générique propre au SIS3315. 
struct RegisterDescriptor {
    const uint32_t* adresses;  // ce sont les static const correspondant aux regroupements de registres selon le parametre
    uint32_t        addrCount;  // nombre de groupes (ex. 4 pour les registres avec un groupe par 4 canaux)
    uint32_t mask; // masque de bits à appliquer selon les bits à configurer indiqués dans le datasheet. Par exemple, si les bits 0 et 1 sont à configurer pour un certain param et que le reste des bits càd de 2 à 31 sont reserved, alors on aura 00000000000000000000000000000011 = 0x00000003 en hexa
    uint32_t shift; // par exemple si les bits à configurer sont les bits 4 à 6, alors shift = 4
    std::function<uint32_t(uint32_t)> encode; // fonction qui prend la valeur entrée en paramètre, on la passe dans l'argument d'une lambda qui ne capture rien, et cette fonction renvoie un entier de 32 bits qui correspond à la valeur à écrire dans le registre après application du masque et du shift. Par exemple, si on a une valeur de 5 à configurer pour un paramètre dont les bits à configurer sont les bits 4 à 6, alors la lambda va faire : (5 << 4) & 0x00000070 et renvoyer cette valeur.
};

static const std::unordered_map<ADCParameterType, RegisterDescriptor> kParamRegistry = {
    {
        ADCParameterType::AveragingMode,
        {
            .adresses = kAveragingModeAddrs,
            .addrCount = 4,
            .mask = 0x00000007, //average mode bit : 0-2
            .shift = 0,
            // .encode = // TODO : encode
        }
    },

    {
        ADCParameterType::ClockSource,
        {
            .adresses = kClockSourceAddrs,
            .addrCount = 1, // tout est appliqué à tous les canaux d'un coup 
            .mask = 0x40, //TODO : verify the value
            .shift = 0,
            // .encode = 
            // TODO : encode
        }
    },
    {
        ADCParameterType::InputTermination,
        {
            .adresses = kIputTerminationAddrs,
            .addrCount = // TODO
            .mask = //TODO
            .shift = //TODO
            // .encode =
             // TODO : encode
        }
    },
        {
        ADCParameterType::PreTriggerLength,
        {
            .adresses = kPretriggerAddrs,
            .addrCount = 4,
            .mask = 0x00003FFF, // bits de 0 à 13.
            .shift = 0,
            // .encode =
             // TODO : encode
        }
    },
    {
        ADCParameterType::SampleLength,
        {
            .adresses = kSampleLengthAddrs,
            .addrCount = 4,
            .mask = 0xFFFF0000,  // bits 16 à 31
            .shift = 16,
            // .encode = // TODO : encode
        }
    },
    {
        ADCParameterType::SamplingRate,
        {

            /*
            You can take the value for “Input tap delay” from the Tap-Delay Table (previous page) for a
specific device and the desired sampling frequency and then use this value and the
“configure_adc_fpga_iob_delays” method to program the input tap delays for the FPGA
ADC-inputs.
            */
            .adresses = //TODO
            .addrCount = 
            .mask = 
            .shift = 
            // .encode = // TODO : encode
        }
    },
    {
        ADCParameterType::TIPort,
        {
            .adresses = kTIPortsAddrs,
            .addrCount = 1,
            .mask = 0x80, // write NIM Input TI Function on bit 7
            .shift = 7,
            // .encode = // TODO : encode
        }
    },
    {
        ADCParameterType::TOPort,
        {
            .adresses = kTOPortsAddrs,
            .addrCount = 1,
            .mask = 0x20000000, // set on bit 30
            .shift = 30,
            // .encode = // TODO : encode
        }
    },
    {
        ADCParameterType::TriggerEnable,
        {
            .adresses = kTriggerEnableAddrs,
            .addrCount = 4,
            .mask = 0x00000003,// bits 0 et 1
            .shift = 0,
            // .encode = // TODO : encode
        }
    },
    {
        ADCParameterType::TriggerThreshold
        {
            .adresses = kTrigThreshAddrs,
            .addrCount = 16,
            .mask = // bits 0-27: threshold value, bit 31 : trigger enable 
            // TODO : convertir binaire 0-27 en hexa
            .shift = 0,
            // .encode = // TODO : encode
        }
    },
    {
        ADCParameterType::VoltageRange,
        {
            .adresses = kVoltageRangeAddrs,
            .addrCount = 1,
            .mask = // Bit value decoding for the field ‘Assembly variant (Bit 2-0)’: for input voltage configuration. If 000, pm 2.5 V
            .shift = 
            // .encode = // TODO : encode
        }
    },
    
};
// construceur de ADCModule de la forme : ADCModule()
// constructeur de sis3315_adc de la forme : sis3315_adc(vme_interface_class* crate, unsigned int baseaddress)
class SIS3315Module : public ADCModule, public sis3315_adc {
public:
    SIS3315Module(vme_interface_class* crate, unsigned int baseaddress);

    //fonctions concenant les paramètres de l'adc. // probleme suivant : on peut configurer la plupart des registres par groupe de canaux. on ajoute donc ChannelGroup en param 

    void SetParameter(ADCParameterType param, float value, int channelGroup = 0);
    bool isValueSupported(ADCParameterType param, float value);
    float GetValue();
    
    

    
};  

