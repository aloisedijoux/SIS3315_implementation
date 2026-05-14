#include "sis3315-software/libraries_and_includes/sis3315_class_library/sis3315_class.h"
#ifndef SIS3315MODULE_H
#define SIS3315MODULE_H
#include "ADCModule.h"
    
class SIS3315Module : public ADCModule, public sis3315_adc {
public:
void ReadoutLoop();
void Connect() override {
    // code de connexion spécifique au SIS3315
};

#endif // SIS3315MODULE_H