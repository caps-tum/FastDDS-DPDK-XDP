//
// Created by Vincent Bode on 08/07/2024.
//

#ifndef FASTDDS_DPDKTRANSPORTDESCRIPTOR_H
#define FASTDDS_DPDKTRANSPORTDESCRIPTOR_H

#include "fastdds/rtps/transport/TransportDescriptorInterface.hpp"
#include "fastdds/rtps/transport/PortBasedTransportDescriptor.hpp"

namespace eprosima {
    namespace fastdds {
        namespace rtps {

            // TODO
            static constexpr uint32_t XDP_MAXIMUM_MESSAGE_SIZE = 1000;
            static constexpr uint32_t XDP_MAXIMUM_INITIAL_PEERS_RANGE = 1;



            class ddsi_XDPTransportDescriptor : public PortBasedTransportDescriptor {

            public:

                ddsi_XDPTransportDescriptor();

                TransportInterface *create_transport() const override;

                uint32_t min_send_buffer_size() const override;

            };

        }
    }
}

#endif //FASTDDS_DPDKTRANSPORTDESCRIPTOR_H
