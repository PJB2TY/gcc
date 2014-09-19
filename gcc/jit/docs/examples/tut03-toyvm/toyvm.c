/* A simple stack-based virtual machine to demonstrate
   JIT-compilation.  */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libgccjit.h>

/* Typedefs.  */
typedef struct toyvm_op toyvm_op;
typedef struct toyvm_function toyvm_function;
typedef struct toyvm_frame toyvm_frame;

/* Functions are compiled to this function ptr type.  */
typedef int (*toyvm_compiled_func) (int);

enum opcode {
  /* Ops taking no operand.  */
  DUP,
  ROT,
  BINARY_ADD,
  BINARY_SUBTRACT,
  BINARY_MULT,
  BINARY_COMPARE_LT,
  RECURSE,
  RETURN,

  /* Ops taking an operand.  */
  PUSH_CONST,
  JUMP_ABS_IF_TRUE
};

#define FIRST_UNARY_OPCODE (PUSH_CONST)

const char * const opcode_names[] = {
  "DUP",
  "ROT",
  "BINARY_ADD",
  "BINARY_SUBTRACT",
  "BINARY_MULT",
  "BINARY_COMPARE_LT",
  "RECURSE",
  "RETURN",

  "PUSH_CONST",
  "JUMP_ABS_IF_TRUE",
};

struct toyvm_op
{
  /* Which operation.  */
  enum opcode op_opcode;

  /* Some opcodes take an argument.  */
  int op_operand;

  /* The line number of the operation within the source file.  */
  int op_linenum;
};

#define MAX_OPS  (64)

struct toyvm_function
{
  const char *fn_filename;
  int         fn_num_ops;
  toyvm_op    fn_ops[MAX_OPS];
};

#define MAX_STACK_DEPTH (8)

struct toyvm_frame
{
  toyvm_function *frm_function;
  int             frm_pc;
  int             frm_stack[MAX_STACK_DEPTH];
  int             frm_cur_depth;
};

static void
add_op (toyvm_function *fn, enum opcode opcode,
	int operand, int linenum)
{
  toyvm_op *op;
  assert (fn->fn_num_ops < MAX_OPS);
  op = &fn->fn_ops[fn->fn_num_ops++];
  op->op_opcode = opcode;
  op->op_operand = operand;
  op->op_linenum = linenum;
}

static void
add_unary_op (toyvm_function *fn, enum opcode opcode,
	      const char *rest_of_line, int linenum)
{
  int operand = atoi (rest_of_line);
  add_op (fn, opcode, operand, linenum);
}

static toyvm_function *
toyvm_function_parse (const char *filename)
{
  FILE *f = NULL;
  toyvm_function *fn = NULL;
  char *line = NULL;
  ssize_t linelen;
  size_t bufsize;
  int linenum = 0;

  assert (filename);

  f = fopen (filename, "r");
  if (!f)
    {
      fprintf (stderr,
	       "cannot open file %s: %s\n",
	       filename, strerror (errno));
      goto error;
    }

  fn = (toyvm_function *)calloc (1, sizeof (toyvm_function));
  if (!fn)
    {
      fprintf (stderr, "out of memory allocating toyvm_function\n");
      goto error;
    }
  fn->fn_filename = filename;

  /* Read the lines of the file.  */
  while ((linelen = getline (&line, &bufsize, f)) != -1)
    {
      /* Note that this is a terrible parser, but it avoids the need to
	 bring in lex/yacc as a dependency.  */
      linenum++;

      if (0)
	fprintf (stdout, "%3d: %s", linenum, line);

      /* Lines beginning with # are comments.  */
      if (line[0] == '#')
	continue;

      /* Skip blank lines.  */
      if (line[0] == '\n')
	continue;

#define LINE_MATCHES(OPCODE) (0 == strncmp ((OPCODE), line, strlen (OPCODE)))
      if (LINE_MATCHES ("DUP\n"))
	add_op (fn, DUP, 0, linenum);
      else if (LINE_MATCHES ("ROT\n"))
	add_op (fn, ROT, 0, linenum);
      else if (LINE_MATCHES ("BINARY_ADD\n"))
	add_op (fn, BINARY_ADD, 0, linenum);
      else if (LINE_MATCHES ("BINARY_SUBTRACT\n"))
	add_op (fn, BINARY_SUBTRACT, 0, linenum);
      else if (LINE_MATCHES ("BINARY_MULT\n"))
	add_op (fn, BINARY_MULT, 0, linenum);
      else if (LINE_MATCHES ("BINARY_COMPARE_LT\n"))
	add_op (fn, BINARY_COMPARE_LT, 0, linenum);
      else if (LINE_MATCHES ("RECURSE\n"))
	add_op (fn, RECURSE, 0, linenum);
      else if (LINE_MATCHES ("RETURN\n"))
	add_op (fn, RETURN, 0, linenum);
      else if (LINE_MATCHES ("PUSH_CONST "))
	add_unary_op (fn, PUSH_CONST,
		      line + strlen ("PUSH_CONST "), linenum);
      else if (LINE_MATCHES ("JUMP_ABS_IF_TRUE "))
	add_unary_op (fn, JUMP_ABS_IF_TRUE,
		      line + strlen("JUMP_ABS_IF_TRUE "), linenum);
      else
	{
	  fprintf (stderr, "%s:%d: parse error\n", filename, linenum);
	  free (fn);
	  fn = NULL;
	  goto error;
	}
#undef LINE_MATCHES
    }
  free (line);
  fclose (f);

  return fn;

 error:
  free (line);
  fclose (f);
  free (fn);
  return NULL;
}

