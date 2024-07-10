//
// Created by Vincent Bode on 08/07/2024.
//

#include "DPDKTransportDescriptor.h"
#include "DPDKTransport.h"

namespace eprosima {
    namespace fastdds {
        namespace rtps {

            DPDKTransportDescriptor::DPDKTransportDescriptor()
                    : PortBasedTransportDescriptor(DPDK_MAXIMUM_MESSAGE_SIZE, DPDK_MAXIMUM_INITIAL_PEERS_RANGE) {}


            TransportInterface *DPDKTransportDescriptor::create_transport() const {
                return new DPDKTransport(*this);
            }

            // TODO
            uint32_t eprosima::fastdds::rtps::DPDKTransportDescriptor::min_send_buffer_size() const {
                return 0;
            }

        }
    }
}