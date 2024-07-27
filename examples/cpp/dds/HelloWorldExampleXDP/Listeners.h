//
// Created by Vincent Bode on 27/07/2024.
//

#ifndef FASTRTPS_LISTENERS_H
#define FASTRTPS_LISTENERS_H


#include "dds/domain/DomainParticipantListener.hpp"


class ParticipantListener : public eprosima::fastdds::dds::DomainParticipantListener {

public:
    void on_participant_discovery(eprosima::fastdds::dds::DomainParticipant *participant,
                                  eprosima::fastrtps::rtps::ParticipantDiscoveryInfo &&info) override;

    void on_participant_discovery(eprosima::fastdds::dds::DomainParticipant *participant,
                                  eprosima::fastrtps::rtps::ParticipantDiscoveryInfo &&info,
                                  bool &should_be_ignored) override;

    void on_subscriber_discovery(eprosima::fastdds::dds::DomainParticipant *participant,
                                 eprosima::fastrtps::rtps::ReaderDiscoveryInfo &&info) override;

    void on_subscriber_discovery(eprosima::fastdds::dds::DomainParticipant *participant,
                                 eprosima::fastrtps::rtps::ReaderDiscoveryInfo &&info,
                                 bool &should_be_ignored) override;

    void on_publisher_discovery(eprosima::fastdds::dds::DomainParticipant *participant,
                                eprosima::fastrtps::rtps::WriterDiscoveryInfo &&info) override;

    void on_publisher_discovery(eprosima::fastdds::dds::DomainParticipant *participant,
                                eprosima::fastrtps::rtps::WriterDiscoveryInfo &&info, bool &should_be_ignored) override;

    void on_type_discovery(eprosima::fastdds::dds::DomainParticipant *participant,
                           const eprosima::fastrtps::rtps::SampleIdentity &request_sample_id,
                           const eprosima::fastrtps::string_255 &topic,
                           const eprosima::fastrtps::types::TypeIdentifier *identifier,
                           const eprosima::fastrtps::types::TypeObject *object,
                           eprosima::fastrtps::types::DynamicType_ptr dyn_type) override;

    void on_type_dependencies_reply(eprosima::fastdds::dds::DomainParticipant *participant,
                                    const eprosima::fastrtps::rtps::SampleIdentity &request_sample_id,
                                    const eprosima::fastrtps::types::TypeIdentifierWithSizeSeq &dependencies) override;

    void on_type_information_received(eprosima::fastdds::dds::DomainParticipant *participant,
                                      const eprosima::fastrtps::string_255 topic_name,
                                      const eprosima::fastrtps::string_255 type_name,
                                      const eprosima::fastrtps::types::TypeInformation &type_information) override;

};

class TopicListener : public eprosima::fastdds::dds::TopicListener {
public:
    void on_inconsistent_topic(eprosima::fastdds::dds::Topic *topic,
                               eprosima::fastdds::dds::InconsistentTopicStatus status) override;
};

extern ParticipantListener exampleParticipantListener;
extern TopicListener exampleTopicListener;


#endif //FASTRTPS_LISTENERS_H
