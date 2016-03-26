/* Xep128: Minimalistic Enterprise-128 emulator with focus on "exotic" hardware
   Copyright (C)2016 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>
   http://xep128.lgb.hu/

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */


#include "xepem.h"


enum configItemEnum_t {
	CONFITEM_INT,
	CONFITEM_STR,
	CONFITEM_BOOL,
	CONFITEM_FLOAT
};

struct configOption_st {
	const char *name;
	const int   type;
	const char *defval;
	const int   subopt;
	const char *help;
};

struct configSetting_st {
	const struct configOption_st *opt;
	int   subopt;
	char *value;
};


#define DEBUGFILE_OPT "debug"

/* Default keyboard mapping can be found in keyboard_mapping.c */
static const struct configOption_st configOptions[] = {
	{ "audio",	CONFITEM_BOOL,	"0",		0, "Enable audio output"	},
#ifdef _WIN32
	{ "conwin",	CONFITEM_BOOL,	"0",		0, "Keep (1) console window open" },
#endif
	{ DEBUGFILE_OPT,CONFITEM_STR,	"none",		0, "Enable debug messages written to a specified file" },
	{ "fullscreen",	CONFITEM_BOOL,	"0",		0, "Start in full screen"	},
	{ "mousemode",	CONFITEM_INT,	"1",		0, "Set mouse mode, 1-3 = J-column 2,4,8 bytes and 4-6 the same for K-column" },
	{ "primo",	CONFITEM_STR,	"none",		0, "Start in Primo emulator mode (if not \"none\")" },
	{ "printfile",	CONFITEM_STR,	PRINT_OUT_FN,	0, "Printing into this file"	},
	{ "ram",	CONFITEM_STR,	"128",		0, "RAM size in Kbytes (decimal) or segment specification(s) prefixed with @ in hex (VRAM is always assumed), like: @C0-CF,E0,E3-E7" 	},
	{ "rom",	CONFITEM_STR,	NULL,		1, "ROM image, format is \"rom@xx=filename\" (xx=start segment in hex), use rom@00 for EXOS or combined ROM set" },
	{ "sdimg",	CONFITEM_STR,	SDCARD_IMG_FN,	0, "SD-card disk image (VHD) file name/path" },
	/* should be the last on the list, as this is handled specially not in the config storage for real */
	{ "epkey",	CONFITEM_STR,	NULL,		1, "Define a given EP/emu key, format epkey@xy=SDLname, where x/y are row/col in hex or spec code (ie screenshot, etc)." },
	{ NULL, 0, NULL, 0, NULL }
};


static struct configSetting_st *config = NULL;
static int config_size = 0;
char *app_pref_path, *app_base_path;
char current_directory[PATH_MAX + 1];
SDL_version sdlver_compiled, sdlver_linked;
FILE *debug_file = NULL;
int sdl_v204;




static const char *disclaimer =
	"LICENSE: " WINDOW_TITLE " is a GNU/GPL version 2 (or later) software. <http://gnu.org/licenses/gpl.html>" NL
	"LICENSE: This is free software; you are free to change and redistribute it." NL
	"LICENSE: There is NO WARRANTY, to the extent permitted by law.";




FILE *open_emu_file ( const char *name, const char *mode, char *pathbuffer )
{
	const char *name_used = name;
	const char *policy = "guessing";
	const char *prefixes[] = {
		current_directory,	// try in the current directory first
#ifndef _WIN32
		DATADIR "/",		// try in the DATADIR, it makes sense on UNIX like sys
#endif
		app_base_path,		// try at base path (where executable is)
		app_pref_path,		// try at pref path (user writable area)
		NULL
	};
	int a = 0;
	FILE *f;
	// try to detect absolue path, Win32 related part tries to detect the possibility of X:\... syntax
	if (
		name[0] == DIRSEP[0]
#ifdef _WIN32
		|| (strlen(name) > 3 && name[1] == ':' && name[2] == DIRSEP[0])
#endif
	) {
		prefixes[0] = "";
		prefixes[1] = NULL;
		policy = "absolue";
	} else if (name[0] == '@') {		// @ means user preference directory related path names
		prefixes[0] = app_pref_path;
		prefixes[1] = NULL;
		name_used = name + 1;
		policy = "pref-dir";
	}
	while (prefixes[a] != NULL)
		if (strcmp(prefixes[a], "?")) {
			sprintf(pathbuffer, "%s%s", prefixes[a], name_used);
			DEBUGPRINT("OPEN: trying file \"%s\" [mode: %s] as path \"%s\" [%s]: ",
				name, mode, pathbuffer, policy
			);
			f = fopen(pathbuffer, mode);
			if (f == NULL) {
				a++;
				DEBUGPRINT("*FAILED*: %s" NL, ERRSTR());
			} else {
				DEBUGPRINT("(fd=%d) OK" NL, fileno(f));
				return f;
			}
		}
	DEBUGPRINT("OPEN: no file could be open for \"%s\"" NL, name);
	strcpy(pathbuffer, name);
	return NULL;
}



