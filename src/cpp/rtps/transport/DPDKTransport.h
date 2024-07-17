//
// Created by Vincent Bode on 08/07/2024.
//

#ifndef FASTDDS_DPDKTRANSPORT_H
#define FASTDDS_DPDKTRANSPORT_H

#include <fastdds/rtps/transport/TransportInterface.h>
#include "fastdds/rtps/transport/DPDKTransportDescriptor.h"
#include "ddsi_l2_transport.h"
#include "ddsi_UserspaceL2Utils.h"
#include <rte_eal.h>
#include <rte_mbuf.h>
#include <rte_ethdev.h>
#include <rte_version.h>
#include <thread>
#include <fastdds/rtps/attributes/PropertyPolicy.h>


// DPDK renamed some fields :/
#if RTE_VER_YEAR == 23 && RTE_VER_MONTH == 11
#define RTE_SRC_ADDR src_addr
#define RTE_DST_ADDR dst_addr
#else
#define RTE_SRC_ADDR s_addr
#define RTE_DST_ADDR d_addr
#endif


namespace eprosima {
namespace fastdds {
namespace rtps {

typedef struct dpdk_l2_packet {
    // An over-the-wire packet, consisting of an ethernet header and the payload.
    struct rte_ether_hdr header;
    unsigned char payload[0];
} *dpdk_l2_packet_t;

class DPDKTransport : public ddsi_l2_transport {

protected:

    struct rte_mempool *m_dpdk_memory_pool_rx;

    TransportReceiverInterface* receiverInterface = nullptr;
    std::thread incomingDataThread;

public:

    explicit DPDKTransport(const DPDKTransportDescriptor &descriptor);

    bool init(const fastrtps::rtps::PropertyPolicy *properties, const uint32_t &max_msg_size_no_frag) override;

    bool IsInputChannelOpen(const Locator &locator) const override;

    bool OpenOutputChannel(SendResourceList &sender_resource_list, const Locator &locator) override;

    bool OpenInputChannel(const Locator &locator, TransportReceiverInterface *anInterface,
                          uint32_t maxMessageSize) override;

    bool CloseInputChannel(const Locator &locator) override;

    TransportDescriptorInterface *get_configuration() override;

    uint32_t max_recv_buffer_size() const override;

    int dpdk_port_init(uint16_t port, rte_mempool *rx_mbuf_pool);

    int32_t transport_kind_ = DPDK_TRANSPORT_KIND;

    struct rte_mempool *m_dpdk_memory_pool_tx;
    uint16_t dpdk_port_identifier;
    uint16_t dpdk_queue_identifier = 0;

    void processIncomingData();

    const DPDKTransportDescriptor *transportDescriptor;
};


}
}
}

#endif //FASTDDS_DPDKTRANSPORT_H
