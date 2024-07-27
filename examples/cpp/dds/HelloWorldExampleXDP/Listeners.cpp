//
// Created by Vincent Bode on 27/07/2024.
//

#include "Listeners.h"

void ParticipantListener::on_participant_discovery(eprosima::fastdds::dds::DomainParticipant *participant,
                                                   eprosima::fastrtps::rtps::ParticipantDiscoveryInfo &&info) {
    std::cout << "ParticipantListener::on_participant_discovery: " << info.info.m_participantName << " " << info.status << std::endl;
}

void ParticipantListener::on_participant_discovery(eprosima::fastdds::dds::DomainParticipant *participant,
                                                   eprosima::fastrtps::rtps::ParticipantDiscoveryInfo &&info,
                                                   bool &should_be_ignored) {
    std::cout << "ParticipantListener::on_participant_discovery: "
              << info.info.m_participantName << " "
              << info.status << " "
              << (should_be_ignored ? "ignored" : "not ignored") << std::endl;
}

void ParticipantListener::on_subscriber_discovery(eprosima::fastdds::dds::DomainParticipant *participant,
                                                  eprosima::fastrtps::rtps::ReaderDiscoveryInfo &&info) {
    std::cout << "ParticipantListener::on_subscriber_discovery" << std::endl;
}

void ParticipantListener::on_subscriber_discovery(eprosima::fastdds::dds::DomainParticipant *participant,
                                                  eprosima::fastrtps::rtps::ReaderDiscoveryInfo &&info,
                                                  bool &should_be_ignored) {
    std::cout << "ParticipantListener::on_subscriber_discovery" << std::endl;
}

void ParticipantListener::on_publisher_discovery(eprosima::fastdds::dds::DomainParticipant *participant,
                                                 eprosima::fastrtps::rtps::WriterDiscoveryInfo &&info) {
    std::cout << "ParticipantListener::on_publisher_discovery" << std::endl;
}

void ParticipantListener::on_publisher_discovery(eprosima::fastdds::dds::DomainParticipant *participant,
                                                 eprosima::fastrtps::rtps::WriterDiscoveryInfo &&info,
                                                 bool &should_be_ignored) {
    std::cout << "ParticipantListener::on_publisher_discovery" << std::endl;
}

void ParticipantListener::on_type_discovery(eprosima::fastdds::dds::DomainParticipant *participant,
                                            const eprosima::fastrtps::rtps::SampleIdentity &request_sample_id,
                                            const eprosima::fastrtps::string_255 &topic,
                                            const eprosima::fastrtps::types::TypeIdentifier *identifier,
                                            const eprosima::fastrtps::types::TypeObject *object,
                                            eprosima::fastrtps::types::DynamicType_ptr dyn_type) {
    std::cout << "ParticipantListener::on_type_discovery" << std::endl;
}

void ParticipantListener::on_type_dependencies_reply(eprosima::fastdds::dds::DomainParticipant *participant,
                                                     const eprosima::fastrtps::rtps::SampleIdentity &request_sample_id,
                                                     const eprosima::fastrtps::types::TypeIdentifierWithSizeSeq &dependencies) {
    std::cout << "ParticipantListener::on_type_dependencies_reply" << std::endl;
}

void ParticipantListener::on_type_information_received(eprosima::fastdds::dds::DomainParticipant *participant,
                                                       const eprosima::fastrtps::string_255 topic_name,
                                                       const eprosima::fastrtps::string_255 type_name,
                                                       const eprosima::fastrtps::types::TypeInformation &type_information) {
    std::cout << "ParticipantListener::on_type_information_received" << std::endl;
}

void TopicListener::on_inconsistent_topic(eprosima::fastdds::dds::Topic *topic,
                                          eprosima::fastdds::dds::InconsistentTopicStatus status) {
    std::cout << "TopicListener::on_inconsistent_topic" << std::endl;
}

ParticipantListener exampleParticipantListener{};
TopicListener exampleTopicListener{};
