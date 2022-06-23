/* Eliza version 2.10  by: Ted Felix.  */
/* A port to something more modern started 10/1/2015. */
// Continuation of this work, June 2022.  See the git repo for
// more details!

// TODO
// - Use std::string.
// - Use STL.
//   - Conjugates should be a std::map.

/* - - - - Includes - - - - */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

/* - - - - Defines - - - - */
#define Input_size 255

// ********************************************************************

/* These structures define the keyword and response chains.  Each    */
/* element in the keyword chain points to the anchor of a response   */
/* chain.  When a keyword is found, this pointer is used to find a   */
/* response.  The response chain has a currPtr which points to the  */
/* next reply that has not been used.                                */
struct ResponseElement
{
    struct ResponseElement *nextPtr;  /* Pointer to next response */
    char response;  /* The response text */
};

struct ResponseAnchor
{
    // Ptr to head of response chain.
    struct ResponseElement *headPtr;
    // Ptr to current response.
    struct ResponseElement *currPtr;
};

/* Keyword element definition. */
struct KeyElement
{
    // Pointer to next keyword element.
    struct KeyElement *nextPtr;
    // Pointer to response chain.
    struct ResponseAnchor *Response_ptr;
    // The keyword itself.
    char Keyword;
};


/****************************************************************/
/* Substrcpy - Copy a portion of a string to another string.    */
/*   Input:  Fstr - String from which copying will be done      */
/*           Start - Position in Fstr to start at (0 is first)  */
/*           Len - Length of portion to copy                    */
/*           Tstr - String to which portion will be copied      */
/*           Null - Yes = Add null to end, No = don't           */
/*   Output: Tstr is updated                                    */
/****************************************************************/
void Substrcpy(
    char *Fstr, int Start, int Len, char *Tstr, bool nullTerminate)
{
    int I;

    /* For each character in Fstr from Start for Len, copy to Tstr */
    for (I = 0; Fstr[Start + I]  &&  I < Len; ++I)
    {
        Tstr[I] = Fstr[Start + I];
    }

    /* If null requested, add it */
    if (nullTerminate)
        Tstr[I] = '\0';
}


/****************************************************************/
/* Remus - Remove underscores, replace with spaces              */
/*   Input:  String - String to remove underscores from         */
/*   Output: String - No more underscores                       */
/****************************************************************/
void Remus(char *String)
{
    for (unsigned I = 0; I < strlen(String); ++I)
    {
        if (String[I] == '_')
            String[I] = ' ';
    }
}


/****************************************************************/
/* Clean - Capitalize, and remove punctuation from a string     */
/*   Input:  String - String to process                         */
/*   Output: String processed                                   */
/****************************************************************/
void Clean(char *String)
{
    int Shift = 0;

    for (unsigned I = 0; I < strlen(String) + 1; ++I)
    {
        // Deal with dropped characters.
        String[I - Shift] = String[I];

        // Capitalize any letters.
        if (String[I - Shift] >= 'a'  &&  String[I - Shift] <= 'z')
        {
            String[I - Shift] = String[I - Shift] - 32;
            continue;
        }

        // Remove punctuation.
        if (strchr("!?.,'", String[I - Shift]))
        {
            ++Shift;
            continue;
        }
    }
}


/******************************************************************/
/* Strshift - Shifts from a position in a string to the end       */
/*   Input:  String - String to shift                             */
/*           Start - Starting position in string (0 is beginning) */
/*           Amt - Number of chars to shift, negative is left     */
/*   Output: String - Shifted                                     */
/******************************************************************/
void Strshift(char *String, int Start, int Amt)
{
    // Shift left.
    if (Amt < 0)
    {
        for (unsigned I = Start; (I - Amt) <= strlen(String); ++I)
        {
            String[I] = String[I - Amt];
        }
        return;
    }

    // Shift right.
    if (Amt > 0)
    {
        for (int I = strlen(String); I >= Start; --I)
        {
            String[I + Amt] = String[I];
        }
        // Backfill with spaces.
        for (int I = Start; I < Start + Amt; ++I)
        {
            String[I] = ' ';
        }
        return;
    }
}


