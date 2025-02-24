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
 * @file HelloWorldPublisher.h
 *
 */

#ifndef HELLOWORLDPUBLISHER_H_
#define HELLOWORLDPUBLISHER_H_

#include "HelloWorldPubSubTypes.h"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/topic/Topic.hpp>

#include "HelloWorld.h"

class HelloWorldPublisher
{
public:

    HelloWorldPublisher();

    virtual ~HelloWorldPublisher();

    //!Initialize
    bool init();

    //!Publish a sample
    bool publish(
            bool waitForListener = true);

    //!Run for number samples
    void run(
            uint32_t number,
            uint32_t sleep);

private:

    std::shared_ptr<HelloWorld> hello_;

    eprosima::fastdds::dds::DomainParticipant* participant_;

    eprosima::fastdds::dds::Publisher* publisher_;

    eprosima::fastdds::dds::Topic* topic_;

    eprosima::fastdds::dds::DataWriter* writer_;

    bool stop;

    class PubListener : public eprosima::fastdds::dds::DataWriterListener
    {
    public:

        PubListener()
            : matched_(0)
            , first_connected_(false)
        {
        }

        ~PubListener() override
        {
        }

        void on_publication_matched(
                eprosima::fastdds::dds::DataWriter* writer,
                const eprosima::fastdds::dds::PublicationMatchedStatus& info) override;

        int matched_;

        bool first_connected_;

        void on_offered_deadline_missed(eprosima::fastdds::dds::DataWriter *writer,
                                        const eprosima::fastdds::dds::OfferedDeadlineMissedStatus &status) override;

        void on_offered_incompatible_qos(eprosima::fastdds::dds::DataWriter *writer,
                                         const eprosima::fastdds::dds::OfferedIncompatibleQosStatus &status) override;

        void on_liveliness_lost(eprosima::fastdds::dds::DataWriter *writer,
                                const eprosima::fastdds::dds::LivelinessLostStatus &status) override;

        void on_unacknowledged_sample_removed(eprosima::fastdds::dds::DataWriter *writer,
                                              const eprosima::fastdds::dds::InstanceHandle_t &instance) override;

    }
    listener_;

    void runThread(
            uint32_t number,
            uint32_t sleep);


    eprosima::fastdds::dds::TypeSupport type_;
};

#endif /* HELLOWORLDPUBLISHER_H_ */
