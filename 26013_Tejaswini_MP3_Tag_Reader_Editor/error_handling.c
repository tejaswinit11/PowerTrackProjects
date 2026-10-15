#include <stdio.h>
#include <string.h>
#include "error_handling.h"

//Function to display the error
void display_error(const char * message)
{
    fprintf(stderr, "\n");
    fprintf(stderr, "+--------------------------------------------------+\n");
    fprintf(stderr, "|                      ERROR                       |\n");
    fprintf(stderr, "+--------------------------------------------------+\n");
    fprintf(stderr, "  %s\n", message);
    fprintf(stderr, "+--------------------------------------------------+\n\n");
}

TagPresenceStatus check_id3_tag_presence(const char *filename)
{
    //Create a file ptr
    FILE *fptr;

    //Open the file in binary mode
    fptr = fopen(filename , "rb");

    //Check whether file is opened successfully
    if(fptr == NULL)
    {
        //Call the display error function
        display_error("File not opened!");
        return TAG_CHECK_ERROR;
    }

    /*ID3V2 CHECK: */

    //To read if ID3 tag is present which is 3 bytes
    unsigned char signature[3];
    
    //Read the first 3 bytes of MP3 file
    size_t bytes_read = fread(signature, 1, sizeof(signature), fptr);

    //Check if all 3 bytes were read successfully
    if( bytes_read == 3)
    {
        //Compare the bytes with ID3 signature
        if(memcmp(signature, "ID3", 3) == 0)
        {
            fclose(fptr);
            return TAG_FOUND;
        }
    } 
    else if(ferror(fptr))
    {
        display_error("Failed to read the file!");
        fclose(fptr);
        return TAG_CHECK_ERROR;
    }

    /*ID3V1 CHECK: */
    
    //Move to the end of file
    if(fseek(fptr, 0, SEEK_END) != 0)
    {
        display_error("Failed to seek to the end of file!");
        fclose(fptr);
        return TAG_CHECK_ERROR;
    }

    //Get the file size
    long file_size = ftell(fptr); 

    //Check whether the file is large enough
    if(file_size < 0)
    {
        display_error("Failed to determine file size!");
        fclose(fptr);
        return TAG_CHECK_ERROR;
    } 

    //Check whether the file is large enough for an ID3v1 tag
    if(file_size >= 128)
    {
        //Move to the beginning of the last 128 bytes
        if(fseek(fptr, -128, SEEK_END)!= 0)
        {
            display_error("Failed to seek the ID3V1 Tag!");
            fclose(fptr);
            return TAG_CHECK_ERROR;
        }

        //Read the first 3 bytes of the ID3v1 Tag
        size_t bytes_read = fread(signature, 1, sizeof(signature), fptr);

        //Check if all 3 bytes were read successfully
        if( bytes_read == 3)
        {
            //Compare the bytes with ID3 signature
            if(memcmp(signature, "TAG", 3) == 0)
            {
                fclose(fptr);
                return TAG_FOUND;
            }
        }
        else if(ferror(fptr))
        {
            display_error("Failed to read the file!");
            fclose(fptr);
            return TAG_CHECK_ERROR;
        }
    }

    //Close the file
    fclose(fptr);

    //If signature is not found
    return TAG_NOT_FOUND;

}