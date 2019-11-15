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


#include "mem/ruby/network/garnet2.0/InputUnit.hh"

#include "base/stl_helpers.hh"
#include "debug/RubyNetwork.hh"
#include "mem/ruby/network/garnet2.0/Credit.hh"
#include "mem/ruby/network/garnet2.0/Router.hh"
#include "mem/ruby/network/garnet2.0/NetworkInterface.hh"


using namespace std;
using m5::stl_helpers::deletePointers;

InputUnit::InputUnit(int id, PortDirection direction, Router *router)
            : Consumer(router)
{
    m_id = id;
    m_direction = direction;
    m_router = router;
    m_num_vcs = m_router->get_num_vcs();
    m_vc_per_vnet = m_router->get_vc_per_vnet();

    m_num_buffer_reads.resize(m_num_vcs/m_vc_per_vnet);
    m_num_buffer_writes.resize(m_num_vcs/m_vc_per_vnet);
    for (int i = 0; i < m_num_buffer_reads.size(); i++) {
        m_num_buffer_reads[i] = 0;
        m_num_buffer_writes[i] = 0;
    }

    creditQueue = new flitBuffer();
    // Instantiating the virtual channels
    m_vcs.resize(m_num_vcs);
    for (int i=0; i < m_num_vcs; i++) {
        m_vcs[i] = new VirtualChannel(i);
    }
}

InputUnit::~InputUnit()
{
    delete creditQueue;
    deletePointers(m_vcs);
}

/*
 * The InputUnit wakeup function reads the input flit from its input link.
 * Each flit arrives with an input VC.
 * For HEAD/HEAD_TAIL flits, performs route computation,
 * and updates route in the input VC.
 * The flit is buffered for (m_latency - 1) cycles in the input VC
 * and marked as valid for SwitchAllocation starting that cycle.
 *
 */

void
InputUnit::wakeup()
{
    flit *t_flit;
    if (m_in_link->isReady(m_router->curCycle())) {

        t_flit = m_in_link->consumeLink();
        int vc = t_flit->get_vc();
        t_flit->increment_hops(); // for stats

        if ((t_flit->get_type() == HEAD_) ||
            (t_flit->get_type() == HEAD_TAIL_)) {

            assert(m_vcs[vc]->get_state() == IDLE_);
            set_vc_active(vc, m_router->curCycle());

            // Route computation for this vc
            int outport = m_router->route_compute(t_flit->get_route(),
                m_id, m_direction);

            // Update output port in VC
            // All flits in this packet will use this output port
            // The output port field in the flit is updated after it wins SA
            grant_outport(vc, outport);

        } else {
            assert(m_vcs[vc]->get_state() == ACTIVE_);
        }


        // Buffer the flit
        m_vcs[vc]->insertFlit(t_flit);

        int vnet = vc/m_vc_per_vnet;
        // number of writes same as reads
        // any flit that is written will be read only once
        m_num_buffer_writes[vnet]++;
        m_num_buffer_reads[vnet]++;

        Cycles pipe_stages = m_router->get_pipe_stages();
        if (pipe_stages == 1) {
            // 1-cycle router
            // Flit goes for SA directly
            t_flit->advance_stage(SA_, m_router->curCycle());
        } else {
            assert(pipe_stages > 1);
            // Router delay is modeled by making flit wait in buffer for
            // (pipe_stages cycles - 1) cycles before going for SA

            Cycles wait_time = pipe_stages - Cycles(1);
            t_flit->advance_stage(SA_, m_router->curCycle() + wait_time);

            // Wakeup the router in that cycle to perform SA
            m_router->schedule_wakeup(Cycles(wait_time));
        }
    }
}


