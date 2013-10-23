#ifndef NANOTAP_H_
#define NANOTAP_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __GNUC__
#define NANOTAP_DECLARE static __attribute__((__used__))
#else
#define NANOTAP_DECLARE static
#endif

#ifdef __cplusplus
#define NANOTAP_INLINE inline
#else
#define NANOTAP_INLINE __inline__
#endif

static int TEST_COUNT = 0;

/**
 * This simply evaluates any expression ("$got eq $expected" is just a
 * simple example) and uses that to determine if the test succeeded or
 * failed.  A true expression passes, a false one fails.  Very simple.
 */
NANOTAP_INLINE NANOTAP_DECLARE void ok(int x, const char *msg) {
    printf("%s %d - %s\n", (x ? "ok" : "not ok"), ++TEST_COUNT, msg ? msg : "");
}

/**
 * display diagnostics message.
 */
NANOTAP_INLINE NANOTAP_DECLARE void diag(const char *msg) {
    fprintf(stderr, "# %s\n", msg ? msg : "");
}
/**
 * display note message.
 */
NANOTAP_INLINE NANOTAP_DECLARE void note(const char *msg) {
    fprintf(stdout, "# %s\n", msg ? msg : "");
}

/**
 * contains_string() searches for $substring in $string.
 */
NANOTAP_INLINE NANOTAP_DECLARE void contains_string(const char *string, const char *substring, const char *msg) {
    ok(strstr(string, substring) != NULL, msg);
}

/**
 *  If you don’t know how many tests you’re going to run, you can issue
 * the plan when you’re done running tests.
 */
NANOTAP_INLINE NANOTAP_DECLARE void done_testing() {
    printf("1..%d\n", TEST_COUNT);
    exit(0);
}

#ifdef __cplusplus
// you can use more convinient stuff if you are using c++.

#include <string>
#include <iostream>

/**
 * shorthand for std::string
 */
inline NANOTAP_DECLARE void diag(const std::string &msg) {
    diag(msg.c_str());
}

/**
 * flexible is() based on C++ template.
 */
template <class T>
inline NANOTAP_DECLARE void is(T got, T expected, const char *msg) {
    if (got == expected) {
        ok(true, msg);
    } else {
        ok(false, msg);
        std::cout << "  # got      : " << got << std::endl;
        std::cout << "  # expected : " << expected << std::endl;
    }
}

template <class T>
inline NANOTAP_DECLARE void is(T got, T expected) {
    is(got, expected, "");
}

inline NANOTAP_DECLARE void is(const std::string& got, const char *expected, const char *msg) {
    is(got, std::string(expected), msg);
}

inline NANOTAP_DECLARE void is(const char* got, const std::string & expected, const char *msg) {
    is(std::string(got), expected, msg);
}

template <class T, class U>
inline NANOTAP_DECLARE void is(T got, U expected) {
    is(got, std::string(expected), NULL);
}

/**
 * shorthand for lazy person
 */
inline NANOTAP_DECLARE void ok(int x) {
    ok(x, "");
}

/**
 * shorthand for std::string
 */
inline NANOTAP_DECLARE void contains_string(const std::string &str, const char *substr, const char *msg) {
    contains_string(str.c_str(), substr, msg);
}

#include <sstream>

/**
 * test binary string
 */
inline NANOTAP_DECLARE void is_binary(const std::string &got, const std::string& expected, const char *msg) {
    if (got.size() != expected.size()) {
        ok(0, msg);
        std::stringstream ss;
        ss << "Expected " << expected.size() << " bytes chars, but got " << got.size() << " bytes chars";
        diag(ss.str());
        return;
    }
    for (size_t i=0; i<got.size(); i++) {
        if (got[i] != expected[i]) {
            ok(0, msg);

            std::stringstream ss;
            ss << "Expected " << std::hex << int(expected[i]) << " but got " << int(got[i]) << ", at " << i;
            diag(ss.str());
            return;
        }
    }
    ok(true, msg);
}

#endif

#endif /* NANOTAP_H_ */
