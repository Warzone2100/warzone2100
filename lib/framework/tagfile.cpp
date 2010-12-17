// Written by Per I Mathisen, 2007
// Released into the public domain, no rights reserved.
// ANSI C + stdint.h
//
// See header file for documentation.
#include "tagfile.h"

#include "frame.h"

#include "string_ext.h"
#include "physfs_ext.h"

// Tags of values 0x0 and 0xFF are reserved as instance separators and group end
// tags, respectively. You cannot use a separator in your definition file, but
// you must use the group end tag whenever you use a group tag.
#define TAG_SEPARATOR 0
#define TAG_GROUP_END 255

// This value is saved in the binary file and is used for fast and safe skipping
// ahead through the file, even though we may not know every tag definition in it.
enum internal_types
{
	TF_INT_U8, TF_INT_U16, TF_INT_U32, TF_INT_S8, TF_INT_S16, TF_INT_S32, TF_INT_FLOAT,
	TF_INT_U16_ARRAY, TF_INT_FLOAT_ARRAY, TF_INT_U8_ARRAY, TF_INT_GROUP, TF_INT_BOOL,
	TF_INT_S32_ARRAY
};

// A definition group
typedef struct _define
{
	unsigned int vm; //! value multiple
	element_t element; //! tag number
	char vr[2]; //! value representation (type)
	struct _define *parent;	//! parent group
	struct _define *group;	//! child group
	struct _define *next;	//! sibling group
	struct _define *current; //! where in the sibling list we currently are
	bool defaultval; //! default value
	union {
		uint32_t uint32_tval;
		int32_t int32_tval;
		float floatval;
	} val; //! actual data
	// debugging temp variables below
	int countItems; // check group items against number of separators given
	int expectedItems; // group items expected in current group
} define_t;


static bool tag_error = false;		// are we in an error condition?
static define_t *first = NULL;	// keep track of first group
static define_t *current = NULL;	// currently iterated group
static int line = 0;			// current definition line, report in error message
static bool readmode = true;		// are we in read or write mode?
static char *bufptr = NULL;		// pointer to definition file buffer
static PHYSFS_file *handle = NULL;
static int lastAccess = -1;		// track last accessed tag to detect user errors
static int countGroups = 0;		// check group recursion count while reading definition
static char saveDefine[PATH_MAX];	// save define file for error messages
static char saveTarget[PATH_MAX];	// save binary file for error messages

#undef DEBUG_TAGFILE

static void tf_error(const char * fmt, ...) WZ_DECL_FORMAT(printf, 1, 2);
static void tf_print_nested_groups(unsigned int level, define_t *group);


#define TF_ERROR(...) \
do { \
	tf_error(__VA_ARGS__); \
	assert(!"tagfile error"); \
} while(0)


#define VALIDATE_TAG(_def, _tag, _name) \
do { \
	if (_def[0] != current->vr[0] || _def[1] != current->vr[1]) \
	TF_ERROR("Tag %#04x is given as a %s but this does not conform to value representation \"%c%c\"", \
	         (unsigned int)tag, _name, current->vr[0], current->vr[1]); \
} while(0)


#define CHECK_VALID_TAG(_tag) \
do { \
	assert(tag != TAG_SEPARATOR && tag != TAG_GROUP_END); \
} while(0)


/*
static inline WZ_DECL_CONST const char * bool2string(bool var)
{
	return (var ? "true" : "false");
}
*/


void tf_error(const char * fmt, ...)
{
	va_list ap;
	char errorBuffer[255];

	tag_error = true;

	va_start(ap, fmt);
	vsnprintf(errorBuffer, sizeof(errorBuffer), fmt, ap);
	va_end(ap);
	debug(LOG_ERROR, "%s", errorBuffer);

	tf_print_nested_groups(0, current);

	debug(LOG_ERROR, "While %s '%s' using definition file '%s'\n", readmode ? "reading" : "writing", saveTarget, saveDefine);
}


// Print the calling strack for nested groups on error
void tf_print_nested_groups(unsigned int level, define_t *group)
{
	if (group == NULL)
	{
		return;
	}

	if (level == 0)
	{
		debug(LOG_ERROR, "Group trace:");
	}
	debug(LOG_ERROR, "  #%u: %#04x %2.2s %5u default:%s", level, (unsigned int)group->element, group->vr, group->vm, bool2string(group->defaultval));

	if (group->parent != NULL)
	{
		tf_print_nested_groups(level+1, group->parent);
	}
}


