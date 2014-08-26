#!/bin/bash

# grantchen 2014/08/23

function print_cpu_usage() {
	mpstat -P ALL | awk -f cpu.awk
}

function print_mem_usage() {
	free | awk -f mem.awk
}

function print_network_info() {
	echo "---------------------- network ----------------------"
	sar -n DEV | awk -f network.awk > nw.tmp
	sar -n EDEV | awk -f enetwork.awk > nwe.tmp
	paste nw.tmp nwe.tmp
	rm -f nw.tmp nwe.tmp > /dev/null 2>&1
}

function print_disk_usage() {
	iostat -xmdz | awk -f disk.awk
}

function print_top_n_mem_comsumer() {
	local topn=$1
	echo "---------------------- top_$topn mem-consumer ----------------------"
	echo -e "PID\tMEM%\tCOMMAND\n"
	ps auxw | sort -rn -k4 | head -$topn | awk '{print $2"\t"$4"%\t"$11}' | \
	sed 's/\([0-9]*\t[0-9\.]*%\t\)\(.*\/\)\([^/]*$\)/\1\3/g'
}

function print_sys_status() {
	print_cpu_usage
	echo -e "\n"
	print_mem_usage
	echo -e "\n"
	print_network_info
	echo -e "\n"
	print_disk_usage
	echo -e "\n"
	print_top_n_mem_comsumer 5
	echo -e "\n"
}

function print_usage() {
	echo "Usage : $0 [print_interval (optional)]"
	echo "        $0 -h for this help"
}

print_interval="default"
if [ $# -gt 0 ] ;then
	print_interval=$1
	if [ $print_interval = "-h" ] ;then
		print_usage
		exit 0
	fi
else
	print_usage
fi

for((;;))
do
	print_sys_status
	if [ $print_interval -gt 0 ] > /dev/null 2>&1 ;then
		sleep $print_interval
	else
		exit 0
	fi
done

