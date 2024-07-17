//
// Created by Vincent Bode on 08/07/2024.
//

#include "DPDKTransport.h"
#include "DPDKTransportDescriptor.h"
#include "DPDKSenderResource.h"
#include "ddsi_UserspaceL2Utils.h"
#include "ddsi_l2_transport.h"
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_hash_crc.h>
#include <cstdio>
#include <cstring>
#include <memory>
#include <thread>
#include <fastdds/rtps/attributes/PropertyPolicy.h>

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024


namespace eprosima {
namespace fastdds {
namespace rtps {

DPDKTransport::DPDKTransport(const DPDKTransportDescriptor &descriptor) : ddsi_l2_transport(DPDK_TRANSPORT_KIND) {
    transportDescriptor = &descriptor;
}

// Implemented with help from here: https://doc.dpdk.org/guides/sample_app_ug/skeleton.html
int DPDKTransport::dpdk_port_init(uint16_t port, struct rte_mempool *rx_mbuf_pool) {
    struct rte_eth_conf port_conf{};
    const uint16_t rx_rings = 1, tx_rings = 1;
    uint16_t nb_rxd = RX_RING_SIZE;
    uint16_t nb_txd = TX_RING_SIZE;
    int retval;
    uint16_t q;
    struct rte_eth_dev_info dev_info{};

    if (!rte_eth_dev_is_valid_port(port))
        return -1;

    memset(&port_conf, 0, sizeof(struct rte_eth_conf));

    retval = rte_eth_dev_info_get(port, &dev_info);
    if (retval != 0) {
        printf("DPDK: Error during getting device (port %u) info: %s\n", port, strerror(-retval));
        return retval;
    }

    // TODO: Unsupported on old DPDK
//        if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)
//            port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;

    /* Configure the Ethernet device. */
    retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
    if (retval != 0) {
        return retval;
    }

    retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
    if (retval != 0) {
        return retval;
    }

    struct rte_eth_rxconf rxconf = dev_info.default_rxconf;
    rxconf.offloads = port_conf.rxmode.offloads;

    /* Allocate and set up 1 RX queue per Ethernet port. */
    for (q = 0; q < rx_rings; q++) {
        retval = rte_eth_rx_queue_setup(port, q, nb_rxd, (unsigned int) rte_eth_dev_socket_id(port),
                                        &rxconf, rx_mbuf_pool);
        if (retval < 0) {
            return retval;
        }
    }

    struct rte_eth_txconf txconf = dev_info.default_txconf;
    txconf.offloads = port_conf.txmode.offloads;
    /* Allocate and set up 1 TX queue per Ethernet port. */
    for (q = 0; q < tx_rings; q++) {
        retval = rte_eth_tx_queue_setup(port, q, nb_txd, (unsigned int) rte_eth_dev_socket_id(port),
                                        &txconf);
        if (retval < 0) {
            return retval;
        }
    }

    /* Starting Ethernet port. 8< */
    retval = rte_eth_dev_start(port);
    /* >8 End of starting of ethernet port. */
    if (retval < 0)
        return retval;

    /* Enable RX in promiscuous mode for the Ethernet device. */
//    retval = rte_eth_promiscuous_enable(port);
    /* End of setting RX port in promiscuous mode. */
//    if (retval != 0)
//        return retval;

//    retval = rte_eth_dev_set_ptypes(port, RTE_PTYPE_UNKNOWN, NULL, 0);
//    if (retval < 0) {
//        return retval;
//    }
    return 0;
}


bool DPDKTransport::init(const fastrtps::rtps::PropertyPolicy *properties, const uint32_t &max_msg_size_no_frag) {


    // We need to initialize EAL
    // TODO: This expects argc and argv
    char *arg0 = "";
    char *pseudoArgs[] = {arg0, NULL};
    int ret = rte_eal_init(0, pseudoArgs);
    if (ret != 0) {
        printf("Unable to initialize DPDK RTE_EAL. Please check the RTE log messages above for errors.");
        return false;
    }
    printf("RTE EAL init success.\n");

    // TX buffers
    m_dpdk_memory_pool_tx = rte_pktmbuf_pool_create(
            "MBUF_POOL_TX", NUM_MBUFS, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, (int) rte_socket_id()
    );
    if (m_dpdk_memory_pool_tx == NULL) {
        printf("Failed to allocate DPDK TX mempool.  Please check the RTE log messages above for errors.");
        return false;
    }
    // RX buffers
    m_dpdk_memory_pool_rx = rte_pktmbuf_pool_create(
            "MBUF_POOL_RX", NUM_MBUFS, MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, (int) rte_socket_id()
    );
    if (m_dpdk_memory_pool_rx == NULL) {
        printf("Failed to allocate DPDK RX mempool.  Please check the RTE log messages above for errors.");
        return false;
    }

    if (dpdk_port_init(dpdk_port_identifier, m_dpdk_memory_pool_rx) != 0) {
        rte_exit(EXIT_FAILURE, "Cannot init port %"
                               PRIu16
                               "\n", dpdk_port_identifier);
    }


    // TODO: max_msg_size_no_frag?
    return true;
}

bool DPDKTransport::IsInputChannelOpen(const eprosima::fastdds::rtps::Locator &locator) const {
    assert(locator.kind == transport_kind_);
    return true;
}

bool DPDKTransport::OpenOutputChannel(SendResourceList &sender_resource_list, const Locator &locator) {
    sender_resource_list.push_back(
            std::unique_ptr<DPDKSenderResource>(new DPDKSenderResource(*this))
    );
    return true;
}

bool DPDKTransport::OpenInputChannel(const Locator &locator, TransportReceiverInterface *anInterface,
                                     uint32_t maxMessageSize) {
    if (receiverInterface != nullptr) {
        rte_exit(RTE_LOG_ERR, "Already registered a receiver interface.");
    }
    receiverInterface = anInterface;
    incomingDataThread = std::thread([this]() { processIncomingData(); });
    return true;
}

static uint16_t calculate_payload_size(struct rte_mbuf *const buf) {
    // Get the length of the actual payload excluding the space required for the header.
    if (buf->data_len == 0) {
        return 0;
    }
    assert(buf->data_len > offsetof(struct dpdk_l2_packet, payload));
    static_assert(offsetof(struct dpdk_l2_packet, payload) < UINT16_MAX, "Packet header too large.");
    return buf->data_len - (uint16_t) offsetof(struct dpdk_l2_packet, payload);
}


void DPDKTransport::processIncomingData() {
    /* Get burst of RX packets, from first port of pair. */
    struct rte_mbuf *mbuf[1];
    uint16_t number_received;
    ssize_t bytes_received;
    uint8_t tries = 0;
    while (true) {
        // TODO VB: Num packets should be divisible by eight for any driver to work.
        number_received = rte_eth_rx_burst(
                dpdk_port_identifier, 0, mbuf, 1
        );
        // Different to original
        if (number_received == 0) {
            tries++;
            if (tries >= 250) {
                // TODO: This introduces latency, test it.
                rte_delay_us_block(500);
//            printf("Read: TRYAGAIN (%i bufs available)\n", rte_mempool_avail_count(mempool));
            }

            continue;
        }

        tries = 0;

        dpdk_l2_packet_t packet = rte_pktmbuf_mtod(mbuf[0], dpdk_l2_packet_t);
        uint16_t payload_size = calculate_payload_size(mbuf[0]);

        Locator srcloc{};
        srcloc.kind = transport_kind_;
        srcloc.port = ddsi_userspace_l2_get_port_for_ethertype(packet->header.ether_type);
        DDSI_USERSPACE_COPY_MAC_ADDRESS_AND_ZERO(srcloc.address, 10, &packet->header.RTE_SRC_ADDR.addr_bytes);

        receiverInterface->OnDataReceived(
                (unsigned char *) packet->payload,
                payload_size,
                localLoc,
                srcloc
        );

//        printf("DPDK: Read complete (port %i, %zi bytes: %02x %02x %02x ... %02x %02x %02x, CRC: %x, %i mbufs free).\n",
//               srcloc->port, bytes_received,
//               buf[0], buf[1], buf[2], buf[bytes_received-3], buf[bytes_received-2], buf[bytes_received-1],
//               rte_hash_crc(packet->payload, bytes_received, 1337),
//               rte_mempool_avail_count(m_factory->m_dpdk_memory_pool_rx)
//        );

        // Packet is only allocated if it was successfully received.
        rte_pktmbuf_free(mbuf[0]);
    }
}

bool DPDKTransport::CloseInputChannel(const Locator &locator) {
    return false;
}

TransportDescriptorInterface *DPDKTransport::get_configuration() {
    return (TransportDescriptorInterface *)transportDescriptor;
}

uint32_t DPDKTransport::max_recv_buffer_size() const {
    return 0;
}

}
}
}
