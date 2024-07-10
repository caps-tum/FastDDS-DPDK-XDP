//
// Created by Vincent Bode on 08/07/2024.
//

#ifndef FASTDDS_DPDKTRANSPORT_H
#define FASTDDS_DPDKTRANSPORT_H

#include <fastdds/rtps/transport/TransportInterface.hpp>
#include "DPDKTransportDescriptor.h"
#include <rte_eal.h>
#include <rte_mbuf.h>
#include <rte_ethdev.h>
#include <thread>

namespace eprosima {
namespace fastdds {
namespace rtps {

typedef struct dpdk_l2_packet {
    // An over-the-wire packet, consisting of an ethernet header and the payload.
    struct rte_ether_hdr header;
    unsigned char payload[0];
} *dpdk_l2_packet_t;

class DPDKTransport : public TransportInterface {

protected:
    int32_t transport_kind_ = 0x64;

    struct rte_mempool *m_dpdk_memory_pool_rx;

    //TODO: Initialize
    uint8_t localMacAddress[6];
    Locator localLoc;

    TransportReceiverInterface* receiverInterface = nullptr;
    std::thread incomingDataThread;

public:

    explicit DPDKTransport(const DPDKTransportDescriptor &descriptor);

    bool init(const PropertyPolicy *properties, const uint32_t &max_msg_size_no_frag) override;

    bool IsInputChannelOpen(const Locator &locator) const override;

    bool IsLocatorSupported(const Locator &locator) const override;

    bool is_locator_allowed(const Locator &locator) const override;

    Locator RemoteToMainLocal(const Locator &remote) const override;

    bool OpenOutputChannel(SendResourceList &sender_resource_list, const Locator &locator) override;

    bool OpenInputChannel(const Locator &locator, TransportReceiverInterface *anInterface,
                          uint32_t maxMessageSize) override;

    bool CloseInputChannel(const Locator &locator) override;

    bool DoInputLocatorsMatch(const Locator &locator, const Locator &locator1) const override;

    LocatorList NormalizeLocator(const Locator &locator) override;

    void select_locators(LocatorSelector &selector) const override;

    bool is_local_locator(const Locator &locator) const override;

    TransportDescriptorInterface *get_configuration() override;

    void AddDefaultOutputLocator(LocatorList &defaultList) override;

    bool getDefaultMetatrafficMulticastLocators(LocatorList &locators,
                                                uint32_t metatraffic_multicast_port) const override;

    bool getDefaultMetatrafficUnicastLocators(LocatorList &locators,
                                              uint32_t metatraffic_unicast_port) const override;

    bool getDefaultUnicastLocators(LocatorList &locators, uint32_t unicast_port) const override;

    bool
    fillMetatrafficMulticastLocator(Locator &locator, uint32_t metatraffic_multicast_port) const override;

    bool fillMetatrafficUnicastLocator(Locator &locator, uint32_t metatraffic_unicast_port) const override;

    bool configureInitialPeerLocator(Locator &locator, const PortParameters &port_params, uint32_t domainId,
                                     LocatorList &list) const override;

    bool fillUnicastLocator(Locator &locator, uint32_t well_known_port) const override;

    uint32_t max_recv_buffer_size() const override;

    int dpdk_port_init(uint16_t port, rte_mempool *rx_mbuf_pool);

    static void setLocatorBroadcastAddress(Locator &locator);

    struct rte_mempool *m_dpdk_memory_pool_tx;
    uint16_t dpdk_port_identifier;
    uint16_t dpdk_queue_identifier = 0;

    void processIncomingData();
};


}
}
}

#endif //FASTDDS_DPDKTRANSPORT_H
