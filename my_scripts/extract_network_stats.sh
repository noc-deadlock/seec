echo > network_stats.txt
grep "packets_injected::total" m5out/stats.txt | sed 's/system.ruby.network.packets_injected::total\s*/packets_injected = /' >> network_stats.txt
grep "packets_received::total" m5out/stats.txt | sed 's/system.ruby.network.packets_received::total\s*/packets_received = /' >> network_stats.txt
grep "average_packet_queueing_latency" m5out/stats.txt | sed 's/system.ruby.network.average_packet_queueing_latency\s*/average_packet_queueing_latency = /' >> network_stats.txt
grep "average_packet_network_latency" m5out/stats.txt | sed 's/system.ruby.network.average_packet_network_latency\s*/average_packet_network_latency = /' >> network_stats.txt
grep "average_packet_latency" m5out/stats.txt | sed 's/system.ruby.network.average_packet_latency\s*/average_packet_latency = /' >> network_stats.txt
grep "flits_injected::total" m5out/stats.txt | sed 's/system.ruby.network.flits_injected::total\s*/flits_injected = /' >> network_stats.txt
grep "flits_received::total" m5out/stats.txt | sed 's/system.ruby.network.flits_received::total\s*/flits_received = /' >> network_stats.txt
grep "average_flit_queueing_latency" m5out/stats.txt | sed 's/system.ruby.network.average_flit_queueing_latency\s*/average_flit_queueing_latency = /' >> network_stats.txt
grep "average_flit_network_latency" m5out/stats.txt | sed 's/system.ruby.network.average_flit_network_latency\s*/average_flit_network_latency = /' >> network_stats.txt
grep "average_flit_latency" m5out/stats.txt | sed 's/system.ruby.network.average_flit_latency\s*/average_flit_latency = /' >> network_stats.txt
grep "average_hops" m5out/stats.txt | sed 's/system.ruby.network.average_hops\s*/average_hops = /' >> network_stats.txt
grep "total_uturn" m5out/stats.txt | sed 's/system.ruby.network.total_uturn\s*/total_uTurn = /' >> network_stats.txt
grep "total_misroute" m5out/stats.txt | sed 's/system.ruby.network.total_misroute\s*/total_misroute = /' >> network_stats.txt
grep "total_spins" m5out/stats.txt | sed 's/system.ruby.network.total_spins\s*/total_spins = /' >> network_stats.txt
grep "misroute_per_pkt" m5out/stats.txt | sed 's/system.ruby.network.misroute_per_pkt\s*/misroute_per_packet = /' >> network_stats.txt
grep "total_uturn_request" m5out/stats.txt | sed 's/system.ruby.network.total_uturn_request\s*/total_uturn_request = /' >> network_stats.txt
grep "total_successful_uturns" m5out/stats.txt | sed 's/system.ruby.network.total_successful_uturns\s*/total_successful_uturns = /' >> network_stats.txt