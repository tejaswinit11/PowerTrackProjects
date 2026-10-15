#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

typedef enum
{
    TAG_CHECK_ERROR = -1,
    TAG_NOT_FOUND = 0,
    TAG_FOUND = 1

}TagPresenceStatus;


void display_error(const char *message);

TagPresenceStatus check_id3_tag_presence(const char *filename);

#endif // ERROR_HANDLING_H
