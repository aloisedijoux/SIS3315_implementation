#include "/home/aloiselkb/sis3315_implementation/sis3315-software/libraries_and_includes/sis3315_header/sis3315.h"
#include "/home/aloiselkb/sis3315_implementation/sis3315-software/libraries_and_includes/sis_vme_master_class_lib/sis3315_ethernet_access_class.h"
#include <cstdint>
#include <stdexcept>
#include <iostream>


int main() {

    unsigned int data;
    int reply;
    sis3315_eth *vme_crate = new sis3315_eth();
    vme_crate->set_UdpSocketOptionBufSize(335544432);
    char local_iface[] = "";
    vme_crate->set_UdpSocketBindMyOwnPort(local_iface);
vme_crate->set_UdpSocketSIS3315_IpAddress((char*)"192.168.0.100");
    vme_crate->udp_reset_cmd();
    vme_crate->vme_A32D32_write(SIS3315_INTERFACE_ACCESS_ARBITRATION_CONTROL, 0x80000000);
    vme_crate->vme_A32D32_write(SIS3315_INTERFACE_ACCESS_ARBITRATION_CONTROL, 0x1);

    reply = vme_crate->vmeopen();
    if (reply != 0) {
        throw std::runtime_error("Failed to open VME connection");
    }

    vme_crate->clear_UdpReceiveBuffer();
    reply = vme_crate->vme_A32D32_read(SIS3315_MODID, &data);
    std::cout << "reply = " << reply << std::endl;
    std::cout << "data  = 0x" << std::hex << data << std::dec << std::endl;
    if ((data & 0xffff0000) != 0x33150000 || reply) {
        std::cerr << "Could not access module" << std::endl;
    } else {
        std::cout << "Module ID: " << std::hex << data << std::dec << std::endl;
    }

    vme_crate->vmeclose();
    delete vme_crate;

    return 0;
}
