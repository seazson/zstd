/* Copyright (C) 2013 Red Hat, Inc.
   This file is part of elfutils.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <inttypes.h>
#include <assert.h>
#include <dwarf.h>
#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <fnmatch.h>
#include <elfutils/libdw.h>
#include <elfutils/libdwfl.h>
#include "elfutils/known-dwarf.h"

#define OPT_DEBUGINFO	0x100
#define OPT_COREFILE	0x101

struct arg_list {
  Dwfl *dwfl;
  int printline;
  int printcfi;
  int backtrace;
  char *func;
  uintmax_t addr;
};

/* Dwarf FL wrappers */
static char *debuginfo_path;	/* Currently dummy */
static const Dwfl_Callbacks offline_callbacks =
  {
    .find_debuginfo = dwfl_standard_find_debuginfo,
    .debuginfo_path = &debuginfo_path,

    .section_address = dwfl_offline_section_address,

    /* We use this table for core files too.  */
    .find_elf = dwfl_build_id_find_elf,
  };

static const Dwfl_Callbacks proc_callbacks =
  {
    .find_debuginfo = dwfl_standard_find_debuginfo,
    .debuginfo_path = &debuginfo_path,

    .find_elf = dwfl_linux_proc_find_elf,
  };

static const Dwfl_Callbacks kernel_callbacks =
  {
    .find_debuginfo = dwfl_standard_find_debuginfo,
    .debuginfo_path = &debuginfo_path,

    .find_elf = dwfl_linux_kernel_find_elf,
    .section_address = dwfl_linux_kernel_module_section_address,
  };

static inline void failure (Dwfl *dwfl, int errnum, const char *msg, struct argp_state *state)
{
  if (dwfl != NULL)
    dwfl_end (dwfl);
  if (errnum == -1)
    argp_failure (state, EXIT_FAILURE, 0, "%s: %s",
                  msg, dwfl_errmsg(-1));
  else
    argp_failure (state, EXIT_FAILURE, errnum, "%s", msg);
}

static inline error_t fail (Dwfl *dwfl, int errnum, const char *msg, struct argp_state *state)
{
  failure (dwfl, errnum, msg, state);
  return errnum == -1 ? EIO : errnum;
}

