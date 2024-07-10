//
// Created by Vincent Bode on 09/07/2024.
//

#ifndef FASTDDS_DPDKSENDERRESOURCE_H
#define FASTDDS_DPDKSENDERRESOURCE_H

#include <fastdds/rtps/transport/SenderResource.hpp>
#include "DPDKTransport.h"

namespace eprosima {
    namespace fastdds {
        namespace rtps {


            class DPDKSenderResource : public SenderResource {

            public:
                explicit DPDKSenderResource(DPDKTransport& transport);

            };

        }
    }
}

#endif //FASTDDS_DPDKSENDERRESOURCE_H
