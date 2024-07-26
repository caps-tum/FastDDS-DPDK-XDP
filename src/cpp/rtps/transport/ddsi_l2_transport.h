//
// Created by Vincent Bode on 10/07/2024.
//

#ifndef FASTDDS_DDSI_L2_TRANSPORT_H
#define FASTDDS_DDSI_L2_TRANSPORT_H

#include <fastdds/rtps/attributes/PropertyPolicy.h>
#include <thread>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_eal.h>
#include "fastdds/rtps/transport/ddsi_DPDKTransportDescriptor.h"
#include <fastdds/rtps/transport/TransportInterface.h>
#include "ddsi_UserspaceL2Utils.h"

namespace eprosima {
namespace fastdds {
namespace rtps {


class ddsi_l2_transport : public TransportInterface {

public:
    ddsi_l2_transport(int32_t transportKind);

    bool IsLocatorSupported(const Locator &locator) const override;

    int32_t transport_kind_ = 0x7FFFFFFF;

    bool is_locator_allowed(const Locator &locator) const override;

    /**
     * Converts a given remote locator (that is, a locator referring to a remote
     * destination) to the main local locator whose channel can write to that
     * destination. In this case it will return the local mac address on that port.
     */
    Locator RemoteToMainLocal(const Locator &remote) const override;

    bool DoInputLocatorsMatch(const Locator &locator, const Locator &locator1) const override;

    LocatorList NormalizeLocator(const Locator &locator) override;

    void select_locators(fastrtps::rtps::LocatorSelector &selector) const override;

    bool is_local_locator(const Locator &locator) const override;

    void AddDefaultOutputLocator(LocatorList &defaultList) override;

    static void setLocatorBroadcastAddress(Locator &locator);

    bool getDefaultMetatrafficMulticastLocators(LocatorList &locators,
                                                uint32_t metatraffic_multicast_port) const override;

    bool getDefaultMetatrafficUnicastLocators(LocatorList &locators,
                                              uint32_t metatraffic_unicast_port) const override;

    bool getDefaultUnicastLocators(LocatorList &locators, uint32_t unicast_port) const override;

    bool
    fillMetatrafficMulticastLocator(Locator &locator, uint32_t metatraffic_multicast_port) const override;

    bool fillMetatrafficUnicastLocator(Locator &locator, uint32_t metatraffic_unicast_port) const override;

    bool
    configureInitialPeerLocator(Locator &locator, const fastrtps::rtps::PortParameters &port_params, uint32_t domainId,
                                LocatorList &list) const override;

    bool fillUnicastLocator(Locator &locator, uint32_t well_known_port) const override;


    //TODO: Initialize
    Locator localLoc;
    userspace_l2_mac_addr localMacAddress;
protected:

};

}
}
}

#endif //FASTDDS_DDSI_L2_TRANSPORT_H