// scan one definition group from definition file; returns true on success
static bool scan_defines(define_t *node, define_t *group)
{
	bool group_end = false;

	while (*bufptr != '\0' && !group_end)
	{
		int count, retval;
		unsigned int readelem;
		char vr[3];

		memset(vr, 0, sizeof(vr));
		node->group = NULL;
		node->parent = group;
		node->val.uint32_tval = 0;
		node->vm = 0;
		node->next = NULL;
		node->element = 0xFF;
		node->expectedItems = 0;
		node->countItems = 0;
		current = node; // for accurate error reporting

		// check line endings
		while (*bufptr == '\n' || *bufptr == '\r')
		{
			if (*bufptr == '\n')
			{
				line++;
			}
			bufptr++;
		}
		// remove whitespace
		while (*bufptr == ' ' || *bufptr == '\t')
		{
			bufptr++;
		}
		// check if # comment line or whitespace
		if (*bufptr == '#' || *bufptr == '\0')
		{
			while (*bufptr != '\n' && *bufptr != '\0')
			{
				bufptr++; // discard rest of line
			}
			continue; // check empty lines and whitespace again
		}

		retval = sscanf(bufptr, "%x %2s %u%n", &readelem, (char *)&vr, &node->vm, &count);
		node->element = readelem;
		while (retval <= 0 && *bufptr != '\n' && *bufptr != '\0') /// TODO: WTF?
		{
			printf("WTF\n");
			bufptr++; // not a valid line, so discard it
		}
		if (retval < 3)
		{
			TF_ERROR("Bad definition, line %d (retval==%d)", line, retval);
			return false;
		}
		node->vr[0] = vr[0];
		node->vr[1] = vr[1];
		bufptr += count;
		if (node->vr[0] == 'S' && node->vr[1] == 'I')
		{
			retval = sscanf(bufptr, " %d", &node->val.int32_tval);
		}
		else if ((node->vr[0] == 'U' && node->vr[1] == 'S') || (node->vr[0] == 'B' && node->vr[1] == 'O'))
		{
			retval = sscanf(bufptr, " %u", &node->val.uint32_tval);
		}
		else if (node->vr[0] == 'F' && node->vr[1] == 'P')
		{
			retval = sscanf(bufptr, " %f", &node->val.floatval);
		}
		else if ((node->vr[0] == 'S' && node->vr[1] == 'T')
		         || (node->vr[0] == 'G' && node->vr[1] == 'R')
		         || (node->vr[0] == 'E' && node->vr[1] == 'N'))
		{
			retval = 0; // these do not take a default value
		}
		else
		{
			TF_ERROR("Invalid value representation %c%c line %d", node->vr[0], node->vr[1], line);
			return false;
		}
		node->defaultval = (retval == 1);
		while (*bufptr != '\n' && *bufptr != '\0')
		{
			bufptr++; // discard rest of line
		}

		if (node->vr[0] == 'G' || node->vr[1] == 'R')
		{
			bool success;

			countGroups++;
			node->group = (define_t *)malloc(sizeof(*node));
			success = scan_defines(node->group, node);

			if (!success)
			{
				return false; // error
			}
		}
		else if (node->vr[0] == 'E' || node->vr[1] == 'N')
		{
			group_end = true; // un-recurse now
			countGroups--;
		}
		if (*bufptr != '\0' && !group_end)
		{
			node->next = (define_t *)malloc(sizeof(*node));
			node = node->next;
		}
	}
	node->next = NULL; // terminate linked list
	return true;
}

static bool init(const char *definition, const char *datafile, bool write)
{
	PHYSFS_file *fp;
	PHYSFS_sint64 fsize, fsize2;
	char *buffer;

	tag_error = false;
	line = 1;
	fp = PHYSFS_openRead(definition);
	debug(LOG_WZ, "Reading...[directory: %s] %s", PHYSFS_getRealDir(definition), definition);
	sstrcpy(saveDefine, definition);
	sstrcpy(saveTarget, datafile);
	if (!fp)
	{
		TF_ERROR("Error opening definition file %s: %s", definition, PHYSFS_getLastError());
		return false;
	}

	fsize = PHYSFS_fileLength(fp);
	assert(fsize > 0);

	buffer = bufptr = (char *)malloc(fsize + 1);
	if (!buffer || !bufptr)
	{
		debug(LOG_FATAL, "init(): Out of memory");
		abort();
		return false;
	}

	bufptr[fsize] = '\0'; // ensure it is zero terminated

 	fsize2 = PHYSFS_read(fp, bufptr, 1, fsize);
	if (fsize != fsize2)
	{
		TF_ERROR("Could not read definitions: %s", PHYSFS_getLastError());
		return false;
	}
	current = NULL; // keeps track of parent group below
	first = (define_t *)malloc(sizeof(*first));
	first->parent = NULL;
	first->group = NULL;
	first->next = NULL;
	first->current = NULL;
	scan_defines(first, NULL);
	free(buffer);
	bufptr = NULL;
	PHYSFS_close(fp);

	if (countGroups != 0)
	{
		TF_ERROR("Error in definition file %s - group tags do not match end tags!", definition);
		countGroups = 0;
		return false;
	}

	if (write)
	{
		fp = PHYSFS_openWrite(datafile);
	}
	else
	{
		fp = PHYSFS_openRead(datafile);
	}
	debug(LOG_WZ, "Reading...[directory: %s] %s", PHYSFS_getRealDir(datafile), datafile);
	handle = fp;
	if (!fp)
	{
		debug(LOG_ERROR, "Error opening data file %s: %s", datafile, PHYSFS_getLastError());
		tagClose();
		return false;
	}
	readmode = !write;
	current = first;
#ifdef DEBUG_TAGFILE
	debug(LOG_ERROR, "opening %s", datafile);
#endif
	return true;
}

bool tagOpenWrite(const char *definition, const char *datafile)
{
	return init(definition, datafile, true);
}

bool tagOpenRead(const char *definition, const char *datafile)
{
	return init(definition, datafile, false);
}

