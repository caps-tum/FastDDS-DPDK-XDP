// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file BuiltinProtocols.cpp
 *
 */

#include <fastdds/rtps/builtin/BuiltinProtocols.h>
#include <fastdds/rtps/common/Locator.h>

#include <fastdds/rtps/builtin/discovery/participant/PDPSimple.h>
#include <fastdds/rtps/builtin/discovery/endpoint/EDP.h>
#include <fastdds/rtps/builtin/discovery/endpoint/EDPStatic.h>

#include <rtps/builtin/discovery/participant/PDPServer.hpp>
#include <rtps/builtin/discovery/participant/PDPClient.h>

#include <fastdds/rtps/builtin/data/ParticipantProxyData.h>

#include <fastdds/rtps/builtin/liveliness/WLP.h>

#include <fastdds/dds/builtin/typelookup/TypeLookupManager.hpp>

#include <rtps/participant/RTPSParticipantImpl.h>

#include <fastdds/dds/log/Log.hpp>
#include <fastrtps/utils/IPFinder.h>

#include <algorithm>

using namespace eprosima::fastrtps;

namespace eprosima {
namespace fastrtps {
namespace rtps {


BuiltinProtocols::BuiltinProtocols()
    : mp_participantImpl(nullptr)
    , mp_PDP(nullptr)
    , mp_WLP(nullptr)
    , tlm_(nullptr)
{
}

BuiltinProtocols::~BuiltinProtocols()
{
    // Send participant is disposed
    if (nullptr != mp_PDP)
    {
        // Send participant is disposed
        mp_PDP->announceParticipantState(true, true);
        // Consider all discovered participants as disposed
        mp_PDP->disable();
    }

    // TODO Auto-generated destructor stub
    delete mp_WLP;
    delete tlm_;
    delete mp_PDP;

}

bool BuiltinProtocols::initBuiltinProtocols(
        RTPSParticipantImpl* p_part,
        BuiltinAttributes& attributes)
{
    mp_participantImpl = p_part;
    m_att = attributes;
    m_metatrafficUnicastLocatorList = m_att.metatrafficUnicastLocatorList;
    m_metatrafficMulticastLocatorList = m_att.metatrafficMulticastLocatorList;
    m_initialPeersList = m_att.initialPeersList;

    {
        std::unique_lock<eprosima::shared_mutex> disc_lock(getDiscoveryMutex());
        m_DiscoveryServers = m_att.discovery_config.m_DiscoveryServers;
    }

    filter_server_remote_locators(p_part->network_factory());

    const RTPSParticipantAllocationAttributes& allocation = p_part->getRTPSParticipantAttributes().allocation;

    // PDP
    switch (m_att.discovery_config.discoveryProtocol)
    {
        case DiscoveryProtocol_t::NONE:
            EPROSIMA_LOG_WARNING(RTPS_PDP, "No participant discovery protocol specified");
            return true;

        case DiscoveryProtocol_t::SIMPLE:
            mp_PDP = new PDPSimple(this, allocation);
            break;

        case DiscoveryProtocol_t::EXTERNAL:
            EPROSIMA_LOG_ERROR(RTPS_PDP, "Flag only present for debugging purposes");
            return false;

        case DiscoveryProtocol_t::CLIENT:
            mp_PDP = new fastdds::rtps::PDPClient(this, allocation);
            break;

        case DiscoveryProtocol_t::SERVER:
            mp_PDP = new fastdds::rtps::PDPServer(this, allocation, DurabilityKind_t::TRANSIENT_LOCAL);
            break;

#if HAVE_SQLITE3
        case DiscoveryProtocol_t::BACKUP:
            mp_PDP = new fastdds::rtps::PDPServer(this, allocation, DurabilityKind_t::TRANSIENT);
            break;
#endif // if HAVE_SQLITE3

        case DiscoveryProtocol_t::SUPER_CLIENT:
            mp_PDP = new fastdds::rtps::PDPClient(this, allocation, true);
            break;

        default:
            EPROSIMA_LOG_ERROR(RTPS_PDP, "Unknown DiscoveryProtocol_t specified.");
            return false;
    }

    if (!mp_PDP->init(mp_participantImpl))
    {
        EPROSIMA_LOG_ERROR(RTPS_PDP, "Participant discovery configuration failed");
        delete mp_PDP;
        mp_PDP = nullptr;
        return false;
    }

    // WLP
    if (m_att.use_WriterLivelinessProtocol)
    {
        mp_WLP = new WLP(this);
        mp_WLP->initWL(mp_participantImpl);
    }

    // TypeLookupManager
    if (m_att.typelookup_config.use_client || m_att.typelookup_config.use_server)
    {
        tlm_ = new fastdds::dds::builtin::TypeLookupManager(this);
        tlm_->init_typelookup_service(mp_participantImpl);
    }

    return true;
}

void BuiltinProtocols::enable()
{
    if (nullptr != mp_PDP)
    {
        mp_PDP->enable();
        mp_PDP->announceParticipantState(true);
        mp_PDP->resetParticipantAnnouncement();
    }
}

bool BuiltinProtocols::updateMetatrafficLocators(
        LocatorList_t& loclist)
{
    m_metatrafficUnicastLocatorList = loclist;
    return true;
}

void BuiltinProtocols::filter_server_remote_locators(
        NetworkFactory& nf)
{
    eprosima::shared_lock<eprosima::shared_mutex> disc_lock(getDiscoveryMutex());

    for (eprosima::fastdds::rtps::RemoteServerAttributes& rs : m_DiscoveryServers)
    {
        LocatorList_t allowed_locators;
        for (Locator_t& loc : rs.metatrafficUnicastLocatorList)
        {
            if (nf.is_locator_remote_or_allowed(loc))
            {
                allowed_locators.push_back(loc);
            }
            else
            {
                EPROSIMA_LOG_WARNING(RTPS_PDP, "Ignoring remote server locator " << loc << " : not allowed.");
            }
        }
        rs.metatrafficUnicastLocatorList.swap(allowed_locators);
    }
}

bool BuiltinProtocols::addLocalWriter(
        RTPSWriter* w,
        const fastrtps::TopicAttributes& topicAtt,
        const fastrtps::WriterQos& wqos)
{
    bool ok = true;

    if (nullptr != mp_PDP)
    {
        ok = mp_PDP->getEDP()->newLocalWriterProxyData(w, topicAtt, wqos);

        if (!ok)
        {
            EPROSIMA_LOG_WARNING(RTPS_EDP, "Failed register WriterProxyData in EDP");
            return false;
        }
    }
    else
    {
        EPROSIMA_LOG_WARNING(RTPS_EDP, "EDP is not used in this Participant, register a Writer is impossible");
    }

    if (nullptr != mp_WLP)
    {
        ok &= mp_WLP->add_local_writer(w, wqos);
    }
    else
    {
        EPROSIMA_LOG_WARNING(RTPS_LIVELINESS,
                "LIVELINESS is not used in this Participant, register a Writer is impossible");
    }
    return ok;
}

bool BuiltinProtocols::addLocalReader(
        RTPSReader* R,
        const fastrtps::TopicAttributes& topicAtt,
        const fastrtps::ReaderQos& rqos,
        const fastdds::rtps::ContentFilterProperty* content_filter)
{
    bool ok = true;

    if (nullptr != mp_PDP)
    {
        ok = mp_PDP->getEDP()->newLocalReaderProxyData(R, topicAtt, rqos, content_filter);

        if (!ok)
        {
            EPROSIMA_LOG_WARNING(RTPS_EDP, "Failed register ReaderProxyData in EDP");
            return false;
        }
    }
    else
    {
        EPROSIMA_LOG_WARNING(RTPS_EDP, "EDP is not used in this Participant, register a Reader is impossible");
    }

    if (nullptr != mp_WLP)
    {
        ok &= mp_WLP->add_local_reader(R, rqos);
    }

    return ok;
}

bool BuiltinProtocols::updateLocalWriter(
        RTPSWriter* W,
        const TopicAttributes& topicAtt,
        const WriterQos& wqos)
{
    bool ok = false;
    if ((nullptr != mp_PDP) && (nullptr != mp_PDP->getEDP()))
    {
        ok = mp_PDP->getEDP()->updatedLocalWriter(W, topicAtt, wqos);
    }
    return ok;
}

bool BuiltinProtocols::updateLocalReader(
        RTPSReader* R,
        const TopicAttributes& topicAtt,
        const ReaderQos& rqos,
        const fastdds::rtps::ContentFilterProperty* content_filter)
{
    bool ok = false;
    if ((nullptr != mp_PDP) && (nullptr != mp_PDP->getEDP()))
    {
        ok = mp_PDP->getEDP()->updatedLocalReader(R, topicAtt, rqos, content_filter);
    }
    return ok;
}

bool BuiltinProtocols::removeLocalWriter(
        RTPSWriter* W)
{
    bool ok = false;
    if (nullptr != mp_WLP)
    {
        ok |= mp_WLP->remove_local_writer(W);
    }
    if ((nullptr != mp_PDP) && (nullptr != mp_PDP->getEDP()))
    {
        ok |= mp_PDP->getEDP()->removeLocalWriter(W);
    }
    return ok;
}

bool BuiltinProtocols::removeLocalReader(
        RTPSReader* R)
{
    bool ok = false;
    if (nullptr != mp_WLP)
    {
        ok |= mp_WLP->remove_local_reader(R);
    }
    if ((nullptr != mp_PDP) && (nullptr != mp_PDP->getEDP()))
    {
        ok |= mp_PDP->getEDP()->removeLocalReader(R);
    }
    return ok;
}

void BuiltinProtocols::announceRTPSParticipantState()
{
    assert(mp_PDP);

    if (mp_PDP)
    {
        mp_PDP->announceParticipantState(false);
    }
    else if (m_att.discovery_config.discoveryProtocol != DiscoveryProtocol_t::NONE)
    {
        EPROSIMA_LOG_ERROR(RTPS_EDP, "Trying to use BuiltinProtocols interfaces before initBuiltinProtocols call");
    }
}

void BuiltinProtocols::stopRTPSParticipantAnnouncement()
{
    // note that participants created with DiscoveryProtocol::NONE
    // may not have mp_PDP available

    if (mp_PDP)
    {
        mp_PDP->stopParticipantAnnouncement();
    }
    else if (m_att.discovery_config.discoveryProtocol != DiscoveryProtocol_t::NONE)
    {
        EPROSIMA_LOG_ERROR(RTPS_EDP, "Trying to use BuiltinProtocols interfaces before initBuiltinProtocols call");
    }
}

void BuiltinProtocols::resetRTPSParticipantAnnouncement()
{
    assert(mp_PDP);

    if (mp_PDP)
    {
        mp_PDP->resetParticipantAnnouncement();
    }
    else if (m_att.discovery_config.discoveryProtocol != DiscoveryProtocol_t::NONE)
    {
        EPROSIMA_LOG_ERROR(RTPS_EDP, "Trying to use BuiltinProtocols interfaces before initBuiltinProtocols call");
    }
}

} // namespace rtps
} /* namespace rtps */
} /* namespace eprosima */
