/* SPDX-License-Identifier: GPL-2.0 */

#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <bpf/bpf_helpers.h>
#include <stdint.h>

#define DDSI_USERSPACE_L2_ETHER_TYPE_BASE 0xA000
#define DDSI_USERSPACE_L2_ETHER_TYPE_MAX 0xBFFF

static inline uint16_t ddsi_userspace_l2_is_valid_ethertype(uint16_t ethertype) {
    return ethertype >= DDSI_USERSPACE_L2_ETHER_TYPE_BASE && ethertype <= DDSI_USERSPACE_L2_ETHER_TYPE_MAX;
}


struct {
	__uint(type, BPF_MAP_TYPE_XSKMAP);
	__type(key, __u32);
	__type(value, __u32);
	__uint(max_entries, 64);
} xsks_map SEC(".maps");

//struct {
//	__uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
//	__type(key, __u32);
//	__type(value, __u32);
//	__uint(max_entries, 64);
//} xdp_stats_map SEC(".maps");

SEC("xdp")
int xdp_sock_prog(struct xdp_md *ctx)
{
//    int index = ctx->rx_queue_index;
    int index = 0;

    if(ctx->data + sizeof(struct ethhdr) >= ctx->data_end) {
        return XDP_PASS;
    }

    struct ethhdr* header = (struct ethhdr*) ctx->data;

    if(ddsi_userspace_l2_is_valid_ethertype(header->h_proto)) {
        /* A set entry here means that the correspnding queue_id
         * has an active AF_XDP socket bound to it. */
        if (bpf_map_lookup_elem(&xsks_map, &index)) {
            return bpf_redirect_map(&xsks_map, index, 0);
        }
    }

    return XDP_PASS;
}

char _license[] SEC("license") = "GPL"; // NOLINT(bugprone-reserved-identifier)