// this function returns true when it transfers the pkt bufferless-ly
bool
InputUnit::make_pkt_bufferless(int vnet) {

    // calcubate VC-base from the VNet
    // look for the flit present in the base-VC of that VNet
    int vc_base = vnet * m_vc_per_vnet;
    flit *t_flit;
    if (m_vcs[vc_base]->isEmpty()) {
        return false;
    } else {
        // remove the flit because you are sure to
        // eject and inject into the destination NI
        t_flit = m_vcs[vc_base]->getTopFlit();
    }

    // SEEC code:
    // Insert the flit here in the respective NI buffer for it
    // to get consumed add updates the latency and hop apporpriately.
    if (m_router->m_network_ptr->m_seec == 1 ) { // if 'SEEC' is set true
        if (m_router->m_network_ptr->m_one_pkt_bufferless == 1) {
            // check if it is the turn of this router:
            if((m_router->curCycle() % (m_router->get_net_ptr()->getNumRouters()) == m_router->get_id())
                && m_router->made_one_pkt_bufferless == false /*&&
                m_direction != "Local"*/) {
                DPRINTF(RubyNetwork, "[InputUnit::make_pkt_bufferless()] "\
                        "Inport direction for which we are ejecting packet: %s\n", m_direction);
                cout << *t_flit << endl;
                // int num_cols = m_router->get_net_ptr()->getNumCols();
                int hop_traversed = -1;
                int num_cols = m_router->get_net_ptr()->getNumCols();
                int my_router_id = m_router->get_id();
                int my_x = my_router_id % num_cols;
                int my_y = my_router_id / num_cols;

                int dest_router_id = t_flit->m_route.dest_router;
                int dest_ni = t_flit->m_route.dest_ni;
                int dest_x = dest_router_id % num_cols;
                int dest_y = dest_router_id / num_cols;
                assert(dest_ni != -1);

                int x_hops = abs(dest_x - my_x);
                int y_hops = abs(dest_y - my_y);

                hop_traversed = x_hops + y_hops;

                if (my_router_id == dest_router_id) {
                    hop_traversed = 1;
                }
                t_flit->m_route.hops_traversed += hop_traversed;
                // update the
                int latency = 2*hop_traversed; // assume: 1-cycle link and 1-cycle router

                assert(latency >= 0);
                // put the flit in the NI special buffer for it to be ejected using
                // NI's consume_bufferless_pkt() API
                /*
                m_router->m_network_ptr->m_nis[dest_ni]->\
                            m_bufferless_pkt->insert(t_flit);

                m_router->m_network_ptr->m_nis[dest_ni]->\
                            consume_bufferless_pkt(latency);
                */
                m_router->made_one_pkt_bufferless = true;
                // update the credits for upstream router here
                // update the state of outVC at upstream router
                increment_credit(vc_base, true, m_router->curCycle());
                // after ejecting the packet from this VC make it IDLE
                set_vc_idle(vc_base, m_router->curCycle());
                // int vnet = vc/m_vc_per_vnet;
                // number of writes same as reads
                // any flit that is written will be read only once
                // m_router->get_net_ptr()->m_num_bufferless[vnet]++;
                // m_num_buffer_writes[vnet]++;
                // m_num_buffer_reads[vnet]++;

                // return true
                // you have made the successful ejection of packet
                return true;
            }
        }
    }


    return false;
}


// Send a credit back to upstream router for this VC.
// Called by SwitchAllocator when the flit in this VC wins the Switch.
void
InputUnit::increment_credit(int in_vc, bool free_signal, Cycles curTime)
{
    Credit *t_credit = new Credit(in_vc, free_signal, curTime);
    creditQueue->insert(t_credit);
    m_credit_link->scheduleEventAbsolute(m_router->clockEdge(Cycles(1)));
}


uint32_t
InputUnit::functionalWrite(Packet *pkt)
{
    uint32_t num_functional_writes = 0;
    for (int i=0; i < m_num_vcs; i++) {
        num_functional_writes += m_vcs[i]->functionalWrite(pkt);
    }

    return num_functional_writes;
}

void
InputUnit::resetStats()
{
    for (int j = 0; j < m_num_buffer_reads.size(); j++) {
        m_num_buffer_reads[j] = 0;
        m_num_buffer_writes[j] = 0;
    }
}