static void remove_defines(define_t *df)
{
	define_t *iter;

	for (iter = df; iter != NULL;)
	{
		define_t *del = iter;

		if (iter->group) // Also remove subgroups
		{
			remove_defines(iter->group);
		}
		iter = iter->next;
		free(del);
	}
}

void tagClose()
{
	if (handle)
	{
		PHYSFS_close(handle);
	}
	remove_defines(first);
	lastAccess = -1; // reset sanity counter
}


bool tagGetError()
{
	return tag_error;
}


// skip ahead in definition list to given element
static bool scan_to(element_t tag)
{
	if (tag == TAG_SEPARATOR)
	{
		return true; // does not exist in definition
	}
	if (lastAccess >= tag)
	{
		TF_ERROR("Trying to access tag %#04x that is not larger than previous tag.", (unsigned int)tag);
		return false;
	}
	lastAccess = tag;
	// Set the right node
	for (; current->next && current->element < tag; current = current->next) {}
	if (current->element != tag)
	{
		TF_ERROR("Unknown element %#04x sought", (unsigned int)tag);
		return false;
	}
	return true;
}


/*********  TAGREAD *********/


// scan forward to after given tag (if read case, we may skip
// several tags to get there)
static bool scanforward(element_t tag)
{
	element_t read_tag;
	uint8_t tag_type;
	PHYSFS_sint64 readsize = 0, fpos;
	uint16_t array_size;
	int groupskip = 0;	// levels of nested groups to skip

	assert(readmode);
	if (tag_error || current == NULL || !readmode)
	{
		return false;
	}

	// Skip in this group-definition until we have reached destination tag
	if (!scan_to(tag) || PHYSFS_eof(handle))
	{
		return false; // error, or try default value
	}
	readsize = PHYSFS_read(handle, &read_tag, 1, 1);
	while (readsize == 1) // Read from file till we've reached the destination
	{
		if (read_tag == tag && groupskip <= 0)
		{
			assert(current->element == tag || tag == TAG_SEPARATOR);
			return true;
		}
		else if (read_tag == TAG_GROUP_END || (read_tag == TAG_SEPARATOR && groupskip > 0))
		{
			/* New element ready already, repeat loop */
			if (read_tag == TAG_GROUP_END)
			{
				groupskip--;
				if (groupskip < 0)
				{
					break; // tag not found
				}
			}
			readsize = PHYSFS_read(handle, &read_tag, 1, 1);
			continue;	// no type, no payload
		}
		else if (read_tag == TAG_SEPARATOR || (read_tag > tag && groupskip <= 0 && tag != TAG_SEPARATOR))
		{
			break; // did not find it
		}

		/* If we got down here, we found something that we need to skip */

		readsize = PHYSFS_read(handle, &tag_type, 1, 1);
		if (readsize != 1)
		{
			TF_ERROR("Error reading tag type when skipping: %s", PHYSFS_getLastError());
			return false;
		}

		/* Skip payload */

		switch (tag_type)
		{
		case TF_INT_U16_ARRAY:
		case TF_INT_FLOAT_ARRAY:
		case TF_INT_U8_ARRAY:
		case TF_INT_S32_ARRAY:
			if (!PHYSFS_readUBE16(handle, &array_size))
			{
				TF_ERROR("Error reading array length when skipping: %s", PHYSFS_getLastError());
				return false;
			}
			break;
		default:
			array_size = 1;
		}

		fpos = PHYSFS_tell(handle);
		switch (tag_type)
		{
		case TF_INT_U8_ARRAY:
		case TF_INT_U8:
		case TF_INT_BOOL:
		case TF_INT_S8: fpos += 1 * array_size; break;
		case TF_INT_FLOAT:
		case TF_INT_U16_ARRAY:
		case TF_INT_U16:
		case TF_INT_S16: fpos += 2 * array_size; break;
		case TF_INT_FLOAT_ARRAY:
		case TF_INT_U32:
		case TF_INT_S32_ARRAY:
		case TF_INT_S32: fpos += 4 * array_size; break;
		case TF_INT_GROUP: 
			fpos += 2;
			groupskip++;
#ifdef DEBUG_TAGFILE
			debug(LOG_ERROR, "skipping group 0x%02x (groupskip==%d)", (unsigned int)read_tag, groupskip);
#endif
			break;
		default:
			TF_ERROR("Invalid value type in buffer");
			return false;
		}

		PHYSFS_seek(handle, fpos);

		/* New element ready now, repeat loop */
		readsize = PHYSFS_read(handle, &read_tag, 1, 1);
	}
	if (readsize != 1)
	{
		TF_ERROR("Could not read tag: %s", PHYSFS_getLastError());
		return false;
	}
	else
	{
		// If we come here, we failed to find the tag. Roll back the last element found, and
		// check for default value.
		fpos = PHYSFS_tell(handle);
		fpos -= sizeof(element_t);
		PHYSFS_seek(handle, fpos);
		assert(current != NULL);
		assert(current->element == tag);

		if (!current->defaultval)
		{
			TF_ERROR("Tag not found and no default: %#04x", (unsigned int)tag);
			return false;
		}
	}

	return false; // tag not found before end of instance, group or file
}

bool tagReadNext()
{
	// scan forward until we find a separator value element tag in this group
	if (!scanforward(TAG_SEPARATOR))
	{
		return false;
	}
	lastAccess = -1; // reset requirement that tags are increasing in value
	if (current->parent == NULL)
	{
		current = first; // topmost group
	}
	else
	{
		current = current->parent->group; // reset to start of group tag index
	}
	current->parent->group->countItems++; // sanity checking
	return true;
}