/****************************************************************/
/* Strcmp2 - Compares a keyword with a portion of a string.     */
/*   Input:  Keyword - Keyword string                           */
/*           String - String to compare keyword against.        */
/*   Output: Strcmp2 - false = Not equal, true = Equal          */
/****************************************************************/
bool Strcmp2(const char *Keyword, const char *String)
{
    int I;

    for (I = 0; Keyword[I]; ++I)
    {
        /* If we're at the end of String, it's over */
        if (!String[I])
        {
            return false;
        }
        
        /* If there is a mis-match */
        if (Keyword[I] != String[I])
        {
            return false;
        }
    }

    return true;
}


/* These are the conversions that need to be done to certain words */
/* used by the user.  This makes Eliza seem remotely intelligent.  */
/* One exception is with YOU to I.  In many cases, YOU should be   */
/* converted to ME when Eliza talks back:                          */
/* User:  I am depending on you.                                   */
/* Eliza: Do you find it normal to be depending on me?             */
/*                                                                 */
/*   Other times, I is more appropriate:                           */
/* User:  I see what you should do.                                */
/* Eliza: Do you often-times see what I should do?                 */
/*   Alas, this is not real artificial intelligence, so we give up */
/* and always convert loose YOUs to Is.                            */
/*                                                                 */
/* Some rules for future thought:                                  */
/*   Prepositional phrases:  YOU becomes ME  (on YOU -> on ME)     */
/*   Interrogative phrases:  YOU becomes I  (what YOU -> what I)   */
/*   To-verb phrases:  YOU becomes ME (to be YOU -> to be ME)      */
#define Num_conj 13
char Conjugates[Num_conj][2][11] =
  {{" YOU WERE ", " I WAS "},
   {" YOU ARE ",  " I AM "},
   {" I ",        " YOU "},  // Converting You to I is not always good.
   {" YOURS ",    " MINE "},
   {" YOUR ",     " MY "},
   {" YOURE ",    " IM "},
   {" IVE ",      " YOUVE "},
   {" MYSELF ",   " YOURSELF "},
   {" ME ",       " !YOU "},   // Exclamation point forces these to
   {" OUR ",      " !YOUR "},  // convert left to right only.
   {" OURS ",     " !YOURS "},
   {" MOM ",      " !MOTHER "},
   {" DAD ",      " !FATHER "}};

/******************************************************************/
/* Conjugate - Conjugates a string                                */
/*   Input:  String - String to conjugate                         */
/*           Start - Starting position in string (0 is beginning) */
/*   Output: String - No more underscores                         */
/******************************************************************/
void Conjugate(char *String, int Start)
{
    for (unsigned I = Start; I < strlen(String); ++I)
    {
        int X;

        /* For each conjugate pair */
        for (X = 0; X < Num_conj; ++X)
        {
            /* If it's the one on the left */
            if (Strcmp2(&(Conjugates[X][0][0]), &(String[I])))
            {
                /* Find the difference in length between the old and new forms */
                int Amt = strlen(&(Conjugates[X][1][0])) -
                          strlen(&(Conjugates[X][0][0]));

                /* Make or take room for the new form */
                Strshift(String, I, Amt);

                /* Place the new form in the string */
                Substrcpy(&(Conjugates[X][1][0]),
                          0,
                          strlen(&(Conjugates[X][1][0])),
                          &(String[I]),
                          false);

                /* Jump over the newly conjugated portion */
                I += strlen(&(Conjugates[X][1][0])) - 2;

                /* Signal to leave the conjugate pair loop */
                X = Num_conj;
            }
            /* If it's the one on the right */
            else if (Strcmp2(&(Conjugates[X][1][0]), &(String[I])))
            {
                /* Find the difference in length between the old and new forms */
                int Amt = strlen(&(Conjugates[X][0][0])) -
                          strlen(&(Conjugates[X][1][0]));

                /* Make or take room for the new form */
                Strshift(String, I, Amt);

                /* Place the new form in the string */
                Substrcpy(&(Conjugates[X][0][0]),
                          0,
                          strlen(&(Conjugates[X][0][0])),
                          &(String[I]),
                          false);

                /* Jump over the newly conjugated portion */
                I += strlen(&(Conjugates[X][0][0])) - 2;

                /* Signal to leave the conjugate pair loop */
                X = Num_conj;
            }
        }
    }

    // Clean up exclamation points.
    Clean(String);
}


