/*
 * Copyright (c) 2008 Princeton University
 * Copyright (c) 2016 Georgia Institute of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Niket Agarwal
 *          Tushar Krishna
 */


#include "mem/ruby/network/garnet2.0/Router.hh"

#include "base/stl_helpers.hh"
#include "debug/RubyNetwork.hh"
#include "mem/ruby/network/garnet2.0/CreditLink.hh"
#include "mem/ruby/network/garnet2.0/CrossbarSwitch.hh"
#include "mem/ruby/network/garnet2.0/GarnetNetwork.hh"
#include "mem/ruby/network/garnet2.0/InputUnit.hh"
#include "mem/ruby/network/garnet2.0/NetworkLink.hh"
#include "mem/ruby/network/garnet2.0/OutputUnit.hh"
#include "mem/ruby/network/garnet2.0/RoutingUnit.hh"
#include "mem/ruby/network/garnet2.0/SwitchAllocator.hh"

using namespace std;
using m5::stl_helpers::deletePointers;

Router::Router(const Params *p)
    : BasicRouter(p), Consumer(this)
{
    m_latency = p->latency;
    m_virtual_networks = p->virt_nets;
    m_vc_per_vnet = p->vcs_per_vnet;
    m_num_vcs = m_virtual_networks * m_vc_per_vnet;

    m_routing_unit = new RoutingUnit(this);
    m_sw_alloc = new SwitchAllocator(this);
    m_switch = new CrossbarSwitch(this);

    m_input_unit.clear();
    m_output_unit.clear();

    // made_one_pkt_bufferless = false;
    m_bufferless_vnet_ptr = 0; // initialized the ptr to point to vnet-0
    num_bufferless_pkts = 0; // this is the current number of packets that a router
                             // has made bufferless so far
}

Router::~Router()
{
    deletePointers(m_input_unit);
    deletePointers(m_output_unit);
    delete m_routing_unit;
    delete m_sw_alloc;
    delete m_switch;
}

void
Router::init()
{
    BasicRouter::init();

    m_sw_alloc->init();
    m_switch->init();
    // make this as Big as number of input ports presents in the router.
    for(int ii=0; ii < m_input_unit.size(); ii++) {
        bufferless_inport_id.push_back(-1); // inport id from where packet was made bufferless
    }
    cout << "bufferless_inport_id.size() : " << bufferless_inport_id.size() << endl;
    assert (bufferless_inport_id.size() == m_input_unit.size());
}

int
Router::get_numFreeVC(PortDirection dirn_) {
    // Caution: This 'dirn_' is the direction of inport
    // of downstream router...
    assert(dirn_ != "Local");
    int inport_id = m_routing_unit->m_inports_dirn2idx[dirn_]; // ahh.. this is wrong!

//    cout << "------------------------------" << endl;
//    cout << "curCycle(): " << curCycle() << endl;
//    cout << "RouterId: " << m_id << endl;
//    cout << "[get_numFreeVC] in direction: " << dirn_ << endl;
//    cout << "[get_numFreeVC] inport_id: " << inport_id << endl;
//    cout << "------------------------------" << endl;
    return (m_input_unit[inport_id]->get_numFreeVC(dirn_));
}


