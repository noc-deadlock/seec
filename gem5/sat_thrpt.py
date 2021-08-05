#!/usr/bin/env python2
import os
import subprocess
# import pdb; pdb.set_trace()
# first compile then run
binary = 'build/Garnet_standalone/gem5.opt'
os.system("scons -j15 {}".format(binary))


bench_caps=[ "BIT_ROTATION", "SHUFFLE", "TRANSPOSE" ]
bench=[ "bit_rotation", "shuffle", "transpose" ]

# bench_caps=[ "BIT_ROTATION", "SHUFFLE" ]
# bench=[ "bit_rotation", "shuffle" ]

routing_algorithm=[ 'TABLE', 'XY', 'RANDOM', 'ADAPT_RAND', 'WestFirst', 'ESCAPE_VC' ]

num_cores = [16, 64, 256]
num_rows = [4, 8, 16]
mode=['SEEC', 'mSEEC']

# os.system('rm -rf ./results_sat_thrpt')
# os.system('mkdir results_sat_thrpt')

out_dir = './results_sat_thrpt'

def bufferless_router(m_, core_count):
	if m_ == 0:
		return int(1)
	elif m_ == 1:
		if core_count == 64:
			return int(8)
		elif core_count == 256:
			return int(16)
	else:
		assert(0)

cycles = 10000
vnet = 0
tr = 1
vc_ = [1, 2, 4]  # make this a list
sat_thrpt = []
rout_ = 3
for m in range(len(mode)):
	for c in range(len(num_cores)):
		for b in range(len(bench)):
			for v in range(len(vc_)):
				print ("cores: {} benchmark: {} vc-{}".format(num_cores[c], bench_caps[b], vc_[v]))
				pkt_lat = 0
				injection_rate = 0.02
				low_load_latency = 0.0
				while (pkt_lat < 500.00):
					############ gem5 command-line ###########
					os.system("{0:s} -d {1:s}/{2:d}/{3:s}/{5:s}/{4:s}/vc-{6:d}/inj-{7:1.2f} configs/example/garnet_synth_traffic.py --topology=Mesh_XY --num-cpus={2:d} --num-dirs={2:d} --mesh-row={8:d} --inj-vnet={9:d} --network=garnet2.0 --router-latency=1 --sim-cycles={10:d} --seec=1 --one-pkt-bufferless=1 --num-bufferless-pkt=1 --bufferless-router={11:d} --injectionrate={7:1.2f} --synthetic={12:s} --routing-algorithm={13:d} ".format(binary, out_dir, num_cores[c],  mode[m], bench_caps[b], routing_algorithm[rout_], vc_[v], injection_rate, num_rows[c], vnet, cycles, int(bufferless_router(m, num_cores[c])), bench[b], rout_))

					#### convert flot to string with required precision ####
					inj_rate="{:1.2f}".format(injection_rate)

					############ gem5 output-directory ##############
					output_dir=("{0:s}/{1:d}/{2:s}/{4:s}/{3:s}/vc-{5:d}/inj-{6:1.2f}".format(out_dir, num_cores[c],  mode[m], bench_caps[b], routing_algorithm[rout_], vc_[v], injection_rate))

					print ("output_dir: %s" %(output_dir))

					packet_latency = subprocess.check_output("grep -nri average_packet_latency  {0:s}  | sed 's/.*system.ruby.network.average_packet_latency\s*//'".format(output_dir), shell=True)

					pkt_lat = float(packet_latency)
					print ("injection_rate={1:1.2f} \t Packet Latency: {0:f} ".format(pkt_lat, injection_rate))

					# Code to capture saturation throughput
					if injection_rate == 0.02:
						low_load_latency = float(pkt_lat)
					elif (float(pkt_lat) > 6.0 * float(low_load_latency)):
						sat_thrpt.append(float(injection_rate))
						break

					if float(low_load_latency) > 70.00:
						sat_thrpt.append(float(injection_rate))
						break

					injection_rate+=0.02


############### Extract results here ###############

# Print the list here
for m in range(len(mode)):
	for c in range(len(num_cores)):
		for b in range(len(bench)):
			for v in range(len(vc_)):
				print ("{0:s} --- cores: {3:d} b: {1:s} vc-{2:d}".format(mode[m], bench_caps[b], vc_[v], num_cores[c]))
				print sat_thrpt[m*(len(num_cores)*len(bench)*len(vc_)) + c*(len(bench)*len(vc_)) + b*(len(vc_)) + v]
