/*

  ----------------------------------------------------
  vsm - vector space model data similarity
  ----------------------------------------------------

  Copyright (c) 2008 Jason Bittel <jason.bittel@gmail.com>

*/

/*
  This is the Porter stemming algorithm, released to the  public domain
  and taken from: http://www.tartarus.org/~martin/PorterStemmer

  Modifications have been made for stylistic and implementation purposes;
  additional optimizations are included based on knowledge of incoming data.
  This implementation should NOT be considered canonical.
*/

#define TRUE 1
#define FALSE 0

#include <string.h>
#include "stem.h"

static int cons(int i);
static int m();
static int vowelinstem();
static int doublec(int j);
static int cvc(int i);
static int ends(char * s);
static void setto(char * s);
static void r(char * s);
static void step1ab();
static void step1c();
static void step2();
static void step3();
static void step4();
static void step5();

/*
 * The main part of the stemming algorithm starts here. b is a buffer
 * holding a word to be stemmed. The letters are in b[0], b[0+1] ...
 * ending at b[k]. k is readjusted downwards as the stemming progresses.
 *
 * Note that only lower case sequences are stemmed. Forcing to lower case
 * should be done before stem(...) is called.
 */

static char *b;   /* Buffer for word to be stemmed */
static int k;     /* Points to the end of the word */
static int j;     /* General offset into the string */

/* cons(i) is TRUE <=> b[i] is a consonant. */
static int cons(int i) {
        switch (b[i]) {
                case 'a': case 'e': case 'i': case 'o': case 'u': return FALSE;
                case 'y': return (i == 0) ? TRUE : !cons(i-1);
                default: return TRUE;
        }
}

/* m() measures the number of consonant sequences between 0 and j. */
static int m() {
        int n = 0;
        int i = 0;

        while (TRUE) {
                if (i > j) return n;
                if (!cons(i)) break;
                i++;
        }
 
        i++;
 
        while (TRUE) {
                while (TRUE) {
                        if (i > j) return n;
                        if (cons(i)) break;
                        i++;
                }

                i++;
                n++;
                while(TRUE) {
                        if (i > j) return n;
                        if (!cons(i)) break;
                        i++;
                }

                i++;
        }
}

/* vowelinstem() is TRUE <=> 0,...j contains a vowel */
static int vowelinstem() {
        int i;
 
        for (i = 0; i <= j; i++)
                if (! cons(i)) return TRUE;
 
        return FALSE;
}

/* doublec(j) is TRUE <=> j,(j-1) contain a double consonant. */
static int doublec(int j) {
        if (j < 1) return FALSE;
        if (b[j] != b[j-1]) return FALSE;
 
        return cons(j);
}

/* cvc(i) is TRUE <=> i-2,i-1,i has the form consonant - vowel - consonant
   and also if the second c is not w,x or y. this is used when trying to
   restore an e at the end of a short word. */
static int cvc(int i) {
        if (i < 2 || !cons(i) || cons(i-1) || !cons(i-2)) return FALSE;
 
        {
                int ch = b[i];
                if (ch == 'w' || ch == 'x' || ch == 'y') return FALSE;
        }
 
        return TRUE;
}

/* ends(s) is TRUE <=> 0,...k ends with the string s. */
static int ends(char *s) {
        int length = s[0];
 
        if (s[length] != b[k]) return FALSE; /* tiny speed-up */
        if (length > k-1) return FALSE;
        if (memcmp(b+k-length+1,s+1,length) != 0) return FALSE;
        j = k-length;
 
        return TRUE;
}

/* setto(s) sets (j+1),...k to the characters in the string s, readjusting k. */
static void setto(char *s) {
        int length = s[0];
 
        memmove(b + j + 1, s + 1, length);
        k = j + length;
}

/* r(s) is used further down. */
static void r(char *s) { 
        if (m() > 0) setto(s);
}

/* step1ab() gets rid of plurals and -ed or -ing. */
static void step1ab() {
        if (b[k] == 's') {
                if (ends("\04" "sses")) k -= 2; else
                if (ends("\03" "ies")) setto("\01" "i"); else
                if (b[k-1] != 's') k--;
        }

        if (ends("\03" "eed")) { if (m() > 0) k--; } else
        if ((ends("\02" "ed") || ends("\03" "ing")) && vowelinstem()) {
                k = j;
                if (ends("\02" "at")) setto("\03" "ate"); else
                if (ends("\02" "bl")) setto("\03" "ble"); else
                if (ends("\02" "iz")) setto("\03" "ize"); else
                if (doublec(k)) {
                        k--;
                        {
                                int ch = b[k];
                                if (ch == 'l' || ch == 's' || ch == 'z') k++;
                        }
                }
                else if (m() == 1 && cvc(k)) setto("\01" "e");
        }
}

/* step1c() turns terminal y to i when there is another vowel in the stem. */
static void step1c() {
        if (ends("\01" "y") && vowelinstem()) b[k] = 'i';
}

