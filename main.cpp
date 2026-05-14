#include "ADCModule.h"
#include <memory>
#include "SIS3316Module.h"
#include "SIS3315Module.h"

int main() {
// "Once" → une seule classe concrète
// "Easily exchanged" → polymorphisme : même type de base, swap en une ligne
std::shared_ptr<ADCModule> adc;
adc = std::make_shared<ADCModule::SIS3315Module>("192.168.1.10"); // option A
adc = std::make_shared<ADCModule::SIS3316Module>("192.168.1.10"); // option B
// Le reste du code ne change pas d'une ligne
// "or" = les deux approches sont valides, on peut combiner
// Constructeur = configure QUOI connecter
// Connect()    = établit la connexion réelle QUAND on veut

SIS3316Module m("192.168.1.10"); // objet créé — pas encore connecté
                                  // on peut configurer, tester, mocker

bool ok = m.Connect();            // connexion établie ici
if (!ok) {
    log("module non disponible");
    return;
}
m.StartAcquisition();

// Avantage clé : l'objet existe sans hardware
// - testable sans module physique présent
// - reconnectable sans recréer l'objet


module->SetEventCallback([](const Event& e) { process(e); });
module->SetEventCallback([this](const Event& e) { this->OnEvent(e); });

std::vector<std::shared_ptr<ADC::ADCModule>> modules;
modules.push_back(
    std::make_shared<SIS3316Module>("192.168.1.10"));
modules.push_back(
    std::make_shared<SIS3316Module>("192.168.1.11"));
modules.push_back(
    std::make_shared<SIS3315Module>("192.168.1.12"));

    for (auto& m : modules) m->Connect();
for (auto& m : modules) m->StartAcquisition();

bool MidasADCFrontend::Init() {
    for (auto& m : modules_)
        if (!m->Connect()) return false; // Connect sur tous
    return true;
}

bool MidasADCFrontend::Begin(int runNumber) {
    SyncAllClocks();
    for (auto& m : modules_)
        if (!m->StartAcquisition()) return false; //  Start sur tous
    return true;
}
}
