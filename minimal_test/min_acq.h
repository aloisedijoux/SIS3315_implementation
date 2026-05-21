// we thought that it woud be more efficient to first write a very minimal code to test each feature of the module, step by step.
// the idea is to list the possible configuration parameters but in a hierarchical way: first the very required one, then the more optional and by doing the bare minimum we can see which are necessary or not.

// without external signal, we have two features : the software trigger, the reg 0x418 generates a trigger by writing on vme, and the adc offset : we get some noise even if there is no ext signal.
// this is sufficient to validate the readout chain.
// readout sequence :
// 1. reset : puisque le module contient deux fpga, un pour les adc et un ctrl, les fpga étabnt basqiuement de la logique combinatoire mêlée à des registres d'état ou bien plus techniquement un sr latch avec une horloge, donc qui a une mémoire il faut reset ces registres qui gardent en mémoire les précédentes valeurs de bits.
#include "drivers/vme_interface_class.h"

#include "sis3315-software/libraries_and_includes/sis3315_header/sis3315.h"
#include "sis3315-software/libraries_and_includes/sis3315_class_library/sis3315_class.h"

class SIS3315Min : public sis3315_adc {
    public :
    void Reset();
    //ctor
    SIS3315Min(vme_interface_class* crate, unsigned int baseAddress)
        : sis3315_adc(crate, baseAddress), crate_(crate), baseAddress_(baseAddress) {}
    unsigned int ReadBaseAddress();
    private:
    vme_interface_class* crate_;
    unsigned int baseAddress_;
};