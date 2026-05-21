#include "min_acq.h"

void SIS3315Min::Reset() {
    /*
    5.69.1 Key address: Register Reset
#define SIS3315_KEY_RESET
0x400
  write only; D32 
A write with arbitrary data to this register (key address) resets the SIS3315 registers to their
power up state.
    */

    int rc;
    rc = register_write(baseAddress_ + SIS3315_KEY_RESET, 0); // on écrit n'importe quelle valeur dans le registre de reset pour réinitialiser les registres du module
}

unsigned int SIS3315Min::ReadBaseAddress() {
    /*
    l'adresse de base est encodée sur la carte via deux rotary switches de aleur hex 0-F/ ils encodent les bits 31-24 de l'addresse de base A32 (Adresse = (SW1 << 28) | (SW0 << 24)).
par défaut, SW1 = 4 et SW2 = 1. on peut lire physiquement sur la cate mais ici on va écrire une méthode qui scanne sur le bus vme  
Pourquoi crate_ plutôt que register_read : register_read dans sis3315_adc utilise le baseaddress fixé à la construction — impossible de scanner d'autres adresses. Il faut donc passer directement par crate_->vme_A32D32_read(addr_absolue, ...).
  */

    // Les deux rotary switches encodent les nibbles hauts de l'adresse A32 :
    // addr = (SW1 << 28) | (SW0 << 24) => 256 candidats espacés de 0x01000000.
    for (unsigned int addr = 0; addr <= 0xFF000000u; addr += 0x01000000u) {
        unsigned int data = 0;
        int rc = crate_->vme_A32D32_read(addr + SIS3315_MODID, &data);
        if (rc == 0 && (data & 0xffff0000) == 0x33150000) {
            return addr;
        }
    }
    return 0; // module non trouvé
}