/****************************************************************/
/*  Get_response - Find appropriate response to input text.     */
/*    Input:  Input - Input text to respond to                  */
/*    Output: Input - Response                                  */
/****************************************************************/
void Get_response(char *Input, struct KeyElement *Keyhead_ptr)
{
    int len;
    bool Found = false;
    unsigned I;
    int Concat_pos;
    struct KeyElement *currPtr;
    struct KeyElement *Key_ptr = NULL;
    char Temp[Input_size];

    // Remove newline from the end.
    len = strlen(Input);
    if (len > 0  &&  Input[len-1] == '\n')
        Input[len-1] = 0;

    // Add a space to the front and copy to Temp.
    Temp[0] = ' ';
    Temp[1] = '\0';
    strcat(Temp, Input);

    // Copy back to Input.
    strcpy(Input, Temp);

    // Add a space to the end of Input.
    Temp[0] = ' ';
    Temp[1] = '\0';
    strcat(Input, Temp);

    //printf(":%s:\n", Input);

    /* Convert string to upper case, and strip away punctuation */
    Clean(Input);

    //printf("%s\n", Input);

    /* For each keyword in the chain */
    for (currPtr = Keyhead_ptr;
         currPtr  &&  !Found;
         currPtr = currPtr -> nextPtr)
    {
        //printf("%s\n", &(currPtr -> Keyword));
        /* For each character in the input string, look for the keyword */
        for (I = 0; I < strlen(Input)  &&  !Found; ++I)
        {
            /* If found in string */
            if (Strcmp2(&(currPtr -> Keyword), &(Input[I])))
            {  /* Break out of the loop, save position in keyword chain */
                Found = true;
                Key_ptr = currPtr;
            }

            /* If we've reached the NOTFOUND keyword */
            if (Strcmp2(&(currPtr -> Keyword), "NOTFOUND"))
            {  /* Set the Key_ptr in case we leave */
                Key_ptr = currPtr;
            }
        }
    }

    /* If key was found, or NOTFOUND was selected */
    if (Key_ptr)
    {  /* Continue */
        /* Get current response to work string */
        strcpy(Temp, &(Key_ptr -> Response_ptr -> currPtr -> response));

        /* Move to next response */
        if (Key_ptr -> Response_ptr -> currPtr -> nextPtr)
        {
            Key_ptr->Response_ptr->currPtr =
                    Key_ptr->Response_ptr->currPtr->nextPtr;
        }
        else  /* Move back to head if out of responses */
        {
            Key_ptr->Response_ptr->currPtr =
                    Key_ptr->Response_ptr->headPtr;
        }

        /* If a concatenation is required */
        if (Temp[strlen(Temp) - 1] == '*')
        {
            /* Save position of concat for later */
            Concat_pos = strlen(Temp) - 1;

            /* Convert * to a space */
            Temp[Concat_pos] = ' ';

            /* Move back to the start of the keyword */
            --I;

            /* Conjugate Input after keyword to end */
            Conjugate(Input, I + strlen(&(Key_ptr -> Keyword)) - 1);

            /* Attach rest of Input after keyword to the end of Temp */
            strcat(Temp, &(Input[I + strlen(&(Key_ptr -> Keyword))]));

            /* Remove extra space at concat position (if any) */
            if (Temp[Concat_pos + 1] == ' ')
                Strshift(Temp, Concat_pos + 1, -1);
        }
    }
    else  /* Indicate problem with definition file */
    {
        printf("NOTFOUND key missing in definition file\n");
    }

    /* Move final response to Input */
    strcpy(Input, Temp);

}