static void
toyvm_function_disassemble_op (toyvm_function *fn, toyvm_op *op, int index, FILE *out)
{
  fprintf (out, "%s:%d: index %d: %s",
	   fn->fn_filename, op->op_linenum, index,
	   opcode_names[op->op_opcode]);
  if (op->op_opcode >= FIRST_UNARY_OPCODE)
    fprintf (out, " %d", op->op_operand);
  fprintf (out, "\n");
}

static void
toyvm_function_disassemble (toyvm_function *fn, FILE *out)
{
  int i;
  for (i = 0; i < fn->fn_num_ops; i++)
    {
      toyvm_op *op = &fn->fn_ops[i];
      toyvm_function_disassemble_op (fn, op, i, out);
    }
}

static void
toyvm_frame_push (toyvm_frame *frame, int arg)
{
  assert (frame->frm_cur_depth < MAX_STACK_DEPTH);
  frame->frm_stack[frame->frm_cur_depth++] = arg;
}

static int
toyvm_frame_pop (toyvm_frame *frame)
{
  assert (frame->frm_cur_depth > 0);
  return frame->frm_stack[--frame->frm_cur_depth];
}

static void
toyvm_frame_dump_stack (toyvm_frame *frame, FILE *out)
{
  int i;
  fprintf (out, "stack:");
  for (i = 0; i < frame->frm_cur_depth; i++)
    {
      fprintf (out, " %d", frame->frm_stack[i]);
    }
  fprintf (out, "\n");
}

/* Execute the given function.  */

static int
toyvm_function_interpret (toyvm_function *fn, int arg, FILE *trace)
{
  toyvm_frame frame;
#define PUSH(ARG) (toyvm_frame_push (&frame, (ARG)))
#define POP(ARG) (toyvm_frame_pop (&frame))

  frame.frm_function = fn;
  frame.frm_pc = 0;
  frame.frm_cur_depth = 0;

  PUSH (arg);

  while (1)
    {
      toyvm_op *op;
      int x, y;
      assert (frame.frm_pc < fn->fn_num_ops);
      op = &fn->fn_ops[frame.frm_pc++];

      if (trace)
	{
	  toyvm_frame_dump_stack (&frame, trace);
	  toyvm_function_disassemble_op (fn, op, frame.frm_pc, trace);
	}

      switch (op->op_opcode)
	{
	  /* Ops taking no operand.  */
	case DUP:
	  x = POP ();
	  PUSH (x);
	  PUSH (x);
	  break;

	case ROT:
	  y = POP ();
	  x = POP ();
	  PUSH (y);
	  PUSH (x);
	  break;

	case BINARY_ADD:
	  y = POP ();
	  x = POP ();
	  PUSH (x + y);
	  break;

	case BINARY_SUBTRACT:
	  y = POP ();
	  x = POP ();
	  PUSH (x - y);
	  break;

	case BINARY_MULT:
	  y = POP ();
	  x = POP ();
	  PUSH (x * y);
	  break;

	case BINARY_COMPARE_LT:
	  y = POP ();
	  x = POP ();
	  PUSH (x < y);
	  break;

	case RECURSE:
	  x = POP ();
	  x = toyvm_function_interpret (fn, x, trace);
	  PUSH (x);
	  break;

	case RETURN:
	  return POP ();

	  /* Ops taking an operand.  */
	case PUSH_CONST:
	  PUSH (op->op_operand);
	  break;

	case JUMP_ABS_IF_TRUE:
	  x = POP ();
	  if (x)
	    frame.frm_pc = op->op_operand;
	  break;

	default:
	  assert (0); /* unknown opcode */

	} /* end of switch on opcode */
    } /* end of while loop */

#undef PUSH
#undef POP
}

