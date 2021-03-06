/** @file

  A brief file description

  @section license License

  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 */

/****************************************************************************

   CoreUtils.h

   Description:  Automated processing of core files on Sparc & Linux
 ****************************************************************************/

#ifndef _CORE_UTILS_H_
#define _CORE_UTILS_H_


#if defined(sparc)
#include  <sys/regset.h>
#include "DynArray.h"

// to be sorted by lwpid, but lwps are assumed to come in order
struct lwpTable
{
  intptr_t lwpId;
  intptr_t framep;
};

// contains local and in registers, frame pointer, and stack base
struct core_stack_state
{
  struct rwindow regs;
  intptr_t framep;
  intptr_t stkbase;
};
#endif

#if (HOST_OS == linux)
#include <stdio.h>
#include <sys/procfs.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <asm/ptrace.h>         // for structure pt_regs
#include <elf.h>
#include "../libinktomi++/DynArray.h"

#define SP_REGNUM 15            /* Contains address of top of stack USP */
#define PC_REGNUM 12            /* Contains program counter EIP */
#define FP_REGNUM 5             /* Virtual frame pointer EBP */
#define NO_OF_ARGS 10           /* The argument depth upto which we would be looking into
                                   the stack */

// contains local and in registers, frame pointer, and stack base
struct core_stack_state
{
  intptr_t framep;                   //int stkbase;
  intptr_t pc;
  intptr_t arg[NO_OF_ARGS];
};
#endif  // linux check

// to be sorted by virtual address
struct memTable
{
  intptr_t vaddr;
  intptr_t offset;
  intptr_t fsize;
};

typedef void (*StuffTest_f) (void *);

class HttpSM;
class HTTPHdr;
class HdrHeap;

class EThread;
class UnixNetVConnection;
class FtpSM;

#ifdef READ_CORE_WMT
class WMT_ServerFileSM;
class WMT_ClientServer;
class WMT_Top;
class TunnelSM;
class WMT_Datapump;
class evHistory;
#endif

class CoreUtils
{
public:

  /**********************************************************************
  * purpose: finds the index of the virtual address or finds what the 
  *          index should be if the virtual address is not there
  * inputs: int vaddr, int upper, int lower
  * outputs: returns the index of the vaddr or the index after where it 
  *          should be
  * notes: upper and lower stands for the lowest and highest indices
  *        the function should search between
  **********************************************************************/
  static intptr_t find_vaddr(intptr_t vaddr, intptr_t upper, intptr_t lower);

  /**********************************************************************
  * purpose: function used to build the virtual address table
  * inputs: int vaddr1, int offset1, int fsize1
  * outputs: none
  **********************************************************************/
  static void insert_table(intptr_t vaddr1, intptr_t offset1, intptr_t fsize1);

  /**********************************************************************
  * purpose: fills the given buffer with characters from the beginning
  *          of the given memory section
  * inputs: int vaddr, int length, char* buf, FILE *fp
  * outputs: returns -1 on read error or 1 if successful
  **********************************************************************/
  static intptr_t read_core_memory(intptr_t vaddr, intptr_t length, char *buf, FILE * fp);

  /**********************************************************************
  * purpose: fills the given buffer with characters from the beginning
  *          of the given memory section + given offset
  * inputs: int offset, int vaddr, int length, char* buf, FILE *fp
  * outputs: returns -1 on read error or 1 if successful
  **********************************************************************/
  static intptr_t read_core_memory(intptr_t offset, intptr_t vaddr, intptr_t length, char *buf, FILE * fp);

  /**********************************************************************
  * purpose: fills the buffer with num of given bytes from the
  *          beginning of the memory section
  * inputs: int vaddr, int bytes, char* buf
  * outputs: returns -1 on read error or num of bytes read
  **********************************************************************/
  static intptr_t read_from_core(intptr_t vaddr, intptr_t bytes, char *buf);

  /**********************************************************************
  * purpose: beats the heck outta me
  * inputs: int vaddr, int bytes, HdrHeap h
  * outputs: returns -1 on read error or num of bytes read
  **********************************************************************/
  static intptr_t read_heap_header(intptr_t vaddr, intptr_t bytes, HdrHeap h);