static const struct argp_option options[] =
{
  { NULL, 0, NULL, 0, "Input selection options:", 0 },
  { "executable", 'e', "FILE", 0, "Find addresses in FILE", 0 },
  { "core", OPT_COREFILE, "COREFILE", 0, "Find addresses from signatures found in COREFILE", 0 },
  { "pid", 'p', "PID", 0, "Find addresses in files mapped into process PID", 0 },
  { "linux-process-map", 'M', "FILE", 0, "Find addresses in files mapped as read from FILE"
       " in Linux /proc/PID/maps format", 0 },
  { "kernel", 'k', NULL, 0, "Find addresses in the running kernel", 0 },
  { "offline-kernel", 'K', "RELEASE", OPTION_ARG_OPTIONAL, "Kernel with all modules", 0 },
  { "debuginfo-path", OPT_DEBUGINFO, "PATH", 0, "Search path for separate debuginfo files", 0 },
  { "backtrace", 'b', NULL, 0, "Print backtrace of process. must use with living process", 0 },
  { "address", 'a', "ADDRESS", 0, "Set address to show detail info", 0 },
  { "func", 'f', "FUNC", 0, "Set func to show detail info", 0 },
  { "line", 'l', NULL, 0, "Print debug line section", 0 },
  { "cfi", 'c', NULL, 0, "Print frame info for funcs/addr", 0 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
	struct arg_list *arg_lp;
	char *endp;
	arg_lp = state->input;
	switch(key) {
		case 'e' :
			arg_lp->dwfl = dwfl_begin(&offline_callbacks);
			assert (arg_lp->dwfl != NULL);

			/*支持两种方式打开文件，一种是直接传递文件名，另一种先手动open，然后软入fd*/
			dwfl_report_offline(arg_lp->dwfl, "", arg, -1);/*将elf信息读取到dwfl中*/
			//fd = open("fd", O_RDONLY);
			//dwfl_report_offline(dwfl, "", "", fd);

			dwfl_report_end(arg_lp->dwfl, NULL, NULL); /*告诉库，加载信息已经完成*/
			break;
		case 'p' :
			if (arg_lp->dwfl == NULL) {
				arg_lp->dwfl = dwfl_begin(&proc_callbacks);
				/*相当于将进程信息读取到dwfl中*/
				int result = dwfl_linux_proc_report(arg_lp->dwfl, atoi (arg));
				if (result != 0)
					return fail (arg_lp->dwfl, result, arg, state);

				/* Non-fatal to not be able to attach to process, ignore error. ptrace_attach指定的进程，会使进程停止*/
				dwfl_linux_proc_attach(arg_lp->dwfl, atoi (arg), false);
			}
			break;
		case 'l' :
			arg_lp->printline = 1;
			break;
		case 'b' :
			arg_lp->backtrace = 1;
			break;
		case 'a' :
			arg_lp->addr = strtoumax(arg, &endp, 0);
			break;
		case 'c' :
			arg_lp->printcfi = 1;
			break;
		case 'f' :
			arg_lp->func = arg;
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static const struct argp op_argp =
  { .options = options, .parser = parse_opt };


void print_dw_lines(Dwfl *dwfl)
{
  Dwarf_Die *cu = NULL;
  Dwarf_Addr bias;
  do
    {
      cu = dwfl_nextcu (dwfl, cu, &bias);
      if (cu != NULL)
	{
	  Dwfl_Module *mod = dwfl_cumodule (cu);
	  const char *modname = (dwfl_module_info (mod, NULL, NULL, NULL,
						   NULL, NULL, NULL, NULL)
				 ?: "<unknown>");
	  const char *cuname = (dwarf_diename (cu) ?: "<unknown>");

	  printf ("mod: %s CU: [%" PRIx64 "] %s\n", modname,
		  dwarf_dieoffset (cu), cuname);

	  size_t lines;
	  if (dwfl_getsrclines (cu, &lines) != 0)
	    continue; // No lines...

	  for (size_t i = 0; i < lines; i++)
	    {
	      Dwfl_Line *line = dwfl_onesrcline (cu, i);

	      Dwarf_Addr addr;
	      int lineno;
	      int colno;
	      Dwarf_Word mtime;
	      Dwarf_Word length;
	      const char *src = dwfl_lineinfo (line, &addr, &lineno, &colno,
					       &mtime, &length);

	      Dwarf_Addr dw_bias;
	      Dwarf_Line *dw_line = dwfl_dwarf_line (line, &dw_bias);
	      assert (bias == dw_bias);

	      Dwarf_Addr dw_addr;
	      if (dwarf_lineaddr (dw_line, &dw_addr) != 0)
		error (EXIT_FAILURE, 0, "dwarf_lineaddr: %s",
		       dwarf_errmsg (-1));
	      assert (addr == dw_addr + dw_bias);

	      unsigned int dw_op_index;
	      if (dwarf_lineop_index (dw_line, &dw_op_index) != 0)
		error (EXIT_FAILURE, 0, "dwarf_lineop_index: %s",
		       dwarf_errmsg (-1));

	      int dw_lineno;
	      if (dwarf_lineno (dw_line, &dw_lineno) != 0)
		error (EXIT_FAILURE, 0, "dwarf_lineno: %s",
		       dwarf_errmsg (-1));
	      assert (lineno == dw_lineno);

	      int dw_colno;
	      if (dwarf_linecol (dw_line, &dw_colno) != 0)
		error (EXIT_FAILURE, 0, "dwarf_lineno: %s",
		       dwarf_errmsg (-1));
	      assert (colno == dw_colno);

	      bool begin;
	      if (dwarf_linebeginstatement (dw_line, &begin) != 0)
		error (EXIT_FAILURE, 0, "dwarf_linebeginstatement: %s",
		       dwarf_errmsg (-1));

	      bool end;
	      if (dwarf_lineendsequence (dw_line, &end) != 0)
		error (EXIT_FAILURE, 0, "dwarf_lineendsequence: %s",
		       dwarf_errmsg (-1));

	      bool pend;
	      if (dwarf_lineprologueend (dw_line, &pend) != 0)
		error (EXIT_FAILURE, 0, "dwarf_lineprologueend: %s",
		       dwarf_errmsg (-1));

	      bool ebegin;
	      if (dwarf_lineepiloguebegin (dw_line, &ebegin) != 0)
		error (EXIT_FAILURE, 0, "dwarf_lineepiloguebegin: %s",
		       dwarf_errmsg (-1));

	      bool block;
	      if (dwarf_lineblock (dw_line, &block) != 0)
		error (EXIT_FAILURE, 0, "dwarf_lineblock: %s",
		       dwarf_errmsg (-1));

	      unsigned int isa;
	      if (dwarf_lineisa (dw_line, &isa) != 0)
		error (EXIT_FAILURE, 0, "dwarf_lineisa: %s",
		       dwarf_errmsg (-1));

	      unsigned int disc;
	      if (dwarf_linediscriminator (dw_line, &disc) != 0)
		error (EXIT_FAILURE, 0, "dwarf_linediscriminator: %s",
		       dwarf_errmsg (-1));

	      const char *dw_src;
	      Dwarf_Word dw_mtime;
	      Dwarf_Word dw_length;
	      dw_src = dwarf_linesrc (dw_line, &dw_mtime, &dw_length);
	      assert (strcmp (src, dw_src) == 0);
	      assert (mtime == dw_mtime);
	      assert (length == dw_length);

	      printf ("%zd %#" PRIx64 " %s:%d:%d\n"
		      " time: %#" PRIX64 ", len: %" PRIu64
		      ", idx: %d, b: %d, e: %d"
		      ", pe: %d, eb: %d, block: %d"
		      ", isa: %d, disc: %d\n",
		      i, addr, src, lineno, colno, mtime, length,
		      dw_op_index, begin, end, pend, ebegin, block, isa, disc);

	      Dwarf_Die *linecu = dwfl_linecu (line);
	      assert (cu == linecu);

	      Dwfl_Module *linemod = dwfl_linemodule (line);
	      assert (mod == linemod);
	    }
	}
    }
  while (cu != NULL);
}

static int dump_modules (Dwfl_Module *mod, void **userdata __attribute__ ((unused)),
	      const char *name, Dwarf_Addr start,
	      void *arg __attribute__ ((unused)))
{
	Dwarf_Addr end;
	dwfl_module_info (mod, NULL, NULL, &end, NULL, NULL, NULL, NULL);
	printf ("MOD %#" PRIx64 "\t%#" PRIx64 "\t%s\n", (uint64_t) start, (uint64_t) end, name);
	return DWARF_CB_OK;
}

static int frame_callback (Dwfl_Frame *state, void *frame_arg)
{
	int *framenop = frame_arg;
	Dwarf_Addr pc;
	bool isactivation;

	if (*framenop > 16) {
		error (0, 0, "Too many frames: %d\n", *framenop);
		return DWARF_CB_ABORT;
    }
	/*找到当前帧对应的pc地址*/
	if (! dwfl_frame_pc (state, &pc, &isactivation)) {
		error (0, 0, "%s", dwfl_errmsg (-1));
		return DWARF_CB_ABORT;
    }
	Dwarf_Addr pc_adjusted = pc - (isactivation ? 0 : 1);

	/* Get PC->SYMNAME.
	 * 根据pc地址找到对应的库名称，然后在模块里面找对应的符号名称*/
	Dwfl_Thread *thread = dwfl_frame_thread (state);
	Dwfl *dwfl = dwfl_thread_dwfl (thread);
	Dwfl_Module *mod = dwfl_addrmodule (dwfl, pc_adjusted);
	const char *symname = NULL;
	if (mod)
		symname = dwfl_module_addrname (mod, pc_adjusted);

	printf ("#%2d %#" PRIx64 "%4s\t%s\n", *framenop, (uint64_t) pc,
		! isactivation ? "- 1" : "", symname ?: "<null>");

	(*framenop)++;

	return DWARF_CB_OK;
}

static int thread_callback (Dwfl_Thread *thread, void *thread_arg __attribute__((unused)))
{
	printf ("TID %ld:\n", (long) dwfl_thread_tid (thread));
	int frameno = 0;
	/*变量线程的栈回溯*/
	switch (dwfl_thread_getframes (thread, frame_callback, &frameno)) {
	case 0:
		break;
    case DWARF_CB_ABORT:
		return DWARF_CB_ABORT;
    case -1:
		error (0, 0, "dwfl_thread_getframes: %s", dwfl_errmsg (-1));
		/* All platforms do not have yet proper unwind termination.  */
		break;
    default:
		abort ();
    }
	return DWARF_CB_OK;
}

void print_backtrace(Dwfl *dwfl)
{
	ptrdiff_t ptrdiff;
	bool err = false;

	/*检查dwfl里面的pid信息，确保之前的attch成功了*/
	if (dwfl_pid (dwfl) < 0)
		error (2, 0, "dwfl_pid: %s", dwfl_errmsg (-1));

	/*遍历所有链接的库，并打印出库地址和名称*/
	ptrdiff = dwfl_getmodules (dwfl, dump_modules, NULL, 0);
	assert (ptrdiff == 0);

	/*遍历该进程下所有的线程，执行callback*/
	switch (dwfl_getthreads (dwfl, thread_callback, NULL)) {
		case 0:
			break;
		case DWARF_CB_ABORT:
			err = true;
			break;
		case -1:
			error (0, 0, "dwfl_getthreads: %s", dwfl_errmsg (-1));
			err = true;
			break;
		default:
			abort ();
    }
	if (err)
		exit (EXIT_FAILURE);
}

/*decode cfi functions*/
static const char *
op_name (unsigned int code)
{
	static const char *const known[] =
    {
#define DWARF_ONE_KNOWN_DW_OP(NAME, CODE) [CODE] = #NAME,
      DWARF_ALL_KNOWN_DW_OP
#undef DWARF_ONE_KNOWN_DW_OP
    };

	if ((code < sizeof (known) / sizeof (known[0])))
		return known[code];

	return NULL;
}

static void
print_cfi_detail (int result, const Dwarf_Op *ops, size_t nops, Dwarf_Addr bias)
{
	if (result < 0)
		printf ("indeterminate (%s)\n", dwarf_errmsg (-1));
	else if (nops == 0)
		printf ("%s\n", ops == NULL ? "same_value" : "undefined");
	else {
		printf ("%s expression:", result == 0 ? "location" : "value");
		for (size_t i = 0; i < nops; ++i) {
			printf (" %s", op_name(ops[i].atom));
			if (ops[i].number2 == 0) {
				if (ops[i].atom == DW_OP_addr)
					printf ("(%#" PRIx64 ")", ops[i].number + bias);
				else if (ops[i].number != 0)
					printf ("(%" PRId64 ")", ops[i].number);
			} else
				printf ("(%" PRId64 ",%" PRId64 ")", ops[i].number, ops[i].number2);
			}
		puts ("");
    }
}

struct stuff
{
  Dwarf_Frame *frame;
  Dwarf_Addr bias;
};

static int print_register (void *arg,
		int regno,
		const char *setname,
		const char *prefix,
		const char *regname,
		int bits __attribute__ ((unused)),
		int type __attribute__ ((unused)))
{
	struct stuff *stuff = arg;

	printf ("\t%s reg%u (%s%s): ", setname, regno, prefix, regname);

	Dwarf_Op ops_mem[3];
	Dwarf_Op *ops;
	size_t nops;
	int result = dwarf_frame_register (stuff->frame, regno, ops_mem, &ops, &nops);
	print_cfi_detail (result, ops, nops, stuff->bias);

	return DWARF_CB_OK;
}

static int handle_cfi (Dwfl *dwfl, const char *which, Dwarf_CFI *cfi,
	    GElf_Addr pc, struct stuff *stuff)
{
	if (cfi == NULL) {
		printf ("handle_cfi no CFI (%s): %s\n", which, dwarf_errmsg (-1));
		return -1;
    }
    /*根据pc找到对应的frame信息*/
	int result = dwarf_cfi_addrframe (cfi, pc - stuff->bias, &stuff->frame);
	if (result != 0) {
		printf ("dwarf_cfi_addrframe (%s): %s\n", which, dwarf_errmsg (-1));
		return 1;
    }

	Dwarf_Addr start = pc;
	Dwarf_Addr end = pc;
	bool signalp; /*找到此帧的开始结束地址，signalp标识是否为一个信号处理帧，这个帧是没法回溯的。返回的保存返回地址的寄存器编号*/
	int ra_regno = dwarf_frame_info (stuff->frame, &start, &end, &signalp);
	if (ra_regno >= 0) {
      start += stuff->bias;
      end += stuff->bias;
    }

	printf ("%s has %#" PRIx64 " => [%#" PRIx64 ", %#" PRIx64 "):\n",
		which, pc, start, end);

	if (ra_regno < 0)
		printf ("\treturn address register unavailable (%s)\n", dwarf_errmsg (0));
	else
		printf ("\treturn address in reg%u%s\n", ra_regno, signalp ? " (signal frame)" : "");

	// Point cfa_ops to dummy to match print_detail expectations.
	// (nops == 0 && cfa_ops != NULL => "undefined")
	Dwarf_Op dummy;
	Dwarf_Op *cfa_ops = &dummy;
	size_t cfa_nops;
	result = dwarf_frame_cfa (stuff->frame, &cfa_ops, &cfa_nops);
	/*解析CFA信息*/
	printf ("\tCFA ");
	print_cfi_detail (result, cfa_ops, cfa_nops, stuff->bias);

	/*解析各个寄存器信息*/
    (void) dwfl_module_register_names (dwfl_addrmodule (dwfl, pc),
				     &print_register, stuff);

	return 0;
}

static int print_address_cfi(Dwfl *dwfl, GElf_Addr pc)
{
	Dwfl_Module *mod = dwfl_addrmodule (dwfl, pc);

	struct stuff stuff;
	stuff.frame = NULL;
	stuff.bias = 0;
	/*首先找到obj对应的CFI，然后在分析CFI的内容*/
	int res = handle_cfi (dwfl, ".eh_frame",
	  		dwfl_module_eh_cfi (mod, &stuff.bias), pc, &stuff);
	free (stuff.frame);

	stuff.frame = NULL;
	stuff.bias = 0;
	res &= handle_cfi (dwfl, ".debug_frame",
	  	     dwfl_module_dwarf_cfi (mod, &stuff.bias), pc, &stuff);
	free (stuff.frame);

	return res;
}

/*funtion decode*/
static void print_function_range (Dwfl_Module *mod, Dwarf_Die * die,Dwarf_Addr dwbias)
{
	const char *src;
	int lineno, linecol;
	Dwarf_Addr lowpc, highpc;
	Dwarf_Addr addr;

	if (dwarf_lowpc (die, &lowpc) == 0 && dwarf_highpc (die, &highpc) == 0) {
		lowpc += dwbias;
		highpc += dwbias;
		/*根据地址获取行号*/
		Dwfl_Line *loline = dwfl_module_getsrc (mod, lowpc);
		Dwfl_Line *hiline = dwfl_module_getsrc (mod, highpc-1);
		if (loline != NULL && (src = dwfl_lineinfo (loline, &addr, &lineno, &linecol, NULL, NULL)) != NULL) {
			if (linecol != 0)
				printf ("%s%#" PRIx64 " (line:%d:%d)",
				":", addr, lineno, linecol);
			else
				printf ("%s%#" PRIx64 " (line:%d)",
				":", addr, lineno);
		}
		else
			printf ("%#" PRIx64 "", addr);

		if (hiline != NULL && (src = dwfl_lineinfo (hiline, &addr, &lineno, &linecol, NULL, NULL)) != NULL) {
			if (linecol != 0)
				printf ("%s%#" PRIx64 " (line:%d:%d, %s)",
				"..", addr, lineno, linecol, src);
			else
				printf ("%s%#" PRIx64 " (line:%d, %s)",
				"..", addr, lineno, src);
		}
		else
			printf ("..%#" PRIx64 "", addr);
	}
	printf ("\n");
}

/*传入funcdie，打印它的参数*/
static void print_vars (unsigned int indent, Dwarf_Die *die)
{
	Dwarf_Die child;
	if (dwarf_child (die, &child) == 0)
		do
			switch (dwarf_tag (&child)) {
			case DW_TAG_variable:
			case DW_TAG_formal_parameter:
				printf ("%*s%-30s[%6" PRIx64 "]\n", indent, "", dwarf_diename (&child),
		        (uint64_t) dwarf_dieoffset (&child));
				break;
			default:
				break;
			}
		while (dwarf_siblingof (&child, &child) == 0);

	Dwarf_Attribute attr_mem;
	Dwarf_Die origin;
	if (dwarf_hasattr (die, DW_AT_abstract_origin)
		&& dwarf_formref_die (dwarf_attr (die, DW_AT_abstract_origin, &attr_mem), &origin) != NULL
		&& dwarf_child (&origin, &child) == 0)
		do
			switch (dwarf_tag (&child)) {
			case DW_TAG_variable:
			case DW_TAG_formal_parameter:
				printf ("%*s%s (abstract)\n", indent, "", dwarf_diename (&child));
				break;
			default:
				break;
			}
		while (dwarf_siblingof (&child, &child) == 0);
}

static void print_retvars (Dwfl_Module *mod, Dwarf_Die *die)
{
	const Dwarf_Op *locops;
	int nlocops = dwfl_module_return_value_location (mod,
				   die, &locops);
	if (nlocops < 0)
		error (EXIT_FAILURE, 0, "dwfl_module_return_value_location: %s",
		dwfl_errmsg (-1));
	else if (nlocops == 0)
		puts ("returns no value");
	else {
		printf ("return value location:");
		for (int i = 0; i < nlocops; ++i)
			printf (" {%#x, %#" PRIx64 "}", locops[i].atom, locops[i].number);
		puts ("");
	}
}


/*decode address to line*/
static const char *symname (const char *name)
{
#ifdef USE_DEMANGLE
  // Require GNU v3 ABI by the "_Z" prefix.
  if (demangle && name[0] == '_' && name[1] == 'Z')
    {
      int status = -1;
      char *dsymname = __cxa_demangle (name, demangle_buffer,
				       &demangle_buffer_len, &status);
      if (status == 0)
	name = demangle_buffer = dsymname;
    }
#endif
  return name;
}

static const char *get_diename (Dwarf_Die *die)
{
  Dwarf_Attribute attr;
  const char *name;

  name = dwarf_formstring (dwarf_attr_integrate (die, DW_AT_MIPS_linkage_name, &attr)
			   ?: dwarf_attr_integrate (die, DW_AT_linkage_name, &attr));

  if (name == NULL)
    name = dwarf_diename (die) ?: "??";

  return name;
}

int pretty = 1;
static bool print_dwarf_function (Dwfl_Module *mod, Dwarf_Addr addr)
{
	Dwarf_Addr bias = 0;
	unsigned int indent = 0;
	Dwarf_Die *scopes;
	Dwarf_Die *cudie = dwfl_module_addrdie (mod, addr, &bias);

	/*在cuDie里面找到包含此地址的所有子结构*/
	int nscopes = dwarf_getscopes (cudie, addr - bias, &scopes);
	if (nscopes <= 0)
		return false;

	Dwarf_Addr start, end;
	const char *fname;
	const char *modname = dwfl_module_info (mod, NULL,
					      &start, &end, NULL, NULL, &fname, NULL); /*返回包含此函数的cuDie所在的模块，cuDie代表一个obj而mod代表一个elf*/
	if (modname == NULL)
		error (EXIT_FAILURE, 0, "dwfl_module_info: %s", dwarf_errmsg (-1));
	if (modname[0] == '\0')
		modname = fname;
	printf ("MOD: %s: %#" PRIx64 " .. %#" PRIx64 "\n", modname, start, end); /*获取MOD级别的名称，以及模块的起始结束地址*/

	bool res = false;
	for (int i = 0; i < nscopes; ++i)
		switch (dwarf_tag (&scopes[i])) {
		case DW_TAG_subprogram: {
			const char *name = get_diename (&scopes[i]);
			if (name == NULL)
				goto done;
			printf ("Func: %s ", symname (name));
            print_function_range (mod, &scopes[i], 0);
			print_vars (indent += 8, &scopes[i]);
			print_retvars (mod, &scopes[i]);
			res = true;
			goto done;
		}

		case DW_TAG_inlined_subroutine: {
			const char *name = get_diename (&scopes[i]);
			if (name == NULL)
				goto done;

			/* When using --pretty-print we only show inlines on their
			own line.  Just print the first subroutine name.  */
			if (pretty) {
				printf ("%s ", symname (name));
				res = true;
				goto done;
			}
			else
				printf ("%s inlined", symname (name));

			Dwarf_Files *files;
			if (dwarf_getsrcfiles (cudie, &files, NULL) == 0) {
				Dwarf_Attribute attr_mem;
				Dwarf_Word val;
				if (dwarf_formudata (dwarf_attr (&scopes[i], DW_AT_call_file, &attr_mem), &val) == 0) {
					const char *file = dwarf_filesrc (files, val, NULL, NULL);
					unsigned int lineno = 0;
					unsigned int colno = 0;
					if (dwarf_formudata (dwarf_attr (&scopes[i], DW_AT_call_line, &attr_mem), &val) == 0)
						lineno = val;
					if (dwarf_formudata (dwarf_attr (&scopes[i], DW_AT_call_column, &attr_mem), &val) == 0)
						colno = val;

					const char *comp_dir = "";
					const char *comp_dir_sep = "";

					if (file == NULL)
						file = "???";
					else if (file[0] != '/') {
						const char *const *dirs;
						size_t ndirs;
						if (dwarf_getsrcdirs (files, &dirs, &ndirs) == 0 && dirs[0] != NULL) {
							comp_dir = dirs[0];
							comp_dir_sep = "/";
						}
					}

					if (lineno == 0)
						printf (" from %s%s%s", comp_dir, comp_dir_sep, file);
					else if (colno == 0)
						printf (" at %s%s%s:%u", comp_dir, comp_dir_sep, file, lineno);
					else
						printf (" at %s%s%s:%u:%u", comp_dir, comp_dir_sep, file, lineno, colno);
				}
			}
			printf (" in ");
			continue;
		}
    }

done:
  free (scopes);
  return res;
}

static void print_addrsym (Dwfl_Module *mod, GElf_Addr addr)
{
	GElf_Sym s;
	GElf_Off off;
	const char *name = dwfl_module_addrinfo (mod, addr, &off, &s,
					   NULL, NULL, NULL);
	if (name == NULL) {
		/* No symbol name.  Get a section name instead.  */
		int i = dwfl_module_relocate_address (mod, &addr);
		if (i >= 0)
			name = dwfl_module_relocation_info (mod, i, NULL);
		if (name == NULL)
			printf ("??%c", pretty ? ' ': '\n');
		else
			printf ("(%s)+%#" PRIx64 "%c", name, addr, pretty ? ' ' : '\n');
    } else {
		name = symname (name);
		if (off == 0)
			printf ("%s", name);
		else
			printf ("%s+%#" PRIx64 "", name, off);

		// Also show section name for address.
		Dwarf_Addr ebias;
		Elf_Scn *scn = dwfl_module_address_section (mod, &addr, &ebias);
		if (scn != NULL) {
			GElf_Shdr shdr_mem;
			GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);
			if (shdr != NULL) {
				Elf *elf = dwfl_module_getelf (mod, &ebias);
				size_t shstrndx;
				if (elf_getshdrstrndx (elf, &shstrndx) >= 0)
					printf (" (%s)", elf_strptr (elf, shstrndx, shdr->sh_name));
			}
	    }
		printf ("%c", pretty ? ' ' : '\n');
    }
}

static void print_src (const char *src, int lineno, int linecol, Dwarf_Die *cu)
{
	const char *comp_dir = "";
	const char *comp_dir_sep = "";

	/*use absolution path*/
	if (src[0] != '/') {
		Dwarf_Attribute attr;
		comp_dir = dwarf_formstring(dwarf_attr (cu, DW_AT_comp_dir, &attr));
		if (comp_dir != NULL)
			comp_dir_sep = "/";
    }

	if (linecol != 0)
		printf ("%s%s%s:%d:%d", comp_dir, comp_dir_sep, src, lineno, linecol);
	else
		printf ("%s%s%s:%d", comp_dir, comp_dir_sep, src, lineno);
}

static inline void show_note (int (*get) (Dwarf_Line *, bool *),
	   Dwarf_Line *info, const char *note)
{
	bool flag;
	if ((*get) (info, &flag) == 0 && flag)
		fputs (note, stdout);
}

static inline void show_int (int (*get) (Dwarf_Line *, unsigned int *),
	  Dwarf_Line *info, const char *name)
{
	unsigned int val;
	if ((*get) (info, &val) == 0 && val != 0)
		printf (" (%s %u)", name, val);
}

static int show_inlines(Dwfl_Module *mod, GElf_Addr addr)
{
	const char *src;
	int lineno, linecol;
	Dwarf_Addr bias = 0;
	Dwarf_Die *cudie = dwfl_module_addrdie (mod, addr, &bias);

	Dwarf_Die *scopes = NULL;
	int nscopes = dwarf_getscopes (cudie, addr - bias, &scopes);
	if (nscopes < 0)
		return 1;

	if (nscopes > 0) {
		Dwarf_Die subroutine;
		Dwarf_Off dieoff = dwarf_dieoffset (&scopes[0]);
		dwarf_offdie (dwfl_module_getdwarf (mod, &bias), dieoff, &subroutine);
		free (scopes);
		scopes = NULL;

		nscopes = dwarf_getscopes_die (&subroutine, &scopes);
		if (nscopes > 1) {
			Dwarf_Die cu;
			Dwarf_Files *files;
			if (dwarf_diecu (&scopes[0], &cu, NULL, NULL) != NULL
				&& dwarf_getsrcfiles (cudie, &files, NULL) == 0) {
				for (int i = 0; i < nscopes - 1; i++) {
					Dwarf_Word val;
					Dwarf_Attribute attr;
					Dwarf_Die *die = &scopes[i];
					if (dwarf_tag (die) != DW_TAG_inlined_subroutine)
						continue;

					if (pretty)
						printf (" (inlined by) ");

					if (1) {
					/* Search for the parent inline or function.  It
					might not be directly above this inline -- e.g.
					there could be a lexical_block in between.  */
						for (int j = i + 1; j < nscopes; j++) {
							Dwarf_Die *parent = &scopes[j];
							int tag = dwarf_tag (parent);
							if (tag == DW_TAG_inlined_subroutine
								|| tag == DW_TAG_entry_point
								|| tag == DW_TAG_subprogram) {
								printf ("%s%s", symname (get_diename (parent)),
								pretty ? " at " : "\n");
								break;
							}
						}
					}

					src = NULL;
					lineno = 0;
					linecol = 0;
					if (dwarf_formudata (dwarf_attr (die, DW_AT_call_file,
					       &attr), &val) == 0)
						src = dwarf_filesrc (files, val, NULL, NULL);

					if (dwarf_formudata (dwarf_attr (die, DW_AT_call_line,
					       &attr), &val) == 0)
						lineno = val;

					if (dwarf_formudata (dwarf_attr (die, DW_AT_call_column,
					       &attr), &val) == 0)
						linecol = val;

					if (src != NULL) {
						print_src (src, lineno, linecol, &cu);
						putchar ('\n');
					} else
						puts ("??:0");
				}
			}
		}
	}
}

#define LINEBUF_SIZE 256
#define NR_ADDITIONAL_LINES 2
#define COLOR_RESET		"\033[m"
#define COLOR_BLUE		"\033[34m"
static int show_one_line(FILE *fp, int l, bool skip, bool show_num)
{
	char buf[LINEBUF_SIZE], sbuf[128];
	const char *color = show_num ? "" : COLOR_BLUE;
	const char *prefix = NULL;

	do {
		if (fgets(buf, LINEBUF_SIZE, fp) == NULL)
			goto error;
		if (skip)
			continue;
		if (!prefix) {
			prefix = show_num ? "%7d  " : "         ";
			fprintf(stdout, "%s", COLOR_BLUE);
			fprintf(stdout, prefix, l);
		}
		fprintf(stdout, "%s", buf);
		fprintf(stdout, "%s", COLOR_RESET);
	} while (strchr(buf, '\n') == NULL);

	return 1;
error:
	if (ferror(fp)) {
		strerror_r(errno, sbuf, sizeof(sbuf));
		printf("File read error: %s\n", sbuf);
		return -1;
	}
	printf("Source file is shorter than expected.\n");
	return -1;
}

static int print_srcline(char *path, int line_start, int line_end)
{
	FILE *fp;
	int l = 1;
	int ret;
	fp = fopen(path, "r");
	if (fp == NULL) {
		printf("Failed to open %s err:%d\n", path, errno);
		return -errno;
	}

	while (l < line_start) {
		ret = show_one_line(fp, l++, true, false);
		if (ret < 0)
			goto end;
	}

	if (line_end == INT_MAX)
		line_end = l + NR_ADDITIONAL_LINES;
	while (l <= line_end) {
		ret = show_one_line(fp, l++, false, true);
		if (ret <= 0)
			break;
	}

end:
	fclose(fp);
	return ret; 
}

static int print_address(Dwfl *dwfl, GElf_Addr addr)
{
	Dwfl_Module *mod = dwfl_addrmodule (dwfl, addr);

	/*从Dwarf信息中获取函数名*/
    if( !print_dwarf_function (mod, addr))
	{
		const char *name = dwfl_module_addrname (mod, addr); /*查找mod中包含这个地址的sym名称*/
		printf ("Func: %s %#" PRIx64 "", name, addr);
	}

    print_addrsym (mod, addr);

	Dwfl_Line *line = dwfl_module_getsrc (mod, addr);

	const char *src;
	int lineno, linecol;

	if (line != NULL && (src = dwfl_lineinfo (line, &addr, &lineno, &linecol,
					    NULL, NULL)) != NULL) {
		print_src (src, lineno, linecol, dwfl_linecu (line));
		if (1) {
			Dwarf_Addr bias;
			Dwarf_Line *info = dwfl_dwarf_line (line, &bias);
			assert (info != NULL);

			show_note (&dwarf_linebeginstatement, info, " (is_stmt)");
			show_note (&dwarf_lineblock, info, " (basic_block)");
			show_note (&dwarf_lineprologueend, info, " (prologue_end)");
			show_note (&dwarf_lineepiloguebegin, info, " (epilogue_begin)");
			show_int (&dwarf_lineisa, info, "isa");
			show_int (&dwarf_linediscriminator, info, "discriminator");
		}
		putchar ('\n');
    } else
		puts ("??:0");

	show_inlines(mod, addr);

	if (lineno >= 4)
		lineno -= 4;
	print_srcline((char*)src, lineno, lineno + 12);
}

#define INDENT 4

struct args
{
  Dwfl *dwfl;
  Dwarf_Die *cu;
  Dwarf_Addr dwbias;
  char **argv;
};

static int
handle_function (Dwarf_Die *funcdie, void *arg)
{
	struct args *a = arg;
	int single_func = 0;

	Dwfl_Module *mod = dwfl_cumodule (a->cu);
	const char *name = dwarf_diename (funcdie);
	char **argv = a->argv;
	if (argv[0] != NULL)
    {
		bool match;
		single_func = 1;
		do
			match = fnmatch (*argv, name, 0) == 0;
		while (!match && *++argv);
		if (!match)
			return 0;
    }

	Dwarf_Addr addr;
	dwarf_lowpc (funcdie, &addr);
	if (single_func)
		print_address(a->dwfl, addr);
	else
		print_dwarf_function (mod, addr);

	return 0;
}

void print_dw_funcs_all(Dwfl *dwfl, char *funcname)
{
	struct args a = { .dwfl = dwfl, .cu = NULL};
	a.argv = &funcname;
	while ((a.cu = dwfl_nextcu (a.dwfl, a.cu, &a.dwbias)) != NULL) {
	    //printf("CU %s\n",dwarf_diename (a.cu));
		dwarf_getfuncs (a.cu, &handle_function, &a, 0); /*遍历CU DIE里面所有的FUNC DIE，并且调用回调函数*/
	}
}

int main (int argc, char *argv[])
{
	int cnt;
	struct arg_list args;

	memset((void *)&args, 0, sizeof(struct arg_list));
	/*libdwfl支持默认的参数设置来开打文件*/
	//(void) argp_parse (dwfl_standard_argp (), argc, argv, 0, &cnt, &dwfl);
	argp_parse(&op_argp, argc, argv, 0, &cnt, &args);

	if (args.printline)
		print_dw_lines(args.dwfl);

	if (args.backtrace)
		print_backtrace(args.dwfl);

	if (args.printcfi)
		print_address_cfi(args.dwfl, args.addr);

	if (args.func == NULL && args.addr == 0)
		print_dw_funcs_all(args.dwfl, NULL);
	else if(args.func != NULL)
		print_dw_funcs_all(args.dwfl, args.func);
	else if (args.addr != 0)
		print_address(args.dwfl, args.addr);

	dwfl_end (args.dwfl);
	return 0;
}
