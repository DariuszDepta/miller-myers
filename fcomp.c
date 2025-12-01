/*
 * fcomp - a file comparison program
 *
 *  A command line has the form
 *       fcomp [-n] file1 file2
 *  where the optional flag  n  is an integer constant that limits the size of
 *  edit scripts that will be considered by fcomp. If all edit scripts changing
 *  file1 to file2 contain more than  n  insertions and deletions, then a message
 *  to that effect is all that is printed. If no  n  is specified, then arbitrarily
 *  long edit scripts are considered.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAXLINES 2000               /* maximum number of lines in a file */
#define ORIGIN MAXLINES             /* subscript for diagonal 0 */
#define INSERT 1
#define DELETE 2

/* edit scripts are stored in linked lists */
struct edit
{
    struct edit* link;              /* previous edit command */
    int op;                         /* INSERT or DELETE */
    int line1;                      /* line number in file1 */
    int line2;                      /* line number in file2 */
};

char *A[MAXLINES], *B[MAXLINES];    /* pointers to lines of file1 and file2 */

main(argc, argv)
int argc;
char* argv[];
{
    int max_d,                      /* bound on size of edit script */
        m,                          /* number of lines in file1 */
        n,                          /* number of lines in file2 */
        lower,                      /* left-most diagonal under consideration */
        upper,                      /* right-most diagonal under consideration */
        d,                          /* current edit distance */
        k,                          /* current diagonal */
        row,                        /* row number */
        col;                        /* column number */

    /* for each diagonal, two items are saved: */
    int last_d[2 * MAXLINES + 1];    /* the row containing the last d */
    struct edit* script[2 * MAXLINES + 1]; /* corresponding edit script. */

    struct edit* new;

    if (argc > 1 && argv[1][0] == '-')
    {
        max_d = atoi(&argv[1][1]);
        ++argv;
        --argc;
    }
    else
    {
        max_d = 2 * MAXLINES;
    }
    if (argc != 3) fatal("fcomp requires two file names.");

    /* Read in file1 and file2. */
    m = in_file(argv[1], A);
    n = in_file(argv[2], B);

    /* initialize: 0 entries in D indicate identical prefixes */
    for (row = 0; row < m && row < n && strcmp(A[row], B[row]) == 0; ++row);
    last_d[ORIGIN] = row;
    script[ORIGIN] = NULL;
    lower = row == m ? ORIGIN + 1 : ORIGIN - 1;
    upper = row == n ? ORIGIN - 1 : ORIGIN + 1;
    if (lower > upper)
    {
        puts("The files are identical.");
        exit(0);
    }

    /* for each value of the edit distance */
    for (d = 1; d <= max_d; ++d)
    {
        /* for each relevant diagonal */
        for (k = lower; k <= upper; k += 2)
        {
            /* Get space for the next edit instruction. */
            new = (struct edit*)malloc(sizeof(struct edit));
            if (new == NULL) exceed(d);

            /* Find a d on diagonal k. */
            if (k == ORIGIN - d || k != ORIGIN + d && last_d[k + 1] >= last_d[k - 1])
            {
                /*
                 * Moving down from the last d-1 on diagonal k + l
                 * puts you farther along diagonal k than does
                 * moving right from the last d-1 on diagonal k-1.
                 */
                row = last_d[k + 1] + 1;
                new->link = script[k + 1];
                new->op = DELETE;
            }
            else
            {
                /* Move right from the last d-1 on diagonal k-1. */
                row = last_d[k - 1];
                new->link = script[k - 1];
                new->op = INSERT;
            }
            /* Code common to the two cases. */
            new->line1 = row;
            new->line2 = col = row + k - ORIGIN;
            script[k] = new;

            /* Slide down the diagonal. */
            while (row < m && col < n && strcmp(A[row], B[col]) == 0)
            {
                ++row;
                ++col;
            }
            last_d[k] = row;

            if (row == m && col == n)
            {
                /* Hit southeast corner; have the answer. */
                put_scr(script[k]);
                exit(0);
            }

            /* Hit last row; don't look to the left. */
            if (row == m) lower = k + 2;
            /* Hit last column; don't look to the right. */
            if (col == n) upper = k - 2;
        }
        --lower;
        ++upper;
    }
    exceed(d);
}

/* in_file - read in a file and return a count of the lines */
in_file(filename, P)
char *filename, *P[];
{
    char buf[100], *save, *b;
    FILE* fp;
    int lines = 0;
    if ((fp = fopen(filename, "r")) == NULL)
    {
        fprintf(stderr, "Cannot open file %s.\n", filename);
        exit(1);
    }
    while (fgets(buf, 100, fp) != NULL)
    {
        if (lines >= MAXLINES) fatal("File is too large for fcomp.");
        if ((save = malloc(strlen(buf) + 1)) == NULL) fatal("Not enough room to save the files.");
        P[lines++] = save;
        for (b = buf; *save++ = *b++;); /* copy the line */
    }
    fclose(fp);
    return lines;
}

/* put_scr - print the edit script */
put_scr(start)
struct edit *start;
{
    struct edit *ep, *behind, *ahead, *a, *b;
    int change;

    /* Reverse the pointers. */
    ahead = start;
    ep = NULL;
    while (ahead != NULL)
    {
        behind = ep;
        ep = ahead;
        ahead = ahead->link;
        ep->link = behind; /* Flip the pointer. */
    }

    /* Print commands. */
    while (ep != NULL)
    {
        b = ep;
        if (ep->op == INSERT)
        {
            printf("inserted after line %d:\n", ep->line1);
        }
        else /* DELETE */
        {
            /* Look for a block of consecutive deleted lines. */
            do
            {
                a = b;
                b = b->link;
            }
            while (b != NULL && b->op == DELETE && b->line1 == a->line1 + 1);
            /* Now b points to the command after the last deletion. */
            change = (b != NULL && b->op == INSERT && b->line1 == a->line1);
            if (change) printf("Changed ");
            else printf("Deleted ");
            if (a == ep) printf("line %d:\n", ep->line1);
            else printf("lines %d-%d:\n", ep->line1, a->line1);
            /* Print the deleted lines. */
            do
            {
                printf(" %s", A[ep->line1 - 1]);
                ep = ep->link;
            }
            while (ep != b);
            if (!change) continue;
            printf("To:\n");
        }
        /* Print the inserted lines. */
        do
        {
            printf(" %s", B[ep->line2 - 1]);
            ep = ep->link;
        }
        while (ep != NULL && ep->op == INSERT && ep->line1 == b->line1);
    }
}

/* fatal - print error message and exit. */
fatal(msg)
char *msg;
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

/* exceed - the difference exceeds d. */
exceed(int d)
{
    fprintf(stderr, "The files differ in at least %d lines.\n", d);
    exit(1);
}
