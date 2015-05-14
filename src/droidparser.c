/**
 * section: xmlReader
 * synopsis: Parse an XML file with an xmlReader
 * purpose: Demonstrate the use of xmlReaderForFile() to parse an XML file
 *          and dump the informations about the nodes found in the process.
 *          (Note that the XMLReader functions require libxml2 version later
 *          than 2.6.)
 * usage: reader1 <filename>
 * test: reader1 test2.xml > reader1.tmp && diff reader1.tmp $(srcdir)/reader1.res
 * author: Daniel Veillard
 * copy: see Copyright for the status of this software.
 */

#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <libxml/xmlreader.h>

#include "droidparser.h"
#include "bluetooth.h"

/* Device Types */
#define BT_DEVICE_TYPE_BREDR   0x01
#define BT_DEVICE_TYPE_BLE     0x02
#define BT_DEVICE_TYPE_DUMO    0x03
uint8_t device_type;

#define DEV_CLASS_LEN   3
uint8_t dev_class[DEV_CLASS_LEN];

xmlChar *tag;

bdaddr_t *addr; /* local adapter mac address */
bdaddr_t *bdaddr; /* remote device mac address */

const char *val_to_str(const uint32_t val, const value_string *vs)
{
	int i = 0;

	if (vs) {
		while (vs[i].strptr) {
			if (vs[i].value == val) {
				return(vs[i].strptr);
			}
			i++;
		}
	}

	return NULL;
}

static void parse_services(xmlTextReaderPtr reader)
{
	char *value;
	char *service;
	uint16_t uuid16;

	value = (char *)xmlTextReaderConstValue(reader);

	while ((service = strsep(&value, " ")) != NULL) {
		if (0x00 == service[0])
			continue;
		printf("\n\t\t\t %s: ", service);

		service = strsep(&service, "-");
		uuid16 = strtol(service, NULL, 16);
		printf("%04x ", uuid16);
		printf("%s", val_to_str(uuid16, bluetooth_uuid_vals));
	}
	printf("\n");
}

static void parse_timestamp(xmlTextReaderPtr reader)
{
	char *value;
	time_t timestamp;

	value = (char *)xmlTextReaderConstValue(reader);
	timestamp = strtol(value, NULL, 10);
	printf(" %s", ctime(&timestamp));
}

static void parse_bdaddr(xmlTextReaderPtr reader)
{
	const char *value;
	value = (char *)xmlTextReaderConstValue(reader);
	if (value != NULL) {
		printf(" %s\n", value);
	}
	addr = strtoba(value);
}

static void parse_cod(xmlTextReaderPtr reader)
{
	const xmlChar *value;

	value = xmlTextReaderConstValue(reader);
	if (value != NULL) {
		printf(" %s\n", value);
	}
}

/* TODO */
static void parse_hogp(xmlTextReaderPtr reader)
{
	char *value;
	char *report;
	uint16_t uuid16;

	value = (char *)xmlTextReaderConstValue(reader);
	while ((report = strsep(&value, " ")) != NULL) {
		printf("\n\t\t\t %s", report);
	}
	printf("\n");
}

/* TODO */
static void parse_hid_descriptor(xmlTextReaderPtr reader)
{
	const xmlChar *value;
	value = xmlTextReaderConstValue(reader);
	if (value != NULL) {
		printf("\n\t\t\t %s", value);
	}
	printf("\n");
}

/* TODO */
static void parse_gatt_attribute(xmlTextReaderPtr reader)
{
	char *value;
	char *attr;

	value = (char *)xmlTextReaderConstValue(reader);
	while ((attr = strsep(&value, " ")) != NULL) {
		printf("\n\t\t\t %s", attr);
	}
	printf("\n");
}

static void parse_text_node(xmlTextReaderPtr reader)
{
	const xmlChar *value;
	value = xmlTextReaderConstValue(reader);

	if (value != NULL) {
		printf(" %s\n", value);
	}
}

/**
 * processNode:
 * @reader: the xmlReader
 *
 * Dump information about the current node
 */
static void processNode(xmlTextReaderPtr reader)
{
	int depth, type;

	type = xmlTextReaderNodeType(reader);
	if ((type == XML_ELEMENT_DECL) || (type == XML_DTD_NODE))
		return;

	depth = xmlTextReaderDepth(reader);

	if ((type == XML_ELEMENT_NODE) && (depth == 2))
		printf("\n");

	if (2 == depth)
		printf("\t");
	else if (3 == depth)
		printf("\t\t");

	xmlNodePtr node = xmlTextReaderCurrentNode(reader);
	if (xmlTextReaderNodeType(reader) == XML_ELEMENT_NODE && node && node->properties) {
		xmlAttr *attribute = node->properties;
		while (attribute && attribute->name && attribute->children) {
			tag = xmlNodeListGetString(node->doc, attribute->children, 1);
			printf ("%s%c", tag, (depth == 2) ? '\n' : ':');

			bdaddr = strtoba((const char *)tag);
			if (bdaddr)
				add_remote_device(bdaddr);

			attribute = attribute->next;
			/* tag name is what we need */
			break;
		}
	}

	if (xmlTextReaderNodeType(reader) == XML_TEXT_NODE) {
		if (xmlStrstr((xmlChar *)"Service", tag))
			parse_services(reader);
		else if (xmlStrstr((xmlChar *)"HidDescriptor", tag))
			parse_hid_descriptor(reader);
		else if (xmlStrstr((xmlChar *)"HogpRpt", tag))
			parse_hogp(reader);
		else if (xmlStrstr((xmlChar *)"GattAttrs", tag))
			parse_gatt_attribute(reader);
		else if (xmlStrstr((xmlChar *)"DevClass", tag))
			parse_cod(reader);
		else if (xmlStrstr((xmlChar *)"Timestamp", tag))
			parse_timestamp(reader);
		else if (xmlStrstr((xmlChar *)"Address", tag))
			parse_bdaddr(reader);
		else
			parse_text_node(reader);
	}
}

/**
 * streamFile:
 * @filename: the file name to parse
 *
 * Parse and print information about an XML file.
 */
static void streamFile(const char *filename)
{
	xmlTextReaderPtr reader;
	int ret;

	reader = xmlReaderForFile(filename, NULL, 0);
	if (reader != NULL) {
		ret = xmlTextReaderRead(reader);
		while (ret == 1) {
			processNode(reader);
			ret = xmlTextReaderRead(reader);
		}
		xmlFree(tag);
		xmlFreeTextReader(reader);
		if (ret != 0) {
			fprintf(stderr, "%s : failed to parse\n", filename);
		}
	} else {
		fprintf(stderr, "Unable to open %s\n", filename);
	}
}

int main(int argc, char **argv)
{
	/*
	 * this initialize the library and check potential ABI mismatches
	 * between the version it was compiled for and the actual shared
	 * library used.
	 */
	LIBXML_TEST_VERSION


	if (argc != 2)
		streamFile("/data/misc/bluedroid/bt_config.xml");
	else
		streamFile(argv[1]);

	bdaddr_conflict_detect(addr);
	dump_remote_device();

	/*
	 * Cleanup function for the XML library.
	 */
	xmlCleanupParser();
	/*
	 * this is to debug memory for regression tests
	 */
	xmlMemoryDump();

	return 0;
}
