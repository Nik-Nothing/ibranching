/*                                * 
 *   ib (indentation branching)   * 
 *                                */

/* todo:
 *   -pp nesting, endif
 */

#define VERSION "0.12"

#define CONT_MAX 256
#define FILE_MAX 256

#include <limits.h>

#ifndef LINE_MAX
	#define LINE_MAX 2048
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#define noreturn _Noreturn

typedef unsigned short psize;

enum ansi_color
{
	blank   = 0,
	red     = 31,
	green   = 32,
	yellow  = 33,
	blue    = 34,
	magenta = 35,
	cyan    = 36,
	white   = 37
};

typedef enum ansi_color color;

static void set_color(const color c)
{
	printf("\033[%dm", c);
}

static void msg_out(const size_t num, ...)
{
	va_list messages;
	va_start(messages, num);
	
	size_t x = 0;
	while (x++ < num)
	{
		printf("%s%s", va_arg(messages, const char *), x != num ? ": " : "\n");
	}
	
	va_end(messages);
}

static noreturn void error(const char *msg, const int code, const psize loc)
{
	set_color(red);
	
	if (!loc)
	{
		msg_out(2, "error", msg);
	}
	else
	{
		char locs[16];
		sprintf(locs, "%d", loc);
		msg_out(4, "error", msg, "at line", locs);
	}
	
	set_color(blank);
	exit(code);
}

static void warn(const char *typ, const char *msg, const psize loc)
{
	set_color(magenta);
	
	if (!loc)
	{
		msg_out(3, "warning", typ, msg);
	}
	else
	{
		char locs[16];
		sprintf(locs, "%d", loc);
		msg_out(5, "warning", typ, msg, "at line", locs);
	}
	
	set_color(blank);
}

struct line_type
{
	psize comment;
	psize tabs;
	psize loc;
	char  str[LINE_MAX];
};

typedef struct line_type line_t;

enum filetype
{
	norm,
	c
};

typedef enum filetype type;

struct parser_context
{
	const type ftype;
	bool       mcom;
	psize      tbranchc;
	psize      tbranchs[CONT_MAX];
	psize      nbranchc;
	psize      nbranchs[CONT_MAX];
	psize      ctermc;
	psize      cterms[CONT_MAX];
	psize      bmsgsc;
	psize      bmsgst[CONT_MAX];
	char       bmsgss[CONT_MAX][LINE_MAX];
};

typedef struct parser_context context;

psize spaces         = 0;
bool  to_stdout      = false;
char  *overwrite_out = NULL;

static void transfere_line(line_t *line, line_t *input_line)
{
	strcpy(line->str, input_line->str);
	
	line->comment = input_line->comment;
	line->tabs    = input_line->tabs;
	line->loc     = input_line->loc;
}

static psize find_valid(const char *line, const char *tok)
{
	char *ptf  = strchr(line, '"');
	char *ptr  = strstr(line, tok);
	bool open  = true;
	
	while (ptf && !(open && ptf > ptr))
	{
		if (open)
		{
			ptr = strstr(ptr + 1, tok);
		}
		ptf  = strchr(ptf + 1, '"');
		
		open = !open;
	}
	
	if (!ptr)
	{
		return strlen(line);
	}
	
	return (psize) (ptr - line);
}

static psize find_end(const char *line, context *cont)
{
	psize sline = find_valid(line, "//");
	psize mline = find_valid(line, "/*");
	psize cline = find_valid(line, "*/");
	
	if (!cont->mcom && mline < sline)
	{
		cont->mcom = true;
	}
	
	if (cont->mcom)
	{
		if (cline < sline && cline < mline)
		{
			cont->mcom = false;
		}
		return 0;
	}
	
	return sline < mline ? sline : mline;
}

static psize get_tabs(const char *line)
{
	return spaces == 0 ? strspn(line, "\t") : strspn(line, " ") / spaces;
}

static psize get_spaces(const psize tab)
{
	return spaces == 0 ? tab : tab * spaces;
}

static void strip_line(const char *in, char *str)
{
	strcpy(str, in + get_tabs(in));
	
	if (str[strlen(str) - 1] == '\n')
	{
		str[strlen(str) - 1] = '\0';
	}
}

static bool get_line(line_t *line, FILE *file, context *cont, psize *gloc)
{
	if (!fgets(line->str, LINE_MAX, file))
	{
		line->tabs = 0;
		return false;
	}
	
	psize size = strlen(line->str);
	
	while (line->str[size - 2] == '\\')
	{
		char apnd[LINE_MAX];
		if (!fgets(apnd, LINE_MAX, file))
		{
			break;
		}
		size           += strlen(apnd);
		strcat(line->str, apnd);
	}
	
	line->loc     = (*gloc)++;
	line->comment = find_end(line->str, cont);
	line->tabs    = get_tabs(line->str);
	
	return true;
}

