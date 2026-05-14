#include "drivers/vme_interface_class.h"

// Toute écriture registre passe par ITransport
// Jamais directement depuis Midas ou PAQS
class ITransport {
    virtual bool     WriteRegister(uint32_t addr, uint32_t val) = 0;
    virtual uint32_t ReadRegister (uint32_t addr)               = 0;
    virtual size_t   ReadBlock    (uint32_t addr,
                                   uint32_t* dest,
                                   size_t n)                    = 0;
};
// addr = 0x0028, 0x1010, 0x4000...
// connus uniquement dans SIS3315Module.cpp et SIS3316Module.cpp