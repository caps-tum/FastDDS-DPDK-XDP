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
 * @file HelloWorldSubscriber.h
 *
 */

#ifndef HELLOWORLDSUBSCRIBER_H_
#define HELLOWORLDSUBSCRIBER_H_

#include "HelloWorldPubSubTypes.h"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>

#include "HelloWorld.h"

class HelloWorldSubscriber
{
public:

    HelloWorldSubscriber();

    virtual ~HelloWorldSubscriber();

    //!Initialize the subscriber
    bool init();

    //!RUN the subscriber
    void run();

    //!Run the subscriber until number samples have been received.
    void run(
            uint32_t number);

private:

    eprosima::fastdds::dds::DomainParticipant* participant_;

    eprosima::fastdds::dds::Subscriber* subscriber_;

    eprosima::fastdds::dds::Topic* topic_;

    eprosima::fastdds::dds::DataReader* reader_;

    eprosima::fastdds::dds::TypeSupport type_;

public:

    class SubListener : public eprosima::fastdds::dds::DataReaderListener
    {
    public:

        SubListener()
            : matched_(0)
            , samples_(0)
        {
            hello_ = std::make_shared<HelloWorld>();
        }

        ~SubListener() override
        {
        }

        void on_data_available(
                eprosima::fastdds::dds::DataReader* reader) override;

        void on_subscription_matched(
                eprosima::fastdds::dds::DataReader* reader,
                const eprosima::fastdds::dds::SubscriptionMatchedStatus& info) override;

        std::shared_ptr<HelloWorld> hello_;

        int matched_;

        uint32_t samples_;

        void on_requested_deadline_missed(eprosima::fastdds::dds::DataReader *reader,
                                          const eprosima::fastrtps::RequestedDeadlineMissedStatus &status) override;

        void on_liveliness_changed(eprosima::fastdds::dds::DataReader *reader,
                                   const eprosima::fastrtps::LivelinessChangedStatus &status) override;

        void on_sample_rejected(eprosima::fastdds::dds::DataReader *reader,
                                const eprosima::fastrtps::SampleRejectedStatus &status) override;

        void on_requested_incompatible_qos(eprosima::fastdds::dds::DataReader *reader,
                                           const eprosima::fastdds::dds::RequestedIncompatibleQosStatus &status) override;

        void on_sample_lost(eprosima::fastdds::dds::DataReader *reader,
                            const eprosima::fastdds::dds::SampleLostStatus &status) override;

    }
    listener_;
};

#endif /* HELLOWORLDSUBSCRIBER_H_ */
