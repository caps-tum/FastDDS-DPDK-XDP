//
// Created by Vincent Bode on 09/07/2024.
//

#ifndef FASTDDS_DDSIXDPSENDERRESOURCE_H
#define FASTDDS_DDSIXDPSENDERRESOURCE_H

#include <fastdds/rtps/transport/SenderResource.h>
#include "ddsi_XDPTransport.h"

namespace eprosima {
    namespace fastdds {
        namespace rtps {


            class ddsi_XDPSenderResource : public fastrtps::rtps::SenderResource {

            public:
                explicit ddsi_XDPSenderResource(ddsi_XDPTransport& transport);

                void add_locators_to_list(fastrtps::rtps::LocatorList_t &locators) const override;

            private:
                ddsi_XDPTransport* transport_;

            };

        }
    }
}

#endif //FASTDDS_DDSIXDPSENDERRESOURCE_H
