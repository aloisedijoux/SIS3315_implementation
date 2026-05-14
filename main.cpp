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
}