//
// Created by Vincent Bode on 11/07/2024.
//

#include "ddsi_XDPTransport.h"
#include "ddsi_XDPSenderResource.h"

#if defined(__linux) && !LWIP_SOCKET

#include <ifaddrs.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include <bpf/bpf.h>
#include <xdp/xsk.h>
#include <xdp/libxdp.h>
#include <linux/if_ether.h>
#include <sys/resource.h>
#include <net/if.h>
#include <csignal>
#include <fastrtps/utils/IPFinder.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <rte_hash_crc.h>

namespace eprosima {
namespace fastdds {
namespace rtps {

static struct xdp_program *prog;

static inline __u32 xsk_ring_prod__free(struct xsk_ring_prod *r) {
    r->cached_cons = *r->consumer + r->size;
    return r->cached_cons - r->cached_prod;
}

static struct xsk_umem_info *configure_xsk_umem(void *buffer, uint64_t size) {
    struct xsk_umem_info *umem;
    int ret;

    umem = static_cast<xsk_umem_info *>(calloc(1, sizeof(*umem)));
    if (!umem)
        return NULL;

    ret = xsk_umem__create(&umem->umem, buffer, size, &umem->rxFillRing, &umem->txCompletionRing, NULL);
    if (ret) {
        errno = -ret;
        return NULL;
    }

    umem->buffer = buffer;
    return umem;
}

uint64_t ddsi_XDPTransport::xsk_alloc_umem_frame(struct xsk_socket_info *xsk, bool is_tx) {
    umem_free_frame_stack *freeFrameStack = is_tx ? &xsk->umem_frames_tx : &xsk->umem_frames_rx;
    uint64_t frame;
    if (freeFrameStack->umem_frame_free == 0) {
        fprintf(stderr, "XDP UMEM: 1 %s frame allocation FAILED.\n", is_tx ? "TX" : "RX");
        return INVALID_UMEM_FRAME;
    }

    freeFrameStack->umem_frame_free--;
    frame = freeFrameStack->umem_frame_addr[freeFrameStack->umem_frame_free];
    freeFrameStack->umem_frame_addr[freeFrameStack->umem_frame_free] = INVALID_UMEM_FRAME;
//    fprintf(stderr, "XDP UMEM: 1 %s frame allocated: %lu.\n", is_tx?"TX":"RX", frame);
    assert(frame >= 0 && frame < NUM_FRAMES * XDP_L2_FRAME_SIZE);
    return frame;
}

void ddsi_XDPTransport::xsk_free_umem_frame(struct xsk_socket_info *xsk, uint64_t frame, bool is_tx) {
    umem_free_frame_stack *freeFrameStack = is_tx ? &xsk->umem_frames_tx : &xsk->umem_frames_rx;
    assert(freeFrameStack->umem_frame_free <
           sizeof(freeFrameStack->umem_frame_addr) / sizeof(freeFrameStack->umem_frame_addr[0]));

    freeFrameStack->umem_frame_addr[freeFrameStack->umem_frame_free] = frame;
    freeFrameStack->umem_frame_free++;
//    fprintf(stderr, "XDP UMEM: 1 %s frame freed: %lu.\n", is_tx?"TX":"RX", frame);
    assert(frame >= 0 && frame < NUM_FRAMES * XDP_L2_FRAME_SIZE);
}

struct xsk_socket_info *ddsi_XDPTransport::xsk_configure_socket(struct xsk_umem_info *umem) {
    struct xsk_socket_config xsk_cfg;
    struct xsk_socket_info *xsk_info;
    int ret;

    xsk_info = static_cast<xsk_socket_info *>(calloc(1, sizeof(*xsk_info)));
    if (!xsk_info)
        return NULL;

