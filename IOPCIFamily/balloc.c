/*
 * Copyright (c) 2012 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 * 
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

typedef uint32_t vtd_baddr_t;

static void
vtd_blog(vtd_space_t * bf)
{
	uint32_t idx;
	vtd_table_entry_t entry;
	vtd_baddr_t next;

	for (idx = 0; idx < bf->bheads_count; idx++)
	{
		next = bf->bheads[idx].free.next;
		while (next)
		{
			VTLOG("[%d]: 0x%x\n", idx, next);
			vtd_space_nfault(bf, next, 1);
			entry = bf->tables[0][next];
			vtassert(entry.free.free);
			next = entry.free.next;
		}
	}
}

static void 
vtd_bchunk_free(vtd_space_t * bf, vtd_baddr_t start, vtd_baddr_t size)
{
	vtd_table_entry_t entry;
	vtd_baddr_t next;

	vtassert(start < (1U << bf->bheads_count));
//	vtd_space_fault(bf, start, 1);

	next = bf->bheads[size].free.next;
	if (next)
	{
		vtassert(next < (1U << bf->bheads_count));
		bf->tables[0][next].free.prev = start;
	}
	entry.bits = 0;
	entry.free.free = 1;
	entry.free.size = size;
	entry.free.prev = size;
	entry.free.next = next;
	bf->tables[0][start] = entry;
	bf->bheads[size].free.next = start;
	STAT_ADD(bf, bcounts[size], 1);
}


static void 
vtd_bfree(vtd_space_t * bf, vtd_baddr_t addr, vtd_baddr_t size)
{
	vtd_table_entry_t  entry;
	vtd_baddr_t buddy;
	uint32_t list;

	list = vtd_log2up(size);

	vtassert(!bf->tables[0][addr].free.free);

	do
	{
		// merge less aggressively
//		if ((list <= 10) && (bf->stats.bcounts[list] < 3)) break;

		buddy = (addr ^ (1 << list));
		if (!vtd_space_present(bf, buddy)) break;
		entry = bf->tables[0][buddy];
		if (!entry.free.free) break;
		if (entry.free.size != list) break;
		//
		vtassert(entry.free.prev < (1U << bf->bheads_count));
		bf->tables[0][entry.free.prev].free.next = entry.free.next;
		vtassert(entry.free.next < (1U << bf->bheads_count));
		bf->tables[0][entry.free.next].free.prev = entry.free.prev;
		STAT_ADD(bf, bcounts[list], -1);
		//
		addr &= ~(1 << list);
		list++;
		vtassert(buddy < (1U << bf->bheads_count));
		bf->tables[0][buddy].free.size = list;
		STAT_ADD(bf, merges, 1);
	}
	while (list < bf->bheads_count);

	vtd_bchunk_free(bf, addr, list);
}

static vtd_baddr_t
vtd_balloc(vtd_space_t * bf, vtd_baddr_t size, 
		   uint32_t mapOptions, const upl_page_info_t * pageList)
{
	uint32_t          list, idx;
	vtd_baddr_t       addr;
	vtd_baddr_t       next;
	vtd_baddr_t       clear;
	vtd_table_entry_t entry;

	list = vtd_log2up(size);

	addr = 0;
	for (idx = list; idx < bf->bheads_count; idx++)
	{
		addr = bf->bheads[idx].free.next;
		if (addr) break;
	}
	if (!addr) return (addr);

	vtd_space_nfault(bf, addr, 1);
	vtassert(bf->tables[0][addr].free.free);
	//
	vtassert(addr < (1U << bf->bheads_count));
	entry = bf->tables[0][addr];
	entry.free.free = 0;
//	bf->tables[0][addr].bits = 0;
//f	bf->tables[0][addr].free.free = 0;

	next = entry.free.next;
	bf->bheads[idx].free.next = next;
	if (next) bf->tables[0][next].free.prev = idx;
	STAT_ADD(bf, bcounts[idx], -1);
	//
	STAT_ADD(bf, breakups, idx - list);
	while (idx != list)
	{
		idx--;
		vtd_bchunk_free(bf, addr + (1 << idx), idx);
	}

//	vtd_space_fault(bf, addr + 1, (1 << list) - 1);

	// init or clear allocation
	next = addr;
	if (pageList)
	{	
		vtd_space_set(bf, addr, size, mapOptions, pageList);
		next += size;
	}
	// clear roundup size
	clear = ((addr + (1 << list)) - next);
	if (clear) bzero(&bf->tables[0][next], clear * sizeof(vtd_table_entry_t));
#if 0
	for (; next < (addr + (1 << list)); next++)
	{
		vtassert(next < (1U << bf->bheads_count));
//f		bf->tables[0][next].free.free = 0;
		bf->tables[0][next].bits = 0;
	}
#endif

	return (addr);
}

static void 
vtd_balloc_fixed(vtd_space_t * bf, vtd_baddr_t addr, vtd_baddr_t size)
{
	int list;
	vtd_table_entry_t  entry;
	vtd_baddr_t end;
	vtd_baddr_t chunk;
	vtd_baddr_t next;
	vtd_baddr_t head;
	vtd_baddr_t tail;
	vtd_baddr_t start;
	vtd_baddr_t finish;
	vtd_baddr_t rem;

	end = addr + size;
	for (list = bf->bheads_count; (--list) >= 0;)
	{
		chunk = bf->bheads[list].free.next;
		while (chunk)
		{
			entry = bf->tables[0][chunk];
			next = entry.free.next;
			// intersect
			start  = (chunk < addr) ? addr : chunk;
			finish = (chunk + (1 << list));
			if (end < finish) finish = end;
			if (finish > start)
			{
				//
				vtassert(entry.free.prev < (1U << bf->bheads_count));
				bf->tables[0][entry.free.prev].free.next = next;
				vtassert(next < (1U << bf->bheads_count));
				bf->tables[0][next].free.prev = entry.free.prev;
				STAT_ADD(bf, bcounts[list], -1);
				//
//				vtd_space_fault(bf, start, finish - start);
				for (head = start; head < finish; head++)
				{
					vtassert(head < (1U << bf->bheads_count));
//f					bf->tables[0][head].free.free = 0;
					bf->tables[0][head].bits = 0;
				}

				head = chunk;
				if (1) while (head != start)
				{
					rem = vtd_log2down(start - head);
//VTLOG("++ %x, %x\n", head, rem);
					vtd_bchunk_free(bf, head, rem);
					head += (1 << rem);
				}
				head = end;
				tail = (chunk + (1 << list));
				if (1) while (head < tail)
				{
					rem = vtd_log2down(tail - head);
//VTLOG("-- %x, %x\n", tail - (1 << rem), rem);
					vtd_bchunk_free(bf, tail - (1 << rem), rem);
					tail -= (1 << rem);
				}
			}
			chunk = next;
		}
	}
}


static void
vtd_bfree_fixed(vtd_space_t * bf, vtd_baddr_t addr, vtd_baddr_t size)
{
	vtd_baddr_t end;
	vtd_baddr_t head;
	vtd_baddr_t tail;

	end = addr + size;

	while (addr != end)
	{
		head = __builtin_ctz((unsigned int) addr);
		tail = __builtin_ctz((unsigned int) end);
		if (head <= tail)
		{
//VTLOG("++ %x, %x\n", addr, head);
			vtd_bfree(bf, addr, (1 << head));
			addr += (1 << head);
		}
		else
		{
			end -= (1 << tail);
//VTLOG("-- %x, %x\n", end, tail);
			vtd_bfree(bf, end, (1 << tail));
		}
	}
}

static void
vtd_ballocator_init(vtd_space_t * bf, uint32_t buddybits)
{
	uint32_t  idx;

	bf->bheads_count = buddybits;
	bf->bheads = bf->tables[0];
	vtd_space_fault(bf, 0, (1 << buddybits));
//	bzero(bf->tables[0], (1 << buddybits));
//	bzero(bf->bheads, sizeof(uint64_t) * bf->bheads_count);

    idx = vtd_log2up(buddybits);
	// reserve 2M of space
    if (idx < 9) idx = 9;
	VTLOG("ballocator count %d, table %p, reserved 0x%x\n",
			bf->bheads_count, bf->tables[0], (1 << idx));

	for (; idx < buddybits; idx++)
	{
		vtd_bchunk_free(bf, (1 << idx), idx);
	}
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef KERNEL

/*
cc balloc.c -o /tmp/balloc -Wall -framework IOKit  -framework CoreFoundation -g 
*/