static const struct configOption_st *search_opt ( const char *name, int subopt )
{
	const struct configOption_st *opt = configOptions;
	while (opt->name && strcasecmp(name, opt->name))
		opt++;
	if (!opt->name)
		return NULL;
	if ((subopt >= 0 && !opt->subopt) || (subopt < 0 && opt->subopt))
		return NULL;
	return opt;
}



static struct configSetting_st *search_setting ( const struct configOption_st *opt, int subopt )
{
	struct configSetting_st *st = config;
	int n = config_size;
	while (n--) {
		if (st->opt == opt && st->subopt == subopt)
			return st;
		st++;
	}
	return NULL;
}



int config_set ( const char *name, int subopt, const char *value )
{
	struct configSetting_st *st;
	const struct configOption_st *opt = search_opt(name, subopt);
	if (!opt)
		return 1;
	st = search_setting(opt, subopt);
	if (st) {
		free(st->value);
	} else {
		config = realloc(config, (config_size + 1) * sizeof(struct configOption_st));
		check_malloc(config);
		st = config + (config_size++);
		st->opt = opt;
		st->subopt = subopt;
	}
	st->value = strdup(value);
	check_malloc(st->value);
	return 0;
}



static void config_set_internal ( const char *name, int subopt, const char *value )
{
	if (config_set(name, subopt, value)) {
		ERROR_WINDOW("Internal built-in configuration error: config_set_internal(\"%s\",%d,\"%s\")", name, subopt, value);
		exit(1);
	}
}



static int config_set_user ( const char *name, int subopt, const char *value, int gui_error )
{
	if (!strcmp(name, "epkey")) {
		if (keymap_set_key_by_name(value, subopt)) {
			if (gui_error)
				ERROR_WINDOW("Invalid SDL PC key scan code name: %s", value);
			else
				fprintf(stderr, "ERROR: Invalid SDL PC key scan code name: %s" NL, value);
			return 1;
		}
		return 0;
	}
	return config_set(name, subopt, value);
}


/* Emulator should call this function to query a value of an option.
   Note: for frequent usage, time critical situation, value should be asked ONCE,
   and then should be stored and used to avoid looking it up again and again.
   Caller must be careful, the casting of "value" is based on the config key type,
   you should give pointer of the right type! You MUST NOT modify the returned
   entity pointed by value!!
   In case of a non-existent key query, program will exit.
   value = NULL can be passed to *test* the existence of a setting (without exiting in case of failure)
   Returned pointer (if not NULL) can be used to re-fetch the value with function config_getopt_pointed()  */
void *config_getopt ( const char *name, const int subopt, void *value )
{
	struct configSetting_st *st;
	const struct configOption_st *opt = search_opt(name, subopt);
	if (!opt) {
		if (!value)
			return NULL;
		ERROR_WINDOW("config_getopt(\"%s\",%d) failed, no optname found", name, subopt);
		exit(1);
	}
	st = search_setting(opt, subopt);
	if (!st) {
		if (!value)
			return NULL;
		ERROR_WINDOW("config_getopt(\"%s\",%d) failed, no setting found", name, subopt);
		exit(1);
	}
	if (value)
		config_getopt_pointed(st, value);
	return st;
}


void config_getopt_pointed ( void *st_in, void *value )
{
	struct configSetting_st *st = st_in;
	switch (st->opt->type) {
		case CONFITEM_FLOAT:
			*(double*)value = atof(st->value);
			break;
		case CONFITEM_BOOL:
		case CONFITEM_INT:
			*(int*)value = atoi(st->value);
			break;
		case CONFITEM_STR:
			*(char**)value = st->value;
			break;
		default:
			ERROR_WINDOW("config_getopt(\"%s\",%d) failed, unknown config item type %d", st->opt->name, st->subopt, st->opt->type);
			exit(1);
	}
}



static void str_rstrip ( char *p )
{
	char *e = p + strlen(p) - 1;
	while (e >= p && (*e == '\r' || *e == '\n' || *e == '\t' || *e == ' '))
		*(e--) = '\0';
}



