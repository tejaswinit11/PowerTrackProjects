#ifndef ID3_WRITER_H
#define ID3_WRITER_H

#include "id3_utils.h"

int write_id3_tags(const char *filename, const TagData *data);

int edit_tag(const char *filename, const char *tag, const char *value);

#endif // ID3_WRITER_H
