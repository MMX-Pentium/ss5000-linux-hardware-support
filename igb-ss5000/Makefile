# SPDX-License-Identifier: GPL-2.0
# Copyright(c) 1999 - 2018 Intel Corporation.
#
# Makefile for the Intel(R) 82575 PCI-Express ethernet driver
#

obj-m += igb.o

# The target kernel was built with GCC 15. The WSL build host currently has
# GCC 13, which does not recognize this GCC 15 code-alignment spelling.
# Removing it only affects function layout, not the module ABI.
ccflags-remove-y += -fmin-function-alignment=16

igb-y := igb_main.o igb_ethtool.o e1000_82575.o \
	 e1000_mac.o e1000_nvm.o e1000_phy.o e1000_mbx.o \
	 e1000_i210.o igb_ptp.o igb_hwmon.o igb_xsk.o
