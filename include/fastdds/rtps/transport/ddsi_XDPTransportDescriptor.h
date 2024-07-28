//
// Created by Vincent Bode on 08/07/2024.
//

#ifndef FASTDDS_XDPTRANSPORTDESCRIPTOR_H
#define FASTDDS_XDPTRANSPORTDESCRIPTOR_H

#include <rtps/transport/ddsi_XDPTransport.h>
#include "fastdds/rtps/transport/PortBasedTransportDescriptor.hpp"

namespace eprosima {
    namespace fastdds {
        namespace rtps {

            // Approximately Ethernet MTU
            static constexpr uint32_t XDP_MAXIMUM_MESSAGE_SIZE = 1400;
            static constexpr uint32_t XDP_MAXIMUM_INITIAL_PEERS_RANGE = 10;



            class ddsi_XDPTransportDescriptor : public PortBasedTransportDescriptor {

            public:

                ddsi_XDPTransportDescriptor();

                TransportInterface *create_transport() const override;

                uint32_t min_send_buffer_size() const override;

            };

        }
    }
}

#endif //FASTDDS_XDPTRANSPORTDESCRIPTOR_H
