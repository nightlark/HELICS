/*

Copyright (C) 2017, Battelle Memorial Institute
All rights reserved.

This software was co-developed by Pacific Northwest National Laboratory, operated by the Battelle Memorial
Institute; the National Renewable Energy Laboratory, operated by the Alliance for Sustainable Energy, LLC; and the
Lawrence Livermore National Laboratory, operated by Lawrence Livermore National Security, LLC.

*/
#ifndef TIME_COORDINATOR_H_
#define TIME_COORDINATOR_H_
#pragma once

#include "core/DependencyInfo.h"
#include "ActionMessage.h"
#include <atomic>
#include <functional>

namespace helics
{

class TimeCoordinator
{
private:
	// the variables for time coordination
	Time time_granted = Time::minVal();  //!< the most recent time granted
	Time time_requested = Time::maxVal();  //!< the most recent time requested
	Time time_next = timeZero;  //!< the next possible internal event time
	Time time_minminDe = timeZero;  //!< the minimum  of the minimum dependency event Time
	Time time_minDe = timeZero;  //!< the minimum event time of the dependencies
	Time time_allow = Time::minVal();  //!< the current allowable time 
	Time time_exec = Time::maxVal();  //!< the time of the next targetted execution
	Time time_message = Time::maxVal();	//!< the time of the earliest message event
	Time time_value = Time::maxVal();	//!< the time of the earliest value event

	std::vector<DependencyInfo> dependencies;  // federates which this Federate is temporally dependent on
	std::vector<Core::federate_id_t> dependents;  // federates which temporally depend on this federate

	CoreFederateInfo info;  //!< basic federate info the core uses
	std::function<void(const ActionMessage &)> sendMessageFunction;
public:
	Core::federate_id_t source_id;  //!< the identifier for inserting into the source id field of any generated messages;
	bool iterating=false; //!< indicator that the coordinator should be iterating if need be
	bool checkingExec = false; //!< flag indicating that the coordinator is trying to enter the exec mode
	bool executionMode = false;	//!< flag that the coordinator has entered the execution Mode
	
private:
	std::atomic<int32_t> iteration{ 0 };  //!< iteration counter
public:
	TimeCoordinator() = default;
	TimeCoordinator(const CoreFederateInfo &info_);

	CoreFederateInfo &getFedInfo()
	{
		return info;
	}

	const CoreFederateInfo &getFedInfo() const
	{
		return info;
	}

	void setInfo(const CoreFederateInfo &info_)
	{
		info = info_;
	}
	void setMessageSender(std::function<void(const ActionMessage &)> sendMessageFunction_)
	{
		sendMessageFunction = std::move(sendMessageFunction_);
	}

	Time getGrantedTime() const
	{
		return time_granted;
	}

	const std::vector<Core::federate_id_t> &getDependents() const { return dependents; }
	/** get the current iteration counter for an iterative call
	@details this will work properly even when a federate is processing
	*/
	int32_t getCurrentIteration() const { return iteration; }
	/** compute updates to time values
	@return true if they have been modified
	*/
	bool updateTimeFactors();
	/** update the time_value variable with a new value if needed
	*/
	void updateValueTime(Time valueUpdateTime);
	/** update the time_message variable with a new value if needed
	*/
	void updateMessageTime(Time messageUpdateTime);

	/** take a global id and get a pointer to the dependencyInfo for the other fed
	will be nullptr if it doesn't exist
	*/
	DependencyInfo *getDependencyInfo(Core::federate_id_t ofed);
	/** check whether a federate is a dependency*/
	bool isDependency(Core::federate_id_t ofed) const;

private:
	/** helper function for computing the next event time*/
	void updateNextExecutionTime();
	/** helper function for computing the next possible time to generate an external event
	*/
	void updateNextPossibleEventTime();
public:
	/** process a message related to time
	@return true if it did anything
	*/
	bool processExternalTimeMessage(ActionMessage &cmd);

	/** add a federate dependency
	@return true if it was actually added, false if the federate was already present
	*/
	bool addDependency(Core::federate_id_t fedID);
	/** add a dependent federate
	@return true if it was actually added, false if the federate was already present
	*/
	bool addDependent(Core::federate_id_t fedID);

	void removeDependency(Core::federate_id_t fedID);
	void removeDependent(Core::federate_id_t fedID);

	/** process a message related to exec request
	@return true if it did anything
	*/
	bool processExecRequest(ActionMessage &cmd);
	/** check if entry to the executing state can be granted*/
	convergence_state checkExecEntry();
	

	void timeRequest(Time nextTime, convergence_state converged, Time newValueTime, Time newMessageTime);
	void enteringExecMode(convergence_state mode);
	/** check if it is valid to grant a time*/
	convergence_state checkTimeGrant();
};
}

#endif