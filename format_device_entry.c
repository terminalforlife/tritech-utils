/*
 * Read an INF file section and output to stdout
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tritech_utils.h"

#define MAXLEN 1024
#define SUBSIZE 32

struct devtypes {
	char label[SUBSIZE];
	char prefix[SUBSIZE];
	char data[SUBSIZE];
};

static struct devtypes id[] = {
	{"vid", ":vid=", ""},
	{"pid", ":pid=", ""},
	{"ven", ":ven=", ""},
	{"dev", ":dev=", ""},
	{"subsys", ":subsys=", ""},
	{"rev", ":rev=", ""},
	{"func", ":func=", ""},
	{"ctlrven", ":cv=", ""},
	{"ctlrdev", ":cd=", ""},
	{"class", ":class=", ""},
	{"cc", ":cc=", ""},
	{"subclass", ":sc=", ""},
	{"prot", ":prot=", ""},
	{"", "", ""},
};


int apply_label_value(char *label, char *value)
{
	int i = 0;

	while (*(id[i].label) != '\0' && *(id[i].label) != '\n' && *(id[i].label) != ' ') {
		if (strcmp(label, id[i].label) == 0) {
			strcpy(id[i].data, id[i].prefix);
			strcat(id[i].data, value);
			return 0;
		}
		i++;
	}

	fprintf(stderr, "error: cannot apply '%s' to label '%s'\n", value, label);

	return -1;
}

/* Get the label value at the specified point in the string */
int get_label_value(char *devstring, char *value)
{
	int i = 0;
	char c;

	while (i < (SUBSIZE - 1)) {
		c = *(devstring + i);
		if (c == '&' || c == '\t' || c == ' ' || c == '\\' || c == '\0') break;
		value[i] = c;
		i++;
	}
	value[i] = '\0';
	//fprintf(stderr, "value: %d: '%s'\n", i, value);

	return i;
}

int main(int argc, char **argv)
{
	FILE *fp = stdin;
	char line[MAXLEN];
	char *devstring;
	char label[SUBSIZE];
	char value[SUBSIZE];

	char type[8] = "";

	int quote;
	int comma;
	int i;
	int lpos;
	int len;

	if (argc != 4) {
		fprintf(stderr, "Usage: %s winver_flags driverver_datecode inf_file_path\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	memset(line, 0, MAXLEN);

	while (fgets(line, MAXLEN, fp)) {
		quote = 0;
		comma = 0;
		lpos = 0;

		i = 0;
		/* Reset existing label data */
		while (*(id[i].label) != '\0') {
			*(id[i].data) = '\0';
			i++;
		}
		i = 0;

		len = strlen(line);
		if (line[len - 1] == '\n') {
			len--;
			line[len] = '\0';
		}

		while (lpos < len) {
			if (line[lpos] == '\0') break;
			/* Don't interpret anything if inside quote marks */
			if (line[lpos] == '"') {
				if (!quote) quote = 1;
				else quote = 0;
			}
	
			/* Find the comma, then skip spaces until the device entry is reached */
			if (!quote) {
				if (line[lpos] == ',') {
					comma = 1;
				} else if (comma == 1 && line[lpos] != ' ') {
					/* Parse and output reformatted device string */
					devstring = line + lpos;

					//fprintf(stderr, "i_dstring: '%s'\n", devstring);

					/* Get type. ACPI requires special handling */
					/* ACPI can begin with a * or 'acpi\' */
					i = 0;
					if (*devstring == '*') i = 1;
					if (strncasecmp(devstring, "acpi\\", 5) == 0) i = 5;
					if (i > 0) {
						strcpy(type, "acpi");
						strcpy(label, "dev");
						devstring += i;
						lpos += i;
						i = get_label_value(devstring, value);
						devstring += i;
						lpos += i;
						if (apply_label_value(label, value)) break;
						continue;
					} else if (strncasecmp(devstring, "hdaudio\\", 8) == 0) {
						strcpy(type, "hdaudio");
						i = 8;
					} else if (strncasecmp(devstring, "usb\\", 4) == 0) {
						strcpy(type, "usb");
						i = 4;
					} else if (strncasecmp(devstring, "pci\\", 4) == 0) {
						strcpy(type, "pci");
						i = 4;
					} else {
						fprintf(stderr, "Can't determine device type for %s\n", devstring);
						break;
					}

					devstring += i;
					lpos += i;

					while (*devstring != '\0' && *devstring != '\n' && *devstring != ' ') {
						/* Ampersands and backslashes are field separators, skip them */
						if (*devstring == '&' || *devstring == '/' || *devstring == '\\') {
							devstring++;
							lpos++;
							continue;
						}

						i = 0;
						//fprintf(stderr, "devstring: '%s'\n", devstring);

						/* Get the device information label being processed */
						if (strncmp(devstring, "ctrl_ven", 8) == 0) {
							strcpy(label, "ctrlven");
							i = 9;
						} else if (strncmp(devstring, "ctrl_dev", 8) == 0) {
							strcpy(label, "ctrldev");
							i = 9;
						} else {
							/* Copy device info label verbatim */
							while (i < (SUBSIZE - 1) && *devstring != '_') {
								label[i] = *devstring;
								//fprintf(stderr, "label[%d] = '%c'\n", i, *devstring);
								devstring++;
								i++;
							}
							label[i] = '\0';
							/* Skip label-value separator */
							devstring++;
							i++;
						}
						lpos += i;

						/* Get the value of the label */
						i = get_label_value(devstring, value);
						devstring += i;
						lpos += i;
						//fprintf(stderr, "l_dstring: '%s'\n", devstring);

						/* Find the correct label and apply the value */
						if (apply_label_value(label, value)) break;
						if (*devstring == '\0' || *devstring == ' ') break;
						continue;
					}
				}
			}
			if (line[lpos] == '\0') break;
			/* Skip chars until the line is exhausted */
			lpos++;
		}

		if (comma == 0) fprintf(stderr, "error: no comma in device line (required)\n");
		i = 0;

		/* Output final device entry (if the "type" field is filled) */
		if (*type != '\0') {
			printf("%s", type);
			while (*(id[i].label) != '\0') {
				printf("%s", id[i].data);
				i++;
			}
			printf(":win=%s:dv=%s:inf=%s\n", argv[1], argv[2], argv[3]);
		}
	}

	exit(EXIT_SUCCESS);
}
