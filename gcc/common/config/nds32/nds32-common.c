/* Common hooks of Andes NDS32 cpu for GNU compiler
   Copyright (C) 2012-2013 Free Software Foundation, Inc.
   Contributed by Andes Technology Corporation.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "diagnostic-core.h"
#include "tm.h"
#include "common/common-target.h"
#include "common/common-target-def.h"
#include "opts.h"
#include "flags.h"

/* ------------------------------------------------------------------------ */

/* Implement TARGET_HANDLE_OPTION.  */
static bool
nds32_handle_option (struct gcc_options *opts ATTRIBUTE_UNUSED,
		     struct gcc_options *opts_set ATTRIBUTE_UNUSED,
		     const struct cl_decoded_option *decoded,
		     location_t loc)
{
  size_t     code  = decoded->opt_index;
  int        value = decoded->value;

  switch (code)
    {
    case OPT_misr_vector_size_:
      /* Check the valid vector size: 4 or 16.  */
      if (value != 4 && value != 16)
	{
	  error_at (loc, "for the option -misr-vector-size=X, the valid X "
			 "must be: 4 or 16");
	  return false;
	}

      return true;

    case OPT_mcache_block_size_:
      /* Check valid value: 4 8 16 32 64 128 256 512.  */
      if (exact_log2 (value) < 2 || exact_log2 (value) > 9)
	{
	  error_at (loc, "for the option -mcache-block-size=X, the valid X "
			 "must be: 4, 8, 16, 32, 64, 128, 256, or 512");
	  return false;
	}

      return true;

    default:
      return true;
    }
}

/* ------------------------------------------------------------------------ */

/* Implement TARGET_OPTION_OPTIMIZATION_TABLE.  */
static const struct default_options nds32_option_optimization_table[] =
{
  /* Enable -fomit-frame-pointer by default at -O1 or higher.  */
  { OPT_LEVELS_1_PLUS, OPT_fomit_frame_pointer, NULL, 1 },
  /* Enable -mv3push by default at -Os, but it is useless under V2 ISA.  */
  { OPT_LEVELS_SIZE,   OPT_mv3push,             NULL, 1 },

  { OPT_LEVELS_NONE,   0,                       NULL, 0 }
};

/* ------------------------------------------------------------------------ */

/* Run-time Target Specification.  */

/* Default enable
     TARGET_GP_DIRECT: Generate gp-imply instruction.
     TARGET_16_BIT   : Generate 16/32 bit mixed length instruction.
     TARGET_PERF_EXT : Generate performance extention instrcution.
     TARGET_CMOV     : Generate conditional move instruction.  */
#undef TARGET_DEFAULT_TARGET_FLAGS
#define TARGET_DEFAULT_TARGET_FLAGS		\
  (MASK_GP_DIRECT				\
   | MASK_16_BIT				\
   | MASK_PERF_EXT				\
   | MASK_CMOV)

#undef TARGET_HANDLE_OPTION
#define TARGET_HANDLE_OPTION nds32_handle_option

#undef TARGET_OPTION_OPTIMIZATION_TABLE
#define TARGET_OPTION_OPTIMIZATION_TABLE nds32_option_optimization_table


/* Defining the Output Assembler Language.  */

#undef TARGET_EXCEPT_UNWIND_INFO
#define TARGET_EXCEPT_UNWIND_INFO sjlj_except_unwind_info

/* ------------------------------------------------------------------------ */

struct gcc_targetm_common targetm_common = TARGETM_COMMON_INITIALIZER;

/* ------------------------------------------------------------------------ */