    xsk_info->umem = umem;
    xsk_cfg.rx_size = XSK_RING_CONS__DEFAULT_NUM_DESCS;
    xsk_cfg.tx_size = XSK_RING_PROD__DEFAULT_NUM_DESCS;
    xsk_cfg.xdp_flags = xdp_flags;
    xsk_cfg.bind_flags = xsk_bind_flags;
//    xsk_cfg.libbpf_flags = (custom_xsk) ? XSK_LIBBPF_FLAGS__INHIBIT_PROG_LOAD: 0;
    xsk_cfg.libbpf_flags = XSK_LIBBPF_FLAGS__INHIBIT_PROG_LOAD;
//    xsk_cfg.libbpf_flags = 0;
    ret = xsk_socket__create(&xsk_info->xsk, ifname,
                             xsk_if_queue, umem->umem, &xsk_info->rxCompletionRing,
                             &xsk_info->txFillRing, &xsk_cfg);
    if (ret) {
        return NULL;
    }

//    if (custom_xsk) {
    ret = xsk_socket__update_xskmap(xsk_info->xsk, xsk_map_fd);
    if (ret) {
        return NULL;
    }
//    } else {
//        /* Getting the program ID must be after the xdp_socket__create() call */
//        if (bpf_xdp_query_id(cfg->ifindex, cfg->xdp_flags, &prog_id))
//            goto error_exit;
//    }

    /* Initialize umem frame allocation */
    for (unsigned int i = 0; i < NUM_FRAMES / 2; i++) {
        xsk_info->umem_frames_tx.umem_frame_addr[i] = i * XDP_L2_FRAME_SIZE;
        xsk_info->umem_frames_rx.umem_frame_addr[i] = (i + NUM_FRAMES / 2) * XDP_L2_FRAME_SIZE;
    }

    xsk_info->umem_frames_tx.umem_frame_free = NUM_FRAMES / 2;
    xsk_info->umem_frames_rx.umem_frame_free = NUM_FRAMES / 2;

    /* Stuff the receive path with buffers, we assume we have enough */
    // We need 1 buffer free on the RX path.
    uint32_t initialRXNumAllocacted = XSK_RING_PROD__DEFAULT_NUM_DESCS - 1;

    uint32_t rxFillRingIndex;
    ret = xsk_ring_prod__reserve(&xsk_info->umem->rxFillRing, initialRXNumAllocacted, &rxFillRingIndex);

    if (ret != initialRXNumAllocacted) {
        return NULL;
    }

    for (unsigned int i = 0; i < initialRXNumAllocacted; i++) {
        *xsk_ring_prod__fill_addr(&xsk_info->umem->rxFillRing, rxFillRingIndex) = xsk_alloc_umem_frame(xsk_info, false);
        rxFillRingIndex++;
    }
    xsk_ring_prod__submit(&xsk_info->umem->rxFillRing, initialRXNumAllocacted);

