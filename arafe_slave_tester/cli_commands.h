#ifndef _CLI_COMMANDS_H
#define _CLI_COMMANDS_H



#include "cli.h"
#include <stdint.h>

// If set to 1, command parsing recognizes multi-word arguments in quotes
#define PARSE_QUOTED_ARGS   0

// If set to 1, performs command lookup using a binary search instead of linear.
#define USE_BINARY_SEARCH   0

// maximum length of a command input line
#define CLI_STRBUF_SIZE    64

// Maximum number of arguments in a command (including command).
#define CLI_MAX_ARGC    3

// Table of commands: {"command_word" , function_name }
// Command words MUST be in alphabetical (ascii) order!! (A-Z then a-z) if using binary search
#define CMDTABLE    {"dump"  , cmd_dump },\
                    {"read"  , cmd_read },\
                    {"write" , cmd_write },\
                    {"flash" , cmd_flash },\
                    {"control", cmd_control },\
                    {"5v", cmd_5v },\
					{"sig", cmd_sig },\
					{"trig", cmd_trig },\
					{"sensor", cmd_sensor },\
					{"12v", cmd_12v }

// Custom command function prototypes:
int cmd_dump(uint16_t argc, char *argv[]);
int cmd_read(uint16_t argc, char *argv[]);
int cmd_write(uint16_t argc, char *argv[]);
int cmd_flash(uint16_t argc, char *argv[]);
int cmd_control(uint16_t argc, char *argv[]);
int cmd_5v(uint16_t argc, char *argv[]);
int cmd_12v(uint16_t argc, char *argv[]);
int cmd_sig(uint16_t argc, char *argv[]);
int cmd_trig(uint16_t argc, char *argv[]);
int cmd_sensor(uint16_t argc, char *argv[]);

#endif