uint16_t tagReadEnter(element_t tag)
{
	uint16_t elements;
	element_t tagtype;

	assert(readmode);
	if (!scanforward(tag) || !readmode)
	{
		return 0; // none found; avoid error reporting here
	}
	lastAccess = -1; // reset requirement that tags are increasing in value
	if (!PHYSFS_readUBE8(handle, &tagtype))
	{
		TF_ERROR("Error reading group type: %s", PHYSFS_getLastError());
		return 0;
	}
	if (tagtype != TF_INT_GROUP)
	{
		TF_ERROR("Error in group VR, tag %#04x", (unsigned int)tag);
		return 0;
	}
	if (!PHYSFS_readUBE16(handle, &elements))
	{
		TF_ERROR("Error accessing group size: %s", PHYSFS_getLastError());
		return 0;
	}
	if (!current->group)
	{
		TF_ERROR("Cannot enter group, none defined for element %x!", (unsigned int)current->element);
		return 0;
	}
#ifdef DEBUG_TAGFILE
	debug(LOG_ERROR, "entering 0x%02x", (unsigned int)tag);
#endif
	assert(current->group->parent != NULL);
	assert(current->element == tag);
	current->group->expectedItems = elements;	// for debugging and consistency checking
	current->group->countItems = 0;			// ditto
	current->group->parent->current = current; // save where we are. ok, this is very contrived code.
	current = current->group;
	return elements;
}

void tagReadLeave(element_t tag)
{
	if (!scanforward(TAG_GROUP_END))
	{
		TF_ERROR("Cannot leave group, group end tag not found!");
		return;
	}
	if (tag_error)
	{
		return;
	}
	if (current->parent == NULL)
	{
		TF_ERROR("Cannot leave group, at highest level already!");
		return;
	}
	if (current->parent->group->expectedItems > current->parent->group->countItems + 1)
	{
		TF_ERROR("Expected to read %d items in group, found %d",
		         current->parent->group->expectedItems, current->parent->group->countItems);
	}
	current = current->parent->current; // resume iteration
	if (current->element != tag)
	{
		TF_ERROR("Trying to leave the wrong group! We are in %x, leaving %#04x",
		         current->parent != NULL ? (unsigned int)current->parent->element : 0, (unsigned int)tag);
		return;
	}
	lastAccess = current->element; // restart iteration requirement
	assert(current != NULL);
}

uint32_t tagRead(element_t tag)
{
	uint8_t tagtype;

	if (!scanforward(tag))
	{
		if (current->defaultval && current->element == tag)
		{
			return current->val.uint32_tval;
		}
		return 0;
	}

	if (!PHYSFS_readUBE8(handle, &tagtype))
	{
		TF_ERROR("tagread: Tag type not found: %#04x", (unsigned int)tag);
		tag_error = true;
		return 0;
	}
	if (tagtype == TF_INT_U32)
	{
		uint32_t val;
		(void) PHYSFS_readUBE32(handle, &val);
		return val;
	}
	else if (tagtype == TF_INT_U16)
	{
		uint16_t val;
		(void) PHYSFS_readUBE16(handle, &val);
		return val;
	}
	else if (tagtype == TF_INT_U8)
	{
		uint8_t val;
		(void) PHYSFS_readUBE8(handle, &val);
		return val;
	}
	else
	{
		TF_ERROR("readtag: Error reading tag %#04x, bad type", (unsigned int)tag);
		return 0;
	}
}

int32_t tagReads(element_t tag)
{
	uint8_t tagtype;

	if (!scanforward(tag))
	{
		if (current->defaultval && current->element == tag)
		{
			return current->val.int32_tval;
		}
		return 0;
	}

	if (!PHYSFS_readUBE8(handle, &tagtype))
	{
		TF_ERROR("tagreads: Tag type not found: %#04x", (unsigned int)tag);
		return 0;
	}
	if (tagtype == TF_INT_S32)
	{
		int32_t val;
		(void) PHYSFS_readSBE32(handle, &val);
		return val;
	}
	else if (tagtype == TF_INT_S16)
	{
		int16_t val;
		(void) PHYSFS_readSBE16(handle, &val);
		return val;
	}
	else if (tagtype == TF_INT_S8)
	{
		int8_t val;
		(void) PHYSFS_readSBE8(handle, &val);
		return val;
	}
	else
	{
		TF_ERROR("readtags: Error reading tag %#04x, bad type", (unsigned int)tag);
		return 0;
	}
}

float tagReadf(element_t tag)
{
	uint8_t tagtype;

	if (!scanforward(tag))
	{
		if (current->defaultval && current->element == tag)
		{
			return current->val.floatval;
		}
		return 0;
	}

	if (!PHYSFS_readUBE8(handle, &tagtype))
	{
		TF_ERROR("tagreadf: Tag type not found: %#04x", (unsigned int)tag);
		return 0;
	}
	if (tagtype == TF_INT_FLOAT)
	{
		float val;
		(void) PHYSFS_readBEFloat(handle, &val);
		return val;
	}
	else
	{
		TF_ERROR("readtagf: Error reading tag %#04x, bad type", (unsigned int)tag);
		return 0;
	}
}