/****************************************************************/
/*  Eliza_Init - Read response file and build keyword and       */
/*               response chains.                               */
/*    Input:  Filename - Name of keyword/response file          */
/*    Output: Eliza_Init - Pointer to head of keyword chain     */
/****************************************************************/
struct KeyElement *Eliza_init(const char *Filename)
{
    FILE *Response_file;
    char Work_string[Input_size];
    struct KeyElement *headPtr = NULL;
    struct KeyElement *Curr_key_ptr = NULL;
    struct KeyElement *Prev_ptr = NULL;
    struct ResponseAnchor *ResponseAnchor_ptr = NULL;
    struct ResponseElement *Response_ptr = NULL;
    bool Done = false;
    bool Title = false;
    enum { Response, Keyword } Mode = Response;

    /* Open the eliza response file */
    Response_file = fopen(Filename, "r");

    if (Response_file == NULL)
    {
        perror(Filename);
        return NULL;
    }

    /* Read each record, and build a linked list */
    while (fgets(Work_string, Input_size, Response_file) != NULL  &&  !Done)
    {
        /* Do appropriate work based on first character of string */
        switch (*Work_string)
        {
        /* Title line */
        case '$':
            /* Write to screen */
            printf("%s", &Work_string[1]);
            Title = true;

            break;

        /* Keyword */
        case '!':
            /* Add to keyword chain */
            /* Convert underscores to blanks */
            Remus(Work_string);

            /* Allocation size explanation:    */
            /* sizeof includes dummy char   -1 for Turbo, -2 for MS */
            /* strlen doesn't include null  +1 */
            /* don't want newline           -1 */
            /* don't want control char      -1 */
            /*        TOTAL . . . . . . . . -2 */
            Curr_key_ptr = (struct KeyElement *)
                malloc(sizeof(struct KeyElement) + strlen(Work_string) - 2);

            if (!headPtr) headPtr = Curr_key_ptr;

            /* Copy the keyword into the new element on chain losing the */
            /* first character. */
            Substrcpy(Work_string, 1, strlen(Work_string) - 2,
                      &(Curr_key_ptr -> Keyword), true);

            /* If we have been doing responses */
            if (Mode == Response)
            {  /* Allocate a new response anchor */
                /* Set old response anchor curr_ptr to head of chain */
                if (ResponseAnchor_ptr)
                    ResponseAnchor_ptr -> currPtr =
                            ResponseAnchor_ptr -> headPtr;

                /* Indicate we are now doing keywords */
                Mode = Keyword;

                /* Allocate a new response anchor, make it the current */
                ResponseAnchor_ptr = (struct ResponseAnchor *)
                        malloc(sizeof(struct ResponseAnchor));
                ResponseAnchor_ptr -> headPtr = NULL;
                ResponseAnchor_ptr -> currPtr = NULL;
            }

            /* Point to current response anchor */
            Curr_key_ptr -> Response_ptr = ResponseAnchor_ptr;
            Curr_key_ptr -> nextPtr = NULL;

            /* Point previous element to this element */
            if (Prev_ptr) Prev_ptr -> nextPtr = Curr_key_ptr;

            /* Update ptr to previous element */
            Prev_ptr = Curr_key_ptr;

            break;

        /* Response text */
        case '-':
            /* Indicate we are now doing responses */
            Mode = Response;

            /* Add to current response chain */
            if (ResponseAnchor_ptr)
            {
                /* Allocate a response element */
                Response_ptr = (struct ResponseElement *)
                        malloc(sizeof(struct ResponseElement) +
                               strlen(Work_string) - 2);

                /* Copy response text into new element */
                Substrcpy(Work_string, 1, strlen(Work_string) - 2,
                          &(Response_ptr -> response), true);

                /* Point new element to NULL */
                Response_ptr -> nextPtr = NULL;

                /* If this is an empty chain, init */
                if (!(ResponseAnchor_ptr -> headPtr))
                {
                    /* Point head and curr ptrs to new element */
                    ResponseAnchor_ptr -> headPtr = Response_ptr;
                    ResponseAnchor_ptr -> currPtr = Response_ptr;
                }
                else  /* Add an element, use currPtr to track tail */
                {
                    /* Point old tail (currPtr) to new element */
                    ResponseAnchor_ptr -> currPtr -> nextPtr = Response_ptr;

                    /* Point curr_ptr in anchor to new element */
                    ResponseAnchor_ptr -> currPtr = Response_ptr;
                }
            }  /* Response anchor allocated */

            break;

        }  /* switch (Work_string) */

    }  /* while reading records from file */