void
Router::wakeup()
{
    DPRINTF(RubyNetwork, "Router %d woke up\n", m_id);

    // make packet bufferless in this turn
    bool my_turn = false;
    if (get_net_ptr()->m_seec == 1) {
        my_turn = myTurn();
        if (my_turn) {
            // make here provision of either injecting in a single VNet or in multi-VNet
            if (get_net_ptr()->m_inj_single_vnet == 1) {
                for (int inport = 0; inport < m_input_unit.size(); inport++) {
                    // int inport = rand() % m_input_unit.size();
                    // if (made_one_pkt_bufferless == false) {
                    if (num_bufferless_pkts < get_net_ptr()->m_num_bufferless_pkt) {
                        m_input_unit[inport]->make_pkt_bufferless(0);
                    }
                }
                DPRINTF(RubyNetwork, "num_bufferless_pkts: %d\n", num_bufferless_pkts);
            }
            else {
                // have VNet_pointer with each router and keep it updating in
                // round-robin fashion
                assert(get_net_ptr()->m_inj_single_vnet == 0);
                for (int inport = 0; inport < m_input_unit.size(); inport++) {
                    // int inport = rand() % m_input_unit.size();
                    if (num_bufferless_pkts < get_net_ptr()->m_num_bufferless_pkt) {
                        m_input_unit[inport]->make_pkt_bufferless(m_bufferless_vnet_ptr);
                    }
                }
                m_bufferless_vnet_ptr++;
                if (m_bufferless_vnet_ptr == m_virtual_networks) {
                    m_bufferless_vnet_ptr = 0;
                }
                DPRINTF(RubyNetwork, "num_bufferless_pkts: %d\n", num_bufferless_pkts);
                DPRINTF(RubyNetwork, "m_bufferless_vnet_ptr: %d\n", m_bufferless_vnet_ptr);
            }
            if((num_bufferless_pkts) && (m_network_ptr->router_cycle.first != -1)) {
                // If this router has made pkt(s) bufferless, then decrement
                // the bufferless router number for that cycle
                m_network_ptr->router_cycle.first--;
            }
        }
    }
     // assert(made_one_pkt_bufferless == false);


    // check for incoming flits
    for (int inport = 0; inport < m_input_unit.size(); inport++) {
        m_input_unit[inport]->wakeup();
    }

    // check for incoming credits
    // Note: the credit update is happening before SA
    // buffer turnaround time =
    //     credit traversal (1-cycle) + SA (1-cycle) + Link Traversal (1-cycle)
    // if we want the credit update to take place after SA, this loop should
    // be moved after the SA request
    for (int outport = 0; outport < m_output_unit.size(); outport++) {
        m_output_unit[outport]->wakeup();
    }

    // Switch Allocation
    m_sw_alloc->wakeup();

    // Switch Traversal
    m_switch->wakeup();


    // SEEC reset logic
    assert(num_bufferless_pkts <=  get_net_ptr()->m_num_bufferless_pkt);
    if (get_net_ptr()->m_seec == 1) {
        if (my_turn) {

            // reset the bufferless inport-ids for all the
            // inports that were made bufferless
            int ii;
            for (ii = 0; ii < m_input_unit.size(); ++ii) {
                bufferless_inport_id[ii] = -1;
            }
            // reset 'num_bufferless_pkts' to 0 for next turn
            num_bufferless_pkts = 0;
        }
    }

}

void
Router::addInPort(PortDirection inport_dirn,
                  NetworkLink *in_link, CreditLink *credit_link)
{
    int port_num = m_input_unit.size();
    InputUnit *input_unit = new InputUnit(port_num, inport_dirn, this);

    input_unit->set_in_link(in_link);
    input_unit->set_credit_link(credit_link);
    in_link->setLinkConsumer(this);
    credit_link->setSourceQueue(input_unit->getCreditQueue());

    m_input_unit.push_back(input_unit);

    m_routing_unit->addInDirection(inport_dirn, port_num);
}

void
Router::addOutPort(PortDirection outport_dirn,
                   NetworkLink *out_link,
                   const NetDest& routing_table_entry, int link_weight,
                   CreditLink *credit_link)
{
    int port_num = m_output_unit.size();
    OutputUnit *output_unit = new OutputUnit(port_num, outport_dirn, this);

    output_unit->set_out_link(out_link);
    output_unit->set_credit_link(credit_link);
    credit_link->setLinkConsumer(this);
    out_link->setSourceQueue(output_unit->getOutQueue());

    m_output_unit.push_back(output_unit);

    m_routing_unit->addRoute(routing_table_entry);
    m_routing_unit->addWeight(link_weight);
    m_routing_unit->addOutDirection(outport_dirn, port_num);
}

PortDirection
Router::getOutportDirection(int outport)
{
    return m_output_unit[outport]->get_direction();
}

