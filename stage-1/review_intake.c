/*
 * ============================================================================
 *  ShopFast -- Product Review Intake System   (EDUCATIONAL DEMO)
 *  STAGE 1 VARIANT -- x86-64, NX OFF (executable stack)
 * ============================================================================
 *
 *  This is the baseline program for the whole project -- a small command-line
 *  tool a store might use to collect product reviews, with ONE deliberately
 *  planted stack buffer overflow in submit_review(). Stages 2 and 3 reuse this
 *  exact source unchanged; the ONLY things that differ between the stages are
 *  the build flags and the exploit technique:
 *
 *    Stage 1 (here) : NX OFF  -> inject shellcode onto the stack and run it.
 *    Stage 2        : NX ON   -> the exact same exploit faults (SIGSEGV).
 *    Stage 3        : NX ON   -> defeat NX by reusing existing code (ROP).
 *
 *  Keeping the program identical across the stages is the whole point: it shows
 *  that a single defense (the NX bit) is what turns the classic code-injection
 *  attack from "works" into "dies", which is exactly what motivates ROP.
 *
 *  >>> This program is intentionally vulnerable. Do NOT deploy it. <<<
 *  >>> It exists only to teach how the attack and the defense work.  <<<
 *
 *  Target architecture: 64-bit x86 (amd64).
 *  Build with the provided Makefile (NX OFF, stack canary off, PIE off).
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_REVIEWS 16
#define PRODUCT_LEN 32

struct review {
    int  rating;                 /* 1..5                       */
    char product[PRODUCT_LEN];   /* product name               */
    char comment[80];            /* a short copy of the comment */
};

static struct review g_reviews[MAX_REVIEWS];
static int           g_count = 0;

/* ------------------------------------------------------------------ */
/*  Small helpers                                                     */
/* ------------------------------------------------------------------ */

static void print_banner(void)
{
    puts("============================================");
    puts("   ShopFast  --  Product Review Intake");
    puts("   (internal tool, build 0.9 'intern')");
    puts("============================================");
}

/* Read a whole line into dst (safe). Returns length without newline. */
static int read_line(char *dst, int size)
{
    if (!fgets(dst, size, stdin))
        return -1;
    int n = (int)strlen(dst);
    if (n > 0 && dst[n - 1] == '\n')
        dst[--n] = '\0';
    return n;
}

/* ------------------------------------------------------------------ */
/*  The VULNERABLE function                                           */
/* ------------------------------------------------------------------ */
/*
 *  A junior developer sized the on-stack scratch buffer at 64 bytes but
 *  then called read() with a length of 256. If the reviewer types more
 *  than 64 bytes, the extra bytes run past the end of `comment` and
 *  overwrite the saved frame pointer and the saved return address --
 *  the textbook stack buffer overflow.
 */
static void submit_review(void)
{
    char comment[64];            /* <-- fixed-size stack buffer (the bug) */
    char product[PRODUCT_LEN];
    int  rating;
    char line[16];

    printf("\nProduct name: ");
    fflush(stdout);
    read_line(product, sizeof(product));

    printf("Rating (1-5): ");
    fflush(stdout);
    read_line(line, sizeof(line));
    rating = atoi(line);
    if (rating < 1) rating = 1;
    if (rating > 5) rating = 5;

    printf("Write your review comment:\n> ");
    fflush(stdout);

    /* ***************  THE VULNERABILITY  ***************
     * comment[] is only 64 bytes, but we read up to 256.
     * read() copies raw bytes (any value, including NULs),
     * which is exactly what makes it convenient to exploit. */
    int n = read(STDIN_FILENO, comment, 256);
    if (n <= 0) return;
    if (n >= (int)sizeof(comment))        /* keep the printf tidy-ish */
        comment[sizeof(comment) - 1] = '\0';
    else
        comment[n] = '\0';
    /* *************************************************** */

    /* Store a trimmed copy so the "list" feature has something to show. */
    if (g_count < MAX_REVIEWS) {
        g_reviews[g_count].rating = rating;
        strncpy(g_reviews[g_count].product, product, PRODUCT_LEN - 1);
        strncpy(g_reviews[g_count].comment, comment, sizeof(g_reviews[0].comment) - 1);
        g_count++;
    }

    printf("\nThanks! Recorded %d/5 for \"%s\".\n", rating, product);
    printf("Comment on file: %s\n", comment);
}

static void list_reviews(void)
{
    if (g_count == 0) {
        puts("\n(no reviews yet)");
        return;
    }
    printf("\n-- %d review(s) --\n", g_count);
    for (int i = 0; i < g_count; i++) {
        printf("  [%d] %d/5  %-20s  \"%s\"\n",
               i + 1, g_reviews[i].rating,
               g_reviews[i].product, g_reviews[i].comment);
    }
}

/* ------------------------------------------------------------------ */
/*  Menu loop                                                         */
/* ------------------------------------------------------------------ */

int main(void)
{
    char line[16];

    /* Make stdout unbuffered so prompts always appear, even over a pipe. */
    setvbuf(stdout, NULL, _IONBF, 0);

    print_banner();

    for (;;) {
        printf("\nMenu:\n");
        printf("  1) Submit a review\n");
        printf("  2) List reviews\n");
        printf("  3) Quit\n");
        printf("Choice: ");

        if (read_line(line, sizeof(line)) < 0)
            break;                      /* EOF */

        switch (atoi(line)) {
            case 1: submit_review(); break;
            case 2: list_reviews();  break;
            case 3: puts("Goodbye."); return 0;
            default: puts("Please choose 1, 2, or 3.");
        }
    }
    return 0;
}
