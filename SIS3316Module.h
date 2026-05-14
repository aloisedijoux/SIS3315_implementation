#ifndef SIS3316MODULE_H
#define SIS3316MODULE_H
#include "ADCModule.h"
#include "drivers/sis3316_class.h"
#include "ITransport.h"
#include "/home/aloiselkb/sis3315_implementation/drivers/sis3316.h"
// "overloaded" = même nom, signatures différentes
// Le compilateur choisit lequel appeler selon les arguments

class SIS3316Module : public ADCModule {
public:
    // Surcharge 1 — Ethernet implicite
    explicit SIS3316Module(const std::string& ip);

    // Surcharge 2 — transport injecté explicitement
    explicit SIS3316Module(const std::string& ip,
                            std::shared_ptr<ITransport> transport);

    // Surcharge 3 — slot VME
    explicit SIS3316Module(uint32_t baseAddress, int crateHandle);

    void ADCModule::Connect() override {
        // code de connexion spécifique au SIS3316
    };

    std::vector<Event> readData();
    const std::vector<Event*> getEventPointers() {
        std::vector<Event*> ptrs;
        for (auto& e : eventBuffer_)
            ptrs.push_back(&e);
        return ptrs;
    }

    void DetectHardwareVersion(); // lit un registre pour différencier 14 bit vs 16 bit
    void DetectFirmwareVersion(); // lit un registre pour adapter les limites de certains paramètres

// Setter dédié pour choisir le connecteur externe
void SetExternalClockConnector(uint32_t sel) {
    // sel = 1 (VXS), 2 (LVDS), 3 (NIM)
    if (sel >= 1 && sel <= 3)
        externalClockSel_ = sel;
}

private:
std::unique_ptr<::SIS3316Module> hwModule_;
std::vector<Event> eventBuffer_; // stocké dans le module
uint32_t maxPreTrigger_ = 16378; // valeur par défaut firmware récent

// Membre privé pour mémoriser le connecteur externe préféré
uint32_t externalClockSel_ = 0b10; // LVDS par défaut

};



#endif // SIS3316MODULE_H