#!/bin/bash
#####	README
# .run_script.sh <num-cores> <routAlgo> <spin-freq> <rot> <uTurnSpinRing(3, 4)> <uTurnCrossbar(0, 1)>
###############################################################################
# bench_caps=( 'BIT_COMPLEMENT' 'BIT_ROTATION' 'SHUFFLE' 'TRANSPOSE' 'UNIFORM_RANDOM' )
# bench=( 'bit_complement' 'bit_rotation' 'shuffle' 'transpose' 'uniform_random' )

# bench_caps=( 'BIT_ROTATION' 'SHUFFLE' 'TRANSPOSE' 'UNIFORM_RANDOM')
# bench=( 'bit_rotation' 'shuffle' 'transpose' 'uniform_random')

bench_caps=( 'UNIFORM_RANDOM' 'BIT_COMPLEMENT' 'BIT_ROTATION' 'TRANSPOSE' 'SHUFFLE' )
bench=( 'uniform_random' 'bit_complement' 'bit_rotation' 'transpose' 'shuffle' )

routing_algorithm=( 'TABLE' 'XY' 'RANDOM' 'ADAPT_RAND' 'WestFirst' 'ESCAPE_VC' )

d="11-24-2020"
# out_dir="/usr/scratch/mayank/thesis/SEEC/$d"
out_dir="/usr/scratch/mayank/seec_isca2021_rslt/$d"
cycles=100000
vnet=0 #for multi-flit pkt: vnet = 2
tr=1
routAlgo=$1
bufferless_pkt=$2
bufferless_router=$3
################# Give attention to the injection rate that you have got#############################
# for routAlgo in 1 2 3 4 5;
# do
	# for b in 0 1 2
	for b in 0 1 2 3 4
	do
		for vc_ in 1
		# for vc_ in 8 12
		do
			for k in 0.02 0.04 0.06 0.08 0.10 0.12 0.14 0.16 0.18 0.20 0.22 0.24 0.26 0.28 0.30 0.32 0.34 0.36 0.38 0.40 0.42 0.44 0.46 0.48 0.50 0.52 0.54 0.56 0.58 0.60 0.62 0.64 0.66 0.68 0.70
			# 0.70 0.72 0.74 0.76 0.78 0.80 0.82 0.84 0.86 0.88 0.90 0.92 0.94 0.96 0.98 1.0
			do
				#################### 4x4 Mesh (multi-SEEC) ###########################
			    # ./build/Garnet_standalone/gem5.opt -d $out_dir/16c/SEEC/num-bufferless-${bufferless_pkt}/bufferless-router-${bufferless_router}/${bench_caps[$b]}/${routing_algorithm[$routAlgo]}/vc-${vc_}/inj-${k} configs/example/garnet_synth_traffic.py --topology=Mesh_XY --num-cpus=16 --num-dirs=16 --mesh-rows=4 --network=garnet2.0 --router-latency=1  --sim-cycles=$cycles --inj-vnet=0 --vcs-per-vnet=${vc_} --seec=1 --one-pkt-bufferless=1 --num-bufferless-pkt=${bufferless_pkt} --bufferless-router=${bufferless_router} --injectionrate=${k} --synthetic=${bench[$b]} --routing-algorithm=${routAlgo} &
			    #################### 4x4 Mesh (Baseline) ###########################
			    # ./build/Garnet_standalone/gem5.opt -d $out_dir/16c/Baseline/num-bufferless-${bufferless_pkt}/${bench_caps[$b]}/${routing_algorithm[$routAlgo]}/vc-${vc_}/inj-${k} configs/example/garnet_synth_traffic.py --topology=Mesh_XY --num-cpus=16 --num-dirs=16 --mesh-rows=4 --network=garnet2.0 --router-latency=1  --sim-cycles=$cycles --inj-vnet=0 --vcs-per-vnet=${vc_} --seec=0 --one-pkt-bufferless=0 --num-bufferless-pkt=0 --injectionrate=${k} --synthetic=${bench[$b]} --routing-algorithm=${routAlgo} &
			    #################### 8x8 Mesh (multi-SEEC) #########################
			    ./build/Garnet_standalone/gem5.opt -d $out_dir/64c/SEEC/num-bufferless-${bufferless_pkt}/bufferless-router-${bufferless_router}/${bench_caps[$b]}/${routing_algorithm[$routAlgo]}/vc-${vc_}/inj-${k} configs/example/garnet_synth_traffic.py --topology=Mesh_XY --num-cpus=64 --num-dirs=64 --mesh-rows=8 --network=garnet2.0 --router-latency=1  --sim-cycles=$cycles --inj-vnet=0 --vcs-per-vnet=${vc_} --seec=1 --one-pkt-bufferless=1 --num-bufferless-pkt=${bufferless_pkt} --bufferless-router=${bufferless_router} --injectionrate=${k} --synthetic=${bench[$b]} --routing-algorithm=${routAlgo} &
			    #################### 8x8 Mesh (Baseline) #########################
			    # ./build/Garnet_standalone/gem5.opt -d $out_dir/64c/Baseline/num-bufferless-${bufferless_pkt}/${bench_caps[$b]}/${routing_algorithm[$routAlgo]}/vc-${vc_}/inj-${k} configs/example/garnet_synth_traffic.py --topology=Mesh_XY --num-cpus=64 --num-dirs=64 --mesh-rows=8 --network=garnet2.0 --router-latency=1  --sim-cycles=$cycles --inj-vnet=0 --vcs-per-vnet=${vc_} --seec=0 --one-pkt-bufferless=0 --num-bufferless-pkt=0 --injectionrate=${k} --synthetic=${bench[$b]} --routing-algorithm=${routAlgo} &
			    #################### 16x16 Mesh (multi-SEEC) #########################
			    # ./build/Garnet_standalone/gem5.opt -d $out_dir/256c/SEEC/num-bufferless-${bufferless_pkt}/bufferless-router-${bufferless_router}/${bench_caps[$b]}/${routing_algorithm[$routAlgo]}/vc-${vc_}/inj-${k} configs/example/garnet_synth_traffic.py --topology=Mesh_XY --num-cpus=256 --num-dirs=256 --mesh-rows=16 --network=garnet2.0 --router-latency=1  --sim-cycles=$cycles --inj-vnet=0 --vcs-per-vnet=${vc_} --seec=1 --one-pkt-bufferless=1 --num-bufferless-pkt=${bufferless_pkt} --bufferless-router=${bufferless_router} --injectionrate=${k} --synthetic=${bench[$b]} --routing-algorithm=${routAlgo} &
			    #################### 16x16 Mesh (Baseline) #########################
			    # ./build/Garnet_standalone/gem5.opt -d $out_dir/256c/Baseline/num-bufferless-${bufferless_pkt}/${bench_caps[$b]}/${routing_algorithm[$routAlgo]}/vc-${vc_}/inj-${k} configs/example/garnet_synth_traffic.py --topology=Mesh_XY --num-cpus=256 --num-dirs=256 --mesh-rows=16 --network=garnet2.0 --router-latency=1  --sim-cycles=$cycles --inj-vnet=0 --vcs-per-vnet=${vc_} --seec=0 --one-pkt-bufferless=0 --num-bufferless-pkt=0 --injectionrate=${k} --synthetic=${bench[$b]} --routing-algorithm=${routAlgo} &


			done
		#sleep 300
		done
	done
# done

