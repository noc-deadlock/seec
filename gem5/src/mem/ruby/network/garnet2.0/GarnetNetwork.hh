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


#ifndef __MEM_RUBY_NETWORK_GARNET2_0_GARNETNETWORK_HH__
#define __MEM_RUBY_NETWORK_GARNET2_0_GARNETNETWORK_HH__

#include <iostream>
#include <vector>

#include "mem/ruby/network/Network.hh"
#include "mem/ruby/network/fault_model/FaultModel.hh"
#include "mem/ruby/network/garnet2.0/CommonTypes.hh"
#include "params/GarnetNetwork.hh"

class FaultModel;
class NetworkInterface;
class Router;
class NetDest;
class NetworkLink;
class CreditLink;

class GarnetNetwork : public Network
{
  public:
    typedef GarnetNetworkParams Params;
    GarnetNetwork(const Params *p);

    ~GarnetNetwork();
    void init();

    // Configuration (set externally)

    // for 2D topology
    int getNumRows() const { return m_num_rows; }
    int getNumCols() { return m_num_cols; }

    // for network
    uint32_t getNiFlitSize() const { return m_ni_flit_size; }
    uint32_t getVCsPerVnet() const { return m_vcs_per_vnet; }
    uint32_t getBuffersPerDataVC() { return m_buffers_per_data_vc; }
    uint32_t getBuffersPerCtrlVC() { return m_buffers_per_ctrl_vc; }
    int getRoutingAlgorithm() const { return m_routing_algorithm; }

    bool isFaultModelEnabled() const { return m_enable_fault_model; }
    FaultModel* fault_model;


    // Internal configuration
    bool isVNetOrdered(int vnet) const { return m_ordered[vnet]; }
    VNET_type
    get_vnet_type(int vc)
    {
        int vnet = vc/getVCsPerVnet();
        return m_vnet_type[vnet];
    }
    int getNumRouters();
    int get_router_id(int ni);

    int get_upstreamId(PortDirection outport_dir, int upstream_id);
    Router* get_upstreamrouter(PortDirection outport_dir, int upstream_id);
    PortDirection get_upstreamOutportDirn(PortDirection outport_dir);


    // Methods used by Topology to setup the network
    void makeExtOutLink(SwitchID src, NodeID dest, BasicLink* link,
                     const NetDest& routing_table_entry);
    void makeExtInLink(NodeID src, SwitchID dest, BasicLink* link,
                    const NetDest& routing_table_entry);
    void makeInternalLink(SwitchID src, SwitchID dest, BasicLink* link,
                          const NetDest& routing_table_entry,
                          PortDirection src_outport_dirn,
                          PortDirection dest_inport_dirn);

    //! Function for performing a functional write. The return value
    //! indicates the number of messages that were written.
    uint32_t functionalWrite(Packet *pkt);

    // Stats
    void collateStats();
    void regStats();
    void print(std::ostream& out) const;

    // increment counters

    void update_flit_latency_histogram(Cycles& latency, int vnet) {

        m_flt_latency_hist.sample(latency);
        if(latency > max_flit_latency)
            max_flit_latency = latency;
        if (latency < min_flit_latency) {
            min_flit_latency = latency;
        }

    }

    void update_flit_network_latency_histogram(Cycles& latency, int vnet) {

        m_flt_network_latency_hist.sample(latency);
        if(latency > max_flit_network_latency)
            max_flit_network_latency = latency;
        if (latency < min_flit_network_latency) {
            min_flit_network_latency = latency;
        }

    }

    void update_flit_queueing_latency_histogram(Cycles& latency, int vnet) {

        m_flt_queueing_latency_hist.sample(latency);
        if(latency > max_flit_queueing_latency)
            max_flit_queueing_latency = latency;
        if (latency < min_flit_queueing_latency) {
            min_flit_queueing_latency = latency;
        }

    }

    void update_network_latency_histogram(Cycles latency) {
            uint64_t lat_ = uint64_t(latency);
            int index = lat_/5;
            if(index < 20){
                m_network_latency_histogram[index]++;
            } else {
                m_network_latency_histogram[20]++;
            }
    }

    void increment_injected_packets(int vnet) { m_packets_injected[vnet]++; }
    void increment_received_packets(int vnet, bool bufferless = false) {
        m_packets_received[vnet]++;
        if(bufferless) {
            m_bufferless_packets_received[vnet]++;
        }
        else {
            m_normal_packets_received[vnet]++;
        }
    }

    void
    increment_packet_network_latency(Cycles latency, int vnet,
                                                    bool bufferless = false)
    {
        m_packet_network_latency[vnet] += latency;
        if(bufferless) {
            // increment bufferless packet latency
            m_bufferless_packet_network_latency[vnet] += latency;
        }
        else {
            // increment normal packet latency
            m_normal_packet_network_latency[vnet] += latency;
        }
    }

    void
    increment_packet_queueing_latency(Cycles latency, int vnet,
                                                    bool bufferless = false)
    {
        m_packet_queueing_latency[vnet] += latency;
        if(bufferless) {
            // increment bufferless packet latency
            m_bufferless_packet_queueing_latency[vnet] += latency;
        }
        else {
            // increment normal packet latency
            m_normal_packet_queueing_latency[vnet] += latency;
        }
    }

    void increment_injected_flits(int vnet) { m_flits_injected[vnet]++; }
    void increment_received_flits(int vnet) {
        m_flits_received[vnet]++;

        // transfer all numbers to stat variable:
        m_max_flit_latency = max_flit_latency;
        m_max_flit_network_latency = max_flit_network_latency;
        m_max_flit_queueing_latency = max_flit_queueing_latency;

        m_min_flit_latency = min_flit_latency;
        m_min_flit_network_latency = min_flit_network_latency;
        m_min_flit_queueing_latency = min_flit_queueing_latency;

    }

