#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "id3_writer.h"
#include "error_handling.h"

/* Copy 'count' bytes from the current position in source to destination. */
static int copy_data(FILE *source, FILE *destination, long count)
{
    unsigned char buffer[1024];

    while (count > 0)
    {
        size_t amount = count > (long)sizeof(buffer)
                        ? sizeof(buffer) : (size_t)count;

        if (fread(buffer, 1, amount, source) != amount)
            return 1;

        if (fwrite(buffer, 1, amount, destination) != amount)
            return 1;

        count -= (long)amount;
    }
    return 0;
}

/* Edit an existing text or comment frame in an ID3v2.3 MP3 file. */
int edit_tag(const char *filename, const char *tag, const char *value)
{
    const char *frame_id = NULL;

    if (!filename || !tag || !value)
    {
        display_error("Invalid arguments!");
        return 1;
    }

    if (strcmp(tag, "-T") == 0)       frame_id = "TIT2";
    else if (strcmp(tag, "-a") == 0)  frame_id = "TPE1";
    else if (strcmp(tag, "-A") == 0)  frame_id = "TALB";
    else if (strcmp(tag, "-Y") == 0)  frame_id = "TYER";
    else if (strcmp(tag, "-M") == 0)  frame_id = "TCOM";
    else if (strcmp(tag, "-C") == 0)  frame_id = "COMM";
    else if (strcmp(tag, "-g") == 0)  frame_id = "TCON";
    else
    {
        display_error("Invalid tag option!");
        return 1;
    }

    /* This version writes ISO-8859-1 frames using ASCII text only. */
    size_t value_length = strlen(value);
    if (value_length == 0 || value_length > 1024)
    {
        display_error("Enter a value between 1 and 1024 characters!");
        return 1;
    }
    for (size_t i = 0; i < value_length; i++)
    {
        if ((unsigned char)value[i] < 32 || (unsigned char)value[i] > 126)
        {
            display_error("Only printable English/ASCII text is supported!");
            return 1;
        }
    }

    FILE *source = fopen(filename, "rb");
    if (!source)
    {
        display_error("Failed to open MP3 file!");
        return 1;
    }

    unsigned char header[10];
    if (fread(header, 1, 10, source) != 10 ||
        memcmp(header, "ID3", 3) != 0 || header[3] != 3 || header[5] != 0)
    {
        display_error("Invalid or unsupported ID3v2.3 header!");
        fclose(source);
        return 1;
    }

    for (int i = 6; i < 10; i++)
    {
        if (header[i] & 0x80)
        {
            display_error("Invalid ID3 tag size!");
            fclose(source);
            return 1;
        }
    }

    unsigned int tag_size = ((unsigned int)header[6] << 21) |
                            ((unsigned int)header[7] << 14) |
                            ((unsigned int)header[8] << 7) |
                            (unsigned int)header[9];

    /* Check the actual file length before using metadata positions. */
    if (fseek(source, 0, SEEK_END) != 0)
    {
        display_error("Cannot read MP3 file size!");
        fclose(source);
        return 1;
    }
    long file_size = ftell(source);
    if (file_size < 10 || (unsigned long)(file_size - 10) < tag_size ||
        fseek(source, 10, SEEK_SET) != 0)
    {
        display_error("Invalid MP3 file size!");
        fclose(source);
        return 1;
    }

    long frame_position = -1;
    unsigned int old_size = 0;
    unsigned int remaining = tag_size;

    /* Find the selected frame, just like in the reader. */
    while (remaining >= 10)
    {
        long position = ftell(source);
        unsigned char frame_header[10];
        if (position < 0 || fread(frame_header, 1, 10, source) != 10)
        {
            display_error("Failed to read frame header!");
            fclose(source);
            return 1;
        }
        remaining -= 10;
        if (frame_header[0] == 0)
            break;  /* Padding begins. */

        unsigned int frame_size = ((uint32_t)frame_header[4] << 24) |
                                  ((uint32_t)frame_header[5] << 16) |
                                  ((uint32_t)frame_header[6] << 8) |
                                  (uint32_t)frame_header[7];
        if (frame_size > remaining)
        {
            display_error("Invalid frame size!");
            fclose(source);
            return 1;
        }

        // Check whether the selected frame is found
    int match = (memcmp(frame_header, frame_id, 4) == 0);

    // For comments, also check TXXX frames
    if (strcmp(tag, "-C") == 0 &&
        memcmp(frame_header, "TXXX", 4) == 0)
    {
        unsigned char content[9];

        // TXXX comment: encoding + "comment" + null terminator
        if (frame_size >= 9)
        {
            if (fread(content, 1, 9, source) == 9)
            {
                if (content[0] == 0 &&
                    memcmp(content + 1, "comment", 7) == 0 &&
                    content[8] == 0)
                {
                    match = 1;
                }
            }

            // Return to the beginning of frame content
            if (fseek(source, position + 10, SEEK_SET) != 0)
            {
                display_error("Failed to seek frame!");
                fclose(source);
                return 1;
            }
        }
    }

    if (match)
    {
        if (frame_header[8] != 0 || frame_header[9] != 0)
        {
            display_error("Editing this special frame is not supported!");
            fclose(source);
            return 1;
        }

        frame_position = position;
        old_size = frame_size;

        // Remember the actual frame type
        if (memcmp(frame_header, "TXXX", 4) == 0)
        {
            frame_id = "TXXX";
        }

        break;
    }
        if (fseek(source, (long)frame_size, SEEK_CUR) != 0)
        {
            display_error("Failed to skip frame data!");
            fclose(source);
            return 1;
        }
        remaining -= frame_size;
    }

    if (frame_position == -1)
    {
        display_error("Selected tag is not available to edit!");
        fclose(source);
        return 1;
    }

    int is_comment = strcmp(frame_id, "COMM") == 0;
    int is_txxx = strcmp(frame_id, "TXXX") == 0;

    unsigned int new_size;

    if (is_comment)
    {
        new_size = (unsigned int)value_length + 5;
    }
    else if (is_txxx)
    {
        new_size = (unsigned int)value_length + 9;
    }
    else
    {
        new_size = (unsigned int)value_length + 1;
    }
    
    unsigned int new_tag_size = tag_size - old_size + new_size;
    if (new_tag_size > 0x0FFFFFFF)
    {
        display_error("Updated tag is too large!");
        fclose(source);
        return 1;
    }

    /* Build a new frame: ID (4), size (4), flags (2), content. */
    unsigned char *new_frame = malloc(10 + new_size);
    if (!new_frame)
    {
        display_error("Memory allocation failed!");
        fclose(source);
        return 1;
    }
    memcpy(new_frame, frame_id, 4);
    new_frame[4] = (new_size >> 24) & 0xFF;
    new_frame[5] = (new_size >> 16) & 0xFF;
    new_frame[6] = (new_size >> 8) & 0xFF;
    new_frame[7] = new_size & 0xFF;
    new_frame[8] = 0;
    new_frame[9] = 0;
    new_frame[10] = 0;  /* ISO-8859-1 encoding; ASCII is compatible. */

    if (is_comment)
    {
        // COMM frame
        memcpy(new_frame + 11, "eng", 3);
        new_frame[14] = 0;
        memcpy(new_frame + 15, value, value_length);
    }
    else if (is_txxx)
    {
        // TXXX frame with description "comment"
        memcpy(new_frame + 11, "comment", 7);
        new_frame[18] = 0;
        memcpy(new_frame + 19, value, value_length);
    }
    else
    {
        // Ordinary text frame
        memcpy(new_frame + 11, value, value_length);
    }

    header[6] = (new_tag_size >> 21) & 0x7F;
    header[7] = (new_tag_size >> 14) & 0x7F;
    header[8] = (new_tag_size >> 7) & 0x7F;
    header[9] = new_tag_size & 0x7F;

    /* Save beside the original file, then replace it when complete. */
    char *temp_name = malloc(strlen(filename) + 5);
    if (!temp_name)
    {
        display_error("Memory allocation failed!");
        free(new_frame);
        fclose(source);
        return 1;
    }
    sprintf(temp_name, "%s.tmp", filename);

    /* Never silently overwrite a pre-existing temporary file. */
    FILE *check = fopen(temp_name, "rb");
    if (check)
    {
        fclose(check);
        display_error("Temporary file already exists! Remove it after checking.");
        free(temp_name);
        free(new_frame);
        fclose(source);
        return 1;
    }

    FILE *temp = fopen(temp_name, "wb");
    if (!temp)
    {
        display_error("Cannot create temporary file!");
        free(temp_name);
        free(new_frame);
        fclose(source);
        return 1;
    }

    /* Write the updated header, earlier frames, and replacement frame. */
    int success = 1;
    if (fwrite(header, 1, 10, temp) != 10 ||
        fseek(source, 10, SEEK_SET) != 0 ||
        copy_data(source, temp, frame_position - 10) != 0 ||
        fwrite(new_frame, 1, 10 + new_size, temp) != 10 + new_size)
    {
        success = 0;
    }

    /* Copy all bytes after the old frame, including the original audio. */
    long next_position = frame_position + 10 + (long)old_size;
    if (success && (fseek(source, next_position, SEEK_SET) != 0 ||
                    copy_data(source, temp, file_size - next_position) != 0))
    {
        success = 0;
    }

    if (fclose(temp) != 0)
        success = 0;
    if (fclose(source) != 0)
        success = 0;

    if (success && rename(temp_name, filename) != 0)
        success = 0;

    if (!success)
    {
        display_error("Failed to save updated MP3 file!");
        remove(temp_name);
    }

    free(temp_name);
    free(new_frame);
    return success ? 0 : 1;
}