/* JIT compilation.  */

struct compilation_state
{
  gcc_jit_context *ctxt;

  gcc_jit_type *int_type;
  gcc_jit_type *bool_type;
  gcc_jit_type *stack_type; /* int[MAX_STACK_DEPTH] */

  gcc_jit_rvalue *const_one;

  gcc_jit_function *fn;
  gcc_jit_param *param_arg;
  gcc_jit_lvalue *stack;
  gcc_jit_lvalue *stack_depth;
  gcc_jit_lvalue *x;
  gcc_jit_lvalue *y;

  gcc_jit_location *op_locs[MAX_OPS];
  gcc_jit_block *initial_block;
  gcc_jit_block *op_blocks[MAX_OPS];

};

/* Stack manipulation.  */

static void
add_push (compilation_state *state,
	  gcc_jit_block *block,
	  gcc_jit_rvalue *rvalue,
	  gcc_jit_location *loc)
{
  /* stack[stack_depth] = RVALUE */
  gcc_jit_block_add_assignment (
    block,
    loc,
    /* stack[stack_depth] */
    gcc_jit_context_new_array_access (
      state->ctxt,
      loc,
      gcc_jit_lvalue_as_rvalue (state->stack),
      gcc_jit_lvalue_as_rvalue (state->stack_depth)),
    rvalue);

  /* "stack_depth++;".  */
  gcc_jit_block_add_assignment_op (
    block,
    loc,
    state->stack_depth,
    GCC_JIT_BINARY_OP_PLUS,
    state->const_one);
}

static void
add_pop (compilation_state *state,
	 gcc_jit_block *block,
	 gcc_jit_lvalue *lvalue,
	 gcc_jit_location *loc)
{
  /* "--stack_depth;".  */
  gcc_jit_block_add_assignment_op (
    block,
    loc,
    state->stack_depth,
    GCC_JIT_BINARY_OP_MINUS,
    state->const_one);

  /* "LVALUE = stack[stack_depth];".  */
  gcc_jit_block_add_assignment (
    block,
    loc,
    lvalue,
    /* stack[stack_depth] */
    gcc_jit_lvalue_as_rvalue (
      gcc_jit_context_new_array_access (
	state->ctxt,
	loc,
	gcc_jit_lvalue_as_rvalue (state->stack),
	gcc_jit_lvalue_as_rvalue (state->stack_depth))));
}

/* The main compilation hook.  */