PortDirection
Router::getInportDirection(int inport)
{
    return m_input_unit[inport]->get_direction();
}

int
Router::route_compute(RouteInfo route, int vc, int inport, PortDirection inport_dirn)
{
    return m_routing_unit->outportCompute(route, vc, inport, inport_dirn);
}

void
Router::grant_switch(int inport, flit *t_flit)
{
    m_switch->update_sw_winner(inport, t_flit);
}

void
Router::schedule_wakeup(Cycles time)
{
    // wake up after time cycles
    scheduleEvent(time);
}

std::string
Router::getPortDirectionName(PortDirection direction)
{
    // PortDirection is actually a string
    // If not, then this function should add a switch
    // statement to convert direction to a string
    // that can be printed out
    return direction;
}

void
Router::regStats()
{
    BasicRouter::regStats();

    m_buffer_reads
        .name(name() + ".buffer_reads")
        .flags(Stats::nozero)
    ;

    m_buffer_writes
        .name(name() + ".buffer_writes")
        .flags(Stats::nozero)
    ;

    m_crossbar_activity
        .name(name() + ".crossbar_activity")
        .flags(Stats::nozero)
    ;

    m_sw_input_arbiter_activity
        .name(name() + ".sw_input_arbiter_activity")
        .flags(Stats::nozero)
    ;

    m_sw_output_arbiter_activity
        .name(name() + ".sw_output_arbiter_activity")
        .flags(Stats::nozero)
    ;
}

void
Router::collateStats()
{
    for (int j = 0; j < m_virtual_networks; j++) {
        for (int i = 0; i < m_input_unit.size(); i++) {
            m_buffer_reads += m_input_unit[i]->get_buf_read_activity(j);
            m_buffer_writes += m_input_unit[i]->get_buf_write_activity(j);
        }
    }

    m_sw_input_arbiter_activity = m_sw_alloc->get_input_arbiter_activity();
    m_sw_output_arbiter_activity = m_sw_alloc->get_output_arbiter_activity();
    m_crossbar_activity = m_switch->get_crossbar_activity();
}

void
Router::resetStats()
{
    for (int j = 0; j < m_virtual_networks; j++) {
        for (int i = 0; i < m_input_unit.size(); i++) {
            m_input_unit[i]->resetStats();
        }
    }

    m_switch->resetStats();
    m_sw_alloc->resetStats();
}

void
Router::printFaultVector(ostream& out)
{
    int temperature_celcius = BASELINE_TEMPERATURE_CELCIUS;
    int num_fault_types = m_network_ptr->fault_model->number_of_fault_types;
    float fault_vector[num_fault_types];
    get_fault_vector(temperature_celcius, fault_vector);
    out << "Router-" << m_id << " fault vector: " << endl;
    for (int fault_type_index = 0; fault_type_index < num_fault_types;
         fault_type_index++) {
        out << " - probability of (";
        out <<
        m_network_ptr->fault_model->fault_type_to_string(fault_type_index);
        out << ") = ";
        out << fault_vector[fault_type_index] << endl;
    }
}

void
Router::printAggregateFaultProbability(std::ostream& out)
{
    int temperature_celcius = BASELINE_TEMPERATURE_CELCIUS;
    float aggregate_fault_prob;
    get_aggregate_fault_probability(temperature_celcius,
                                    &aggregate_fault_prob);
    out << "Router-" << m_id << " fault probability: ";
    out << aggregate_fault_prob << endl;
}

uint32_t
Router::functionalWrite(Packet *pkt)
{
    uint32_t num_functional_writes = 0;
    num_functional_writes += m_switch->functionalWrite(pkt);

    for (uint32_t i = 0; i < m_input_unit.size(); i++) {
        num_functional_writes += m_input_unit[i]->functionalWrite(pkt);
    }

    for (uint32_t i = 0; i < m_output_unit.size(); i++) {
        num_functional_writes += m_output_unit[i]->functionalWrite(pkt);
    }

    return num_functional_writes;
}

Router *
GarnetRouterParams::create()
{
    return new Router(this);
}
