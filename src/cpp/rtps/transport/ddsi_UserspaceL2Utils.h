//
// Created by Vincent Bode on 09/07/2024.
//

#ifndef FASTDDS_USERSPACEL2UTILS_H
#define FASTDDS_USERSPACEL2UTILS_H

#include <cassert>

#define DPDK_TRANSPORT_KIND 64
#define DPDK_TRANSPORT_KIND 128

#define DDSI_USERSPACE_L2_ETHER_TYPE_BASE 0xA000
#define DDSI_USERSPACE_L2_ETHER_TYPE_MAX 0xBFFF


static inline uint16_t ddsi_userspace_l2_get_ethertype_for_port(uint16_t port) {
    assert(port < DDSI_USERSPACE_L2_ETHER_TYPE_MAX);
    return port + DDSI_USERSPACE_L2_ETHER_TYPE_BASE;
}

static inline uint16_t ddsi_userspace_l2_is_valid_ethertype(uint16_t ethertype) {
    return ethertype >= DDSI_USERSPACE_L2_ETHER_TYPE_BASE && ethertype <= DDSI_USERSPACE_L2_ETHER_TYPE_MAX;
}


static inline uint16_t ddsi_userspace_l2_get_port_for_ethertype(uint16_t ethertype) {
    assert(ddsi_userspace_l2_is_valid_ethertype(ethertype));
    return ethertype - DDSI_USERSPACE_L2_ETHER_TYPE_BASE;
}



#endif //FASTDDS_USERSPACEL2UTILS_H
