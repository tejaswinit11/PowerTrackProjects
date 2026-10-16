#include <stdio.h>
#include <string.h>
#include "lexical_analyser.h"


//Function to display the help menu
void display_help(void)
{
    printf("\n========================================================\n");
    printf("                   LEXICAL ANALYSER                \n");
    printf("                      HELP MENU                   \n");
    printf("========================================================\n");

    printf("\n\tUsage:\n\n");
    printf("  \t./lexical_analyser <filename.c> / <filename.txt>\n");
    printf("  \t./lexical_analyser --help\n");

    printf("\n\tExamples:\n\n");
    printf("  \t./lexical_analyser sample.c\n");
    printf("  \t./lexical_analyser sample.txt\n");
    printf("  \t./lexical_analyser --help\n");

    printf("\n========================================================\n\n");
}

int main(int argc, char *argv[])
{
    FILE *fp;
    Report report = {0};

    //check whether help option is entered
    if (argc == 2 && strcmp(argv[1], "--help") == 0)
    {
        //call display function to display menu
        display_help();
        return 0;
    }

    //Validate number of CLAs
    if (argc != 2)
    {
        printf("\n");
        printf("===========================================================\n\n");
        printf("     ERROR: Invalid number of arguments.\n");
        printf("     Use ./lexical_analyser --help for usage information.\n\n");
        printf("===========================================================\n\n");
        return 1;
    }

    //Validate Input File Extension
    char *ext = strrchr(argv[1], '.');

    if (ext == NULL || (strcmp(ext, ".c") != 0 && strcmp(ext, ".txt") != 0))
    {
        printf("\n");
        printf("==========================================================\n\n");
        printf("\tERROR: Only .c and .txt files are supported.\n\n");
        printf("==========================================================\n\n");
        return 1;
    }

    //Open the file in read mode
    fp = fopen(argv[1], "r");

    //Check whether the file is opened
    if (fp == NULL)
    {
        printf("==========================================================\n\n");
        printf("    ERROR: Cannot open file: %s\n\n", argv[1]);
        printf("==========================================================\n\n");
        return 1;
    }

    //Validate Empty Files
    int ch = fgetc(fp);
    
    if (ch == EOF)
    {
        /* Check for file reading error */
        if (ferror(fp))
        {
            printf("==========================================================\n\n");
            printf("        ERROR: Failed to read the file.\n\n");
            printf("==========================================================\n\n");
        }
        else
        {
            printf("==========================================================\n\n");
            printf("              ERROR: File is empty.\n\n");
            printf("==========================================================\n\n");
        }

        fclose(fp);
        return 1;
    }
    
    //Make file position back to beginning
    rewind(fp);

    //Display the heading
    print_heading(argv[1]);

    //Analyze the source file and collect results
    analyze_file(fp, &report);

    //Close the input file
    fclose(fp);

    //Display final analysis
    print_summary(&report);

    //Return failure if any errors exits
    return report.errors > 0 ? 1 : 0;
}