  /**********************************************************************
  * purpose: returns the active lwp id
  * inputs: none
  * outputs: returns the active lwp id
  **********************************************************************/
#if defined(sparc)
  static intptr_t get_active_thread_Id();
#endif

  /**********************************************************************
  * purpose: returns the base core_stack_state for the given thread id 
  * inputs: int threadId, core_stack_state* coress
  * outputs: returns the base core_stack_state for the given thread id
  **********************************************************************/
#if (HOST_OS == linux)
  static void get_base_frame(intptr_t framep, core_stack_state * coress);
#endif
#if defined(sparc)
  static void get_base_frame(intptr_t threadId, core_stack_state * coress);
#endif

  /**********************************************************************
  * purpose: returns the core_stack_state of the next frame up
  * inputs: core_stack_state* coress
  * outputs: returns 0 if current frame is already at the top of the stack
  *          or returns 1 and moves up the stack once
  **********************************************************************/
  static int get_next_frame(core_stack_state * coress);

  /**********************************************************************
  * purpose: loop ups over local & in registers on the stack and calls
  *            f on all of them
  * inputs: none
  * outputs: none
  **********************************************************************/
  static void find_stuff(StuffTest_f f);

  /**********************************************************************
  * purpose: tests whether a given register is an HttpSM
  * inputs: reg_type t, int i, core_stack_state coress
  * outputs: none
  **********************************************************************/
  static void test_HttpSM(void *);
  static void test_HttpSM_from_tunnel(void *);
  static void test_FtpSM(void *);

#ifdef READ_CORE_WMT
  static void test_wmtServerFile(void *);
  static void test_wmtTunnel(void *);
  static void test_wmtTop(void *);
  static void test_wmtClientServer(void *arg);
  static void test_wmtDatapump(void *arg);
#endif

  /**********************************************************************
  * purpose: prints out info about a give HttpSM
  * inputs: HttpSM* core_ptr (ptr to http_sm in core)
  * outputs: none
  **********************************************************************/
  static void process_HttpSM(HttpSM * core_ptr);
  static void process_FtpSM(FtpSM * core_ptr);
  static void process_EThread(EThread * eth_test);
  static void process_NetVC(UnixNetVConnection * eth_test);

#ifdef READ_CORE_WMT
  static void process_WmtServerFileSM(WMT_ServerFileSM * wsfm_test);
  static void process_WmtTunnel(TunnelSM * tunnel_test);
  static void process_WmtTop(WMT_Top * top_test);
  static void process_WmtClientServer(WMT_ClientServer * wcs_test);
  static void process_WmtDatapump(WMT_Datapump * wdp_test);
#endif

  /**********************************************************************
  * purpose: dumps the given state machine's history
  * inputs: HttpSM* hsm
  * outputs: none
  **********************************************************************/
  static void dump_history(HttpSM * hsm);

#ifdef READ_CORE_WMT
  static void wmt_dump_history(evHistory * hist);
#endif

  /**********************************************************************
  * purpose: fills in the given HTTPHdr * live_hdr with live information 
  *          take from core_hdr the core file
  * inputs: HTTPHdr* core_hdr, HTTPHdr* live_hdr
  * outputs: -1 on failure or total bytes in the header heaps
  **********************************************************************/
  static int load_http_hdr(HTTPHdr * core_hdr, HTTPHdr * live_hdr);

  /**********************************************************************
  * purpose: loads the http hdr from handle h in the core file
  * inputs: HTTPHdr* h, char* name
  * outputs: none
  **********************************************************************/
  static void print_http_hdr(HTTPHdr * h, const char *name);

  /**********************************************************************
  * purpose: loads a null terminated string from the core file
  * inputs: core file string address
  * outputs: char* pointing to live string. call deallocs via xfree()
  ***********************************************************************/
  static char *load_string(const char *addr);

  static void test_HdrHeap(void *arg);
};


// parses the core file
void process_core(char *fname);

#endif /* _core_utils_h_ */
