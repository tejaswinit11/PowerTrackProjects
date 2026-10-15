#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "id3_reader.h"
#include "error_handling.h"

// Copy a bounded text value into a dynamically allocated string
static int set_text_field(char **field,const unsigned char *source,size_t max_length)
{
    size_t length = 0;

    // Find the end of the text without reading beyond the frame
    while (length < max_length && source[length] != '\0')
    {
        length++;
    }

    // Allocate memory for the text and its null terminator
    char *value = malloc(length + 1);

    if (value == NULL)
    {
        return -1;
    }

    // Copy the text into the newly allocated memory
    memcpy(value, source, length);

    // Add the null terminator
    value[length] = '\0';

    // Replace the old value
    free(*field);
    *field = value;

    return 0;
}


TagData* read_id3_tags(const char *filename) {
    //Open the file in binary mode
    FILE *fptr = fopen(filename, "rb");


    //Check whether file is opened successfully
    if (fptr == NULL)
    {
        display_error("Unable to open MP3 file. Check the filename!");
        return NULL;
    }

    //Declare a array of size 10 for header
    unsigned char header[10];

    //Read the first 10 bytes of MP3 file
    size_t bytes_read = fread(header, 1, sizeof(header), fptr);

    //Check if all 10 bytes were read successfully
    if (bytes_read != sizeof(header))
    {
        display_error("Failed to read the ID3 header!");
        fclose(fptr);
        return NULL;
    }

    //Check ID3 signature
    if (memcmp(header, "ID3", 3) != 0)
    {
        display_error("ID3 tag not found!");
        fclose(fptr);
        return NULL;
    }

    //Extract major version and revision number
    unsigned char major_version = header[3];
    unsigned char revision_number = header[4];

    // Extract the ID3 tag size from the header
    unsigned int tag_size =
        ((unsigned int)(header[6] & 0x7F) << 21) |
        ((unsigned int)(header[7] & 0x7F) << 14) |
        ((unsigned int)(header[8] & 0x7F) << 7)  |
        ((unsigned int)(header[9] & 0x7F));

    //Check whether ID3v2 version is supported
    if(major_version < 2 || major_version > 4)
    {
        display_error("Unsupported ID3 version!");
        fclose(fptr);
        return NULL;
    }
    
    //Create a structure to store the metadata
    TagData *data = create_tag_data();

    //Check if memory allocation was successful
    if (data == NULL)
    {
        display_error("Memory allocation failed!");
        fclose(fptr);
        return NULL;
    }

    //Allocate memory for version string
    data->version = (char *)malloc(16 * sizeof(char));
    if (data->version == NULL)
    {
        display_error("Memory allocation failed for version!");
        
        free_tag_data(data);
        fclose(fptr);
        
        return NULL;
    }

    //Store the detected ID3 version 
    snprintf(data->version, 16, "ID3v2.%u.%u", (unsigned int)major_version, (unsigned int)revision_number);
    

    if(major_version != 3)
    {
        display_error("Only ID3v2.3 is supported!");
        free_tag_data(data);
        fclose(fptr);
        return NULL;
    }

    //Track the remaining bytes in the ID3 tag
    unsigned int remaining = tag_size;
    
    //Read the frames until the tag has been proccessed
    while(remaining >= 10)
    {
        //Declare array for frame header
        unsigned char frame_header[10];

        //Read the frame header
        size_t header_bytes = fread(frame_header, 1, sizeof(frame_header), fptr);

        //Check whether the complete frame header was read
        if(header_bytes != sizeof(frame_header))
        {
            display_error("Failed to read the frame header!");
            free_tag_data(data);
            fclose(fptr);
            return NULL;
        }

        //Account for the 10-bytes frame header
        remaining -= 10;
        
        //Check for padding at the end of tag
        if(frame_header[0] == 0)
        {
            break;
        }

        //Extract the 4-character frame ID
        char frame_id[5];

        memcpy(frame_id, frame_header, 4);
        frame_id[4] = '\0'; // Null-terminate the string

        //Extract the frame size from the header
        uint32_t frame_size = 
            ((uint32_t)frame_header[4] << 24) |
            ((uint32_t)frame_header[5] << 16) |
            ((uint32_t)frame_header[6] << 8)  |
            ((uint32_t)frame_header[7]);

        //Validate the frame size against remaining tag data
        if(frame_size > remaining)
        {
            display_error("Invalid frame size!");
            free_tag_data(data);
            fclose(fptr);
            return NULL;
        }

        //Allocate memory for the frame content
        unsigned char *frame_content = malloc((size_t)frame_size + 1);

        if(frame_content == NULL)
        {
            display_error("Memory allocation failed for frame content!");
            free_tag_data(data);
            fclose(fptr);
            return NULL;
        }

        //Read the frame content
        size_t content_bytes = fread(frame_content, 1, frame_size, fptr);

        //Check whether the complete frame content was read
        if(content_bytes != frame_size)
        {
            display_error("Failed to read the frame content!");
            free(frame_content);
            free_tag_data(data);
            fclose(fptr);
            return NULL;
        }

        //Add a terminating NULL byte for later text processing
        frame_content[frame_size] = '\0';

        //Identify the destination field for supported text frames
        char **destination = NULL;

        if(strcmp(frame_id, "TIT2") == 0)
        {
            destination = &data->title;
        }
        else if(strcmp(frame_id, "TPE1") == 0)
        {
            destination = &data->artist;
        }
        else if(strcmp(frame_id, "TALB") == 0)
        {
            destination = &data->album;
        }
        else if(strcmp(frame_id, "TYER") == 0)
        {
            destination = &data->year;
        }
        else if(strcmp(frame_id, "TCOM") == 0)
        {
            destination = &data->composer;
        }
        else if(strcmp(frame_id, "TCON") == 0)
        {
            destination = &data->genre;
        }


        //Extract ordinary text frames using encoding 0
        if(destination != NULL && frame_size >= 1 && frame_content[0] == 0)
        {
            //Skip the first byte beacuse it specifies text encoding
            if(set_text_field(destination, frame_content + 1, frame_size - 1)!= 0)
            {
                display_error("Failed to set text field!");
                free(frame_content);
                free_tag_data(data);
                fclose(fptr);
                return NULL;
            }
        }

        //Handle the sample user defined text frame
        //Its description is "comment"
        if(strcmp(frame_id, "TXXX") == 0 && frame_size > 1 && frame_content[0] == 0)
        {
            //The description begins after encoding byte
            size_t description_length = 0;

            while(description_length < frame_size - 1 && frame_content[1 + description_length] != '\0')
            {
                description_length++;
            }

            //Check that the description has a terminating null byte
            if(description_length < frame_size - 1)
            {
                //Check whether the description is "comment"
                if(description_length == 7 && memcmp(frame_content + 1, "comment", 7) == 0)
                {
                    //Locate the value after the description and seperator
                    size_t value_offset = description_length + 2;

                    if(value_offset < frame_size)
                    {
                        if(set_text_field(&data->comment, frame_content + value_offset, frame_size - value_offset) != 0)
                        {
                            display_error("Failed to set comment field!");
                            free(frame_content);
                            free_tag_data(data);
                            fclose(fptr);
                            return NULL;
                        }
                    }
                }
            }
        }

        // Handle standard ID3v2.3 comment frame
        if (strcmp(frame_id, "COMM") == 0 &&
            frame_size >= 5 &&
            frame_content[0] == 0)
        {
            // Skip encoding byte and 3 language bytes
            size_t offset = 4;

            // Find the end of the description
            while (offset < frame_size &&
                frame_content[offset] != '\0')
            {
                offset++;
            }

            // Skip the description terminator
            if (offset < frame_size)
            {
                offset++;

                // Store the actual comment
                if (set_text_field(&data->comment,
                                frame_content + offset,
                                frame_size - offset) != 0)
                {
                    display_error("Failed to store comment!");

                    free(frame_content);
                    free_tag_data(data);
                    fclose(fptr);

                    return NULL;
                }
            }
        }

        // Handle the standard ID3v2.3 comment frame
        if (strcmp(frame_id, "COMM") == 0 &&
            frame_size >= 5 &&
            frame_content[0] == 0)
        {
            // Skip 1 encoding byte and 3 language bytes
            size_t offset = 4;

            // Find the end of the description
            while (offset < frame_size &&
                frame_content[offset] != '\0')
            {
                offset++;
            }

            // Check whether the description terminator exists
            if (offset < frame_size)
            {
                // Move past the description's null terminator
                offset++;

                // Extract the actual comment
                if (set_text_field(&data->comment,
                                frame_content + offset,
                                frame_size - offset) != 0)
                {
                    display_error("Failed to store comment!");

                    free(frame_content);
                    free_tag_data(data);
                    fclose(fptr);

                    return NULL;
                }
            }
        }

        //Release the temporary frame buffer
        free(frame_content);

        //Account for the frame content
        remaining -= frame_size;
    }
    
    //Close the file
    fclose(fptr);

    return data;

}


