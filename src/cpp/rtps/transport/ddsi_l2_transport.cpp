//
// Created by Vincent Bode on 10/07/2024.
//

#include <fastdds/rtps/attributes/PropertyPolicy.h>
#include <thread>
#include <memory>
#include <cstring>
#include <cstdio>
#include <rte_hash_crc.h>
#include <rte_ethdev.h>
#include <rte_eal.h>
#include "ddsi_UserspaceL2Utils.h"
#include "DPDKSenderResource.h"
#include "DPDKTransportDescriptor.h"
#include "DPDKTransport.h"
#include "ddsi_l2_transport.h"

bool eprosima::fastdds::rtps::ddsi_l2_transport::IsLocatorSupported(const Locator &locator) const {
    return locator.kind == transport_kind_;
}

bool eprosima::fastdds::rtps::ddsi_l2_transport::is_locator_allowed(const Locator &locator) const {
    return IsLocatorSupported(locator);
}

eprosima::fastdds::rtps::Locator
eprosima::fastdds::rtps::ddsi_l2_transport::RemoteToMainLocal(const Locator &remote) const {
    // We always send from our interface
//                Locator locator = {
//                    transport_kind_, remote.port
//                };
//                setLocatorBroadcastAddress(locator);
//                return locator;
    return localLoc;
}

bool eprosima::fastdds::rtps::ddsi_l2_transport::DoInputLocatorsMatch(const Locator &locator,
                                                                      const Locator &locator1) const {
    return locator.kind == locator1.kind && locator1.kind == transport_kind_;
}

eprosima::fastdds::rtps::LocatorList
eprosima::fastdds::rtps::ddsi_l2_transport::NormalizeLocator(const Locator &locator) {
    // Normally: If this is an address referring to multiple interfaces, then do a check which interfaces are allowed
    // We just skip this, we know where to go
    auto list = LocatorList();
    list.push_back(locator);
    return list;
}

void eprosima::fastdds::rtps::ddsi_l2_transport::select_locators(fastrtps::rtps::LocatorSelector &selector) const {
    // TODO
}

bool eprosima::fastdds::rtps::ddsi_l2_transport::is_local_locator(const Locator &locator) const {
    assert(locator.kind == transport_kind_);
    return locator == localLoc;
//                return memcmp(
//                        &locator.address[sizeof(locator.address) - sizeof(localMacAddress)],
//                        localMacAddress,
//                        sizeof(localMacAddress)
//                    );
}

void eprosima::fastdds::rtps::ddsi_l2_transport::AddDefaultOutputLocator(LocatorList &defaultList) {
//                Locator locator;
//                locator.kind = transport_kind_;
//                locator.port = 0;
//                setLocatorBroadcastAddress(locator);
    defaultList.push_back(localLoc);
}

void eprosima::fastdds::rtps::ddsi_l2_transport::setLocatorBroadcastAddress(Locator &locator) {
    // FF:FF:FF:FF:FF:FF
    memset(locator.address, 0, 10);
    memset(&locator.address[10], 0xFF, 6);
}

bool eprosima::fastdds::rtps::ddsi_l2_transport::getDefaultMetatrafficMulticastLocators(LocatorList &locators,
                                                                                        uint32_t metatraffic_multicast_port) const {
    Locator locator;
    locator.kind = transport_kind_;
    locator.port = static_cast<uint16_t>(metatraffic_multicast_port);
    setLocatorBroadcastAddress(locator);
    locators.push_back(locator);
    return true;
}

bool eprosima::fastdds::rtps::ddsi_l2_transport::getDefaultMetatrafficUnicastLocators(LocatorList &locators,
                                                                                      uint32_t metatraffic_unicast_port) const {
    Locator locator;
    locator.kind = transport_kind_;
    locator.port = static_cast<uint16_t>(metatraffic_unicast_port);
    locator.set_Invalid_Address();
    locators.push_back(locator);

    return true;
}

bool eprosima::fastdds::rtps::ddsi_l2_transport::getDefaultUnicastLocators(LocatorList &locators,
                                                                           uint32_t unicast_port) const {
    Locator locator;
    locator.kind = transport_kind_;
    locator.port = static_cast<uint16_t>(unicast_port);
    locator.set_Invalid_Address();
    locators.push_back(locator);

    return true;
}

bool eprosima::fastdds::rtps::ddsi_l2_transport::fillMetatrafficMulticastLocator(Locator &locator,
                                                                                 uint32_t metatraffic_multicast_port) const {
    if (locator.port == 0) {
        locator.port = metatraffic_multicast_port;
    }
    return true;
}

bool
eprosima::fastdds::rtps::ddsi_l2_transport::fillMetatrafficUnicastLocator(Locator &locator,
                                                                          uint32_t metatraffic_unicast_port) const {
    if (locator.port == 0) {
        locator.port = metatraffic_unicast_port;
    }
    return true;
}

bool eprosima::fastdds::rtps::ddsi_l2_transport::configureInitialPeerLocator(Locator &locator,
                                                                             const fastrtps::rtps::PortParameters &port_params,
                                                                             uint32_t domainId,
                                                                             LocatorList &list) const {
    if (locator.port == 0) {
        printf("Configure initial peer locator called.\n");

        // Logic simplified, not clear what should be done here.
        Locator auxloc(locator);
        auxloc.port = port_params.getMulticastPort(domainId);
        list.push_back(auxloc);
    } else {
        list.push_back(locator);
    }
    return true;
}

bool eprosima::fastdds::rtps::ddsi_l2_transport::fillUnicastLocator(Locator &locator, uint32_t well_known_port) const {
    if (locator.port == 0) {
        locator.port = well_known_port;
    }
    return true;
}