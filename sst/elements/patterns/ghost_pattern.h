// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _GHOST_PATTERN_H
#define _GHOST_PATTERN_H

#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include "pattern_common.h"

using namespace SST;

#define DBG_GHOST_PATTERN 1
#if DBG_GHOST_PATTERN
#define _GHOST_PATTERN_DBG(lvl, fmt, args...)\
    if (ghost_pattern_debug >= lvl)   { \
	printf("%d:Ghost_pattern::%s():%d: " fmt, _debug_rank, __FUNCTION__, __LINE__, ## args); \
    }
#else
#define _GHOST_PATTERN_DBG(lvl, fmt, args...)
#endif

typedef enum {INIT, COMPUTE, WAIT, DONE} state_t;

class Ghost_pattern : public Component {
    public:
        Ghost_pattern(ComponentId_t id, Params_t& params) :
            Component(id),
            params(params)
        { 

            Params_t::iterator it= params.begin();
	    // Defaults
	    ghost_pattern_debug= 0;
	    latency= -1.0;
	    bandwidth= -1.0;
	    compute_time= -1.0;
	    exchange_msg_len= 128;
	    rcv_cnt= 0;
	    state= INIT;

            while (it != params.end())   {
                _GHOST_PATTERN_DBG(1, "Ghost: key=%s value=%s\n", it->first.c_str(), it->second.c_str());

		if (!it->first.compare("debug"))   {
		    sscanf(it->second.c_str(), "%d", &ghost_pattern_debug);
		}

		if (!it->first.compare("rank"))   {
		    sscanf(it->second.c_str(), "%d", &my_rank);
		}

		if (!it->first.compare("x_dim"))   {
		    sscanf(it->second.c_str(), "%d", &x_dim);
		}

		if (!it->first.compare("y_dim"))   {
		    sscanf(it->second.c_str(), "%d", &y_dim);
		}

		if (!it->first.compare("latency"))   {
		    sscanf(it->second.c_str(), "%lf", &latency);
		}

		if (!it->first.compare("bandwidth"))   {
		    sscanf(it->second.c_str(), "%lf", &bandwidth);
		}

		if (!it->first.compare("compute_time"))   {
		    sscanf(it->second.c_str(), "%lf", &compute_time);
		}

		if (!it->first.compare("exchange_msg_len"))   {
		    sscanf(it->second.c_str(), "%d", &exchange_msg_len);
		}

                ++it;
            }


            // Create a handler for events
	    net= configureLink("NETWORK", new Event::Handler<Ghost_pattern>
		    (this, &Ghost_pattern::handle_events));
	    if (net == NULL)   {
		_GHOST_PATTERN_DBG(0, "The ghost pattern generator expects a link to the network "
		    "named \"Network\" which is missing!\n");
		_ABORT(Ghost_pattern, "Check the input XML file!\n");
	    } else   {
		_GHOST_PATTERN_DBG(1, "Added a link and a handler for the network\n");
	    }

	    // Create a time converter
	    tc= registerTimeBase("1ns", true);


	    // Initialize the common functions we need
	    common= new Patterns();
	    if (!common->init(x_dim, y_dim, my_rank, net, latency, bandwidth))   {
		_ABORT(Ghost_pattern, "Patterns->init() failed!\n");
	    }

	    /* Who are my four neighbors? */
	    int myX= my_rank % x_dim;
	    int myY= my_rank / y_dim;
	    right= ((myX + 1) % x_dim) + (myY * y_dim);
	    left= ((myX - 1 + x_dim) % x_dim) + (myY * y_dim);
	    down= myX + ((myY + 1) % y_dim) * y_dim;
	    up= myX + ((myY - 1 + y_dim) % y_dim) * y_dim;

	    // Send a start event to ourselves without a delay
	    common->event_send(my_rank, START);
        }

    private:

        Ghost_pattern(const Ghost_pattern &c);
	void handle_events(Event *);
	Patterns *common;

	int my_rank;
	int x_dim;
	int y_dim;
	double latency;
	double bandwidth;
	double compute_time;
	int exchange_msg_len;
	state_t state;
	int left, right, up, down;
	int rcv_cnt;
	int ghost_pattern_debug;

        Params_t params;
	Link *net;
	TimeConverter *tc;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    ar & BOOST_SERIALIZATION_NVP(my_rank);
	    ar & BOOST_SERIALIZATION_NVP(x_dim);
	    ar & BOOST_SERIALIZATION_NVP(y_dim);
	    ar & BOOST_SERIALIZATION_NVP(latency);
	    ar & BOOST_SERIALIZATION_NVP(bandwidth);
	    ar & BOOST_SERIALIZATION_NVP(compute_time);
	    ar & BOOST_SERIALIZATION_NVP(exchange_msg_len);
	    ar & BOOST_SERIALIZATION_NVP(state);
	    ar & BOOST_SERIALIZATION_NVP(left);
	    ar & BOOST_SERIALIZATION_NVP(right);
	    ar & BOOST_SERIALIZATION_NVP(up);
	    ar & BOOST_SERIALIZATION_NVP(down);
	    ar & BOOST_SERIALIZATION_NVP(rcv_cnt);
	    ar & BOOST_SERIALIZATION_NVP(ghost_pattern_debug);
	    ar & BOOST_SERIALIZATION_NVP(net);
	    ar & BOOST_SERIALIZATION_NVP(tc);
        }

        template<class Archive>
        friend void save_construct_data(Archive & ar, 
                                        const Ghost_pattern * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Ghost_pattern,"\n");
            ComponentId_t     id     = t->getId();
            Params_t          params = t->params;
            ar << BOOST_SERIALIZATION_NVP(id);
            ar << BOOST_SERIALIZATION_NVP(params);
        } 

        template<class Archive>
        friend void load_construct_data(Archive & ar, 
                                        Ghost_pattern * t, 
                                        const unsigned int file_version)
        {
            _AR_DBG(Ghost_pattern,"\n");
            ComponentId_t     id;
            Params_t          params;
            ar >> BOOST_SERIALIZATION_NVP(id);
            ar >> BOOST_SERIALIZATION_NVP(params);
            ::new(t)Ghost_pattern(id, params);
        } 
};

#endif // _GHOST_PATTERN_H
