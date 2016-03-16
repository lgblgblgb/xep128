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



/* Default keyboard mapping can be found in keyboard_mapping.c */
static const struct configOption_st configOptions[] = {
	{ "ram", CONFITEM_INT, "128", 0, "RAM size in Kbytes" },
	{ "rom", CONFITEM_STR, NULL, 1, "ROM image, format is \"rom@xx=filename\" (xx=start segment in hex), use rom@00 for EXOS or combined ROM set" },
	{ "sdimg", CONFITEM_STR, "sdcard.img", 0, "SD-card disk image (VHD) file name/path" },
	{ "epkey", CONFITEM_STR, NULL, 1, "Define a given EP/emu key, format epkey@xy=SDLname, where x/y are row/col in hex or spec code (ie screenshot, etc)." },
	{ NULL, 0, NULL, 0, NULL }
};


static struct configSetting_st *config = NULL;
static int config_size = 0;



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



/* Emulator should call this function to query a value of an option.
   Note: for frequent usage, time critical situation, value should be asked ONCE,
   and then should be stored and used to avoid looking it up again and again.
   Caller must be careful, the casting of "value" is based on the config key type,
   you should give pointer of the right type! You MUST NOT modify the returned
   entity pointed by value!!
   In case of a non-existent key query, system error occures!  */
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



int config_init ( int argc, char **argv )
{
	const char *config_name = "config";	// name of the used config file, can be overwritten via CLI
	const struct configOption_st *opt;
	printf("%s Enterprise-128 Emulator %s %s" NL
		"GIT %s compiled by %s at %s with %s %s" NL NL,
		WINDOW_TITLE, VERSION, COPYRIGHT,
		BUILDINFO_GIT, BUILDINFO_ON, BUILDINFO_AT, CC_TYPE, BUILDINFO_CC
	);
	argc--;
	argv++;
	if (argc) {
		if (!strcasecmp(argv[0], "-h") || !strcasecmp(argv[0], "--h") || !strcasecmp(argv[0], "-help") || !strcasecmp(argv[0], "--help")) {
			opt = configOptions;
			while (opt->name) {
				printf("-%s" NL "\t%s [default: %s]" NL, opt->name, opt->help, opt->defval ? opt->defval : "-");
				opt++;
			}
			return 1;
		}
		// TODO: we may want to detect custom configuration file declaration here!
		//if (!strcasecmp(argv[0], "-config") || !strcasecmp(argv[0], "--config"))	

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
	/* now parse config file (not the sample one!) if there is any */
	/* parse command line ... */


	/* Check, if we have written config file, if no, let's create one! */
	dump_config(stdout);
	//exit(0);
	//config_set_internal("ram", -1, "128");
	return 0;
}

