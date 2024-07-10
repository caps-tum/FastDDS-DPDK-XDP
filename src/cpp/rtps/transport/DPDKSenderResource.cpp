//
// Created by Vincent Bode on 09/07/2024.
//

#include <rte_ethdev.h>
#include "DPDKSenderResource.h"
#include "UserspaceL2Utils.h"
#include "DPDKTransport.h"

namespace eprosima {
namespace fastdds {
namespace rtps {


eprosima::fastdds::rtps::DPDKSenderResource::DPDKSenderResource(DPDKTransport& transport) : SenderResource(DPDK_TRANSPORT_KIND) {
    clean_up = [this](){
        // TODO
    };

    send_buffers_lambda_ = [this, &transport](
            const std::vector<NetworkBuffer>& buffers,
            uint32_t total_bytes,
            LocatorsIterator* destination_locators_begin,
            LocatorsIterator* destination_locators_end,
            const std::chrono::steady_clock::time_point& max_blocking_time_point) -> bool {

        // We only do multicast between two participants so destination locators don't really matter
        // We assume there is only one target
        LocatorsIterator& it = *destination_locators_begin;
        Locator dst = *it;
        ++it;
        assert(it == *destination_locators_end);

        size_t bytes_transferred = 0;
        size_t total_iov_size = total_bytes;

        struct rte_mbuf *buf = rte_pktmbuf_alloc(transport.m_dpdk_memory_pool_tx);
        assert(total_iov_size < UINT16_MAX - sizeof(struct dpdk_l2_packet));
        dpdk_l2_packet_t data_loc = (dpdk_l2_packet_t) rte_pktmbuf_append(
                buf, (uint16_t) sizeof(struct dpdk_l2_packet) + (uint16_t) total_iov_size
        );
        assert(data_loc);
        assert(dst.port < UINT16_MAX);
        data_loc->header.ether_type = ddsi_userspace_l2_get_ethertype_for_port(dst.port);
        // VB: Source address: Current interface mac address. Destination address: Broadcast.
        rte_eth_macaddr_get(0, &data_loc->header.s_addr);
        memset(data_loc->header.d_addr.addr_bytes, 0xFF, sizeof(data_loc->header.d_addr.addr_bytes));

        for(size_t i = 0; i < buffers.size(); i++) {
            memcpy(data_loc->payload + bytes_transferred, buffers[i].buffer, buffers[i].size);
            bytes_transferred += buffers[i].size;
        }

        int transmitted = rte_eth_tx_burst(transport.dpdk_port_identifier, transport.dpdk_queue_identifier, &buf, 1);
        rte_pktmbuf_free(buf);
        if(transmitted == 0) {
            return false;
        }
        else if(transmitted > 1) {
            printf("DPDK: Transferred more than 1 packet after sending 1 packet. Something is really wrong\n");
            abort();
        }

//    printf("DPDK: Write complete (port %i, %zu iovs, %zi bytes: %02x %02x %02x ... %02x %02x %02x, CRC: %x, %i mbufs free).\n",
//           dst->port, niov, bytes_transferred,
//           data_loc->payload[0], data_loc->payload[1], data_loc->payload[2], data_loc->payload[bytes_transferred-3], data_loc->payload[bytes_transferred-2], data_loc->payload[bytes_transferred-1],
//           rte_hash_crc(data_loc->payload, bytes_transferred, 1337),
//           rte_mempool_avail_count(factory->m_dpdk_memory_pool_tx)
//    );

       return true;

    };
}

}
}
}