bool tagReadBool(element_t tag)
{
	uint8_t tagtype;

	if (!scanforward(tag))
	{
		if (current->defaultval && current->element == tag)
		{
			return current->val.uint32_tval;
		}
		return 0;
	}

	if (!PHYSFS_readUBE8(handle, &tagtype))
	{
		TF_ERROR("tagReadBool: Tag type not found: %#04x", (unsigned int)tag);
		return 0;
	}
	if (tagtype == TF_INT_BOOL || tagtype == TF_INT_U8)
	{
		uint8_t val;
		(void) PHYSFS_readUBE8(handle, &val);
		return val;
	}
	else
	{
		TF_ERROR("readTagBool: Error reading tag %#04x, bad type", (unsigned int)tag);
		return 0;
	}
}

bool tagReadfv(element_t tag, uint16_t size, float *vals)
{
	uint8_t tagtype;
	uint16_t count;
	int i;

	if (!scanforward(tag))
	{
		return false;
	}
	if (!PHYSFS_readUBE8(handle, &tagtype) || tagtype != TF_INT_FLOAT_ARRAY)
	{
		TF_ERROR("tagreadfv: Tag type not found: %#04x", (unsigned int)tag);
		return false;
	}
	if (!PHYSFS_readUBE16(handle, &count) || count != size)
	{
		TF_ERROR("tagreadfv: Bad size: %#04x", (unsigned int)tag);
		return false;
	}
	for (i = 0; i < size; i++)
	{
		if (!PHYSFS_readBEFloat(handle, &vals[i]))
		{
			TF_ERROR("tagreadfv: Error reading index %d, tag %#04x", i, (unsigned int)tag);
			return false;
		}
	}
	return true;
}

uint8_t *tagRead8vDup(element_t tag, int *size)
{
	uint8_t tagtype, *values;
	uint16_t count;
	int i;

	if (!scanforward(tag))
	{
		*size = 0;
		return NULL;
	}
	if (!PHYSFS_readUBE8(handle, &tagtype) || tagtype != TF_INT_U8_ARRAY)
	{
		TF_ERROR("tagread8vDup: Tag type not found: %#04x", (unsigned int)tag);
		*size = -1;
		return NULL;
	}
	if (!PHYSFS_readUBE16(handle, &count))
	{
		TF_ERROR("tagread8vDup: Read error (end of file?): %s", PHYSFS_getLastError());
		*size = -1;
		return NULL;
	}
	values = (uint8_t *)malloc(count);
	*size = count;
	for (i = 0; i < count; i++)
	{
		if (!PHYSFS_readUBE8(handle, &values[i]))
		{
			TF_ERROR("tagread8vDup: Error reading index %d, tag %#04x", i, (unsigned int)tag);
			return NULL;
		}
	}
	return values;
}

bool tagRead8v(element_t tag, uint16_t size, uint8_t *vals)
{
	uint8_t tagtype;
	uint16_t count;
	int i;

	if (!scanforward(tag))
	{
		return false;
	}
	if (!PHYSFS_readUBE8(handle, &tagtype) || tagtype != TF_INT_U8_ARRAY)
	{
		TF_ERROR("tagread8v: Tag type not found: %#04x", (unsigned int)tag);
		return false;
	}
	if (!PHYSFS_readUBE16(handle, &count) || count != size)
	{
		TF_ERROR("tagread8v: Bad size: %#04x", (unsigned int)tag);
		return false;
	}
	for (i = 0; i < size; i++)
	{
		if (!PHYSFS_readUBE8(handle, &vals[i]))
		{
			TF_ERROR("tagread8v: Error reading index %d, tag %#04x", i, (unsigned int)tag);
			return false;
		}
	}
	return true;
}

bool tagRead16v(element_t tag, uint16_t size, uint16_t *vals)
{
	uint8_t tagtype;
	uint16_t count;
	int i;

	if (!scanforward(tag))
	{
		return false;
	}
	if (!PHYSFS_readUBE8(handle, &tagtype) || tagtype != TF_INT_U16_ARRAY)
	{
		TF_ERROR("tagread16v: Tag type not found: %#04x", (unsigned int)tag);
		return false;
	}
	if (!PHYSFS_readUBE16(handle, &count) || count != size)
	{
		TF_ERROR("tagread16v: Bad size: %#04x", (unsigned int)tag);
		return false;
	}
	for (i = 0; i < size; i++)
	{
		if (!PHYSFS_readUBE16(handle, &vals[i]))
		{
			TF_ERROR("tagread16v: Error reading index %d, tag %#04x", i, (unsigned int)tag);
			return false;
		}
	}
	return true;
}

bool tagReads32v(element_t tag, uint16_t size, int32_t *vals)
{
	uint8_t tagtype;
	uint16_t count;
	int i;

	if (!scanforward(tag))
	{
		return false;
	}
	if (!PHYSFS_readUBE8(handle, &tagtype) || tagtype != TF_INT_S32_ARRAY)
	{
		TF_ERROR("tagreads32v: Tag type not found: %d", (int)tag);
		return false;
	}
	if (!PHYSFS_readUBE16(handle, &count) || count != size)
	{
		TF_ERROR("tagreads32v: Bad size: %d", (int)tag);
		return false;
	}
	for (i = 0; i < size; i++)
	{
		if (!PHYSFS_readSBE32(handle, &vals[i]))
		{
			TF_ERROR("tagreads32v: Error reading index %d, tag %d", i, (int)tag);
			return false;
		}
	}
	return true;
}

