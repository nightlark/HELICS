/*
Copyright © 2017-2018,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC
All rights reserved. See LICENSE file and DISCLAIMER for more details.
*/

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/floating_point_comparison.hpp>

#include <future>

#include "ValueFederateTestTemplates.hpp"
#include "helics/application_api/Publications.hpp"
#include "helics/application_api/Subscriptions.hpp"
#include "helics/application_api/ValueFederate.hpp"
#include "helics/core/BrokerFactory.hpp"
#include "helics/core/CoreFactory.hpp"
#include "testFixtures.hpp"

/** these test cases test out the value federates
 */
namespace bdata = boost::unit_test::data;
namespace utf = boost::unit_test;

BOOST_FIXTURE_TEST_SUITE (value_federate_key_tests, FederateTestFixture, *utf::label ("ci"))

BOOST_DATA_TEST_CASE (value_federate_subscriber_and_publisher_registration,
                      bdata::make (core_types_single),
                      core_type)
{
    std::this_thread::sleep_for (std::chrono::milliseconds (100));
    using namespace helics;
    SetupTest<ValueFederate> (core_type, 1);
    auto vFed1 = GetFederateAs<ValueFederate> (0);

    // register the publications
    Publication pubid (vFed1.get (), "pub1", helicsType<std::string> ());
    PublicationT<int> pubid2 (GLOBAL, vFed1, "pub2");

    Publication pubid3 (vFed1, "pub3", helicsType<double> (), "V");

    // these aren't meant to match the publications
    Subscription subid1 (vFed1, "sub1");

    SubscriptionT<int> subid2 (vFed1, "sub2");

    Subscription subid3 (vFed1, "sub3", "V");
    // enter execution
    vFed1->enterExecutingMode ();

    BOOST_CHECK (vFed1->getCurrentState () == Federate::op_states::execution);
    // check subscriptions
    const auto &sv = subid1.getTarget ();
    const auto &sv2 = subid2.getTarget ();
    BOOST_CHECK_EQUAL (sv, "sub1");
    BOOST_CHECK_EQUAL (sv2, "sub2");
    const auto &sub3name = subid3.getTarget ();
    BOOST_CHECK_EQUAL (sub3name, "sub3");

    BOOST_CHECK_EQUAL (subid1.getType (), "def");  // def is the default type
    BOOST_CHECK_EQUAL (subid2.getType (), "int32");
    BOOST_CHECK_EQUAL (subid3.getType (), "def");
    BOOST_CHECK_EQUAL (subid3.getUnits (), "V");

    // check publications

    auto pk = pubid.getKey ();
    auto pk2 = pubid2.getKey ();
    BOOST_CHECK_EQUAL (pk, "fed0/pub1");
    BOOST_CHECK_EQUAL (pk2, "pub2");
    auto pub3name = pubid3.getKey ();
    BOOST_CHECK_EQUAL (pub3name, "fed0/pub3");

    BOOST_CHECK_EQUAL (pubid3.getType (), "double");
    BOOST_CHECK_EQUAL (pubid3.getUnits (), "V");
    vFed1->finalize ();

    BOOST_CHECK (vFed1->getCurrentState () == Federate::op_states::finalize);
}

BOOST_DATA_TEST_CASE (value_federate_single_transfer_publisher, bdata::make (core_types_single), core_type)
{
    SetupTest<helics::ValueFederate> (core_type, 1);
    auto vFed1 = GetFederateAs<helics::ValueFederate> (0);
    BOOST_REQUIRE (vFed1);
    // register the publications
    helics::Publication pubid (helics::GLOBAL, vFed1.get (), "pub1", helics::helics_type_t::helicsString);

    auto &subid = vFed1->registerSubscription ("pub1");
    vFed1->setTimeProperty (TIME_DELTA_PROPERTY, 1.0);
    vFed1->enterExecutingMode ();
    // publish string1 at time=0.0;
    pubid.publish ("string1");
    auto gtime = vFed1->requestTime (1.0);

    BOOST_CHECK_EQUAL (gtime, 1.0);
    std::string s;
    // get the value
    subid.getValue (s);
    // make sure the string is what we expect
    BOOST_CHECK_EQUAL (s, "string1");
    // publish a second string
    pubid.publish ("string2");
    // make sure the value is still what we expect
    subid.getValue (s);

    BOOST_CHECK_EQUAL (s, "string1");
    // advance time
    gtime = vFed1->requestTime (2.0);
    // make sure the value was updated
    BOOST_CHECK_EQUAL (gtime, 2.0);
    subid.getValue (s);

    BOOST_CHECK_EQUAL (s, "string2");
    vFed1->finalize ();
}

