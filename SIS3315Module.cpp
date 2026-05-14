#include "SIS3315Module.h"
#include "sis3315-software/libraries_and_includes/sis3315_class_library/sis3315_class.h"
#include "EventClass.h"
// pour mutex
#include <mutex>
#include <thread>
#include <atomic>

void SIS3315Module::SetParameter(const std::string& name, int value)
{
    if (name == "threshold")
    {
        register_write(0x0028, value);
    }
}


// Boucle dans un thread dédié — non bloquante pour l'appelant
void SIS3315Module::ReadoutLoop() {
    Event e; // alloué une fois, réutilisé — pas de new à chaque événement

    while (running_.load()) {           // atomic : thread-safe sans mutex
        {
            std::lock_guard<std::mutex> lock(paramMutex_);
            if (hwModule_->DataAvailable()) {
                e = parse(hwModule_->FetchEvent());
            }
        }                               // mutex relâché ici
        DispatchEvent(e);               // callback hors mutex
        std::this_thread::sleep_for(
            std::chrono::microseconds(100)); // ~10kHz max, pas de busy-wait
    }
}

void SIS3315Module::DispatchEvent(const Event& e) const {
    if (cb_) cb_(e); // appel direct, zéro copie
}