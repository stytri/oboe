/*
MIT License

Copyright (c) 2019 Tristan Styles

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "nobreak.h"
#include "version.h"
#include "ast.h"
#include "parse.h"
#include "builtins.h"
#include "searchpaths.h"
#include "sources.h"
#include "system.h"
#include "optget.h"
#include "errorf.h"
#include "trace.h"
#include "tostr.h"
#include "graph.h"
#include "array.h"
#include "rand.h"
#include "eval.h"
#include "gc.h"

//------------------------------------------------------------------------------

static int
interactive(
	unsigned *linop,
	bool      timed,
	bool      quiet,
	bool      doeval,
	FILE     *gfile
);

static int
process(
	char const *cs,
	unsigned    source,
	unsigned   *linop,
	bool        timed,
	bool        quiet,
	bool        doeval,
	char const *gtitle,
	FILE       *gfile
);

static char *
readline(
	FILE *stream
);

static int
print_entry(
	void    *context,
	size_t   index,
	uint64_t hash
) {
	Ast env = context;
	Ast ast = array_at(env->m.env, Ast, index);

	switch(ast->type) {
		size_t      len;
		char const *cs;
	case AST_Reference:
	case AST_BuiltinOperator:
	case AST_BuiltinFunction:
		cs = StringToCharLiteral(ast->m.sval, &len);
		puts(cs);
		break;
	default:
		break;
	}


	return 0;
	(void)hash;
}

static void
initialise(
	char const *generator,
	bool        has_math,
	bool        list_builtins
) {
	initialise_rand(generator);

	initialise_ast();
	initialise_env();
	initialise_sources();
	initialise_searchpaths();
	initialise_builtins(has_math);
	initialise_system_environment();
	initialise_system_stdio();
	initialise_system_ctype();
	initialise_system_bits();

	if(list_builtins) {
		array_foreach(operators->m.env         , print_entry, operators);
		array_foreach(globals->m.env           , print_entry, globals);
		array_foreach(system_environment->m.env, print_entry, system_environment);
	}
}

//------------------------------------------------------------------------------

int
main(
	int   argc,
	char *argv__actual[]
) {
#ifndef NDEBUG
	char *argv[] = {
		argv__actual[0],
#	if 0
		"--math",
#	endif
#	if 0
		"--noeval",
#	endif
#	if 0
		"--graph", "test.dot",
#	endif
#	if 0
		"-x",
		"i:1234567890;\n"
		"f:0.123456789;\n"
		"s:'hello world!\\n';\n"
		,
#	elif 0
		"test.oboe",
#	endif
		NULL
	};
	argc = (sizeof(argv) / sizeof(argv[0])) - 1;
#else
	char **const argv = argv__actual;
#endif

	static struct optget options[] = {
		{ 0, "usage: oboe [options] [FILE...]", NULL },
		{ 0, "options:",                        NULL },
		{ 1, "-h, --help",                      "display help" },
		{ 2, "-o, --output FILE",               "output to FILE" },
		{ 3, "-e, --error FILE",                "errors to FILE" },

		{ 8, "    --version",                   "output version" },

		{10, "-q, --quiet",                     "suppress result output" },
		{11, "-t, --trace",                     "enable basic trace output" },
		{12, "-v, --verbose",                   "enable verbose trace output" },
		{13, "    --list-builtins",             "output names of all builtins" },
		{19, "-n, --noeval",                    "parse, but do not evaluate" },
		{ 9, "-g, --graph FILE",                "output graphs of the AST to FILE in DOT format" },
		{18, "-T, --timed",                     "timed execution" },

		{20, "-m, --math",                      "enable math functions in the global namespace" },
		{21, "-r, --rand GENERATOR",            "select random number GENERATOR" },
		{90, "-x, --evaluate EXPRESSION*",      "evaluates EXPRESSIONs up to -" },
		{92, "-I, --import-path PATH",          "add search PATH for import" },
		{91, "-i, --import FILE",               "imports FILE" },
		{ 0, "FILE",                            "executes FILE" }
	};
	static size_t const n_options = (sizeof(options) / sizeof(options[0]));

	int exit_status = EXIT_SUCCESS;

	unsigned    line          = 1;
	char const *generator     = NULL;
	bool        has_math      = false;
	bool        list_builtins = false;
	bool        timed         = false;
	bool        quiet         = false;
	bool        doeval        = true;
	FILE       *gfile         = NULL;
	bool        unprocessed   = true;

	for(int argi = 1; argi < argc;) {
		char const *args = argv[argi++];
		char const *argp = NULL;

		do {
			int argn   = argc - argi;
			int params = 0;

			switch(optget(n_options - 3, options + 2, &argp, args, argn, &params)) {
			case 1:
				optuse(n_options, options, stdout);
				goto end;

			case 2:
				if(!freopen(argv[argi], "w", stdout)) {
					errorf("freopen(%s): %s", argv[argi], strerror(errno));
					exit_status = EXIT_FAILURE;
					goto end;
				}
				break;

			case 3:
				if(!freopen(argv[argi], "w", stderr)) {
					exit_status = EXIT_FAILURE;
					goto end;
				}
				break;

			case 8:
				unprocessed = false;
				printf("oboe %.2d.%.2d.%.3d\n", VERSION/100000, (VERSION/1000)%100, VERSION%1000);
				printf("Copyright (c) 20%.2d Tristan Styles\n", VERSION/100000);
				printf("MIT License\n");
				break;

			case 9:
				if(gfile) {
					graph_footer(gfile);
					fclose(gfile);
					gfile = NULL;
				}
				gfile = fopen(argv[argi], "w");
				if(!gfile) {
					exit_status = EXIT_FAILURE;
					goto end;
				}
				graph_header(gfile);
				break;

			case 10:
				quiet = true;
				break;

			case 11:
				trace_enabled = true;
				trace_verbose = false;
				break;

			case 12:
				trace_enabled = true;
				trace_verbose = true;
				break;

			case 13:
				list_builtins = true;
				break;

			case 18:
				timed = true;
				break;

			case 19:
				doeval = false;
				break;

			case 20:
				has_math = true;
				break;

			case 21:
				if(strcmp(argv[argi], "?") && strcmp(argv[argi], "help")) {
					generator = argv[argi];
					break;
				}
				puts("default");
				puts("jsf");
				puts("sfc");
				puts("splitmix     (default)");
				puts("xoshiro256**");
				puts("xsm");
				goto end;

			case 90:
				unprocessed = false;

				initialise(generator, has_math, list_builtins);

				for(;
					(argi < argc) && (strcmp(argv[argi], "-") != 0);
					++argi
				) {
					exit_status = process(argv[argi], 0, &line, timed, quiet, doeval, argv[argi], gfile);
					if(exit_status != EXIT_SUCCESS) {
						goto end;
					}
				}
				break;

			case 0:
				initialise(generator, has_math, list_builtins);

				--argi;
				addenv_argv(system_environment, 0, argc - argi, &argv[argi]);

				unprocessed = false;
				nobreak;
			case 91: {
				initialise(generator, has_math, list_builtins);

				size_t ts   = gc_topof_stack();
				size_t n    = strlen(argv[argi]);
				String file = CharLiteralToString(argv[argi], n);
				assert(file != NULL);
				String s = mapoboefile(gc_push(file));
				if(s) {
					args = StringToCharLiteral(s, NULL);
					line = 1;

					unsigned source = add_source(0, file);
					exit_status = process(args, source, &line, timed, true, doeval, argv[argi], gfile);

					StringDelete(s);
				} else {
					exit_status = EXIT_FAILURE;
				}
				gc_revert(ts);

				if(!unprocessed || (exit_status != EXIT_SUCCESS)) {
					goto end;
				}
				break;
			}
			case 92: {
				initialise(generator, has_math, list_builtins);

				size_t ts   = gc_topof_stack();
				size_t n    = strlen(argv[argi]);
				String path = CharLiteralToString(argv[argi], n);
				assert(path != NULL);
				add_searchpath(0, gc_push(path));
				gc_revert(ts);
				break;
			}
			default:
				if((params > 0) && (argi < argc)) {
					errorf("invalid option: %s %s\n", args, argv[argi]);
				} else {
					errorf("invalid option: %s\n", args);
				}
				optuse(n_options, options, stderr);
				exit_status = EXIT_FAILURE;
				goto end;
			}

			argi += params;

		} while(argp)
			;
	}

	if(unprocessed) {
		initialise(generator, has_math, list_builtins);

		exit_status = interactive(&line, timed, quiet, doeval, gfile);
	}
end:
	if(gfile) {
		graph_footer(gfile);
		fclose(gfile);
	}

	return exit_status;
}

//------------------------------------------------------------------------------

static void
print(
	Ast ast
) {
	if(ast_isnotZen(ast)) {
		String s = tostr(ast, false);
		if(s) {
			size_t      n;
			char const *cs = StringToCharLiteral(s, &n);

			puts(cs);

			StringDelete(s);
		}
	}
}

static inline bool
is_command(
	char const *args,
	char const *cmds
) {
	return (*args == '@')
		&& (strcmp(args+1, cmds) == 0)
	;
}

static int
interactive(
	unsigned *linop,
	bool      timed,
	bool      quiet,
	bool      doeval,
	FILE     *gfile
) {
	for(;;) {
		size_t mem_used    = gc_total_size();
		size_t stack_depth = gc_topof_stack();

		if(!quiet) {
			fprintf(stdout,
				"[%zu](%zu)> ",
				mem_used,
				stack_depth
			);
		}
		fflush(stdout);

		char const *args = readline(stdin);
		if(!args) {
			break;
		} else if(is_command(args, "exit")) {
			break;
		} else if(is_command(args, "sources")) {
			print(sources);
		} else {
			process(args, 0, linop, timed, quiet, doeval, args, gfile);
		}
	}

	return EXIT_SUCCESS;
}

static int
process(
	char const *args,
	unsigned    source,
	unsigned   *linop,
	bool        timed,
	bool        quiet,
	bool        doeval,
	char const *gtitle,
	FILE       *gfile
) {
	clock_t t1;
	if(timed) {
		t1 = clock();
	}

	size_t gts = gc_topof_stack();

	Ast env = source_env(source);

	while(*args) {
		size_t ts = gc_topof_stack();

		Ast ast = parse(args, &args, source, linop, new_ast_from_lexeme, false);
		if(ast && gfile) {
			graph(gfile, gtitle, ast);
		}
		if(ast && doeval) {
			ast = refeval(env, ast);
			if(!quiet) {
				print(ast);
			}
		}

		gc_revert(ts);
		run_gc();
	}

	gc_revert(gts);
	run_gc();

	if(timed) {
		clock_t     t2     = clock();
		double      tx     = (double)(t2 - t1) / CLOCKS_PER_SEC;
		char const *prefix = "";
		char const *units  = "seconds";
		if(tx >= 300) {
			tx /= 60;
			units = "minutes";
			if(tx >= 180) {
				tx /= 60;
				units = "hours";
				if(tx >= 48) {
					tx /= 24;
					units = "days";
				}
			}
		} else {
			if(tx < 1) {
				tx    *= 1000;
				prefix = "milli";
			}
			if(tx < 1) {
				tx    *= 1000;
				prefix = "micro";
			}
			if(tx < 1) {
				tx    *= 1000;
				prefix = "nano";
			}
			if(tx < 1) {
				tx    *= 1000;
				prefix = "pico";
			}
		}
		errorf("\nexecution time: %g %s%s\n", tx, prefix, units);
	}

	return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------

static char *
readline(
	FILE *stream
) {
	static struct array buf = ARRAY();

	clearerr(stream);
	array_clear(&buf);

	for(int c = fgetc(stream);
		c && (c != EOF) && (c != '\n');
		c = fgetc(stream)
	) {
		if(c == '\\') {
			c = fgetc(stream);

			if(c == '\n') {
				continue;
			}

			if(!(c && (c != EOF))) {
				break;
			}

			if(!array_push_back(&buf, char, '\\')) {
				return NULL;
			}
		}

		if(!array_push_back(&buf, char, c)) {
			return NULL;
		}
	}

	if(ferror(stream) || feof(stream)) {
		return NULL;
	}

	if(!array_push_back(&buf, char, '\0')) {
		return NULL;
	}

	return array_ptr(&buf, char, 0);
}