static bool dual_transfer_test (std::shared_ptr<helics::ValueFederate> &vFed1,
                         std::shared_ptr<helics::ValueFederate> &vFed2,
                         helics::Publication &pubid,
                         helics::Input &subid)
{
    vFed1->setTimeProperty (TIME_DELTA_PROPERTY, 1.0);
    vFed2->setTimeProperty (TIME_DELTA_PROPERTY, 1.0);

    bool correct = true;

    auto f1finish = std::async (std::launch::async, [&]() { vFed1->enterExecutingMode (); });
    vFed2->enterExecutingMode ();
    f1finish.wait ();
    // publish string1 at time=0.0;
    vFed1->publish (pubid, "string1");
    auto f1time = std::async (std::launch::async, [&]() { return vFed1->requestTime (1.0); });
    auto gtime = vFed2->requestTime (1.0);

    BOOST_CHECK_EQUAL (gtime, 1.0);
    if (gtime != 1.0)
    {
        correct = false;
    }
    gtime = f1time.get ();
    BOOST_CHECK_EQUAL (gtime, 1.0);
    if (gtime != 1.0)
    {
        correct = false;
    }
    std::string s;
    // get the value
    vFed2->getValue (subid, s);
    // make sure the string is what we expect
    BOOST_CHECK_EQUAL (s, "string1");
    if (s != "string1")
    {
        correct = false;
    }
    // publish a second string
    vFed1->publish (pubid, "string2");
    // make sure the value is still what we expect
    vFed2->getValue (subid, s);

    BOOST_CHECK_EQUAL (s, "string1");
    if (s != "string1")
    {
        correct = false;
    }
    // advance time
    f1time = std::async (std::launch::async, [&]() { return vFed1->requestTime (2.0); });
    gtime = vFed2->requestTime (2.0);

    BOOST_CHECK_EQUAL (gtime, 2.0);
    if (gtime != 2.0)
    {
        correct = false;
    }
    gtime = f1time.get ();
    BOOST_CHECK_EQUAL (gtime, 2.0);
    if (gtime != 2.0)
    {
        correct = false;
    }
    // make sure the value was updated

    vFed2->getValue (subid, s);

    BOOST_CHECK_EQUAL (s, "string2");
    if (s != "string2")
    {
        correct = false;
    }
        vFed1->finalize ();
    vFed2->finalize ();
    return correct;
}

BOOST_DATA_TEST_CASE (value_federate_dual_transfer, bdata::make (core_types_all), core_type)
{
    SetupTest<helics::ValueFederate> (core_type, 2);
    auto vFed1 = GetFederateAs<helics::ValueFederate> (0);
    auto vFed2 = GetFederateAs<helics::ValueFederate> (1);

    // register the publications
    auto &pubid = vFed1->registerGlobalPublication<std::string> ("pub1");

    auto &subid = vFed2->registerSubscription ("pub1");
    bool res = dual_transfer_test (vFed1, vFed2, pubid, subid);
    BOOST_CHECK (res);
}

BOOST_DATA_TEST_CASE (value_federate_dual_transfer_inputs, bdata::make (core_types_all), core_type)
{
    SetupTest<helics::ValueFederate> (core_type, 2);
    auto vFed1 = GetFederateAs<helics::ValueFederate> (0);
    auto vFed2 = GetFederateAs<helics::ValueFederate> (1);

    // register the publications
    auto &pubid = vFed1->registerGlobalPublication<std::string> ("pub1");

    auto &inpid = vFed2->registerInput<std::string> ("inp1");
    vFed2->addTarget (inpid, "pub1");
    bool res = dual_transfer_test (vFed1, vFed2, pubid, inpid);
   BOOST_CHECK (res);
}

BOOST_DATA_TEST_CASE (value_federate_dual_transfer_pubtarget, bdata::make (core_types_all), core_type)
{
    SetupTest<helics::ValueFederate> (core_type, 2);
    auto vFed1 = GetFederateAs<helics::ValueFederate> (0);
    auto vFed2 = GetFederateAs<helics::ValueFederate> (1);

    // register the publications
    auto &pubid = vFed1->registerGlobalPublication<std::string> ("pub1");
    vFed1->addTarget (pubid, "inp1");

    auto &inpid = vFed2->registerGlobalInput<std::string> ("inp1");
    bool res = dual_transfer_test (vFed1, vFed2, pubid, inpid);
    BOOST_CHECK (res);
}