static toyvm_compiled_func
toyvm_function_compile (toyvm_function *fn)
{
  compilation_state state;
  int pc;
  char *funcname;

  memset (&state, 0, sizeof (state));

  /* Copy filename to funcname.  */
  funcname = (char *)malloc (strlen (fn->fn_filename) + 1);
  strcpy (funcname, fn->fn_filename);

  /* Convert "." to NIL terminator.  */
  *(strchr (funcname, '.')) = '\0';

  state.ctxt = gcc_jit_context_acquire ();

  gcc_jit_context_set_bool_option (state.ctxt,
				   GCC_JIT_BOOL_OPTION_DUMP_INITIAL_GIMPLE,
				   0);
  gcc_jit_context_set_bool_option (state.ctxt,
				   GCC_JIT_BOOL_OPTION_DUMP_GENERATED_CODE,
				   0);
  gcc_jit_context_set_int_option (state.ctxt,
				  GCC_JIT_INT_OPTION_OPTIMIZATION_LEVEL,
				  3);
  gcc_jit_context_set_bool_option (state.ctxt,
				   GCC_JIT_BOOL_OPTION_KEEP_INTERMEDIATES,
				   0);
  gcc_jit_context_set_bool_option (state.ctxt,
				   GCC_JIT_BOOL_OPTION_DUMP_EVERYTHING,
				   0);
  gcc_jit_context_set_bool_option (state.ctxt,
				   GCC_JIT_BOOL_OPTION_DEBUGINFO,
				   1);

  /* Create types.  */
  state.int_type =
    gcc_jit_context_get_type (state.ctxt, GCC_JIT_TYPE_INT);
  state.bool_type =
    gcc_jit_context_get_type (state.ctxt, GCC_JIT_TYPE_BOOL);
  state.stack_type =
    gcc_jit_context_new_array_type (state.ctxt, NULL,
				    state.int_type, MAX_STACK_DEPTH);

  /* The constant value 1.  */
  state.const_one = gcc_jit_context_one (state.ctxt, state.int_type);

  /* Create locations.  */
  for (pc = 0; pc < fn->fn_num_ops; pc++)
    {
      toyvm_op *op = &fn->fn_ops[pc];

      state.op_locs[pc] = gcc_jit_context_new_location (state.ctxt,
							fn->fn_filename,
							op->op_linenum,
							0); /* column */
    }

  /* Creating the function.  */
  state.param_arg =
    gcc_jit_context_new_param (state.ctxt, state.op_locs[0],
			       state.int_type, "arg");
  state.fn =
    gcc_jit_context_new_function (state.ctxt,
				  state.op_locs[0],
				  GCC_JIT_FUNCTION_EXPORTED,
				  state.int_type,
				  funcname,
				  1, &state.param_arg, 0);

  /* Create stack lvalues.  */
  state.stack =
    gcc_jit_function_new_local (state.fn, NULL,
				state.stack_type, "stack");
  state.stack_depth =
    gcc_jit_function_new_local (state.fn, NULL,
				state.int_type, "stack_depth");
  state.x =
    gcc_jit_function_new_local (state.fn, NULL,
				state.int_type, "x");
  state.y =
    gcc_jit_function_new_local (state.fn, NULL,
				state.int_type, "y");

  /* 1st pass: create blocks, one per opcode. */

  /* We need an entry block to do one-time initialization, so create that
     first.  */
  state.initial_block = gcc_jit_function_new_block (state.fn, "initial");

  /* Create a block per operation.  */
  for (pc = 0; pc < fn->fn_num_ops; pc++)
    {
      char buf[16];
      sprintf (buf, "instr%i", pc);
      state.op_blocks[pc] = gcc_jit_function_new_block (state.fn, buf);
    }

  /* Populate the initial block.  */

  /* "stack_depth = 0;".  */
  gcc_jit_block_add_assignment (
    state.initial_block,
    state.op_locs[0],
    state.stack_depth,
    gcc_jit_context_zero (state.ctxt, state.int_type));

  /* "PUSH (arg);".  */
  add_push (&state,
	    state.initial_block,
	    gcc_jit_param_as_rvalue (state.param_arg),
	    state.op_locs[0]);

  /* ...and jump to insn 0.  */
  gcc_jit_block_end_with_jump (state.initial_block,
			       state.op_locs[0],
			       state.op_blocks[0]);

  /* 2nd pass: fill in instructions.  */
  for (pc = 0; pc < fn->fn_num_ops; pc++)
    {
      gcc_jit_location *loc = state.op_locs[pc];

      gcc_jit_block *block = state.op_blocks[pc];
      gcc_jit_block *next_block = (pc < fn->fn_num_ops
				   ? state.op_blocks[pc + 1]
				   : NULL);

      toyvm_op *op;
      op = &fn->fn_ops[pc];

      /* Helper macros.  */

#define X_EQUALS_POP()\
      add_pop (&state, block, state.x, loc)
#define Y_EQUALS_POP()\
      add_pop (&state, block, state.y, loc)
#define PUSH_RVALUE(RVALUE)\
      add_push (&state, block, (RVALUE), loc)
#define PUSH_X()\
      PUSH_RVALUE (gcc_jit_lvalue_as_rvalue (state.x))
#define PUSH_Y() \
      PUSH_RVALUE (gcc_jit_lvalue_as_rvalue (state.y))

      gcc_jit_block_add_comment (block, loc, opcode_names[op->op_opcode]);

      /* Handle the individual opcodes.  */

      switch (op->op_opcode)
	{
	case DUP:
	  X_EQUALS_POP ();
	  PUSH_X ();
	  PUSH_X ();
	  break;

	case ROT:
	  Y_EQUALS_POP ();
	  X_EQUALS_POP ();
	  PUSH_Y ();
	  PUSH_X ();
	  break;

	case BINARY_ADD:
	  Y_EQUALS_POP ();
	  X_EQUALS_POP ();
	  PUSH_RVALUE (
	   gcc_jit_context_new_binary_op (
	     state.ctxt,
	     loc,
	     GCC_JIT_BINARY_OP_PLUS,
	     state.int_type,
	     gcc_jit_lvalue_as_rvalue (state.x),
	     gcc_jit_lvalue_as_rvalue (state.y)));
	  break;

	case BINARY_SUBTRACT:
	  Y_EQUALS_POP ();
	  X_EQUALS_POP ();
	  PUSH_RVALUE (
	   gcc_jit_context_new_binary_op (
	     state.ctxt,
	     loc,
	     GCC_JIT_BINARY_OP_MINUS,
	     state.int_type,
	     gcc_jit_lvalue_as_rvalue (state.x),
	     gcc_jit_lvalue_as_rvalue (state.y)));
	  break;

	case BINARY_MULT:
	  Y_EQUALS_POP ();
	  X_EQUALS_POP ();
	  PUSH_RVALUE (
	   gcc_jit_context_new_binary_op (
	     state.ctxt,
	     loc,
	     GCC_JIT_BINARY_OP_MULT,
	     state.int_type,
	     gcc_jit_lvalue_as_rvalue (state.x),
	     gcc_jit_lvalue_as_rvalue (state.y)));
	  break;

	case BINARY_COMPARE_LT:
	  Y_EQUALS_POP ();
	  X_EQUALS_POP ();
	  PUSH_RVALUE (
	     /* cast of bool to int */
	     gcc_jit_context_new_cast (
	       state.ctxt,
	       loc,
	       /* (x < y) as a bool */
	       gcc_jit_context_new_comparison (
		 state.ctxt,
		 loc,
		 GCC_JIT_COMPARISON_LT,
		 gcc_jit_lvalue_as_rvalue (state.x),
		 gcc_jit_lvalue_as_rvalue (state.y)),
	       state.int_type));
	  break;

	case RECURSE:
	  {
	    X_EQUALS_POP ();
	    gcc_jit_rvalue *arg = gcc_jit_lvalue_as_rvalue (state.x);
	    PUSH_RVALUE (
	      gcc_jit_context_new_call (
		state.ctxt,
		loc,
		state.fn,
		1, &arg));
	    break;
	  }

	case RETURN:
	  X_EQUALS_POP ();
	  gcc_jit_block_end_with_return (
	    block,
	    loc,
	    gcc_jit_lvalue_as_rvalue (state.x));
	  break;

	  /* Ops taking an operand.  */
	case PUSH_CONST:
	  PUSH_RVALUE (
	    gcc_jit_context_new_rvalue_from_int (
	      state.ctxt,
	      state.int_type,
	      op->op_operand));
	  break;

	case JUMP_ABS_IF_TRUE:
	  X_EQUALS_POP ();
	  gcc_jit_block_end_with_conditional (
	    block,
	    loc,
	    /* "(bool)x".  */
	    gcc_jit_context_new_cast (
	      state.ctxt,
	      loc,
	      gcc_jit_lvalue_as_rvalue (state.x),
	      state.bool_type),
	    state.op_blocks[op->op_operand], /* on_true */
	    next_block); /* on_false */
	  break;

	default:
	  assert(0);
	} /* end of switch on opcode */

      /* Go to the next block.  */
      if (op->op_opcode != JUMP_ABS_IF_TRUE
	  && op->op_opcode != RETURN)
	gcc_jit_block_end_with_jump (
	  block,
	  loc,
	  next_block);

    } /* end of loop on PC locations.  */

  /* We've now finished populating the context.  Compile it.  */
  gcc_jit_result *result = gcc_jit_context_compile (state.ctxt);
  gcc_jit_context_release (state.ctxt);

  return (toyvm_compiled_func)gcc_jit_result_get_code (result,
						       funcname);
  /* (this leaks "result" and "funcname") */
}

int
main (int argc, char **argv)
{
  const char *filename = NULL;
  toyvm_function *fn = NULL;

  if (argc != 3)
    {
      fprintf (stdout,
	"%s FILENAME INPUT: Parse and run a .toy file\n",
	argv[0]);
      exit (1);
    }

  filename = argv[1];
  fn = toyvm_function_parse (filename);
  if (!fn)
    exit (1);

  if (0)
    toyvm_function_disassemble (fn, stdout);

  printf ("interpreter result: %d\n",
	  toyvm_function_interpret (fn, atoi (argv[2]), NULL));

  /* JIT-compilation.  */
  toyvm_compiled_func code = toyvm_function_compile (fn);
  printf ("compiler result: %d\n",
	  code (atoi (argv[2])));

 return 0;
}
