#include <stdio.h>
#include <string.h>

#include "main.h"
#include "id3_reader.h"
#include "id3_writer.h"
#include "error_handling.h"

void display_usage(void)
{
    printf("\nUsage:\n");
    printf("  View tags : ./mp3_tag -v <mp3_filename>\n");
    printf("  Edit tags : ./mp3_tag -e <option> <new_text> <mp3_filename>\n");
    printf("  Help      : ./mp3_tag --help\n");
}

void display_help(void)
{
    printf("\n");
    printf("==================================================\n");
    printf("             MP3 TAG READER & EDITOR\n");
    printf("                    HELP MENU\n");
    printf("==================================================\n");

    printf("\nDescription:\n");
    printf("  This application allows users to view and edit\n");
    printf("  ID3 metadata information stored in MP3 files.\n");

    display_usage();

    printf("\nEdit Options:\n");
    printf("  -T    Edit song title\n");
    printf("  -a    Edit artist name\n");
    printf("  -A    Edit album name\n");
    printf("  -Y    Edit release year\n");
    printf("  -M    Edit music composer\n");
    printf("  -C    Edit comment\n");
    printf("  -g    Edit genre\n");

    printf("\nExamples:\n");
    printf("  ./mp3_tag -v sample.mp3\n");
    printf("  ./mp3_tag -e -T \"New Song Title\" sample.mp3\n");
    printf("  ./mp3_tag -e -a \"New Artist\" sample.mp3\n");
    printf("  ./mp3_tag --help\n");

    printf("\n==================================================\n");
}

int main(int argc, char *argv[])
{
    // Check whether the user provided arguments
    if (argc < 2)
    {
        display_error("No command-line arguments provided!");
        display_usage();
        return 1;
    }

    // Display help information
    if (strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-h") == 0)
    {
        if (argc != 2)
        {
            display_error("Invalid arguments for help!");
            display_usage();
            return 1;
        }

        display_help();
        return 0;
    }

    // View MP3 metadata
    else if (strcmp(argv[1], "-v") == 0)
    {
        if (argc != 3)
        {
            display_error("Invalid arguments for viewing tags!");
            display_usage();
            return 1;
        }
        
        view_tags(argv[2]);

        return 0;
    }

    // Edit MP3 metadata
    else if (strcmp(argv[1], "-e") == 0)
    {
        if (argc != 5)
        {
            display_error("Invalid arguments for editing tags!");
            display_usage();
            return 1;
        }

        // Extract command-line arguments
        const char *tag = argv[2];
        const char *value = argv[3];
        const char *filename = argv[4];

        // Identify the selected metadata field
        const char *tag_name = NULL;

        if (strcmp(tag, "-T") == 0)
        {
            tag_name = "Title";
        }
        else if (strcmp(tag, "-a") == 0)
        {
            tag_name = "Artist";
        }
        else if (strcmp(tag, "-A") == 0)
        {
            tag_name = "Album";
        }
        else if (strcmp(tag, "-Y") == 0)
        {
            tag_name = "Year";
        }
        else if (strcmp(tag, "-M") == 0)
        {
            tag_name = "Composer";
        }
        else if (strcmp(tag, "-C") == 0)
        {
            tag_name = "Comment";
        }
        else if (strcmp(tag, "-g") == 0)
        {
            tag_name = "Genre";
        }
        else
        {
            display_error("Invalid editing option!");
            display_usage();
            return 1;
        }

        // Edit the selected metadata field
        if (edit_tag(filename, tag, value) != 0)
        {
            return 1;
        }

        // Display successful operation
        printf("\nSUCCESS: %s updated successfully!\n", tag_name);

        // Display updated metadata
        printf("\nUpdated MP3 Metadata:\n");
        view_tags(filename);

        return 0;
    }

    // Handle invalid commands
    else
    {
        display_error("Invalid command-line option!");
        display_usage();
        return 1;
    }
}