    return xsk_info;
}

bool ddsi_XDPTransport::OpenOutputChannel(SendResourceList &sender_resource_list, const Locator &locator) {
    assert(locator.kind == transport_kind_);
    printf(
            "XDP: Connection %i opened on locator %02x:%02x:%02x:%02x:%02x:%02x port %i\n",
            output_channels_open,
            locator.address[10], locator.address[11], locator.address[12], locator.address[13], locator.address[14], locator.address[15],
            locator.port
    );
    if(output_channels_open == 0) {
        sender_resource_list.push_back(
                std::unique_ptr<ddsi_XDPSenderResource>(new ddsi_XDPSenderResource(*this))
        );
    }
    output_channels_open++;
//    assert(output_channels_open == 1);
    return true;
}


bool ddsi_XDPTransport::OpenInputChannel(const Locator &locator, TransportReceiverInterface *anInterface,
                                         uint32_t maxMessageSize) {
    if (receiverInterface != nullptr) {
        rte_exit(RTE_LOG_ERR, "Already registered a receiver interface.");
    }
    receiverInterface = anInterface;
    incomingDataThread = std::thread([this]() { processIncomingData(); });
    return true;
}

void ddsi_XDPTransport::processIncomingData() {

    struct xsk_socket_info *xsk = xskSocketInfo;
    unsigned int packetsReceived, i;
    uint32_t idx_rx = 0, idx_fq = 0;
    int ret;

    printf("XDP: Read thread started.\n");

    while (true) {

        for (uint8_t tries = 0; tries < 200; tries++) {
            packetsReceived = xsk_ring_cons__peek(&xsk->rxCompletionRing, 1, &idx_rx);
            if (packetsReceived > 0) {
                break;
            }
            if (xsk_ring_prod__needs_wakeup(&xsk->umem->rxFillRing)) {
                recvfrom(xsk_socket__fd(xsk->xsk), NULL, 0, MSG_DONTWAIT, NULL, NULL);
            }
        }

        if (packetsReceived == 0) {
//            std::this_thread::sleep_for(std::chrono::microseconds(500));
            continue;
        }

        /* Stuff the ring with as many frames as possible */
        unsigned int stock_frames = packetsReceived;

        if (stock_frames > 0) {

            ret = xsk_ring_prod__reserve(&xsk->umem->rxFillRing, stock_frames, &idx_fq);

            /* This should not happen, but just in case */
            while (ret != stock_frames) {
                ret = xsk_ring_prod__reserve(&xsk->umem->rxFillRing, packetsReceived, &idx_fq);
            }

            for (i = 0; i < stock_frames; i++) {
                *xsk_ring_prod__fill_addr(&xsk->umem->rxFillRing, idx_fq++) = xsk_alloc_umem_frame(xsk, false);
            }

            xsk_ring_prod__submit(&xsk->umem->rxFillRing, stock_frames);
        }

        /* Process received packets */
        const struct xdp_desc *rxDescriptor = xsk_ring_cons__rx_desc(&xsk->rxCompletionRing, idx_rx);

        struct xdp_l2_packet *packet = (struct xdp_l2_packet *) xsk_umem__get_data(xsk->umem->buffer,
                                                                                   rxDescriptor->addr);

        size_t bytes_received = 0;
        if (ddsi_userspace_l2_is_valid_ethertype(packet->header.h_proto)) {
            Locator srcloc{};
            srcloc.kind = XDP_TRANSPORT_KIND;
            srcloc.port = ddsi_userspace_l2_get_port_for_ethertype(packet->header.h_proto);
            DDSI_USERSPACE_COPY_MAC_ADDRESS_AND_ZERO(srcloc.address, 10, &packet->header.h_source);

            bytes_received = DDSI_USERSPACE_GET_PAYLOAD_SIZE(rxDescriptor->len, struct xdp_l2_packet);

            Locator dstloc{};
            dstloc.kind = XDP_TRANSPORT_KIND;
            dstloc.port = ddsi_userspace_l2_get_port_for_ethertype(packet->header.h_proto);
            DDSI_USERSPACE_COPY_MAC_ADDRESS_AND_ZERO(dstloc.address, 10, &localMacAddress.bytes);

            printf("XDP: Read complete (src %02x:%02x:%02x:%02x:%02x:%02x port %i, %zu bytes: %02x %02x %02x ... %02x %02x %02x, CRC: %x, %lu umems free).\n",
                   packet->header.h_source[0], packet->header.h_source[1], packet->header.h_source[2],
                   packet->header.h_source[3], packet->header.h_source[4], packet->header.h_source[5],
                   srcloc.port, bytes_received,
                   packet->payload[0], packet->payload[1], packet->payload[2], packet->payload[bytes_received-3], packet->payload[bytes_received-2], packet->payload[bytes_received-1],
                   rte_hash_crc(packet->payload, bytes_received, 1337),
                   xsk_umem_free_frames(xsk, false)
            );

            receiverInterface->OnDataReceived(
                    packet->payload,
                    bytes_received,
                    dstloc,
                    srcloc
            );
//            memcpy(buf, packet->payload, bytes_received);

//            std::this_thread::sleep_for(std::chrono::milliseconds(500));

        } else {
            printf("XDP: Frame ethertype %i ignored.\n", packet->header.h_proto);
        }

        xsk_free_umem_frame(xsk, rxDescriptor->addr, false);
        idx_rx++;

        // This signals that we finished processing packetsProcessed (not necessarily == packetsReceived) packets from the
        // rxCompletionRing, freeing up the descriptor slots
        xsk_ring_cons__release(&xsk->rxCompletionRing, 1);

    }
}

// ChatGPT
void get_mac_address(const std::string &interface_name, userspace_l2_mac_addr &mac_addr) {
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("socket");
        return;
    }

    strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) == -1) {
        perror("ioctl");
        close(fd);
        return;
    }

    close(fd);

    memcpy(mac_addr.bytes, ifr.ifr_hwaddr.sa_data, sizeof(userspace_l2_mac_addr));
}

static userspace_l2_mac_addr get_xdp_interface_mac_address(const char *ifname) {
    userspace_l2_mac_addr address;
    get_mac_address(ifname, address);
    return address;

//    int retval = ddsrt_eth_get_mac_addr(ifname, address.bytes);
//    if (retval != DDS_RETCODE_OK) {
//        abort();
//    }
//    return address;
}

bool ddsi_XDPTransport::OpenOutputChannels(SendResourceList &sender_resource_list,
                                           const fastrtps::rtps::LocatorSelectorEntry &locator_selector_entry) {
//    if(!ddsi_userspace_l2_is_valid_port(port)) {
//        DDS_CERROR(&fact->gv->logconfig, "ddsi_dpdk2_l2_create_conn: DDSI requested too large port number %i.", port);
//        return DDS_RETCODE_ERROR;
//    }

//    printf("XDP: Connection opened on port %i\n", locator_selector_entry);
    assert(locator_selector_entry.unicast.size() + locator_selector_entry.multicast.size() == 1);
    Locator selectedLocator;
    if(locator_selector_entry.unicast.empty()) {
        selectedLocator = locator_selector_entry.multicast[0];
    } else {
        selectedLocator = locator_selector_entry.unicast[0];
    }
    ddsi_XDPTransport::OpenOutputChannel(sender_resource_list, selectedLocator);
    return true;
}

//static dds_return_t ddsi_xdp_l2_create_conn (struct ddsi_tran_conn **conn_out, struct ddsi_tran_factory * fact, uint32_t port, const struct ddsi_tran_qos *qos)
//{
////    ddsrt_socket_t sock;
////    dds_return_t rc;
//    ddsi_xdp_l2_conn_t  uc = NULL;
////    struct sockaddr_ll addr;
//    bool mcast = (qos->m_purpose == DDSI_TRAN_QOS_RECV_MC);
//    assert(mcast);
//    struct ddsi_domaingv const * const gv = fact->gv;
//    struct ddsi_network_interface const * const intf = qos->m_interface ? qos->m_interface : &gv->interfaces[0];
//
//    /* If port is zero, need to create dynamic port */
//    // TODO: It looks like raweth uses ethernet type as port number
//    assert(port < UINT16_MAX);
//    if(!ddsi_userspace_l2_is_valid_port(port)) {
//        DDS_CERROR(&fact->gv->logconfig, "ddsi_dpdk2_l2_create_conn: DDSI requested too large port number %i.", port);
//        return DDS_RETCODE_ERROR;
//    }
//
//    if ((uc = (ddsi_xdp_l2_conn_t) ddsrt_malloc (sizeof (*uc))) == NULL)
//    {
////        ddsrt_close(sock);
//        return DDS_RETCODE_ERROR;
//    }
//
//    memset (uc, 0, sizeof (*uc));
////    uc->m_sock = sock;
////    uc->m_ifindex = addr.sll_ifindex;
//    ddsi_factory_conn_init (fact, intf, &uc->m_base);
//    uc->m_base.m_base.m_port = port;
//    uc->m_base.m_base.m_trantype = DDSI_TRAN_CONN;
//    uc->m_base.m_base.m_multicast = mcast;
//    uc->m_base.m_base.m_handle_fn = ddsi_dpdk_l2_conn_handle;
//    uc->m_base.m_locator_fn = ddsi_xdp_l2_conn_locator;
//    uc->m_base.m_read_fn = ddsi_xdp_l2_conn_read;
//    uc->m_base.m_write_fn = ddsi_xdp_l2_conn_write;
//    uc->m_base.m_disable_multiplexing_fn = 0;
//
//    DDS_CTRACE (&fact->gv->logconfig, "ddsi_xdp_l2_create_conn %s socket port %u\n", mcast ? "multicast" : "unicast", uc->m_base.m_base.m_port);
//    *conn_out = &uc->m_base;
//    printf("XDP: Connection opened on port %i\n", port);
//    return DDS_RETCODE_OK;
//}


