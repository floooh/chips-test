/*
 Copyright (c) 2010,2014 Michael Steil, Brian Silverman, Barry Silverman

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include <stdio.h>
#include "types.h"
#include "netlist_sim.h"
/* nodes & transistors */
#include "netlist_6502.h"

/************************************************************
 *
 * 6502-specific Interfacing
 *
 ************************************************************/

uint16_t
readAddressBus(void *state)
{
	return readNodes(state, 16, (nodenum_t[]){ ab0, ab1, ab2, ab3, ab4, ab5, ab6, ab7, ab8, ab9, ab10, ab11, ab12, ab13, ab14, ab15 });
}

uint8_t
readDataBus(void *state)
{
	return readNodes(state, 8, (nodenum_t[]){ db0, db1, db2, db3, db4, db5, db6, db7 });
}

void
writeDataBus(void *state, uint8_t d)
{
	writeNodes(state, 8, (nodenum_t[]){ db0, db1, db2, db3, db4, db5, db6, db7 }, d);
}

BOOL
readRW(void *state)
{
	return isNodeHigh(state, rw);
}

uint8_t
readA(void *state)
{
	return readNodes(state, 8, (nodenum_t[]){ a0,a1,a2,a3,a4,a5,a6,a7 });
}

uint8_t
readX(void *state)
{
	return readNodes(state, 8, (nodenum_t[]){ x0,x1,x2,x3,x4,x5,x6,x7 });
}

uint8_t
readY(void *state)
{
	return readNodes(state, 8, (nodenum_t[]){ y0,y1,y2,y3,y4,y5,y6,y7 });
}

uint8_t
readP(void *state)
{
	return readNodes(state, 8, (nodenum_t[]){ p0,p1,p2,p3,p4,p5,p6,p7 });
}

uint8_t
readIR(void *state)
{
	return readNodes(state, 8, (nodenum_t[]){ notir0,notir1,notir2,notir3,notir4,notir5,notir6,notir7 }) ^ 0xFF;
}

uint8_t
readSP(void *state)
{
	return readNodes(state, 8, (nodenum_t[]){ s0,s1,s2,s3,s4,s5,s6,s7 });
}

uint8_t
readPCL(void *state)
{
	return readNodes(state, 8, (nodenum_t[]){ pcl0,pcl1,pcl2,pcl3,pcl4,pcl5,pcl6,pcl7 });
}

uint8_t
readPCH(void *state)
{
	return readNodes(state, 8, (nodenum_t[]){ pch0,pch1,pch2,pch3,pch4,pch5,pch6,pch7 });
}

uint16_t
readPC(void *state)
{
	return (readPCH(state) << 8) | readPCL(state);
}

/************************************************************
 *
 * Address Bus and Data Bus Interface
 *
 ************************************************************/

uint8_t memory[65536];

static uint8_t
mRead(uint16_t a)
{
	return memory[a];
}

static void
mWrite(uint16_t a, uint8_t d)
{
	memory[a] = d;
}

static inline void
handleMemory(void *state)
{
	if (isNodeHigh(state, rw))
		writeDataBus(state, mRead(readAddressBus(state)));
	else
		mWrite(readAddressBus(state), readDataBus(state));
}

/************************************************************
 *
 * Main Clock Loop
 *
 ************************************************************/

static unsigned int cycle;

void
step(void *state)
{
	BOOL clk = isNodeHigh(state, clk0);

	/* invert clock */
	setNode(state, clk0, !clk);
	recalcNodeList(state);

	/* handle memory reads and writes */
	if (!clk)
		handleMemory(state);

	cycle++;
}

void *
initAndResetChip()
{
	/* set up data structures for efficient emulation */
	nodenum_t nodes = sizeof(netlist_6502_node_is_pullup)/sizeof(*netlist_6502_node_is_pullup);
	nodenum_t transistors = sizeof(netlist_6502_transdefs)/sizeof(*netlist_6502_transdefs);
	void *state = setupNodesAndTransistors(netlist_6502_transdefs,
										   netlist_6502_node_is_pullup,
										   nodes,
										   transistors,
										   vss,
										   vcc);

	setNode(state, res, 0);
	setNode(state, clk0, 1);
	setNode(state, rdy, 1);
	setNode(state, so, 0);
	setNode(state, irq, 1);
	setNode(state, nmi, 1);

	stabilizeChip(state);

	/* hold RESET for 8 cycles */
	for (int i = 0; i < 16; i++)
		step(state);

	/* release RESET */
	setNode(state, res, 1);
	recalcNodeList(state);

	cycle = 0;

	return state;
}

void
destroyChip(void *state)
{
    destroyNodesAndTransistors(state);
}

/************************************************************
 *
 * Tracing/Debugging
 *
 ************************************************************/

void
chipStatus(void *state)
{
	BOOL clk = isNodeHigh(state, clk0);
	uint16_t a = readAddressBus(state);
	uint8_t d = readDataBus(state);
	BOOL r_w = isNodeHigh(state, rw);

	printf("halfcyc:%d phi0:%d AB:%04X D:%02X RnW:%d PC:%04X A:%02X X:%02X Y:%02X SP:%02X P:%02X IR:%02X",
		   cycle,
		   clk,
		   a,
		   d,
		   r_w,
		   readPC(state),
		   readA(state),
		   readX(state),
		   readY(state),
		   readSP(state),
		   readP(state),
		   readIR(state));

	if (clk) {
		if (r_w)
		printf(" R$%04X=$%02X", a, memory[a]);
		else
		printf(" W$%04X=$%02X", a, d);
	}
	printf("\n");
}