static int parse_cfgfile_line ( char *buffer, char **key, char **value )
{
	char *p;
	*key = *value = NULL;
	while (*buffer == ' ' || *buffer == '\t') buffer++;
	p = strchr(buffer, '#');
	if (p) *p = 0;
	str_rstrip(buffer);
	if (!*buffer) return 0;	// empty line, or only comment in this line
	p = strchr(buffer, '=');
	if (!p) return 1;
	*(p++) = 0;
	while (*p == ' ' || *p == '\t') p++;
	*value = p;
	*key = buffer;
	//str_rstrip(p);
	str_rstrip(buffer);
	return !*p || !*buffer;
}



static int separate_key ( const char *buffer, char *key )
{
	int ret;
	char *p = strchr(buffer, '@');
	if (!p) {
		strcpy(key, buffer);
		return -1;
	}
	memcpy(key, buffer, p - buffer);
	key[p - buffer] = 0;
	p++;
	if (strlen(p) != 2)
		return -2;
	if (sscanf(p, "%02x", &ret) != 1)
		return -2;
	return ret;
}



static int load_config_file_stream ( FILE *f, const char *name )
{
	char buffer[1024];
	int lineno = 1;
	while (fgets(buffer, sizeof(buffer), f)) {
		char *key, *val;
		char orig_line[sizeof(buffer)];
		if (strlen(buffer) > sizeof(buffer) - 4) {
			ERROR_WINDOW("Config file %s has too long line at lineno %d", name, lineno);
			return 1;
		}
		strcpy(orig_line, buffer);
		str_rstrip(orig_line);
		if (parse_cfgfile_line(buffer, &key, &val)) {
			ERROR_WINDOW("Config file %s has syntax error at lineno %d line \"%s\"", name, lineno, orig_line);
			return 1;
		}
		if (key && val) {
			char cleankey[sizeof(buffer)];
			int subopt = separate_key(buffer, cleankey);
			//str_rstrip(buffer);
			//printf("[%d][%s]%s = %s\n", lineno, orig_line, key, val);
			DEBUG("CONFIG: FILE: %s@%d = %s" NL, cleankey, subopt, val);
			if (subopt < -1 || config_set_user(cleankey, subopt, val, 1)) {
				ERROR_WINDOW("Config file %s has invalid option key/value at lineno %d: %s = %s", name, lineno, key, val);
				return 1;
			}
		}
		lineno++;
	}
	return 0;
}



/* Must be already sure, that even number of arguments exist in the command line */
static int parse_command_line ( int argc , char **argv )
{
	while (argc) {
		char cleankey[1024];
		int subopt;
		if (argv[0][0] != '-') {
			fprintf(stderr, "FATAL: Command line error: option name should begin with '-' for option: %s" NL, argv[0]);
			return 1;
		}
		subopt = separate_key(argv[0] + 1, cleankey);
		DEBUG("CONFIG: CLI: %s@%d = %s" NL, cleankey, subopt, argv[1]);
		if (subopt < -1 || config_set_user(cleankey, subopt, argv[1], 0)) {
			fprintf(stderr, "FATAL: Command line error: invalid key/value %s %s" NL, argv[0], argv[1]);
			return 1;
		}
		argv += 2;
		argc -= 2;
	}
	return 0;
}



static void dump_config ( FILE *fp )
{
	const struct configOption_st *opt = configOptions;
	struct configSetting_st *st = config;
	int n = config_size;
	/* header */
	fprintf(fp, 
		"# Xep128 default built-in configuration as a sample / template file." NL 
		"# Feel free to customize for your needs, and rename to config to be loaded automatically." NL
		"# Delete this _template_ file (not the one renamed as config) Xep128 to allow to re-create" NL
		"# in case of some new options with a new version." NL NL
		"# Generic, simple options" NL NL
	);
	/* dump options without subopt */
	while (n--) {
		if (st->subopt == -1)
			fprintf(fp, "%s = %s\t# %s" NL, st->opt->name, st->value, st->opt->help);
		st++;
	}
	/* dump options with subopt */
	while (opt->name) {
		if (opt->subopt) {
			/*char *keydesc = NULL;
			if (!strcmp(opt->name, "epkey"))
				keydesc = "\t# ";*/
			n = config_size;
			st = config;
			fprintf(fp, NL "# %s" NL NL, opt->help);
			while (n--) {
				if (st->opt == opt)
					fprintf(fp, "%s@%02x = %s" NL,
						opt->name, st->subopt, st->value
					);
				st++;
			}
		}
		opt++;
	}
	// dump the keyboard mapping ...
	keymap_dump_config(fp);
}