#include <assert.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOKitKeys.h>

#define IONew(type,number)        (type*)malloc(sizeof(type) * (number) )
#define IODelete(ptr,type,number) free( (ptr) )

#define atop(n)   ((n) >> 12)
#define page_size 4096

#define L	if (0)

int main(int argc, char **argv)
{
	vtd_space_t * bf;
	int       idx;
	uint32_t  bits = 20;
	vtd_baddr_t    allocs[256];
	vtd_baddr_t    sizes [256] = { 1, atop(1024*1024), atop(4*1024*1024), 1, 3, 6, 1, 10, 99, 100, 50, 30, 0 };
	vtd_baddr_t    aligns[256] = { 1, atop(1024*1024), atop(2*1024*1024), 1, 4, 8, 1, 0 };

	bf = vtd_ballocator_alloc(bits);
	vtd_blog(bf);

	for (idx = 0; idx < 1*999; idx++)
	{

VTLOG("fixed 0x%x, 0x%x\n", 0x40 + idx, (idx << 1) ^ (idx >> 3));
		vtd_balloc_fixed(bf, 0x40 + idx, (idx << 1) ^ (idx >> 3));
VTLOG("unfix 0x%x, 0x%x\n", 0x40 + idx, (idx << 1) ^ (idx >> 3));
		vtd_bfree_fixed(bf, 0x40 + idx, (idx << 1) ^ (idx >> 3));
	}
	vtd_blog(bf);


	vtd_balloc_fixed(bf, 0x43, 0x4);
	vtd_blog(bf);

	if (1)
	{
		srandomdev();
		long seed = random();
		long count = random();
		VTLOG("seed %ld, count %ld\n", seed, count);

		srandom(seed);
		bzero(&allocs[0], sizeof(allocs));
		while (count--)
		{
			long r = random();
			idx = (r & 255);
			if (allocs[idx]) 
			{
L				VTLOG("free(0x%x, 0x%x)\n", allocs[idx], sizes[idx]);
				vtd_bfree(bf, allocs[idx], sizes[idx]);
L				vtd_blog(bf);
				allocs[idx] = sizes[idx] = 0;
			}
			sizes[idx] = (r >> 20);
			if (!sizes[idx]) sizes[idx] = 1;
			allocs[idx] = vtd_balloc(bf, sizes[idx], aligns[idx]);
L			VTLOG("alloc(0x%x) 0x%x\n", sizes[idx], allocs[idx]);
L			vtd_blog(bf);
		}
	
		for (idx = 0; idx < arrayCount(allocs); idx++)
		{
			if (allocs[idx]) 
			{
L				VTLOG("free(0x%x, 0x%x)\n", allocs[idx], sizes[idx]);
				vtd_bfree(bf, allocs[idx], sizes[idx]);
L				vtd_blog(bf);
				allocs[idx] = sizes[idx] = 0;
			}
		}
	}

	vtd_bfree_fixed(bf, 0x43, 0x4);

	for (idx = 0; idx < bits; idx++)
	{
		vtd_baddr_t check;
		check = (idx < vtd_log2up(bits)) ? 0 : (1 << idx);
		vtassert(check == bf->bheads[idx].free.next);
	}

	vtd_blog(bf);
	VTLOG("OK\n");

    exit(0);    
}

#endif	/* !KERNEL */
