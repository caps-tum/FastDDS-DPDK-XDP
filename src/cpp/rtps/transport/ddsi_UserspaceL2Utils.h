//
// Created by Vincent Bode on 09/07/2024.
//

#ifndef FASTDDS_USERSPACEL2UTILS_H
#define FASTDDS_USERSPACEL2UTILS_H

#include <cassert>

#define DPDK_TRANSPORT_KIND 64
#define XDP_TRANSPORT_KIND 128

#define DDSI_USERSPACE_L2_ETHER_TYPE_BASE 0xA000
#define DDSI_USERSPACE_L2_ETHER_TYPE_MAX 0xBFFF

typedef struct {
    unsigned char bytes[6];
} userspace_l2_mac_addr;


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

static inline uint16_t ddsi_userspace_get_payload_size__(size_t packetSize, size_t payloadOffset) {
    if(packetSize < payloadOffset || payloadOffset > UINT16_MAX || packetSize > UINT16_MAX) {
        return 0;
    }
    return (uint16_t)(packetSize - payloadOffset);
}
#define DDSI_USERSPACE_GET_PAYLOAD_SIZE(packetSize, type) ddsi_userspace_get_payload_size__(packetSize, offsetof(type, payload))


// MAC address handling
static inline void ddsi_userspace_copy_mac_address_and_zero__(void* dest, size_t offset, void *addr) {
    // Zeros all bytes from dest to dest + offset (exclusive), copies the MAC address to dest + offset.
    // User is responsible for ensuring that there is sufficient space.
    memset(dest, 0, offset);
    memcpy(static_cast<uint8_t*>(dest) + offset, addr, 6);
}
#define DDSI_USERSPACE_COPY_MAC_ADDRESS_AND_ZERO(destArray, offset, macAddr) { \
    static_assert(sizeof(destArray) == offset + sizeof(*macAddr), "Unexpected struct sizes in userspace utils macro."); \
    ddsi_userspace_copy_mac_address_and_zero__(destArray, offset, macAddr); \
}


// Packet utils
static inline uint16_t ddsi_userspace_get_packet_size__(size_t dataSize, size_t payloadOffset) {
    if(dataSize + payloadOffset > UINT16_MAX) {
        return 0;
    }
    return (uint16_t)(dataSize + payloadOffset);
}
#define DDSI_USERSPACE_GET_PACKET_SIZE(dataSize, type) ddsi_userspace_get_packet_size__(dataSize, offsetof(type, payload))


#endif //FASTDDS_USERSPACEL2UTILS_H
