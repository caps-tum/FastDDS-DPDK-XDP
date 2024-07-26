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
#include "fastdds/rtps/transport/DPDKTransportDescriptor.h"
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
    (void)remote;
    return localLoc;
}

bool eprosima::fastdds::rtps::ddsi_l2_transport::DoInputLocatorsMatch(const Locator &locator,
                                                                      const Locator &locator1) const {
    bool is_match = locator.kind == locator1.kind
            && locator1.kind == transport_kind_
            && locator.port == locator1.port;
    std::cout << "L2Transport: DoInputLocatorsMatch: " << locator << " " << locator1 << ": " << is_match << std::endl;
    return is_match;
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

    auto entries = selector.transport_starts();
    assert(selector.selected_size() == 0);
    assert(entries.size() == 1);
    selector.select(0);
    fastrtps::rtps::LocatorSelectorEntry &entry = *entries.at(0);
    assert(entry.multicast.size() == 1);
    assert(entry.unicast.empty());
    std::cout << "L2Transport: Select locators: " << entry.remote_guid
              << " multicast " << entry.multicast.at(0) << " unicast none " << std::endl;
}

bool eprosima::fastdds::rtps::ddsi_l2_transport::is_local_locator(const Locator &locator) const {
    assert(locator.kind == transport_kind_);
    bool is_local = locator == localLoc;
    std::cout << "L2Transport: is local locator: " << locator << ": " << is_local << std::endl;
    return is_local;
//                return memcmp(
//                        &locator.address[sizeof(locator.address) - sizeof(localMacAddress)],
//                        localMacAddress,
//                        sizeof(localMacAddress)
//                    );
}

void eprosima::fastdds::rtps::ddsi_l2_transport::AddDefaultOutputLocator(LocatorList &defaultList) {
    assert(defaultList.empty());
    Locator locator;
    locator.kind = transport_kind_;
    locator.port = 0;
    setLocatorBroadcastAddress(locator);
    std::cout << "L2Transport: Add default output locator: " << locator << std::endl;
    defaultList.push_back(locator);
}

void eprosima::fastdds::rtps::ddsi_l2_transport::setLocatorBroadcastAddress(Locator &locator) {
    // FF:FF:FF:FF:FF:FF
    memset(locator.address, 0, 10);
    memset(locator.address + 10, 0xFF, 6);
    assert(locator.address[0] == 0x00);
    assert(locator.address[9] == 0x00);
    assert(locator.address[10] == 0xFF);
    assert(locator.address[15] == 0xFF);
}

bool eprosima::fastdds::rtps::ddsi_l2_transport::getDefaultMetatrafficMulticastLocators(LocatorList &locators,
                                                                                        uint32_t metatraffic_multicast_port) const {
    assert(locators.empty());
    auto locator = Locator(transport_kind_, metatraffic_multicast_port);
    setLocatorBroadcastAddress(locator);
    std::cout << "L2Transport: Get default metatraffic multicast locator: " << locator << std::endl;
    locators.push_back(locator);

    return true;
}

bool eprosima::fastdds::rtps::ddsi_l2_transport::getDefaultMetatrafficUnicastLocators(LocatorList &locators,
                                                                                      uint32_t metatraffic_unicast_port) const {
    assert(locators.empty());
    auto locator = Locator(transport_kind_, metatraffic_unicast_port);
    locator.set_Invalid_Address();
    memcpy(locator.address, localLoc.address, sizeof(locator.address));
//    locators.push_back(locator);
    std::cout << "L2Transport: Get default metatraffic unicast locator: <none set>"  << std::endl;

    return true;
}

bool eprosima::fastdds::rtps::ddsi_l2_transport::getDefaultUnicastLocators(LocatorList &locators,
                                                                           uint32_t unicast_port) const {
    assert(locators.empty());
    auto locator = Locator(transport_kind_, unicast_port);
//    locator.set_Invalid_Address();
    memcpy(locator.address, localLoc.address, sizeof(locator.address));
//    locators.push_back(locator);
    std::cout << "L2Transport: Get default unicast locator: <none set>" << std::endl;

    return true;
}

bool eprosima::fastdds::rtps::ddsi_l2_transport::fillMetatrafficMulticastLocator(Locator &locator,
                                                                                 uint32_t metatraffic_multicast_port) const {
    if (locator.port == 0) {
        locator.port = metatraffic_multicast_port;
    }
    std::cout << "L2Transport: Fill metatraffic multicast locator: " << locator << std::endl;

    return true;
}

bool
eprosima::fastdds::rtps::ddsi_l2_transport::fillMetatrafficUnicastLocator(Locator &locator,
                                                                          uint32_t metatraffic_unicast_port) const {
    if (locator.port == 0) {
        locator.port = metatraffic_unicast_port;
    }
    std::cout << "L2Transport: Fill metatraffic unicast locator: " << locator << std::endl;
    return true;
}

bool eprosima::fastdds::rtps::ddsi_l2_transport::configureInitialPeerLocator(Locator &locator,
                                                                             const fastrtps::rtps::PortParameters &port_params,
                                                                             uint32_t domainId,
                                                                             LocatorList &list) const {
    printf("L2Transport: Configure initial peer locator called.\n");
    if (locator.port == 0) {

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
//    memcpy(locator.address, localLoc.address, sizeof(locator.address));
    std::cout << "L2Transport: Fill unicast locator: " << locator << std::endl;
    return true;
}

eprosima::fastdds::rtps::ddsi_l2_transport::ddsi_l2_transport(int32_t transportKind)
        : TransportInterface(transportKind) {
    transport_kind_ = transportKind;
    localMacAddress = userspace_l2_mac_addr{};
}
