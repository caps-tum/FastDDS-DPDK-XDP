//
// Created by Vincent Bode on 08/07/2024.
//

#include "fastdds/rtps/transport/ddsi_DPDKTransportDescriptor.h"
#include "ddsi_DPDKTransport.h"

namespace eprosima {
    namespace fastdds {
        namespace rtps {

            ddsi_DPDKTransportDescriptor::ddsi_DPDKTransportDescriptor()
                    : PortBasedTransportDescriptor(DPDK_MAXIMUM_MESSAGE_SIZE, DPDK_MAXIMUM_INITIAL_PEERS_RANGE) {}


            TransportInterface *ddsi_DPDKTransportDescriptor::create_transport() const {
                return new ddsi_DPDKTransport(*this);
            }

            // TODO
            uint32_t eprosima::fastdds::rtps::ddsi_DPDKTransportDescriptor::min_send_buffer_size() const {
                return 0;
            }

        }
    }
}