static void save_sample_config ( const char *name )
{
	char path[PATH_MAX + 1];
	FILE *f = open_emu_file(name, "r", path);
	if (f) {
		fclose(f);
		DEBUGPRINT("CONFIG: sample configuration %s (%s) already existis, skipping to create." NL, name, path);
		return;
	}
	f = open_emu_file(name, "w", path);
	if (f) {
		dump_config(f);
		fclose(f);
		INFO_WINDOW("Note: created sample config file %s", path);
	} else
		INFO_WINDOW("Note: cannot create sample config file %s", path);
}



#if !SDL_VERSION_ATLEAST(2, 0, 2)
#	error "We need SDL2 version 2.0.2 at least!"
#endif



int is_help_request_option ( const char *opt )
{
	char c = *(opt++);
	if ((
			c == '-' || c ==  '/'
		) && (
			!strcasecmp(opt, "h")    || !strcasecmp(opt, "-h")    ||
			!strcasecmp(opt, "help") || !strcasecmp(opt, "-help") ||
			!strcasecmp(opt, "?")    || !strcasecmp(opt, "-?")
	)) {
		if (c == '/')
			ERROR_WINDOW("Plesse note that you MUST use - instead of / with options!\nSo use -h or -? instead of ugly /h or /?");
		return 1;
	}
	return 0;
}