static bool check_word(const char *line, const char *word, const char *stop)
{
	char line_cpy[LINE_MAX];
	strcpy(line_cpy, line);
	
	char *save = NULL;
	char *tok  = strtok_r(line_cpy, " ", &save);
	
	while (tok && (!stop ||tok < stop))
	{
		if (!strcmp(tok, word))
		{
			return true;
		}
		
		tok = strtok_r(NULL, word, &save);
	}
	
	return false;
}

static void terminate(line_t *line, const char tchar)
{
	if (line->comment <= get_spaces(line->tabs) + 1)
	{
		return;
	}
	
	if (line->str[line->comment - 2] == ';')
	{
		char log[LINE_MAX];
		strip_line(line->str, log);
		warn("line is already terminated ", log, line->loc);
	}
	
	memmove(line->str + line->comment, line->str + line->comment - 1, strlen(line->str + line->comment - 2));
	line->str[line->comment - 1] = tchar;
}

static void brackinate(FILE *out, const psize tabs, const char *br, context *cont)
{
	psize t = 0;
	
	while(!spaces && t++ < tabs)
	{
		fputc('\t', out);
	}
	
	while(spaces && t++ < get_spaces(tabs))
	{
		fputc(' ', out);
	}
	
	fputs(br, out);
	
	if (br[0] == '}' && cont->tbranchc && cont->tbranchs[cont->tbranchc - 1] == tabs)
	{
		fputc(';', out);
		--cont->tbranchc;
	}
	
	fputc('\n', out);
}

static void branchcheck(FILE *out, psize *tabs, const psize tar, context *cont, line_t *line)
{
	const char  dir = tar < *tabs ? -1 : (*tabs < tar ? 1 : 0);
	const psize dec = dir == 1 ? 1 : 0;
	
	while (tar != *tabs)
	{
		*tabs += (psize) dir;
		
		if (cont->bmsgsc && cont->bmsgst[cont->bmsgsc - 1] == *tabs - dec)
		{
			if (dir == -1)
			{
				--cont->bmsgsc;
				if (strncmp(line->str + get_spaces(*tabs), "#elif", 5) && \
				    strncmp(line->str + get_spaces(*tabs), "#elifdef", 8) && \
				    strncmp(line->str + get_spaces(*tabs), "#elifndef", 9) && \
				    strncmp(line->str + get_spaces(*tabs), "#else", 5))
				{
					brackinate(out, *tabs - dec, cont->bmsgss[cont->bmsgsc], cont);
				}
			}
			continue;
		}
		
		if (cont->nbranchc && cont->nbranchs[cont->nbranchc - 1] == *tabs - dec)
		{
			if (dir == -1)
			{
				--cont->nbranchc;
			}
			continue;
		}
		
		if (dir == -1 && (cont->ctermc && cont->cterms[cont->ctermc - 1] == *tabs))
		{
			--cont->ctermc;
		}
		
		brackinate(out, *tabs - dec, dir == 1 ? "{" : "}", cont);
		
	}
}

static void termcheck(line_t *line, line_t *l_line, context *cont)
{
	if (cont->ftype == c && l_line->str[get_spaces(l_line->tabs)] == '#')
	{
		if (line->tabs > l_line->tabs)
		{
			strcpy(cont->bmsgss[cont->bmsgsc], "#endif");
			cont->bmsgst[cont->bmsgsc++] = l_line->tabs;
		}
		
		return;
	}
	if (line->tabs <= l_line->tabs)
	{
		if (cont->ctermc && cont->cterms[cont->ctermc - 1] == l_line->tabs - 1)
		{
			if(line->tabs == l_line->tabs)
			{
				terminate(l_line, ',');
			}
			
			return;
		}
		
		terminate(l_line, ';');
		
	}
	else if ((!strchr(l_line->str, '(') && l_line->comment > get_spaces(l_line->tabs) + 1 && \
	          strncmp("else", l_line->str + get_spaces(l_line->tabs), 4) && \
	          strncmp("do", l_line->str + get_spaces(l_line->tabs), 2)))
	{
		if (l_line->str[l_line->comment - 2] == '=' || \
		    check_word(l_line->str, "enum", strchr(l_line->str, '(')))
		{
			cont->cterms[cont->ctermc++] = l_line->tabs;
		}
		
		if (l_line->str[l_line->comment - 2] == ':')
		{
			cont->nbranchs[cont->nbranchc++] = l_line->tabs;
			return;
		}
		
		cont->tbranchs[cont->tbranchc++] = l_line->tabs;
	}
}