BOOST_DATA_TEST_CASE (value_federate_dual_transfer_nameless_pub, bdata::make (core_types_all), core_type)
{
    SetupTest<helics::ValueFederate> (core_type, 2);
    auto vFed1 = GetFederateAs<helics::ValueFederate> (0);
    auto vFed2 = GetFederateAs<helics::ValueFederate> (1);

    // register the publications
    auto &pubid = vFed1->registerPublication<std::string> ("");
    vFed1->addTarget (pubid, "inp1");

    auto &inpid = vFed2->registerGlobalInput<std::string> ("inp1");
    bool res = dual_transfer_test (vFed1, vFed2, pubid, inpid);
    BOOST_CHECK (res);
}

BOOST_DATA_TEST_CASE (value_federate_dual_transfer_broker_link, bdata::make (core_types_all), core_type)
{
    SetupTest<helics::ValueFederate> (core_type, 2);
    auto vFed1 = GetFederateAs<helics::ValueFederate> (0);
    auto vFed2 = GetFederateAs<helics::ValueFederate> (1);

    auto &broker = brokers[0];
    broker->dataLink ("pub1", "inp1");
    // register the publications
    auto &pubid = vFed1->registerGlobalPublication<std::string> ("pub1");

    auto &inpid = vFed2->registerGlobalInput<std::string> ("inp1");
    bool res = dual_transfer_test (vFed1, vFed2, pubid, inpid);
    BOOST_CHECK (res);
}

BOOST_DATA_TEST_CASE (value_federate_dual_transfer_broker_link_late, bdata::make (core_types_all), core_type)
{
    SetupTest<helics::ValueFederate> (core_type, 2);
    auto vFed1 = GetFederateAs<helics::ValueFederate> (0);
    auto vFed2 = GetFederateAs<helics::ValueFederate> (1);

    auto &broker = brokers[0];
    
    // register the publications
    auto &pubid = vFed1->registerGlobalPublication<std::string> ("pub1");
    std::this_thread::sleep_for (std::chrono::milliseconds (200));
    broker->dataLink ("pub1", "inp1");
    auto &inpid = vFed2->registerGlobalInput<std::string> ("inp1");
    bool res = dual_transfer_test (vFed1, vFed2, pubid, inpid);
    BOOST_CHECK (res);
}

BOOST_DATA_TEST_CASE (value_federate_dual_transfer_broker_link_direct, bdata::make (core_types_all), core_type)
{
    SetupTest<helics::ValueFederate> (core_type, 2);
    auto vFed1 = GetFederateAs<helics::ValueFederate> (0);
    auto vFed2 = GetFederateAs<helics::ValueFederate> (1);

    auto &broker = brokers[0];
    
    // register the publications
    auto &pubid = vFed1->registerGlobalPublication<std::string> ("pub1");
    
    auto &inpid = vFed2->registerGlobalInput<std::string> ("inp1");
	std::this_thread::sleep_for (std::chrono::milliseconds (200));
    broker->dataLink ("pub1", "inp1");
    bool res = dual_transfer_test (vFed1, vFed2, pubid, inpid);
    BOOST_CHECK (res);
}

BOOST_DATA_TEST_CASE (value_federate_dual_transfer_core_link, bdata::make (core_types_all), core_type)
{
    SetupTest<helics::ValueFederate> (core_type, 2);
    auto vFed1 = GetFederateAs<helics::ValueFederate> (0);
    auto vFed2 = GetFederateAs<helics::ValueFederate> (1);

    auto core = vFed1->getCorePointer();
    core->dataLink ("pub1", "inp1");
    core = nullptr;
    // register the publications
    auto &pubid = vFed1->registerGlobalPublication<std::string> ("pub1");

    auto &inpid = vFed2->registerGlobalInput<std::string> ("inp1");
    bool res = dual_transfer_test (vFed1, vFed2, pubid, inpid);
    BOOST_CHECK (res);
}

