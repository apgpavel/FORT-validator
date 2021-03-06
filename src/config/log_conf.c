#include "config/log_conf.h"

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "log.h"
#include "config/str.h"

#define LOG_LEVEL_VALUE_ERROR "error"
#define LOG_LEVEL_VALUE_WARNING "warning"
#define LOG_LEVEL_VALUE_INFO "info"
#define LOG_LEVEL_VALUE_DEBUG "debug"

#define LOG_OUTPUT_VALUE_SYSLOG "syslog"
#define LOG_OUTPUT_VALUE_CONSOLE "console"

#define DEREFERENCE_UINT(void_value) (*((uint8_t *) void_value))
#define DEREFERENCE_ENUM(void_value) (*((enum log_output *) void_value))

static void
print_log_level(struct option_field const *field, void *value)
{
	char const *str = "<unknown>";

	switch (DEREFERENCE_UINT(value)) {
	case LOG_ERR:
		str = LOG_LEVEL_VALUE_ERROR;
		break;
	case LOG_WARNING:
		str = LOG_LEVEL_VALUE_WARNING;
		break;
	case LOG_INFO:
		str = LOG_LEVEL_VALUE_INFO;
		break;
	case LOG_DEBUG:
		str = LOG_LEVEL_VALUE_DEBUG;
		break;
	}

	pr_info("%s: %s", field->name, str);
}

static void
print_log_output(struct option_field const *field, void *value)
{
	char const *str = "<unknown>";

	switch (DEREFERENCE_ENUM(value)) {
	case SYSLOG:
		str = LOG_OUTPUT_VALUE_SYSLOG;
		break;
	case CONSOLE:
		str = LOG_OUTPUT_VALUE_CONSOLE;
		break;
	}

	pr_info("%s: %s", field->name, str);
}

static int
parse_argv_log_level(struct option_field const *field, char const *str,
    void *result)
{
	if (strcmp(str, LOG_LEVEL_VALUE_ERROR) == 0)
		DEREFERENCE_UINT(result) = LOG_ERR;
	else if (strcmp(str, LOG_LEVEL_VALUE_WARNING) == 0)
		DEREFERENCE_UINT(result) = LOG_WARNING;
	else if (strcmp(str, LOG_LEVEL_VALUE_INFO) == 0)
		DEREFERENCE_UINT(result) = LOG_INFO;
	else if (strcmp(str, LOG_LEVEL_VALUE_DEBUG) == 0)
		DEREFERENCE_UINT(result) = LOG_DEBUG;
	else
		return pr_err("Unknown log level: '%s'", str);

	return 0;
}

static int
parse_argv_log_output(struct option_field const *field, char const *str,
    void *result)
{
	if (strcmp(str, LOG_OUTPUT_VALUE_SYSLOG) == 0)
		DEREFERENCE_ENUM(result) = SYSLOG;
	else if (strcmp(str, LOG_OUTPUT_VALUE_CONSOLE) == 0)
		DEREFERENCE_ENUM(result) = CONSOLE;
	else
		return pr_err("Unknown log output: '%s'", str);

	return 0;
}

static int
parse_json_log_level(struct option_field const *opt, json_t *json,
    void *result)
{
	char const *string;
	int error;

	error = parse_json_string(json, opt->name, &string);
	return error ? error : parse_argv_log_level(opt, string, result);
}

static int
parse_json_log_output(struct option_field const *opt, json_t *json,
    void *result)
{
	char const *string;
	int error;

	error = parse_json_string(json, opt->name, &string);
	return error ? error : parse_argv_log_output(opt, string, result);
}

const struct global_type gt_log_level = {
	.has_arg = required_argument,
	.size = sizeof(uint8_t),
	.print = print_log_level,
	.parse.argv = parse_argv_log_level,
	.parse.json = parse_json_log_level,
	.arg_doc = LOG_LEVEL_VALUE_ERROR
	    "|" LOG_LEVEL_VALUE_WARNING
	    "|" LOG_LEVEL_VALUE_INFO
	    "|" LOG_LEVEL_VALUE_DEBUG,
};

const struct global_type gt_log_output = {
	.has_arg = required_argument,
	.size = sizeof(enum log_output),
	.print = print_log_output,
	.parse.argv = parse_argv_log_output,
	.parse.json = parse_json_log_output,
	.arg_doc = LOG_OUTPUT_VALUE_SYSLOG "|" LOG_OUTPUT_VALUE_CONSOLE,
};
