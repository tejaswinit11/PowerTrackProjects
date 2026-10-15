#ifndef ID3_READER_H
#define ID3_READER_H

#include "id3_utils.h"


typedef enum
{
    FAILURE = 0,
    SUCCESS = 1

} ReadStatus;

TagData* read_id3_tags(const char *filename);

void display_metadata(const TagData *data, const char *operation, const char *status);

void view_tags(const char *filename);

#endif // ID3_READER_H