static void remove_xdp_programs(int ifindex, const char *ifname) {// VB: Remove XDP program
    struct xdp_multiprog *mp = NULL;
    DECLARE_LIBBPF_OPTS(bpf_object_open_opts, opts);
    int err = 0;

    mp = xdp_multiprog__get_from_ifindex(ifindex);
    if (libxdp_get_error(mp)) {
        fprintf(stderr, "XDP: Unable to get xdp_dispatcher program: %s\n", strerror(errno));
        goto out;
    } else if (!mp) {
        fprintf(stderr, "XDP: No XDP program loaded on %s\n", ifname);
        mp = NULL;
        goto out;
    }

    // VB: Unload all == true
    err = xdp_multiprog__detach(mp);
    if (err) {
        fprintf(stderr, "XDP: Unable to detach XDP program: %s\n", strerror(-err));
        goto out;
    }

    out:
    xdp_multiprog__close(mp);
}

void ddsi_XDPTransport::ddsi_xdp_l2_deinit() {
    printf("dpdk l2 de-initialized\n");

    /* Cleanup */
    xsk_socket__delete(xskSocketInfo->xsk);
    if (xskSocketInfo->umem != NULL) {
        xsk_umem__delete(xskSocketInfo->umem->umem);
    }
    remove_xdp_programs(ifindex, ifname);
}

void ddsi_XDPTransport::shutdown() {
    ddsi_xdp_l2_deinit();
    TransportInterface::shutdown();
}


