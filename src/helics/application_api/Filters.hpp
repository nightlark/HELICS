/*
Copyright © 2017-2018,
Battelle Memorial Institute; Lawrence Livermore National Security, LLC; Alliance for Sustainable Energy, LLC
All rights reserved. See LICENSE file and DISCLAIMER for more details.
*/
#pragma once

#include "../core/Core.hpp"
#include "Federate.hpp"
#include "helics/helics-config.h"

namespace helics
{
class FilterOperations;

/** a set of common defined filters*/
enum class defined_filter_types
{
    custom = 0,
    delay = 1,
    randomDelay = 2,
    randomDrop = 3,
    reroute = 4,
    clone = 5,
	firewall=6,
    unrecognized = 7

};

#define EMPTY_STRING std::string ()

/** get the filter type from a string*/
defined_filter_types filterTypeFromString (const std::string &filterType) noexcept;

/** class for managing a particular filter*/
class Filter
{
  protected:
    Core *corePtr = nullptr;  //!< the Core to use
    interface_handle id;  //!< the id as generated by the Federate
    filter_id_t fid = invalid_id_value;  //!< id for interacting with a federate
  private:
    std::shared_ptr<FilterOperations> filtOp;  //!< a class running any specific operation of the Filter
  public:
    /** default constructor*/
    Filter () = default;
    /** construct through a federate*/
    explicit Filter (Federate *fed, const std::string &name = EMPTY_STRING);
    /** construct through a federate*/
    Filter(interface_visibility locality, Federate *fed, const std::string &name = EMPTY_STRING);
    /** construct through a core object*/
    explicit Filter (Core *cr, const std::string &name = EMPTY_STRING);

    /** generate filter through its index*/
    Filter (Federate *fed, int filtInd);
    /** generate filter through its index*/
    Filter (Core *cr, int filtInd);
    /** virtual destructor*/
    virtual ~Filter () = default;

    /** set a message operator to process the message*/
    void setOperator (std::shared_ptr<FilterOperator> mo);

    /** get the underlying filter id for use with a federate*/
    filter_id_t getID () const { return fid; }

    /** get the underlying core handle for use with a core*/
    interface_handle getCoreHandle () const { return id; }
    /** get the name for the filter*/
    const std::string &getName () const;
    /** get the specified input type of the filter*/
    const std::string &getInputType () const;
    /** get the specified output type of the filter*/
    const std::string &getOutputType () const;
    /** set a property on a filter
    @param property the name of the property of the filter to change
    @param val the numerical value of the property
    */
    virtual void set (const std::string &property, double val);
    /** set a string property on a filter
    @param property the name of the property of the filter to change
    @param val the numerical value of the property
    */
    virtual void setString (const std::string &property, const std::string &val);
    /** add a sourceEndpoint to the list of endpoint to clone*/
    virtual void addSourceTarget (const std::string &sourceName);
    /** add a destination endpoint to the list of endpoints to clone*/
    virtual void addDestinationTarget (const std::string &destinationName);

    /** remove a sourceEndpoint to the list of endpoint to clone*/
    virtual void removeTarget (const std::string &sourceName);

  protected:
    /** set a filter operations object */
    void setFilterOperations (std::shared_ptr<FilterOperations> filterOps);
    friend void addOperations (Filter *filt, defined_filter_types type, Core *cptr);
};

/** class used to clone message for delivery to other endpoints*/
class CloningFilter : public Filter
{
  public:
    /** default constructor*/
    CloningFilter () = default;
    /** construct from a core object
     */
    explicit CloningFilter (Core *cr, const std::string &name = EMPTY_STRING);
    /** construct from a Federate
     */
    explicit CloningFilter (Federate *fed, const std::string &name = EMPTY_STRING);
    /** construct from a Federate
    */
    explicit CloningFilter(interface_visibility locality, Federate *fed, const std::string &name = EMPTY_STRING);
    
    /** generate a cloningfilter through its index*/
    CloningFilter (Federate *fed, int filtInd) : Filter (fed, filtInd) {}
    /** generate cloning filter through its index*/
    CloningFilter (Core *cr, int filtInd) : Filter (cr, filtInd) {}

    /** add a delivery address this is the name of an endpoint to deliver the message to*/
    void addDeliveryEndpoint (const std::string &endpoint);

    /** remove a delivery address this is the name of an endpoint to deliver the message to*/
    void removeDeliveryEndpoint (const std::string &endpoint);

    virtual void setString (const std::string &property, const std::string &val) override;

  private:
    std::vector<std::string> destEndpoints;  //!< the names of the destination endpoints
};

/** create a  filter
@param type the type of filter to create
@param fed the federate to create the filter through
@param target the target endpoint all message with the specified target as a destination will route through the
filter
@param name the name of the filter (optional)
@return a unique pointer to a destination Filter object,  note destroying the object does not deactivate the filter
*/
std::unique_ptr<Filter>
make_filter (defined_filter_types type, Federate *fed, const std::string &name = EMPTY_STRING);

/** create a  filter
@param type the type of filter to create
@param fed the federate to create the filter through
@param target the target endpoint all message with the specified target as a destination will route through the
filter
@param name the name of the filter (optional)
@return a unique pointer to a destination Filter object,  note destroying the object does not deactivate the filter
*/
std::unique_ptr<Filter>
make_filter(interface_visibility locality, defined_filter_types type, Federate *fed, const std::string &name = EMPTY_STRING);

/** create a filter
@param type the type of filter to create
@param cr the core to create the filter through
@param target the target endpoint all message coming from the specified source will route through the filter
@param name the name of the filter (optional)
@return a unique pointer to a source Filter object,  note destroying the object does not deactivate the filter
*/
std::unique_ptr<Filter> make_filter (defined_filter_types type, Core *cr, const std::string &name = EMPTY_STRING);

/** create a  filter
@param type the type of filter to create
@param fed the federate to create the filter through
@param target the target endpoint all message with the specified target as a destination will route through the
filter
@param name the name of the filter (optional)
@return a unique pointer to a destination Filter object,  note destroying the object does not deactivate the filter
*/
std::unique_ptr<CloningFilter> make_cloning_filter (defined_filter_types type,
                                                    Federate *fed,
                                                    const std::string &delivery,
                                                    const std::string &name = EMPTY_STRING);

/** create a  filter
@param type the type of filter to create
@param fed the federate to create the filter through
@param target the target endpoint all message with the specified target as a destination will route through the
filter
@param name the name of the filter (optional)
@return a unique pointer to a destination Filter object,  note destroying the object does not deactivate the filter
*/
std::unique_ptr<CloningFilter> make_cloning_filter(interface_visibility locality, defined_filter_types type,
    Federate *fed,
    const std::string &delivery,
    const std::string &name = EMPTY_STRING);

/** create a filter
@param type the type of filter to create
@param cr the core to create the filter through
@param target the target endpoint all message coming from the specified source will route through the filter
@param name the name of the filter (optional)
@return a unique pointer to a source Filter object,  note destroying the object does not deactivate the filter
*/
std::unique_ptr<CloningFilter> make_cloning_filter (defined_filter_types type,
                                                    Core *cr,
                                                    const std::string &delivery,
                                                    const std::string &name = EMPTY_STRING);

}  // namespace helics