/* step2() maps double suffices to single ones. so -ization ( = -ize plus
   -ation) maps to -ize etc. note that the string before the suffix must give
   m() > 0. */
static void step2() {
        switch (b[k-1]) {
                case 'a':
                        if (ends("\07" "ational")) { r("\03" "ate"); break; }
                        if (ends("\06" "tional")) { r("\04" "tion"); break; }
                        break;
                case 'c':
                        if (ends("\04" "enci")) { r("\04" "ence"); break; }
                        if (ends("\04" "anci")) { r("\04" "ance"); break; }
                        break;
                case 'e':
                        if (ends("\04" "izer")) { r("\03" "ize"); break; }
                        break;
                case 'l':
                        if (ends("\03" "bli")) { r("\03" "ble"); break; }
                        if (ends("\04" "alli")) { r("\02" "al"); break; }
                        if (ends("\05" "entli")) { r("\03" "ent"); break; }
                        if (ends("\03" "eli")) { r("\01" "e"); break; }
                        if (ends("\05" "ousli")) { r("\03" "ous"); break; }
                        break;
                case 'o':
                        if (ends("\07" "ization")) { r("\03" "ize"); break; }
                        if (ends("\05" "ation")) { r("\03" "ate"); break; }
                        if (ends("\04" "ator")) { r("\03" "ate"); break; }
                        break;
                case 's':
                        if (ends("\05" "alism")) { r("\02" "al"); break; }
                        if (ends("\07" "iveness")) { r("\03" "ive"); break; }
                        if (ends("\07" "fulness")) { r("\03" "ful"); break; }
                        if (ends("\07" "ousness")) { r("\03" "ous"); break; }
                        break;
                case 't':
                        if (ends("\05" "aliti")) { r("\02" "al"); break; }
                        if (ends("\05" "iviti")) { r("\03" "ive"); break; }
                        if (ends("\06" "biliti")) { r("\03" "ble"); break; }
                        break;
                case 'g':
                        if (ends("\04" "logi")) { r("\03" "log"); break; }
        }
}

/* step3() deals with -ic-, -full, -ness etc. similar strategy to step2. */
static void step3() {
        switch (b[k]) {
                case 'e':
                        if (ends("\05" "icate")) { r("\02" "ic"); break; }
                        if (ends("\05" "ative")) { r("\00" ""); break; }
                        if (ends("\05" "alize")) { r("\02" "al"); break; }
                        break;
                case 'i':
                        if (ends("\05" "iciti")) { r("\02" "ic"); break; }
                        break;
                case 'l':
                        if (ends("\04" "ical")) { r("\02" "ic"); break; }
                        if (ends("\03" "ful")) { r("\00" ""); break; }
                        break;
                case 's':
                        if (ends("\04" "ness")) { r("\00" ""); break; }
                        break;
        }
}

/* step4() takes off -ant, -ence etc., in context <c>vcvc<v>. */
static void step4() {
        switch (b[k-1]) {
                case 'a':
                        if (ends("\02" "al")) break; return;
                case 'c':
                        if (ends("\04" "ance")) break;
                        if (ends("\04" "ence")) break; return;
                case 'e':
                        if (ends("\02" "er")) break; return;
                case 'i':
                        if (ends("\02" "ic")) break; return;
                case 'l':
                        if (ends("\04" "able")) break;
                        if (ends("\04" "ible")) break; return;
                case 'n':
                        if (ends("\03" "ant")) break;
                        if (ends("\05" "ement")) break;
                        if (ends("\04" "ment")) break;
                        if (ends("\03" "ent")) break; return;
                case 'o':
                        if (ends("\03" "ion") && (b[j] == 's' || b[j] == 't')) break;
                        if (ends("\02" "ou")) break; return; /* takes care of -ous */
                case 's':
                        if (ends("\03" "ism")) break; return;
                case 't':
                        if (ends("\03" "ate")) break;
                        if (ends("\03" "iti")) break; return;
                case 'u':
                        if (ends("\03" "ous")) break; return;
                case 'v':
                        if (ends("\03" "ive")) break; return;
                case 'z':
                        if (ends("\03" "ize")) break; return;
                default: return;
        }

        if (m() > 1) k = j;
}

/* step5() removes a final -e if m() > 1, and changes -ll to -l if m() > 1. */
static void step5() {
        j = k;
 
        if (b[k] == 'e') {
                int a = m();
                if (a > 1 || (a == 1 && !cvc(k-1))) k--;
        }
 
        if (b[k] == 'l' && doublec(k) && m() > 1) k--;
}

/*
 * stem() takes a pointer to the word to be stemmed, adjusts the characters
 * p[0] ... p[strlen(p)-1] and returns the offset of the new end point of the
 * string, k. Stemming never increases word length, so b[0] <= k <= j.
 */
int stem(char *p) {
        b = p;                  /* Copy initial values into statics */
        k = strlen(p) - 1;      /* Set last char offset */
 
        if (k <= 2) return k;   /* Skip short words */

        step1ab(); step1c(); step2(); step3(); step4(); step5();

        b[k + 1] = '\0';
 
        return k;
}