bool ddsi_XDPTransport::init(const fastrtps::rtps::PropertyPolicy *properties, const uint32_t &max_msg_size_no_frag) {
    (void)properties;
    assert(max_msg_size_no_frag <= XDP_MAXIMUM_MESSAGE_SIZE && "XDP: Maximum message size must be 1000 bytes.");

    // XDP setup
    void *packet_buffer;
    uint64_t packet_buffer_size;
    DECLARE_LIBBPF_OPTS(bpf_object_open_opts, opts);
//    DECLARE_LIBXDP_OPTS(xdp_program_opts, xdp_opts, 0);
    struct rlimit rlim = {RLIM_INFINITY, RLIM_INFINITY};
    int err;
    char errmsg[1024];

    ifname = "eno2";
    ifindex = if_nametoindex(ifname);
    xsk_if_queue = 0;

    localMacAddress = get_xdp_interface_mac_address(ifname);
    localLoc = { transport_kind_, 0 };
    DDSI_USERSPACE_COPY_MAC_ADDRESS_AND_ZERO(localLoc.address, 10, &localMacAddress.bytes);

    xdp_flags = 0;
    xsk_bind_flags = XDP_USE_NEED_WAKEUP;


//    /* Load custom program if configured */
//    if (cfg.filename[0] != 0) {
//        struct bpf_map *map;
//
//        custom_xsk = true;
//        xdp_opts.open_filename = cfg.filename;
//        xdp_opts.prog_name = cfg.progname;
//        xdp_opts.opts = &opts;
//
//        if (cfg.progname[0] != 0) {
//            xdp_opts.open_filename = cfg.filename;
//            xdp_opts.prog_name = cfg.progname;
//            xdp_opts.opts = &opts;
//
//            prog = xdp_program__create(&xdp_opts);
//        } else {
//            prog = xdp_program__open_file(cfg.filename, NULL, &opts);
//        }
    const char *xdp_module_path = "./ddsi_xdp_l2_kern.o";
    printf("Assuming XDP module path: %s\n", xdp_module_path);
    prog = xdp_program__open_file(xdp_module_path, NULL, &opts);
    err = libxdp_get_error(prog);
    if (err) {
        libxdp_strerror(err, errmsg, sizeof(errmsg));
        fprintf(stderr, "XDP: error loading program: %s\n", errmsg);
        exit(err);
    }
    fprintf(stderr, "XDP: BPF program loaded.\n");

    // Remove existing programs if we crashed last time
    remove_xdp_programs(ifindex, ifname);

    err = xdp_program__attach(prog, ifindex, XDP_MODE_NATIVE, 0);
    if (err) {
        libxdp_strerror(err, errmsg, sizeof(errmsg));
        fprintf(stderr, "XDP: Couldn't attach XDP program on iface '%s' : %s (%d)\n", ifname, errmsg, err);
        exit(err);
    }
    fprintf(stderr, "XDP: Program attached to %s.\n", ifname);

    /* We also need to load the xsks_map */
    struct bpf_map *map = bpf_object__find_map_by_name(xdp_program__bpf_obj(prog), "xsks_map");
    xsk_map_fd = bpf_map__fd(map);
    if (xsk_map_fd < 0) {
        fprintf(stderr, "ERROR: no xsks map found: %s\n", strerror(xsk_map_fd));
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "XDP: Found xsks_map with file descriptor %i.\n", xsk_map_fd);
//    }

    /* Allow unlimited locking of memory, so all memory needed for packet
     * buffers can be locked.
     */
    if (setrlimit(RLIMIT_MEMLOCK, &rlim)) {
        fprintf(stderr, "ERROR: setrlimit(RLIMIT_MEMLOCK) \"%s\"\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Allocate memory for NUM_FRAMES of the default XDP frame size */
    packet_buffer_size = NUM_FRAMES * XDP_L2_FRAME_SIZE;
    /* PAGE_SIZE aligned */
    if (posix_memalign(&packet_buffer, getpagesize(), packet_buffer_size)) {
        fprintf(stderr, "ERROR: Can't allocate buffer memory \"%s\"\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Initialize shared packet_buffer for umem usage */
    struct xsk_umem_info *umem = configure_xsk_umem(packet_buffer, packet_buffer_size);
    if (umem == NULL) {
        fprintf(stderr, "ERROR: Can't create umem \"%s\"\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Open and configure the AF_XDP (xsk) socket */
    xskSocketInfo = xsk_configure_socket(umem);
    if (xskSocketInfo == NULL) {
        fprintf(stderr, "ERROR: Can't setup AF_XDP socket \"%s\"\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "XDP: Initialization success!\n");
    return true;
}

bool ddsi_XDPTransport::IsInputChannelOpen(const eprosima::fastdds::rtps::Locator &locator) const {
    assert(locator.kind == transport_kind_);
    return receiverInterface != nullptr;
}

bool ddsi_XDPTransport::CloseInputChannel(const Locator &locator) {
    (void)locator;
    return false;
}

TransportDescriptorInterface *ddsi_XDPTransport::get_configuration() {
    return (TransportDescriptorInterface *)transportDescriptor;
}

uint32_t ddsi_XDPTransport::max_recv_buffer_size() const {
//    assert(0);
    return 4096;
}

ddsi_XDPTransport::ddsi_XDPTransport(const ddsi_XDPTransportDescriptor &descriptor)
        : ddsi_l2_transport(XDP_TRANSPORT_KIND), transportDescriptor(&descriptor) {

}

}
}
}

#endif