bool tagReadString(element_t tag, uint16_t size, char *buffer)
{
	uint8_t tagtype = 255;
	PHYSFS_uint16 actualsize = 0;
	PHYSFS_sint64 readsize = 0;

	if (!scanforward(tag))
	{
		return false;
	}
	if (!PHYSFS_readUBE8(handle, &tagtype) || !PHYSFS_readUBE16(handle, &actualsize) || tagtype != TF_INT_U8_ARRAY)
	{
		TF_ERROR("Error reading string size: %s", PHYSFS_getLastError());
		return false;
	}
	if (actualsize > size)
	{
		TF_ERROR("String size %d larger than user maxlen %d", (int)actualsize, (int)size);
		return false;
	}
	readsize = PHYSFS_read(handle, buffer, 1, actualsize);
	if (readsize != actualsize)
	{
		TF_ERROR("Unfinished string: %s", PHYSFS_getLastError());
		return false;
	}
	// Return string MUST be zero terminated!
	if (*(buffer + actualsize - 1) != '\0')
	{
		TF_ERROR("Element %#04x is a string that is not zero terminated", (unsigned int)tag);
		return false;
	}
	return true;
}

char *tagReadStringDup(element_t tag)
{
	uint8_t tagtype;
	PHYSFS_uint16 actualsize;
	PHYSFS_sint64 readsize;
	char *buffer;

	if (!scanforward(tag))
	{
		return NULL;
	}
	if (!PHYSFS_readUBE8(handle, &tagtype) || !PHYSFS_readUBE16(handle, &actualsize))
	{
		TF_ERROR("tagread_stringdup: Error reading string size: %s", PHYSFS_getLastError());
		return NULL;
	}
	if (tagtype != TF_INT_U8_ARRAY)
	{
		TF_ERROR("tagread_stringdup: Bad string format");
		return NULL;
	}
	buffer = (char *)malloc(actualsize);
	assert(buffer != NULL);
	readsize = PHYSFS_read(handle, buffer, 1, actualsize);
	if (readsize != actualsize || *(buffer + actualsize - 1) != '\0')
	{
		TF_ERROR("tagread_stringdup: Unfinished or unterminated string: %s", PHYSFS_getLastError());
		free(buffer);
		return NULL;
	}
	return buffer;
}


/*********  TAGWRITE *********/


static bool write_tag(element_t tag)
{
	PHYSFS_sint64 size;

	if (tag_error || current == NULL || readmode)
	{
		return false;
	}
	size = PHYSFS_write(handle, &tag, 1, 1);
	if (size != 1)
	{
		TF_ERROR("Could not write tag: %s", PHYSFS_getLastError());
		return false;
	}
	return true;
}

bool tagWriteNext()
{
	if (tag_error || !write_tag(TAG_SEPARATOR))
	{
		return false;
	}
	if (current->parent == NULL)
	{
		current = first; // topmost group
	}
	else
	{
		current = current->parent->group; // reset to start of group tag index
	}
	lastAccess = -1; // reset sanity counter
	current->parent->group->countItems++; // sanity checking
	// it has no payload
	return true;
}

bool tagWriteEnter(element_t tag, uint16_t elements)
{
	CHECK_VALID_TAG(tag);
	if (tag_error || !write_tag(tag) || !scan_to(tag))
	{
		return false;
	}
	assert(current->element == tag);
	VALIDATE_TAG("GR", tag, "group");
	ASSERT(current->group != NULL, "Cannot write group, none defined for element %#04x!", (unsigned int)tag);
	assert(current->group->parent != NULL);
	(void) PHYSFS_writeUBE8(handle, TF_INT_GROUP);
	(void) PHYSFS_writeUBE16(handle, elements);
	current->group->expectedItems = elements;	// for debugging and consistency checking
	current->group->countItems = 0;			// ditto
	current->group->parent->current = current; // save where we are. ok, this is very contrived code.
	current = current->group;
	lastAccess = -1; // reset sanity counter, since we are in a new group
	return true;
}

bool tagWriteLeave(element_t tag)
{
	CHECK_VALID_TAG(tag);
	if (tag_error || !write_tag(TAG_GROUP_END) || !scan_to(TAG_GROUP_END))
	{
		return false;
	}
	ASSERT(current->parent != NULL, "Cannot leave group %d, at highest level already!", (int)tag);
	if (current->parent == NULL)
	{
		TF_ERROR("Cannot leave group, at highest level already!");
		return false;
	}
	if (current->parent->group->expectedItems > current->parent->group->countItems + 1) // may omit last separator
	{
		TF_ERROR("Expected %d items in group when writing, found %d",
		         current->parent->group->expectedItems, current->parent->group->countItems);
	}
	current = current->parent->current; // resume at next tag
	assert(current != NULL);
	if (current->element != tag)
	{
		TF_ERROR("Trying to leave the wrong group! We are in %x, leaving %#04x",
		         current->parent != NULL ? (unsigned int)current->parent->element : 0, (unsigned int)tag);
		return false;
	}
	lastAccess = current->element; // restart iteration requirement
	// it has no payload
	return true;
}

