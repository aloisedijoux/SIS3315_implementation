#include "sis3315-software/libraries_and_includes/sis3315_class_library/sis3315_class.h"
#ifndef SIS3315MODULE_H
#define SIS3315MODULE_H
#include "ADCModule.h"
    
class SIS3315Module : public ADCModule, public sis3315_adc {
public:
void ReadoutLoop();
void Connect() override {
    // code de connexion spécifique au SIS3315
}
std::vector<Event> readData();
const std::vector<Event*> getEventPointers() {
        std::vector<Event*> ptrs;
        for (auto& e : eventBuffer_)
            ptrs.push_back(&e);
        return ptrs;
    }
private:
std::unique_ptr<::SIS3315Module> hwModule_;   
std::vector<Event> eventBuffer_; // stocké dans le module
};

#endif // SIS3315MODULE_H