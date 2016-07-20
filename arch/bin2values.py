#!/usr/bin/env python
# -*- coding: UTF-8 -*-

# Xep128: Minimalistic Enterprise-128 emulator with focus on "exotic" hardware
# Copyright (C)2015,2016 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>
# http://xep128.lgb.hu/

from sys import argv, stderr, exit
from textwrap import wrap

if __name__ == "__main__":
	if len(argv) != 3:
		stderr.write("Bad usage.\n")
		exit(1)
	try:
		with open(argv[1], "rb") as bb:
			bb = bytearray(bb.read())
	except IOError as a:
		stderr.write("File open/read error: " + str(a) + "\n")
		exit(1)
	bb = "\n".join(wrap(", ".join(map("0x{:02X}".format, bb)))) + "\n"
	try:
		with open(argv[2], "w") as f:
			f.write(bb)
	except IOError as a:
		stderr.write("File create/write error: " + str(a) + "\n")
		exit(1)
	exit(0)