    void
    increment_flit_network_latency(Cycles latency, int vnet)
    {
        m_flit_network_latency[vnet] += latency;
    }

    void
    increment_flit_queueing_latency(Cycles latency, int vnet)
    {
        m_flit_queueing_latency[vnet] += latency;
    }

    void
    increment_total_hops(int hops)
    {
        m_total_hops += hops;
    }


   uint32_t m_seec;
   uint32_t m_one_pkt_bufferless;
   uint32_t m_inj_single_vnet;
   uint32_t m_num_bufferless_pkt;
   int bufferless_routers;
   std::pair<int, Cycles> router_cycle;
   Stats::Scalar m_total_bufferless_latency;
   Stats::Scalar m_total_bufferless_pkt_normal_latency;

   Cycles max_flit_latency;
   Cycles max_flit_network_latency;
   Cycles max_flit_queueing_latency;

   Cycles min_flit_latency;
   Cycles min_flit_network_latency;
   Cycles min_flit_queueing_latency;

   Stats::Vector m_flt_dist;
   Stats::Vector m_network_latency_histogram;
   Stats::Scalar m_max_flit_latency;
   Stats::Scalar m_max_flit_network_latency;
   Stats::Scalar m_max_flit_queueing_latency;

   Stats::Scalar m_min_flit_latency;
   Stats::Scalar m_min_flit_network_latency;
   Stats::Scalar m_min_flit_queueing_latency;

   Stats::Histogram m_flt_latency_hist;
   Stats::Histogram m_flt_network_latency_hist;
   Stats::Histogram m_flt_queueing_latency_hist;


   // SEEC related Stats:
   Stats::Scalar m_bufferless_pkts;
   Stats::Scalar m_total_ff_hops;
   // Stats::Scalar m_total_normal_hops;
   // this is per VNet
   Stats::Vector m_bufferless_packets_received;
   Stats::Vector m_normal_packets_received;

   std::vector<NetworkInterface *> m_nis;   // All NI's in Network

  protected:
    // Configuration
    int m_num_rows;
    int m_num_cols;
    uint32_t m_ni_flit_size;
    uint32_t m_vcs_per_vnet;
    uint32_t m_buffers_per_ctrl_vc;
    uint32_t m_buffers_per_data_vc;
    int m_routing_algorithm;
    bool m_enable_fault_model;

    // Statistical variables
    Stats::Vector m_packets_received;
    Stats::Vector m_packets_injected;
    Stats::Vector m_packet_network_latency;
    Stats::Vector m_packet_queueing_latency;


    Stats::Vector m_bufferless_packet_network_latency;
    Stats::Vector m_bufferless_packet_queueing_latency;

    Stats::Vector m_normal_packet_network_latency;
    Stats::Vector m_normal_packet_queueing_latency;

    Stats::Formula m_avg_packet_vnet_latency;
    Stats::Formula m_avg_packet_vqueue_latency;
    Stats::Formula m_avg_packet_network_latency;
    Stats::Formula m_avg_packet_queueing_latency;
    Stats::Formula m_avg_packet_latency;

    // bufferless
    Stats::Formula m_avg_bufferless_packet_vnet_latency;
    Stats::Formula m_avg_bufferless_packet_vqueue_latency;
    Stats::Formula m_avg_bufferless_packet_network_latency;
    Stats::Formula m_avg_bufferless_packet_queueing_latency;
    Stats::Formula m_avg_bufferless_packet_latency;
    Stats::Formula m_avg_bufferless_network_latency; // this is the latency
                                                    // of packet in FF state
    Stats::Formula m_avg_bufferless_normal_network_latency;

    // normal
    Stats::Formula m_avg_normal_packet_vnet_latency;
    Stats::Formula m_avg_normal_packet_vqueue_latency;
    Stats::Formula m_avg_normal_packet_network_latency;
    Stats::Formula m_avg_normal_packet_queueing_latency;
    Stats::Formula m_avg_normal_packet_latency;


    Stats::Vector m_flits_received;
    Stats::Vector m_flits_injected;
    Stats::Vector m_flit_network_latency;
    Stats::Vector m_flit_queueing_latency;

    Stats::Formula m_avg_flit_vnet_latency;
    Stats::Formula m_avg_flit_vqueue_latency;
    Stats::Formula m_avg_flit_network_latency;
    Stats::Formula m_avg_flit_queueing_latency;
    Stats::Formula m_avg_flit_latency;

    Stats::Scalar m_total_ext_in_link_utilization;
    Stats::Scalar m_total_ext_out_link_utilization;
    Stats::Scalar m_total_int_link_utilization;
    Stats::Scalar m_average_link_utilization;
    Stats::Vector m_average_vc_load;

    Stats::Scalar  m_total_hops;
    Stats::Formula m_avg_hops;

  private:
    GarnetNetwork(const GarnetNetwork& obj);
    GarnetNetwork& operator=(const GarnetNetwork& obj);

    std::vector<VNET_type > m_vnet_type;
    std::vector<Router *> m_routers;   // All Routers in Network
    std::vector<NetworkLink *> m_networklinks; // All flit links in the network
    std::vector<CreditLink *> m_creditlinks; // All credit links in the network
};

inline std::ostream&
operator<<(std::ostream& out, const GarnetNetwork& obj)
{
    obj.print(out);
    out << std::flush;
    return out;
}

#endif //__MEM_RUBY_NETWORK_GARNET2_0_GARNETNETWORK_HH__
