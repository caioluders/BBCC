#!/usr/bin/env python
# -*- coding: utf-8 -*-

import pyshark , string , binascii
from reedsolo import *

n = 0 
sniff_time = 10 # change this to fit your needs
packet_equalness = 70
packets_on_packets = []
ip = "127.0.0.1" # server ip
decoder_buffer = []
base = string.digits + string.ascii_letters

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

def check_new_packet(p) :
	global ip , decoder_buffer

	if p[2].src == ip : 
		for i in range(0,packets_on_packets) :
			if compare_packets(p,packets_on_packets[i][0]) >= packet_equalness :
				decoder_buffer.append(base[i])
				d = binascii.unhexlify(hex(int(''.join(decoder_buffer),n)))
				rs = RSCodec(10)
				print(rs.decode(d))
				
def main() :
	global packets_on_packets , n

	capture = pyshark.LiveCapture(interface='eth0',use_json=True, include_raw=True) # remember to set eth0 to promiscuous
	capture.set_debug()

	capture.sniff(timeout=sniff_time)

	parse_packets(capture)

	n = len(packets_on_packets) if len(packets_on_packets) < 32 else 32 # 32 just for test purposes
	capture = pyshark.LiveCapture(interface='eth0',use_json=True, include_raw=True)
	capture.apply_on_packets(check_new_packet,timeout=50)
	
if __name__ == '__main__':
	main()