BOOST_DATA_TEST_CASE (value_federate_dual_transfer_core_link_late, bdata::make (core_types_all), core_type)
{
    SetupTest<helics::ValueFederate> (core_type, 2);
    auto vFed1 = GetFederateAs<helics::ValueFederate> (0);
    auto vFed2 = GetFederateAs<helics::ValueFederate> (1);

   auto core = vFed1->getCorePointer ();
    

    // register the publications
    auto &pubid = vFed1->registerGlobalPublication<std::string> ("pub1");
    std::this_thread::sleep_for (std::chrono::milliseconds (200));
    core->dataLink ("pub1", "inp1");
    core = nullptr;
    auto &inpid = vFed2->registerGlobalInput<std::string> ("inp1");
    bool res = dual_transfer_test (vFed1, vFed2, pubid, inpid);
    BOOST_CHECK (res);
}

BOOST_DATA_TEST_CASE (value_federate_dual_transfer_core_link_late_switch, bdata::make (core_types_all), core_type)
{
    SetupTest<helics::ValueFederate> (core_type, 2);
    auto vFed1 = GetFederateAs<helics::ValueFederate> (0);
    auto vFed2 = GetFederateAs<helics::ValueFederate> (1);

    auto core = vFed1->getCorePointer ();

   
    auto &inpid = vFed2->registerGlobalInput<std::string> ("inp1");
    std::this_thread::sleep_for (std::chrono::milliseconds (200));
    core->dataLink ("pub1", "inp1");
    core = nullptr;
    // register the publications
    auto &pubid = vFed1->registerGlobalPublication<std::string> ("pub1");
    bool res = dual_transfer_test (vFed1, vFed2, pubid, inpid);
    BOOST_CHECK (res);
}

BOOST_DATA_TEST_CASE (value_federate_dual_transfer_core_link_direct1, bdata::make (core_types_all), core_type)
{
    SetupTest<helics::ValueFederate> (core_type, 2);
    auto vFed1 = GetFederateAs<helics::ValueFederate> (0);
    auto vFed2 = GetFederateAs<helics::ValueFederate> (1);

     auto core = vFed1->getCorePointer ();

    // register the publications
    auto &pubid = vFed1->registerGlobalPublication<std::string> ("pub1");

    auto &inpid = vFed2->registerGlobalInput<std::string> ("inp1");
    std::this_thread::sleep_for (std::chrono::milliseconds (200));
    core->dataLink ("pub1", "inp1");
    core = nullptr;
    bool res = dual_transfer_test (vFed1, vFed2, pubid, inpid);
    BOOST_CHECK (res);
}


BOOST_DATA_TEST_CASE (value_federate_dual_transfer_core_link_direct2, bdata::make (core_types_all), core_type)
{
    SetupTest<helics::ValueFederate> (core_type, 2);
    auto vFed1 = GetFederateAs<helics::ValueFederate> (0);
    auto vFed2 = GetFederateAs<helics::ValueFederate> (1);

    auto core = vFed2->getCorePointer ();

    // register the publications
    auto &pubid = vFed1->registerGlobalPublication<std::string> ("pub1");

    auto &inpid = vFed2->registerGlobalInput<std::string> ("inp1");
    std::this_thread::sleep_for (std::chrono::milliseconds (200));
    core->dataLink ("pub1", "inp1");
    core = nullptr;
    bool res = dual_transfer_test (vFed1, vFed2, pubid, inpid);
    BOOST_CHECK (res);
}

BOOST_DATA_TEST_CASE (value_federate_single_init_publish, bdata::make (core_types_single), core_type)
{
    SetupTest<helics::ValueFederate> (core_type, 1);
    auto vFed1 = GetFederateAs<helics::ValueFederate> (0);

    // register the publications
    auto &pubid = vFed1->registerGlobalPublication<double> ("pub1");

    auto &subid = vFed1->registerSubscription ("pub1");
    vFed1->setTimeProperty (TIME_DELTA_PROPERTY, 1.0);
    vFed1->enterInitializingMode ();
    vFed1->publish (pubid, 1.0);

    vFed1->enterExecutingMode ();
    // get the value set at initialization
    double val;
    vFed1->getValue (subid, val);
    BOOST_CHECK_EQUAL (val, 1.0);
    // publish string1 at time=0.0;
    vFed1->publish (pubid, 2.0);
    auto gtime = vFed1->requestTime (1.0);

    BOOST_CHECK_EQUAL (gtime, 1.0);

    // get the value
    vFed1->getValue (subid, val);
    // make sure the string is what we expect
    BOOST_CHECK_EQUAL (val, 2.0);
    // publish a second string
    vFed1->publish (pubid, 3.0);
    // make sure the value is still what we expect
    vFed1->getValue (subid, val);

    BOOST_CHECK_EQUAL (val, 2.0);
    // advance time
    gtime = vFed1->requestTime (2.0);
    // make sure the value was updated
    BOOST_CHECK_EQUAL (gtime, 2.0);
    vFed1->getValue (subid, val);

    BOOST_CHECK_EQUAL (val, 3.0);
    vFed1->finalize ();
}