int config_init ( int argc, char **argv )
{
	const char *config_name = DEFAULT_CONFIG_FILE;	// name of the used config file, can be overwritten via CLI
	const struct configOption_st *opt;
	const char *exe = argv[0];
	int default_config = 1;
	int testparsing = 0;
	argc--; argv++;
#ifdef _WIN32
	console_open_window();
#endif
	SDL_VERSION(&sdlver_compiled);
	SDL_GetVersion(&sdlver_linked);
	sdl_v204 = (sdlver_linked.major >= 2 && (sdlver_linked.patch >= 4 || sdlver_linked.minor)); // used to work-around linked/compiled mismatch and using 2.0.4 features ...
	/* SDL info on paths */
	app_pref_path = SDL_GetPrefPath("nemesys.lgb", "xep128");
	app_base_path = SDL_GetBasePath();
	if (app_pref_path == NULL) app_pref_path = SDL_strdup("?");
	if (app_base_path == NULL) app_base_path = SDL_strdup("?");
	if (getcwd(current_directory, sizeof current_directory) == NULL) {
		ERROR_WINDOW("Cannot query the current directory: %s", ERRSTR());
		return 1;
	}
	strcat(current_directory, DIRSEP);
	/* ugly hack: pre-parse comand line to find debug statement (to be worse, it does not handle single argument options too well ... */
	while (testparsing < argc) {
		if (!strcmp(argv[testparsing], "-" DEBUGFILE_OPT) && testparsing != argc - 1 && strcmp(argv[testparsing + 1], "none")) {
			debug_file = fopen(argv[testparsing + 1], "w");
			DEBUGPRINT("DEBUG: enable logging into file: %s" NL, argv[testparsing + 1]);
			if (debug_file == NULL)
				fprintf(stderr, "Cannot open debug logging file: %s" NL, argv[testparsing + 1]);
			break;
		}
		testparsing++;
	}
	/* end of ugly hack */
	/* let's continue with the info block ... */
	testparsing = 0;
	DEBUGPRINT("%s %s v%s %s %s" NL
		"GIT %s compiled by (%s) at (%s) with (%s)-(%s)" NL
		"Platform: (%s) (%d-bit), video: (%s), audio: (%s), "
		"SDL version compiled: (%d.%d.%d) and linked: (%d.%d.%d) rev (%s)" NL NL,
		WINDOW_TITLE, DESCRIPTION, VERSION, COPYRIGHT, PROJECT_PAGE,
		BUILDINFO_GIT, BUILDINFO_ON, BUILDINFO_AT, CC_TYPE, BUILDINFO_CC,
		SDL_GetPlatform(), ARCH_BITS, SDL_GetCurrentVideoDriver(), SDL_GetCurrentAudioDriver(),
		sdlver_compiled.major, sdlver_compiled.minor, sdlver_compiled.patch,
		sdlver_linked.major, sdlver_linked.minor, sdlver_linked.patch, SDL_GetRevision()
	);
	DEBUGPRINT("PATH: executable: %s" NL, exe);
	/* SDL path info block printout */
	DEBUGPRINT("PATH: SDL base path: %s" NL, app_base_path);
	DEBUGPRINT("PATH: SDL pref path: %s" NL, app_pref_path);
#ifndef _WIN32
	DEBUGPRINT("PATH: data directory: %s/" NL, DATADIR);
#endif
	DEBUGPRINT("PATH: Current directory: %s" NL NL, current_directory);
	/* Look the very basic command line switches first */
	if (argc && is_help_request_option(argv[0])) {
		opt = configOptions;
		printf("USAGE:" NL NL
			"\t%s -optname optval -optname2 optval2 ..." NL NL "OPTIONS:" NL NL
			"-config" NL "\tUse config file (or do not use the default one, if \"none\" is specified). This must be the first option if used! [default: @config]" NL,
			exe
		);
		while (opt->name) {
			printf("-%s" NL "\t%s [default: %s]" NL, opt->name, opt->help, opt->defval ? opt->defval : "-");
			opt++;
		}
		printf(NL "%s" NL, disclaimer);
#ifdef _WIN32
		if (!console_is_open)
			ERROR_WINDOW("Could not dump help, since console couldn't be allocated.");
#endif
		exit(0);
	}
	DEBUGPRINT("%s" NL NL, disclaimer);
	if (argc && !strcasecmp(argv[0], "-testparsing")) {
		testparsing = 1;
		argc--; argv++;
	}
	if (argc & 1) {
		fprintf(stderr, "FATAL: Bad command line: should be even number of parameters (two for an option as key and its value)" NL);
		return 1;
	}
	if (argc > 1 && !strcmp(argv[0], "-config")) {
		default_config = 0;
		config_name = argv[1];
		argc -= 2;
		argv += 2;
	}
	/* Set default (built-in) values */
	opt = configOptions;
	while (opt->name) {
		if (opt->defval)
			config_set_internal(opt->name, -1, opt->defval);
		opt++;
	}
	config_set_internal("rom", 0, COMBINED_ROM_FN);	// set default "combined" ROM image set (from segment 0, starting with EXOS)
	/* Default values for the keyboard follows ... */
	keymap_preinit_config_internal();
	/* check if we have written sample config file, if there is not, let's create one */
	save_sample_config(DEFAULT_CONFIG_SAMPLE_FILE);
	/* now parse config file (not the sample one!) if there is any */
	if (strcasecmp(config_name, "none")) {
		char path[PATH_MAX + 1];
		FILE *f = open_emu_file(config_name, "r", path);
		DEBUGPRINT("CONFIG: config file: %s (%s)" NL, config_name, f ? path : "*** CANNOT OPEN, NOT USING CONFIG FILE ***");
		if (f) {
			if (load_config_file_stream(f, path)) {
				fclose(f);
				return 1;
			}
			fclose(f);
		} else if (!default_config) {
			fprintf(stderr, "FATAL: Cannot open requested config file: %s" NL, config_name);
			return 1;
		} else
			DEBUGPRINT("CONFIG: Skipping default config file (cannot open), using built-in defaults." NL);
	} else
		DEBUGPRINT("CONFIG: Using config file: DISABLED in command line" NL);
	/* parse command line ... */
	if (parse_command_line(argc, argv))
		return -1;
	/* open debug file, if it was not requested via command line at the beginning ... */
	if (!debug_file && strcmp(config_getopt_str(DEBUGFILE_OPT), "none")) {
		debug_file = fopen(config_getopt_str(DEBUGFILE_OPT), "w");
		DEBUGPRINT("DEBUG: enable logging into file: %s" NL, config_getopt_str(DEBUGFILE_OPT));
		if (!debug_file)
                	ERROR_WINDOW("Cannot open debug messages log file requested: %s", config_getopt_str(DEBUGFILE_OPT));
	}
	if (debug_file)
		INFO_WINDOW("DEBUG: Debug messages logging is active");
	else
		printf("DEBUG: No debug messages logging is active." NL);
	/* test parsing mode? */
	if (testparsing) {
		printf(NL "--- TEST DUMP OF *PARSED* CONFIGURATION (requested)" NL NL);
		dump_config(stdout);
		printf(NL "--- END OF TEST PARSING MODE (requested)" NL);
		exit(0);
	}
	DEBUG("CONFIG: End of configuration step." NL NL);
#ifdef _WIN32
	/* Close console, unless user requested it with the -conwin option */
	if (!config_getopt_str("conwin"))
		console_close_window();
#endif
	return 0;
}

