#
# perforate 1.00 - utilities to free disk space by finding holes and
# duplicate files.
#
# Oleg Kibirev * April 1995 * oleg@gd.cs.CSUFresno.EDU
#
# This code is covered by General Public License, version 2 or any later
# version of your choice. You should recieve file "COPYING" which contains
# text of the license with any distribution of this program; if you don't 
# have it, a copy is available from ftp.gnu.ai.mit.edu.
#

CC=gcc
CFLAGS=-O2 -fomit-frame-pointer -m486 -s

all: zum
