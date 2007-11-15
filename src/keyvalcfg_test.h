#ifndef KEYVALCFG_TEST_INCLUDED
#define KEYVALCFG_TEST_INCLUDED

/* unit tests of some sort. note that these spit things to standard output.
 * each returns 1 on success and 0 on failure. */
unsigned char test_strip_comments(void);
unsigned char test_collapse(void);
unsigned char test_strip_multiple_spaces(void);
unsigned char test_skip_leading_whitespace(void);
unsigned char test_skip_trailing_whitespace(void);
unsigned char test_sanitize_str(void);

#endif
