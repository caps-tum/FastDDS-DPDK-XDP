//
// Created by Vincent Bode on 11/07/2024.
//

#ifndef FASTRTPS_DDSI_XDPTRANSPORT_H
#define FASTRTPS_DDSI_XDPTRANSPORT_H

#include "ddsi_l2_transport.h"
#include "ddsi_UserspaceL2Utils.h"
#include "ddsi_XDPTransportDescriptor.h"


#define XDP_L2_FRAME_SIZE XSK_UMEM__DEFAULT_FRAME_SIZE
#define XDP_L2_FRAME_DATA_SIZE (XDP_L2_FRAME_SIZE - offsetof(struct xdp_l2_packet, payload))
#define RX_BATCH_SIZE      64
#define INVALID_UMEM_FRAME UINT64_MAX


namespace eprosima {
namespace fastdds {
namespace rtps {

struct xsk_umem_info {
    struct xsk_ring_prod rxFillRing;
    struct xsk_ring_cons txCompletionRing;
    struct xsk_umem *umem;
    // Actual data storage pointer, use with xsk_umem__get_data
    void *buffer;
};

typedef struct xdp_l2_packet {
    // An over-the-wire packet, consisting of an ethernet header and the payload.
    struct ethhdr header;
    char payload[0];
} *xdp_l2_packet_t;



class ddsi_XDPTransport : ddsi_l2_transport {
protected:

    TransportReceiverInterface *receiverInterface = nullptr;
    std::thread incomingDataThread;

public:

    explicit ddsi_XDPTransport(const ddsi_XDPTransportDescriptor &descriptor);

    bool init(const fastrtps::rtps::PropertyPolicy *properties, const uint32_t &max_msg_size_no_frag) override;

    bool IsInputChannelOpen(const Locator &locator) const override;

    bool OpenOutputChannel(SendResourceList &sender_resource_list, const Locator &locator) override;

    bool OpenInputChannel(const Locator &locator, TransportReceiverInterface *anInterface,
                          uint32_t maxMessageSize) override;

    bool CloseInputChannel(const Locator &locator) override;

    TransportDescriptorInterface *get_configuration() override;

    uint32_t max_recv_buffer_size() const override;

    int32_t transport_kind_ = XDP_TRANSPORT_KIND;

    const ddsi_XDPTransportDescriptor *transportDescriptor;

    const char *ifname;
    unsigned int ifindex;
    unsigned int xsk_if_queue;
    unsigned int xdp_flags;
    unsigned int xsk_bind_flags;

    struct xsk_socket_info *xskSocketInfo;

    xsk_socket_info *xsk_configure_socket(eprosima::fastdds::rtps::xsk_umem_info *umem);

    void processIncomingData() const;

    uint64_t xsk_alloc_umem_frame(xsk_socket_info *xsk, bool is_tx);

    void xsk_free_umem_frame(xsk_socket_info *xsk, uint64_t frame, bool is_tx);
};

}
}
}

#endif //FASTRTPS_DDSI_XDPTRANSPORT_H