bool tagWrite(element_t tag, uint32_t val)
{
	CHECK_VALID_TAG(tag);
	if (!scan_to(tag))
	{
		return false;
	}
	VALIDATE_TAG("US", tag, "unsigned");
	if (current->defaultval && current->val.uint32_tval == val)
	{
		return true; // using default value to save disk space
	}
	if (!write_tag(tag))
	{
		return false;
	}
	if (val > UINT16_MAX)
	{
		(void) PHYSFS_writeUBE8(handle, TF_INT_U32);
		(void) PHYSFS_writeUBE32(handle, val);
	}
	else if (val > UINT8_MAX)
	{
		(void) PHYSFS_writeUBE8(handle, TF_INT_U16);
		(void) PHYSFS_writeUBE16(handle, val);
	}
	else
	{
		(void) PHYSFS_writeUBE8(handle, TF_INT_U8);
		(void) PHYSFS_writeUBE8(handle, val);
	}
	return true;
}

bool tagWrites(element_t tag, int32_t val)
{
	CHECK_VALID_TAG(tag);
	if (!scan_to(tag))
	{
		return false;
	}
	VALIDATE_TAG("SI", tag, "signed");
	if (current->defaultval && current->val.int32_tval == val)
	{
		return true; // using default value to save disk space
	}
	if (!write_tag(tag))
	{
		return false;
	}
	if (val > INT16_MAX)
	{
		(void) PHYSFS_writeUBE8(handle, TF_INT_S32);
		(void) PHYSFS_writeSBE32(handle, val);
	}
	else if (val > INT8_MAX)
	{
		(void) PHYSFS_writeUBE8(handle, TF_INT_S16);
		(void) PHYSFS_writeSBE16(handle, val);
	}
	else
	{
		(void) PHYSFS_writeUBE8(handle, TF_INT_S8);
		(void) PHYSFS_writeSBE8(handle, val);
	}
	return true;
}

bool tagWritef(element_t tag, float val)
{
	CHECK_VALID_TAG(tag);
	if (!scan_to(tag))
	{
		return false;
	}
	VALIDATE_TAG("FP", tag, "floating point");
	if (current->defaultval && current->val.floatval == val)
	{
		return true; // using default value to save disk space
	}
	if (!write_tag(tag))
	{
		return false;
	}
	(void) PHYSFS_writeUBE8(handle, TF_INT_FLOAT);
	(void) PHYSFS_writeBEFloat(handle, val);
	return true;
}

bool tagWriteBool(element_t tag, bool val)
{
	CHECK_VALID_TAG(tag);
	if (!scan_to(tag))
	{
		return false;
	}
	VALIDATE_TAG("BO", tag, "boolean");
	if (current->defaultval && current->val.uint32_tval == val)
	{
		return true; // using default value to save disk space
	}
	if (!write_tag(tag))
	{
		return false;
	}
	(void) PHYSFS_writeUBE8(handle, TF_INT_BOOL);
	(void) PHYSFS_writeUBE8(handle, val);
	return true;
}

bool tagWritefv(element_t tag, uint16_t count, const float *vals)
{
	int i;

	CHECK_VALID_TAG(tag);
	if (!scan_to(tag) || !write_tag(tag))
	{
		return false;
	}
	VALIDATE_TAG("FP", tag, "floating point (multiple)");
	(void) PHYSFS_writeUBE8(handle, TF_INT_FLOAT_ARRAY);
	(void) PHYSFS_writeUBE16(handle, count);
	for (i = 0; i < count; i++)
	{
		(void) PHYSFS_writeBEFloat(handle, vals[i]);
	}
	return true;
}

bool tagWrite8v(element_t tag, uint16_t count, const uint8_t *vals)
{
	int i;

	CHECK_VALID_TAG(tag);
	if (!scan_to(tag) || !write_tag(tag))
	{
		return false;
	}
	VALIDATE_TAG("US", tag, "unsigned (multiple)");
	(void) PHYSFS_writeUBE8(handle, TF_INT_U8_ARRAY);
	(void) PHYSFS_writeUBE16(handle, count);
	for (i = 0; i < count; i++)
	{
		(void) PHYSFS_writeUBE8(handle, vals[i]);
	}
	return true;
}

bool tagWrite16v(element_t tag, uint16_t count, const uint16_t *vals)
{
	int i;

	CHECK_VALID_TAG(tag);
	if (!scan_to(tag) || !write_tag(tag))
	{
		return false;
	}
	VALIDATE_TAG("US", tag, "unsigned (multiple)");
	(void) PHYSFS_writeUBE8(handle, TF_INT_U16_ARRAY);
	(void) PHYSFS_writeUBE16(handle, count);
	for (i = 0; i < count; i++)
	{
		(void) PHYSFS_writeUBE16(handle, vals[i]);
	}
	return true;
}

bool tagWrites32v(element_t tag, uint16_t count, const int32_t *vals)
{
	int i;

	CHECK_VALID_TAG(tag);
	if (!scan_to(tag) || !write_tag(tag))
	{
		return false;
	}
	VALIDATE_TAG("SI", tag, "signed (multiple)");
	(void) PHYSFS_writeUBE8(handle, TF_INT_S32_ARRAY);
	(void) PHYSFS_writeUBE16(handle, count);
	for (i = 0; i < count; i++)
	{
		(void) PHYSFS_writeSBE32(handle, vals[i]);
	}
	return true;
}

