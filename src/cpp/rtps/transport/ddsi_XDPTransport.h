//
// Created by Vincent Bode on 11/07/2024.
//

#ifndef FASTRTPS_DDSI_XDPTRANSPORT_H
#define FASTRTPS_DDSI_XDPTRANSPORT_H

#include "ddsi_l2_transport.h"
#include "ddsi_UserspaceL2Utils.h"
#include "fastdds/rtps/transport/ddsi_XDPTransportDescriptor.h"

#include <xdp/xsk.h>
#include <linux/if_ether.h>


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
    unsigned char payload[0];
} *xdp_l2_packet_t;

#define NUM_FRAMES         4096

typedef struct {
    uint64_t umem_frame_addr[NUM_FRAMES / 2];
    uint32_t umem_frame_free;
} umem_free_frame_stack;

struct xsk_socket_info {
    // Documentation on rings: https://www.kernel.org/doc/html/latest/networking/af_xdp.html
    // The UMEM uses two rings: FILL and COMPLETION. Each socket associated with the UMEM must have an RX queue, TX
    // queue or both. Say, that there is a setup with four sockets (all doing TX and RX). Then there will be one FILL
    // ring, one COMPLETION ring, four TX rings and four RX rings.
    struct xsk_ring_cons rxCompletionRing;
    struct xsk_ring_prod txFillRing;
    struct xsk_umem_info *umem;
    struct xsk_socket *xsk;

    umem_free_frame_stack umem_frames_tx;
    umem_free_frame_stack umem_frames_rx;
};

static uint64_t xsk_umem_free_frames(struct xsk_socket_info *xsk, bool is_tx)
{
    umem_free_frame_stack *freeFrameStack = is_tx ? &xsk->umem_frames_tx : &xsk->umem_frames_rx;
    return freeFrameStack->umem_frame_free;
}

class ddsi_XDPTransport : public ddsi_l2_transport {
protected:

    TransportReceiverInterface *receiverInterface = nullptr;
    std::thread incomingDataThread;

    int xsk_map_fd;


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

    void processIncomingData();

    uint64_t xsk_alloc_umem_frame(xsk_socket_info *xsk, bool is_tx);

    void xsk_free_umem_frame(xsk_socket_info *xsk, uint64_t frame, bool is_tx);

    void ddsi_xdp_l2_deinit();

    void shutdown() override;

    bool OpenOutputChannels(SendResourceList &sender_resource_list,
                            const fastrtps::rtps::LocatorSelectorEntry &locator_selector_entry) override;
};

}
}
}

#endif //FASTRTPS_DDSI_XDPTRANSPORT_H