    /* Close the response file */
    fclose(Response_file);

    /* If no title lines in .ELZ file and File was read successfully */
    if (!Title  &&  headPtr)
    {  /* Display default title lines */
        printf("ELIZA  v2.10\n");
        printf("Adapted By: Ted Felix from the Creative Computing version\n\n");
        printf("Hi!  I'm Eliza.  What's your problem?\n");
    }

    /* Set last response anchor curr_ptr to head of chain */
    if (ResponseAnchor_ptr)
    {
        ResponseAnchor_ptr -> currPtr =
                ResponseAnchor_ptr -> headPtr;
    }

    return(headPtr);

}

// ********************************************************************

/* Debugging routine to format keyword/response chains */
void Format(struct KeyElement *Key_head)
{
    struct KeyElement *currPtr;
    struct ResponseElement *Response_ptr;

    /* For each keyword */
    for (currPtr = Key_head; currPtr; currPtr = currPtr -> nextPtr)
    {
        printf("%s\n", &(currPtr -> Keyword));

        /* Format responses */
        for (Response_ptr = currPtr -> Response_ptr -> headPtr;
             Response_ptr;
             Response_ptr = Response_ptr -> nextPtr)
        {
            printf("   - %s\n", &(Response_ptr -> response));
        }
    }
}

// ********************************************************************

int main(int argc, char *argv[])
{
	// Buffer for I/O.
    char Work_string[Input_size];
    /* Pointer to head of response chain */
    struct KeyElement *Key_head;
    /* Response file name */
    char Filename[128];

    /* Logging indication flag */
    bool Log = false;
    /* Log file session pointer */
    FILE *Log_file;
    /* Time and Date stuff for logging */
    //time_t T;
    //struct tm *Local;

    /* If a command line argument was passed */
    if (argc > 1)
    {
        strcpy(Filename, argv[1]);

        /* Use default extension if none specified */
        if (!strchr(Filename, '.'))
           strcat(Filename, ".elz");
    }
    else  /* Use default file name */
    {
        strcpy(Filename, "standard.elz");
    }

    Key_head = Eliza_init(Filename);

    /* If open failed, bail. */
    if (!Key_head)
        return 1;

    // Processing loop...
    for ( ;; )
    {
        /* Print user prompt */
        printf("> ");

        /* Get user input */
        //Input(Work_string, 255);
        fgets(Work_string, 255, stdin);

        /* If this is the "logf" command, start logging */
        if (Strcmp2("logf", Work_string))
        {
            Log = true;   /* Indicate logging enabled */

            // Remove the trailing \n.
            Work_string[strlen(Work_string) - 1] = 0;

            /* Open log file for append */
            Log_file = fopen(&Work_string[5], "a");

            /* Handle any open errors */
            if (Log_file == NULL)
            {
                perror(&Work_string[5]);
                Log = false;
            }
            else
            {
                fprintf(Log_file, "\n*******************************************************************************\n");
//                fprintf(Log_file,
//                        "Logging started, %02d/%02d/%d  ",
//                        Dos_date.da_mon, Dos_date.da_day, Dos_date.da_year);
//                fprintf(Log_file,
//                        "%02d:%02d,  Responses: %s\n\n",
//                        Dos_time.ti_hour, Dos_time.ti_min, Filename);
                fflush(Log_file);
            }

            continue;
        }

        if (Strcmp2("bye", Work_string))
            return 0;
        if (Strcmp2("exit", Work_string))
            return 0;
        if (Strcmp2("quit", Work_string))
            return 0;

        /* If logging is on, write user's input to log file */
        if (Log)
        {
            fprintf(Log_file, "> ");
            fputs(Work_string, Log_file);
            fprintf(Log_file, "\n");
            fflush(Log_file);
        }

        /* Get Eliza's response to user input */
        Get_response(Work_string, Key_head);

        /* Log response if logging is on */
        if (Log)
        {
            fputs(Work_string, Log_file);
            fprintf(Log_file, "\n");
            fflush(Log_file);
        }

        /* Display response to user */
        puts(Work_string);
    }
}

