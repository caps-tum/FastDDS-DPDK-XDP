//
// Created by Vincent Bode on 08/07/2024.
//

#include "fastdds/rtps/transport/ddsi_XDPTransportDescriptor.h"
#include "ddsi_XDPTransport.h"

namespace eprosima {
    namespace fastdds {
        namespace rtps {

            ddsi_XDPTransportDescriptor::ddsi_XDPTransportDescriptor()
                    : PortBasedTransportDescriptor(XDP_MAXIMUM_MESSAGE_SIZE, XDP_MAXIMUM_INITIAL_PEERS_RANGE) {}


            TransportInterface *ddsi_XDPTransportDescriptor::create_transport() const {
                return new ddsi_XDPTransport(*this);
            }

            // TODO
            uint32_t ddsi_XDPTransportDescriptor::min_send_buffer_size() const {
                return 0;
            }

        }
    }
}