bool tagWriteString(element_t tag, const char *buffer)
{
	size_t size;

	CHECK_VALID_TAG(tag);
	if (!scan_to(tag) || !write_tag(tag))
	{
		return false;
	}
	VALIDATE_TAG("ST", tag, "text string");

	// find size of string
	size = strlen(buffer) + 1;
	if (size > current->vm && current->vm != 0)
	{
		TF_ERROR("Given string is too long (size %d > limit %u)", (int)size, current->vm);
		return false;
	}
	(void) PHYSFS_writeUBE8(handle, TF_INT_U8_ARRAY);
	(void) PHYSFS_writeUBE16(handle, size);
	(void) PHYSFS_write(handle, buffer, 1, size);
	return true;
}


/*********  TAGFILE UNIT TEST *********/


#define BLOB_SIZE 11
// unit test function
void tagTest()
{
	static const char virtual_definition[] = "tagdefinitions/tagfile_virtual.def";
	static const char basic_definition[]   = "tagdefinitions/tagfile_basic.def";
	static const char writename[] = "test.wzs";
	static const char cformat[] = "WZTAGFILE1";
	char format[300], *formatdup;
	uint16_t droidpos[3];
	uint8_t blob[BLOB_SIZE];
	float fv[3];

	// Set our testing blob to a bunch of 0x01 bytes
	memset(blob, 1, BLOB_SIZE); // 1111111...

	tagOpenWrite(virtual_definition, writename);
	tagWrites(0x05, 11);
	tagWriteString(0x06, cformat);
	tagClose();

	tagOpenRead(virtual_definition, writename);
	assert(tagRead(0x01) == 1);
	assert(tagReads(0x02) == 2);
	assert(tagReadf(0x03) == 3);
	assert(tagRead(0x04) == 4);
	assert(tagReads(0x05) == 11);
	formatdup = tagReadStringDup(0x06);
	assert(strncmp(formatdup, cformat, 9) == 0);
	free(formatdup);
	assert(tagReads(0x07) == 1);
	tagClose();

	tagOpenWrite(basic_definition, writename);
	tagWriteString(0x01, cformat);
	tagWriteEnter(0x02, 1);
		tagWrite(0x01, 101);
		droidpos[0] = 11;
		droidpos[1] = 13;
		droidpos[2] = 15;
		tagWrite16v(0x02, 3, droidpos);
		fv[0] = 0.1f;
		fv[1] = 1.1f;
		fv[2] = -1.3f;
		tagWritefv(0x03, 3, fv);
		tagWrite8v(0x05, BLOB_SIZE, blob);
		tagWrite8v(0x06, BLOB_SIZE, blob);
		tagWriteEnter(0x07, 1);		// group to test skipping
			tagWrite(0x01, 0);	// using default
			tagWrite(0x02, 1);
			tagWriteEnter(0x03, 1);
			tagWriteLeave(0x03);
			tagWriteNext();
			tagWrite(0x01, 1);
			tagWrite(0x02, 0);
			tagWriteEnter(0x03, 1);
				tagWrite(0x01, 1);
				tagWriteNext();
			tagWriteLeave(0x03);
			// deliberately no 'next' here
		tagWriteLeave(0x07);
		tagWriteEnter(0x08, 1);
			// empty, skipped group
		tagWriteLeave(0x08);
		tagWriteEnter(0x09, 1);
		{
			int32_t v[3] = { -1, 0, 1 };

			tagWrite(0x01, 1);
			tagWrites32v(0x05, 3, v);
		}
		tagWriteLeave(0x09);
	tagWriteLeave(0x02);
	tagClose();

	memset(droidpos, 0, 6);
	memset(fv, 0, 6);
	memset(blob, 0, BLOB_SIZE);
	tagOpenRead(basic_definition, writename);
	tagReadString(0x01, 200, format);
	assert(strncmp(format, cformat, 9) == 0);
	tagReadEnter(0x02);
	{
		int32_t v[3];
		int size;
		uint8_t *blobptr;

		assert(tagRead(0x01) == 101);
		tagRead16v(0x02, 3, droidpos);
		assert(droidpos[0] == 11);
		assert(droidpos[1] == 13);
		assert(droidpos[2] == 15);
		tagReadfv(0x03, 3, fv);
		assert(fv[0] - 0.1f < 0.001);
		assert(fv[1] - 1.1f < 0.001);
		assert(fv[2] + 1.3f < 0.001);
		tagRead8v(0x05, BLOB_SIZE, blob);
		blobptr = tagRead8vDup(0x06, &size);
		assert(size == BLOB_SIZE);
		assert(blob[BLOB_SIZE / 2] == 1);
		assert(blobptr[BLOB_SIZE / 2] == 1);
		tagReadEnter(0x09);
			tagReads32v(0x05, 3, v);
			assert(v[0] == -1);
			assert(v[1] == 0);
			assert(v[2] == 1);
		tagReadLeave(0x09);
		free(blobptr);
	}
	tagReadLeave(0x02);
	assert(tagRead(0x04) == 9);
	tagClose();
	fprintf(stdout, "\tTagfile self-test: PASSED\n");
}