static void parser(FILE *out, FILE *src, const type ftype)
{
	context cont =
	{
		.ftype    = ftype,
		.tbranchc = 0,
		.nbranchc = 0,
		.ctermc   = 0,
		.bmsgsc   = 0,
		.mcom     = 0
	};
	line_t line, l_line;
	psize  gloc = 1;
	
	get_line(&l_line, src, &cont, &gloc);
	while (true)
	{
		const bool cond = get_line(&line, src, &cont, &gloc);
		
		if (line.comment != 0)
		{
			termcheck(&line, &l_line, &cont);
		}
		
		fputs(l_line.str, out);
		
		if (line.comment != 0)
		{
			branchcheck(out, &l_line.tabs, line.tabs, &cont, &line);
		}
		
		if (!cond)
		{
			break;
		}
		
		transfere_line(&l_line, &line);
	}
}

static type modeset(const char *path)
{
	const char *dot = strrchr(path, '.');
	
	if (dot)
	{
		if (!strcmp(dot, ".c")   || !strcmp(dot, ".h"))
		{
			return c;
		}
		if (!strcmp(dot, ".cpp") || !strcmp(dot, ".hpp"))
		{
			return c;
		}
	}
	
	warn("unable to detect filetype, disabling all language specific rules", path, 0);
	return norm;
}

static void load_file(char *path)
{
	FILE *out, *src = fopen(path, "r");
	
	if (!src)
	{
		error("failed to open source", 1, 0);
	}
	
	if (to_stdout)
	{
		out = stdout;
	}
	else if (overwrite_out)
	{
		out = fopen(overwrite_out, "wa");
	}
	else
	{
		char out_path[255];
		
		if (strlen(path) < 4 || strcmp(path + strlen(path) - 3, ".ib"))
		{
			warn("defined input is not a ib file", path, 0);
			strcpy(out_path, path);
			strcat(out_path, ".unib");
		}
		else
		{
			strncpy(out_path, path, strrchr(path,'.') - path);
			path[strrchr(path, '.') - path] = '\0';
		}
		
		out = fopen(path, "w");
	}
	
	if (!out)
	{
		error("failed to open destination", 1, 0);
	}
	
	parser(out, src, modeset(path));
	
	fclose(src);
	if (!to_stdout)
	{
		fclose(out);
	}
}

static noreturn void help()
{
	puts("Usage ib: [FILE] ...\n\n" \
	     "ib is a transpiler for languages which are not line based\n\n" \
	     " -h --help    -> print this page\n" \
	     " -v --version -> show current version\n" \
	     " -o --output  -> overwrite output path for *ALL* defined ib files\n" \
	     " -s --spaces  -> use defined amount of spaces as indentation\n" \
	     " -S --stdout  -> output to stdout instead of file");
	
	exit(0);
}

static noreturn void version()
{
	fputs("Ib version "VERSION"\n", stdout);
	
	exit(0);
}

enum arg_mode
{
	noarg,
	nothing,
	space,
	soutput
};

typedef enum arg_mode amode;

static amode long_arg_parser(const char *arg)
{
	if (!strcmp("version", arg))
	{
		version();
	}
	
	if (!strcmp("help", arg))
	{
		help();
	}
	
	if (!strcmp("tab", arg))
	{
		spaces = 0;
		return nothing;
	}
	
	if (!strcmp("spaces", arg))
	{
		return space;
	}
	
	if (!strcmp("stdout", arg))
	{
		to_stdout = true;
		return nothing;
	}
	
	if (!strcmp("output", arg))
	{
		return soutput;
	}
	
	warn("invalid option", arg, 0);
	return nothing;
}

static amode short_arg_parser(const char *arg)
{
	psize i = 0 ;
	while(arg[i])
	{
		switch (arg[i])
		{
			case 'h':
				help();
			
			case 'V':
				version();
			
			case 'o':
				return soutput;
			
			case 's':
				return space;
			
			case 'S':
				to_stdout = true;
				return nothing;
			
			default :
				char invalid[1];
				invalid[0] = arg[i];
				
				warn("invalid option", invalid, 0);
				
				return nothing;
		}
		i++;
	}
	return nothing;
}

static amode arg_parser(char *arg, const amode last)
{
	if (arg[0] != '-' || strlen(arg) < 2)
	{
		switch (last)
		{
			case space:
				spaces = atoi(arg);
				return nothing;
			
			case soutput:
				overwrite_out = arg;
				return nothing;
			
			case nothing:;
			case noarg:
				return noarg;
		}
	}
	
	if (arg[1] == '-')
	{
		return long_arg_parser(arg + 2);
	}
	
	return short_arg_parser(arg + 1);
}

int main(const int argc, char **argv)
{
	if (argc < 2)
	{
		help();
	}
	
	char  *paths[FILE_MAX];
	psize pathc    = 0;
	amode argument = nothing;
	
	while (*++argv)
	{
		argument = arg_parser(*argv, argument);
		
		if (argument == noarg)
		{
			if (pathc == FILE_MAX)
			{
				error("too many files", 1, 0);
			}
			
			paths[pathc++] = *argv;
		}
	}
	
	while (pathc)
	{
		load_file(paths[--pathc]);
	}
	
	exit(0);
}
