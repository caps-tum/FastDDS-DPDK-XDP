//
// Created by Vincent Bode on 09/07/2024.
//

#ifndef FASTDDS_DPDKSENDERRESOURCE_H
#define FASTDDS_DPDKSENDERRESOURCE_H

#include <fastdds/rtps/transport/SenderResource.h>
#include "ddsi_DPDKTransport.h"

namespace eprosima {
    namespace fastdds {
        namespace rtps {


            class ddsi_DPDKSenderResource : public fastrtps::rtps::SenderResource {

            public:
                explicit ddsi_DPDKSenderResource(ddsi_DPDKTransport& transport);

            };

        }
    }
}

#endif //FASTDDS_DPDKSENDERRESOURCE_H