BOOST_DATA_TEST_CASE (test_block_send_receive, bdata::make (core_types_single), core_type)
{
    SetupTest<helics::ValueFederate> (core_type, 1);
    auto vFed1 = GetFederateAs<helics::ValueFederate> (0);

    vFed1->registerPublication<std::string> ("pub1");
    vFed1->registerGlobalPublication<int> ("pub2");

    auto &pubid3 = vFed1->registerPublication ("pub3", "");

    auto &sub1 = vFed1->registerSubscription ("fed0/pub3", "");

    helics::data_block db (547, ';');

    vFed1->enterExecutingMode ();
    vFed1->publish (pubid3, db);
    vFed1->requestTime (1.0);
    BOOST_CHECK (vFed1->isUpdated (sub1));
    auto res = vFed1->getValueRaw (sub1);
    BOOST_CHECK_EQUAL (res.size (), db.size ());
    BOOST_CHECK (vFed1->isUpdated (sub1) == false);
}

/** test the all callback*/

BOOST_DATA_TEST_CASE (test_all_callback, bdata::make (core_types_single), core_type)
{
    SetupTest<helics::ValueFederate> (core_type, 1, 1.0);
    auto vFed1 = GetFederateAs<helics::ValueFederate> (0);

    auto &pubid1 = vFed1->registerPublication<std::string> ("pub1");
    auto &pubid2 = vFed1->registerGlobalPublication<int> ("pub2");

    auto &pubid3 = vFed1->registerPublication ("pub3", "");

    auto &sub1 = vFed1->registerSubscription ("fed0/pub1", "");
    auto &sub2 = vFed1->registerSubscription ("pub2", "");
    auto &sub3 = vFed1->registerSubscription ("fed0/pub3", "");

    helics::data_block db (547, ';');
    helics::interface_handle lastId;
    helics::Time lastTime;
    vFed1->registerInputNotificationCallback ([&](const helics::Input & subid, helics::Time callTime) {
        lastTime = callTime;
        lastId = subid.getHandle();
    });
    vFed1->enterExecutingMode ();
    vFed1->publish (pubid3, db);
    vFed1->requestTime (1.0);
    // the callback should have occurred here
    BOOST_CHECK (lastId == sub3);
    if (lastId == sub3)
    {
        BOOST_CHECK_EQUAL (lastTime, 1.0);
        BOOST_CHECK_EQUAL (vFed1->getLastUpdateTime (sub3), lastTime);
    }
    else
    {
        BOOST_FAIL (" missed callback\n");
    }

    vFed1->publish (pubid2, 4);
    vFed1->requestTime (2.0);
    // the callback should have occurred here
    BOOST_CHECK (lastId == sub2);
    BOOST_CHECK_EQUAL (lastTime, 2.0);
    vFed1->publish (pubid1, "this is a test");
    vFed1->requestTime (3.0);
    // the callback should have occurred here
    BOOST_CHECK (lastId == sub1);
    BOOST_CHECK_EQUAL (lastTime, 3.0);

    int ccnt = 0;
    vFed1->registerInputNotificationCallback ([&](const helics::Input &, helics::Time) { ++ccnt; });

    vFed1->publish (pubid3, db);
    vFed1->publish (pubid2, 4);
    vFed1->requestTime (4.0);
    // the callback should have occurred here
    BOOST_CHECK_EQUAL (ccnt, 2);
    ccnt = 0;  // reset the counter
    vFed1->publish (pubid3, db);
    vFed1->publish (pubid2, 4);
    vFed1->publish (pubid1, "test string2");
    vFed1->requestTime (5.0);
    // the callback should have occurred here
    BOOST_CHECK_EQUAL (ccnt, 3);
    vFed1->finalize ();
}

BOOST_AUTO_TEST_SUITE_END ()
