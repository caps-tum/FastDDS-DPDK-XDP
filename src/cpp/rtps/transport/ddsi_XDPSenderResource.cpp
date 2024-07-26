//
// Created by Vincent Bode on 09/07/2024.
//

#include <rte_ethdev.h>
#include <rte_hash_crc.h>
#include "ddsi_XDPSenderResource.h"
#include "ddsi_UserspaceL2Utils.h"
#include "ddsi_XDPTransport.h"

namespace eprosima {
namespace fastdds {
namespace rtps {


eprosima::fastdds::rtps::ddsi_XDPSenderResource::ddsi_XDPSenderResource(ddsi_XDPTransport& transport) : SenderResource(XDP_TRANSPORT_KIND) {
    transport_ = &transport;
    clean_up = [this](){
        // TODO
    };

    send_lambda_ = [this, &transport](
            const uint8_t* data,
            uint32_t total_bytes,
            LocatorsIterator* destination_locators_begin,
            LocatorsIterator* destination_locators_end,
            const std::chrono::steady_clock::time_point& max_blocking_time_point) -> bool {

//        printf("XDP: Write start.\n");


        // We only do multicast between two participants so destination locators don't really matter
        // We assume there is only one target
        LocatorsIterator& it = *destination_locators_begin;
        Locator dst = *it;
        ++it;
        assert(it == *destination_locators_end);


        struct xsk_socket_info *xsk = transport.xskSocketInfo;
        static int pendingTransmits = 0;

        /* Here we sent the packet out of the receive port. Note that
     * we allocate one entry and schedule it. Your design would be
     * faster if you do batch processing/transmission */

        uint64_t frame = transport.xsk_alloc_umem_frame(xsk, true);
        if (frame == INVALID_UMEM_FRAME) {
            assert(0);
            return false;
        }

        uint32_t tx_idx = 0;
        uint32_t ret = xsk_ring_prod__reserve(&xsk->txFillRing, 1, &tx_idx);
        if (ret != 1) {
            /* No more transmit slots, drop the packet */
            transport.xsk_free_umem_frame(xsk, frame, true);
            return false;
        }

        xdp_l2_packet_t frame_buffer = static_cast<xdp_l2_packet_t>(xsk_umem__get_data(xsk->umem->buffer, frame));

        // Fill the ethernet header
        assert(dst.port < UINT16_MAX);
        frame_buffer->header.h_proto = ddsi_userspace_l2_get_ethertype_for_port((uint16_t) dst.port);
        assert(ddsi_userspace_l2_is_valid_ethertype(frame_buffer->header.h_proto));

//        static_assert(sizeof(dst.address) == 16 && sizeof(frame_buffer->header.h_dest) == 6, "Copy buffer sizes incorrect.");
//        memcpy(frame_buffer->header.h_dest, &dst.address[10], sizeof(frame_buffer->header.h_dest));
        // We ignore supposed destination and send to broadcast address
        memset(frame_buffer->header.h_dest, 0xFF, sizeof(frame_buffer->header.h_dest));

        static_assert(sizeof(frame_buffer->header.h_source) == sizeof(transport.localMacAddress), "Unexpected MAC address buffer sizes.");
        memcpy(frame_buffer->header.h_source, &transport.localMacAddress, sizeof(frame_buffer->header.h_source));

        // Fill the data
        if(total_bytes >= XDP_L2_FRAME_DATA_SIZE) {
            printf("XDP: Message too big to handle.\n");
            return false;
        }
        memcpy(&frame_buffer->payload, data, total_bytes);
//        size_t data_copied = ddsi_userspace_copy_iov_to_packet(niov, iov, &frame_buffer->payload, XDP_L2_FRAME_DATA_SIZE);
//        if (data_copied == 0) {
//            transport.xsk_free_umem_frame(xsk, frame, true);
//            return false;
//        }

        // Create the TX Descriptor and actually send the message
        struct xdp_desc *txDescriptor = xsk_ring_prod__tx_desc(&xsk->txFillRing, tx_idx);
        txDescriptor->addr = frame;
        txDescriptor->len = DDSI_USERSPACE_GET_PACKET_SIZE(total_bytes, struct xdp_l2_packet);
        xsk_ring_prod__submit(&xsk->txFillRing, 1);
        pendingTransmits++;

        // We don't actually send anything here. This is just to notify the kernel.
        // Therefore, should have 0 bytes transferred.
        if (xsk_ring_prod__needs_wakeup(&xsk->txFillRing)) {
            sendto(xsk_socket__fd(xsk->xsk), NULL, 0, MSG_DONTWAIT, NULL, 0);
        }

    printf("XDP: Write complete (dest %02x:%02x:%02x:%02x:%02x:%02x port %i, %u bytes: %02x %02x %02x ... %02x %02x %02x, CRC: %x, %lu umems free, %i pending).\n",
           frame_buffer->header.h_dest[0], frame_buffer->header.h_dest[1], frame_buffer->header.h_dest[2],
           frame_buffer->header.h_dest[3], frame_buffer->header.h_dest[4], frame_buffer->header.h_dest[5],
           dst.port, total_bytes,
           frame_buffer->payload[0], frame_buffer->payload[1], frame_buffer->payload[2],
           frame_buffer->payload[total_bytes-3], frame_buffer->payload[total_bytes-2], frame_buffer->payload[total_bytes-1],
           rte_hash_crc(frame_buffer->payload, total_bytes, 1337),
           xsk_umem_free_frames(xsk, true),
           pendingTransmits
    );
//        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        /* Collect/free completed TX buffers */
        uint32_t indexTXCompletionRing;
        unsigned int completed = xsk_ring_cons__peek(
                &xsk->umem->txCompletionRing, XSK_RING_CONS__DEFAULT_NUM_DESCS, &indexTXCompletionRing
        );

        if (completed > 0) {
            for (unsigned int i = 0; i < completed; i++) {
                transport.xsk_free_umem_frame(
                        xsk,
                        *xsk_ring_cons__comp_addr(&xsk->umem->txCompletionRing, indexTXCompletionRing),
                        true
                );
                indexTXCompletionRing++;
            }

            xsk_ring_cons__release(&xsk->umem->txCompletionRing, completed);
            pendingTransmits -= completed;
        }

        return true;

    };
}

void ddsi_XDPSenderResource::add_locators_to_list(fastrtps::rtps::LocatorList_t &locators) const {
    std::cout << "XDPSenderResource: Add locators to list: " << transport_->localLoc << std::endl;
    locators.push_back(transport_->localLoc);
}

}
}
}