#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pyshark , random
from reedsolo import *
from scapy.all import *

n = 0 
sniff_time = 10 # change this to fit your needs
packet_equalness = 70
packets_on_packets = []

def compare_packets(p1,p2) :

	p1 = p1.get_raw_packet()
	p2 = p2.get_raw_packet()

	equalness = 0

	for i in range( len(p2) if len(p1) > len(p2) else len(p1) ) :
		if p1[i] == p2[i] :
			equalness += 1 

	return (100*equalness)/i

def parse_packets(packets) :
	global packets_on_packets
	
	for k in range(len(packets)) :
		f = 0
		for i in range(len(packets_on_packets)) :
			if compare_packets(packets[k],packets_on_packets[i][0]) >= packet_equalness :
				packets_on_packets[i].append(packets[k])
				f = 1
				break
		if f == 0 :
			packets_on_packets.append([packets[k]])

	packets_on_packets.sort(key=len)

	packets_on_packets = packets_on_packets[::-1]

	print(packets_on_packets)

def send_packet(packet) :
	netpacket = packet.get_raw_packet()
	print(netpacket)
	sendp(netpacket,iface='eth0')

def encode_data(data) :
	global n
	out = []
	for c in data : 
		out.append(numberToBase(c,n))

	return out

def numberToBase(n, b):
	if n == 0:
		return [0]
	digits = []
	while n:
		digits.append(int(n % b))
		n //= b

	return digits[::-1]

def PoC() :
	flag = "ELTN0J4P40"
	rs = RSCodec(10)
	flag = rs.encode(flag)
	print(flag)
	flag = encode_data(flag)

	print(flag)

	for x in flag : 
		for c in x : 
			send_packet(packets_on_packets[c][random.randint(0,len(packets_on_packets[c])-1)])

def main() :
	global packets_on_packets , n

	capture = pyshark.LiveCapture(interface='eth0',use_json=True, include_raw=True)
	capture.set_debug()

	capture.sniff(timeout=sniff_time)

	parse_packets(capture)

	n = len(packets_on_packets) if len(packets_on_packets) < 32 else 32
	input()
	PoC()

if __name__ == '__main__':
	main()