void display_metadata(const TagData *data, const char *operation, const char *status)
{
    // Check if metadata is available
    if (data == NULL)
    {
        display_error("No metadata to display.");
        return;
    }

    printf("\n+--------------------------------------------------+\n");
    printf("             MP3 TAG READER & EDITOR\n");
    printf("+--------------------------------------------------+\n\n");

    printf("  Operation   : %s\n", operation);

    // Display status only for EDIT operation
    if (status != NULL)
    {
        printf("  Status      : %s\n", status);
    }

    printf("  ID3 Version : %s\n",
           data->version != NULL ? data->version : "Unknown");

    printf("\n");

    // Display metadata fields
    printf("  Title       : %s\n",
           data->title != NULL ? data->title : "Not available");

    printf("  Artist      : %s\n",
           data->artist != NULL ? data->artist : "Not available");

    printf("  Album       : %s\n",
           data->album != NULL ? data->album : "Not available");

    printf("  Year        : %s\n",
           data->year != NULL ? data->year : "Not available");

    printf("  Composer    : %s\n",
           data->composer != NULL ? data->composer : "Not available");

    printf("  Comment     : %s\n",
           data->comment != NULL ? data->comment : "Not available");

    printf("  Genre       : %s\n",
           data->genre != NULL ? data->genre : "Not available");

    printf("\n+--------------------------------------------------+\n");
    printf("  Details displayed successfully!\n");
}

void view_tags(const char *filename)
{
    TagData *data = read_id3_tags(filename);

    if (data == NULL)
    {
        return;
    }

    display_metadata(data, "VIEW", NULL);
    free_tag_data(data);
}