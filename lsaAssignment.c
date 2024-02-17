#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLACK_THRESHOLD 900 // Anything below is black

int main()
{
    FILE *inputFile, *outputFile;

    inputFile = fopen("text1.txt", "r");
    if (inputFile == NULL)
    {
        printf("Error opening file: %s\n", inputFilename);
        return 1;
    }

    outputFile = fopen("lsa1.txt", "w");
    if (outputFile == NULL)
    {
        printf("Error creating output file: %s\n", inputFilename);
        fclose(inputFile);
        return 1;
    }

    int lsa1, lsa2, lsa3, lsa4, lsa5;
    char color1[10], color2[10], color3[10], color4[10], color5[10];
    while (fscanf(inputFile, "LSA_1: %d \t LSA_2: %d \t LSA_3: %d \t LSA_4: %d \t LSA_5: %d",
                  &lsa1, &lsa2, &lsa3, &lsa4, &lsa5) == 5)
    {

        printf("1"); // Check for if the while loop is executing which it is not
        if (lsa1 >= BLACK_THRESHOLD)
        {
            strcpy(color1, "WHITE-");
        }
        else
        {
            strcpy(color1, "BLACK-");
        }

        if (lsa2 >= BLACK_THRESHOLD)
        {
            strcpy(color2, "WHITE-");
        }
        else
        {
            strcpy(color2, "BLACK-");
        }

        // Repeat for lsa3, lsa4, lsa5
        if (lsa3 >= BLACK_THRESHOLD)
        {
            strcpy(color3, "WHITE-");
        }
        else
        {
            strcpy(color3, "BLACK-");
        }

        if (lsa4 >= BLACK_THRESHOLD)
        {
            strcpy(color4, "WHITE-");
        }
        else
        {
            strcpy(color4, "BLACK-");
        }

        if (lsa5 >= BLACK_THRESHOLD)
        {
            strcpy(color5, "WHITE-");
        }
        else
        {
            strcpy(color5, "BLACK-");
        }

        // Write colors to output file
        fprintf(outputFile, "%s\t%s\t%s\t%s\t%s\n", color1, color2, color3, color4, color5);
    }

    // Check for errors after the loop
    if (ferror(inputFile))
    {
        printf("Error reading file.\n");
    }

    // Close files
    fclose(inputFile);
    fclose(outputFile);

    printf("Analysis complete. Check output file:\n");

    return 0;
}
