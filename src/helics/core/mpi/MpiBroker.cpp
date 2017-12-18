/*

Copyright (C) 2017, Battelle Memorial Institute
All rights reserved.

This software was co-developed by Pacific Northwest National Laboratory, operated by the Battelle Memorial
Institute; the National Renewable Energy Laboratory, operated by the Alliance for Sustainable Energy, LLC; and the
Lawrence Livermore National Laboratory, operated by Lawrence Livermore National Security, LLC.

*/
#include "MpiBroker.h"
#include "MpiComms.h"
#include "../../common/blocking_queue.h"
#include "helics/helics-config.h"
#include "../core-data.h"
#include "../core.h"
#include "../helics-time.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>

#include "../argParser.h"

static const std::string DEFAULT_BROKER = "tcp://localhost:5555";

namespace helics
{
using namespace std::string_literals;
static const argDescriptors extraArgs{{"brokerinit"s, "string"s, "the initialization string for the broker"s}};

MpiBroker::MpiBroker (bool rootBroker) noexcept : CommsBroker (rootBroker) {}

MpiBroker::MpiBroker (const std::string &broker_name) : CommsBroker (broker_name) {}

MpiBroker::~MpiBroker () {}

void MpiBroker::displayHelp (bool local_only)
{
    std::cout << " Help for MPI Broker: \n";
    namespace po = boost::program_options;
    po::variables_map vm;
    const char *const argV[] = {"", "--help"};
    argumentParser (2, argV, vm, extraArgs);
    if (!local_only)
    {
        CoreBroker::displayHelp ();
    }
}
void MpiBroker::initializeFromArgs (int argc, const char *const *argv)
{
    namespace po = boost::program_options;
    if (brokerState == broker_state_t::created)
    {
        po::variables_map vm;
        argumentParser (argc, argv, vm, extraArgs);

        if (vm.count ("broker") > 0)
        {
            auto brstring = vm["broker"].as<std::string> ();
            // tbroker = findTestBroker(brstring);
        }

        if (vm.count ("brokerinit") > 0)
        {
            // tbroker->Initialize(vm["brokerinit"].as<std::string>());
        }
        CoreBroker::initializeFromArgs (argc, argv);
    }
}

bool MpiBroker::brokerConnect ()
{
    comms = std::make_unique<MpiComms> ("", "");
    comms->setCallback ([this](ActionMessage M) { addActionMessage (std::move (M)); });
    // comms->setMessageSize(maxMessageSize, maxMessageCount);
    return comms->connect ();
}

std::string MpiBroker::getAddress () const { return ""; }
}  // namespace helics
