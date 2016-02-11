#include <libcut.h>
#include "rejit.h"

LIBCUT_TEST(test_tokenize) {
    rejit_parse_error err;
    err.kind = RJ_PE_NONE;
    err.pos = 0;
    const char s[] = "A(bC)*+?[abc]d\\+|e";
    rejit_token_list tokens = rejit_tokenize(s, &err);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_NONE);

    LIBCUT_TEST_EQ(tokens.len, 11);

    LIBCUT_TEST_EQ(tokens.tokens[0].kind, RJ_TWORD);
    LIBCUT_TEST_EQ(tokens.tokens[0].pos, (char*)s);
    LIBCUT_TEST_EQ(tokens.tokens[0].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[1].kind, RJ_TLP);
    LIBCUT_TEST_EQ(tokens.tokens[1].pos, s+1);
    LIBCUT_TEST_EQ(tokens.tokens[1].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[2].kind, RJ_TWORD);
    LIBCUT_TEST_EQ(tokens.tokens[2].pos, s+2);
    LIBCUT_TEST_EQ(tokens.tokens[2].len, 2);

    LIBCUT_TEST_EQ(tokens.tokens[3].kind, RJ_TRP);
    LIBCUT_TEST_EQ(tokens.tokens[3].pos, s+4);
    LIBCUT_TEST_EQ(tokens.tokens[3].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[4].kind, RJ_TSTAR);
    LIBCUT_TEST_EQ(tokens.tokens[4].pos, s+5);
    LIBCUT_TEST_EQ(tokens.tokens[4].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[5].kind, RJ_TPLUS);
    LIBCUT_TEST_EQ(tokens.tokens[5].pos, s+6);
    LIBCUT_TEST_EQ(tokens.tokens[5].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[6].kind, RJ_TQ);
    LIBCUT_TEST_EQ(tokens.tokens[6].pos, s+7);
    LIBCUT_TEST_EQ(tokens.tokens[6].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[7].kind, RJ_TSET);
    LIBCUT_TEST_EQ(tokens.tokens[7].pos, s+8);
    LIBCUT_TEST_EQ(tokens.tokens[7].len, 5);

    LIBCUT_TEST_EQ(tokens.tokens[8].kind, RJ_TWORD);
    LIBCUT_TEST_EQ(tokens.tokens[8].pos, s+13);
    LIBCUT_TEST_EQ(tokens.tokens[8].len, 3);

    LIBCUT_TEST_EQ(tokens.tokens[9].kind, RJ_TP);
    LIBCUT_TEST_EQ(tokens.tokens[9].pos, s+16);
    LIBCUT_TEST_EQ(tokens.tokens[9].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[10].kind, RJ_TWORD);
    LIBCUT_TEST_EQ(tokens.tokens[10].pos, s+17);
    LIBCUT_TEST_EQ(tokens.tokens[10].len, 1);
}

#define PARSE(s) res = rejit_parse(s, &err); LIBCUT_TEST_EQ(err.kind, RJ_PE_NONE);

LIBCUT_TEST(test_parse_word) {
    rejit_parse_error err;
    rejit_parse_result res;
    PARSE("ab")

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IWORD);
    LIBCUT_TEST_STREQ((char*)res.instrs[0].value, "ab");

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_INULL);
}

LIBCUT_TEST(test_parse_suffix) {
    rejit_parse_error err;
    rejit_parse_result res;
    PARSE("ab+")

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IPLUS);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IWORD);
    LIBCUT_TEST_STREQ((char*)res.instrs[1].value, "ab");

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_INULL);

    PARSE("ab+?")

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IMPLUS);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IWORD);
    LIBCUT_TEST_STREQ((char*)res.instrs[1].value, "ab");

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_INULL);

    rejit_parse("+", &err);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_SYNTAX);
    LIBCUT_TEST_EQ(err.pos, 0);
}

LIBCUT_TEST(test_parse_group) {
    rejit_parse_error err;
    rejit_parse_result res;
    PARSE("(ab?)")

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_ICGROUP);
    LIBCUT_TEST_EQ((void*)res.instrs[0].value, (void*)&res.instrs[3]);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IOPT);

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_IWORD);
    LIBCUT_TEST_STREQ((char*)res.instrs[2].value, "ab");

    LIBCUT_TEST_EQ(res.instrs[3].kind, RJ_INULL);

    PARSE("(?:ab?)")

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IGROUP);
    LIBCUT_TEST_EQ((void*)res.instrs[0].value, (void*)&res.instrs[3]);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IOPT);

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_IWORD);
    LIBCUT_TEST_STREQ((char*)res.instrs[2].value, "ab");

    LIBCUT_TEST_EQ(res.instrs[3].kind, RJ_INULL);

    rejit_parse("((a)", &err);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_UBOUND);
    LIBCUT_TEST_EQ(err.pos, 4);

    rejit_parse("(a))b", &err);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_UBOUND);
    LIBCUT_TEST_EQ(err.pos, 3);
}

LIBCUT_TEST(test_parse_set) {
    rejit_parse_error err;
    rejit_parse_result res;

    PARSE("[abc]")

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_ISET);
    LIBCUT_TEST_STREQ((char*)res.instrs[0].value, "abc");
    LIBCUT_TEST_STREQ((char*)res.instrs[0].value+4, "   ");

    rejit_parse("[abc", &err);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_UBOUND);
    LIBCUT_TEST_EQ(err.pos, 0);
}

LIBCUT_TEST(test_parse_pipe) {
    rejit_parse_error err;
    rejit_parse_result res;

    PARSE("a|b")

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IOR);
    LIBCUT_TEST_EQ(res.instrs[0].value, (intptr_t)&res.instrs[2]);
    LIBCUT_TEST_EQ(res.instrs[0].value2, (intptr_t)&res.instrs[3]);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IWORD);
    LIBCUT_TEST_STREQ((char*)res.instrs[1].value, "a");

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_IWORD);
    LIBCUT_TEST_STREQ((char*)res.instrs[2].value, "b");

    LIBCUT_TEST_EQ(res.instrs[3].kind, RJ_INULL);

    PARSE("a|(b|c)")

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IOR);
    LIBCUT_TEST_EQ((void*)res.instrs[0].value, (void*)&res.instrs[2]);
    LIBCUT_TEST_EQ((void*)res.instrs[0].value2, (void*)&res.instrs[6]);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IWORD);
    LIBCUT_TEST_STREQ((char*)res.instrs[1].value, "a");

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_ICGROUP);
    LIBCUT_TEST_EQ((void*)res.instrs[2].value, (void*)&res.instrs[6]);

    LIBCUT_TEST_EQ(res.instrs[3].kind, RJ_IOR);
    LIBCUT_TEST_EQ((void*)res.instrs[3].value, (void*)&res.instrs[5]);
    LIBCUT_TEST_EQ((void*)res.instrs[3].value2, (void*)&res.instrs[6]);

    LIBCUT_TEST_EQ(res.instrs[4].kind, RJ_IWORD);
    LIBCUT_TEST_STREQ((char*)res.instrs[4].value, "b");

    LIBCUT_TEST_EQ(res.instrs[5].kind, RJ_IWORD);
    LIBCUT_TEST_STREQ((char*)res.instrs[5].value, "c");

    LIBCUT_TEST_EQ(res.instrs[6].kind, RJ_INULL);
}

LIBCUT_TEST(test_parse_other) {
    rejit_parse_error err;
    rejit_parse_result res;

    PARSE("^a.b$")

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IBEGIN);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IWORD);
    LIBCUT_TEST_STREQ((char*)res.instrs[1].value, "a");

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_IDOT);

    LIBCUT_TEST_EQ(res.instrs[3].kind, RJ_IWORD);
    LIBCUT_TEST_STREQ((char*)res.instrs[3].value, "b");

    LIBCUT_TEST_EQ(res.instrs[4].kind, RJ_IEND);

    LIBCUT_TEST_EQ(res.instrs[5].kind, RJ_INULL);
}

LIBCUT_TEST(test_chr) {
    // ca
    rejit_instruction instrs[] = {{RJ_IWORD, (intptr_t)"ca"}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "ca", NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "", NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "c", NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "cb", NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "ab", NULL), -1);
}

LIBCUT_TEST(test_dot) {
    // .
    rejit_instruction instrs[] = {{RJ_IDOT}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "c", NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "a", NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "\n", NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "", NULL), -1);
}

LIBCUT_TEST(test_plus) {
    // c+
    rejit_instruction instrs[] = {{RJ_IPLUS}, {RJ_IWORD, (intptr_t)"c"},
                                  {RJ_INULL}};
    instrs[0].value = (intptr_t)&instrs[1];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "c", NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "a", NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "cc", NULL), 2);
}

LIBCUT_TEST(test_star) {
    // c*
    rejit_instruction instrs[] = {{RJ_ISTAR}, {RJ_IWORD, (intptr_t)"c"},
                                  {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "c", NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "a", NULL), 0);
    LIBCUT_TEST_EQ(rejit_match(m, "cc", NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "", NULL), 0);
}

LIBCUT_TEST(test_opt) {
    // ac?$
    rejit_instruction instrs[] = {{RJ_IWORD, (intptr_t)"a"}, {RJ_IOPT},
                                  {RJ_IWORD, (intptr_t)"c"}, {RJ_IEND},
                                  {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "ac", NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "a", NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "g", NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "acc", NULL), -1);
}

LIBCUT_TEST(test_begin) {
    // c*^
    rejit_instruction instrs[] = {{RJ_ISTAR}, {RJ_IWORD, (intptr_t)"c"},
                                  {RJ_IBEGIN}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "", NULL), 0);
    LIBCUT_TEST_EQ(rejit_match(m, "c", NULL), -1);
}

LIBCUT_TEST(test_end) {
    // $
    rejit_instruction instrs[] = {{RJ_IEND}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "", NULL), 0);
    LIBCUT_TEST_EQ(rejit_match(m, "c", NULL), -1);
}

LIBCUT_TEST(test_set) {
    // [xyz]
    char s[] = "\txyz\0    ";
    rejit_instruction instrs[] = {{RJ_ISET, (intptr_t)s}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "\t", NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "x", NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "y", NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "z", NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "a", NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "", NULL), -1);
}

LIBCUT_TEST(test_or) {
    // a|b
    rejit_instruction instrs[] = {{RJ_IOR}, {RJ_IWORD, (intptr_t)"a"},
                                  {RJ_IWORD, (intptr_t)"b"}, {RJ_INULL}};
    instrs[0].value = (intptr_t)&instrs[2];
    instrs[0].value2 = (intptr_t)&instrs[3];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "a", NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "b", NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "", NULL), -1);
}

LIBCUT_TEST(test_group) {
    // (a)
    rejit_instruction instrs[] = {{RJ_IGROUP}, {RJ_IWORD, (intptr_t)"a"},
                                  {RJ_INULL}};
    instrs[0].value = (intptr_t)&instrs[2];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "a", NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "", NULL), -1);
}

LIBCUT_TEST(test_cgroup) {
    // (a(b)?)
    rejit_group groups[2];
    const char str1[] = "a", str2[] = "ab";
    rejit_instruction instrs[] = {{RJ_ICGROUP, 0, 0}, {RJ_IWORD, (intptr_t)"a"},
                                  {RJ_IOPT}, {RJ_ICGROUP, 0, 1},
                                  {RJ_IWORD, (intptr_t)"b"}, {RJ_INULL}};
    instrs[0].value = instrs[3].value = (intptr_t)&instrs[5];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);

    memset(groups, 0, sizeof(groups));
    LIBCUT_TEST_EQ(rejit_match(m, str1, groups), 1);
    LIBCUT_TEST_EQ(groups[0].begin, (char*)str1);
    LIBCUT_TEST_EQ(groups[0].end, str1+1);
    LIBCUT_TEST_EQ(groups[1].begin, NULL);
    LIBCUT_TEST_EQ(groups[1].end, NULL);

    memset(groups, 0, sizeof(groups));
    LIBCUT_TEST_EQ(rejit_match(m, str2, groups), 2);
    LIBCUT_TEST_EQ(groups[0].begin, (char*)str2);
    LIBCUT_TEST_EQ(groups[0].end, str2+2);
    LIBCUT_TEST_EQ(groups[1].begin, str2+1);
    LIBCUT_TEST_EQ(groups[1].end, str2+2);

    memset(groups, 0, sizeof(groups));
    LIBCUT_TEST_EQ(rejit_match(m, "", groups), -1);
    LIBCUT_TEST_EQ(groups[0].begin, NULL);
    LIBCUT_TEST_EQ(groups[0].end, NULL);
    LIBCUT_TEST_EQ(groups[1].begin, NULL);
    LIBCUT_TEST_EQ(groups[1].end, NULL);
}

LIBCUT_TEST(test_opt_group) {
    // (ab)?
    rejit_instruction instrs[] = {{RJ_IOPT}, {RJ_IGROUP},
                                  {RJ_IWORD, (intptr_t)"ab"}, {RJ_INULL}};
    instrs[1].value = (intptr_t)&instrs[3];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "ab", NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "a", NULL), 0);
    LIBCUT_TEST_EQ(rejit_match(m, "b", NULL), 0);
    LIBCUT_TEST_EQ(rejit_match(m, "", NULL), 0);
}

LIBCUT_TEST(test_star_group) {
     // (ab)*
    rejit_instruction instrs[] = {{RJ_ISTAR}, {RJ_IGROUP},
                                  {RJ_IWORD, (intptr_t)"ab"}, {RJ_INULL}};
    instrs[1].value = (intptr_t)&instrs[3];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "ab", NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "abab", NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, "ababab", NULL), 6);
    LIBCUT_TEST_EQ(rejit_match(m, "ababa", NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, "a", NULL), 0);
    LIBCUT_TEST_EQ(rejit_match(m, "", NULL), 0);
}

LIBCUT_TEST(test_plus_group) {
    // (ab)+
    rejit_instruction instrs[] = {{RJ_IPLUS}, {RJ_IGROUP},
                                  {RJ_IWORD, (intptr_t)"ab"}, {RJ_INULL}};
    instrs[1].value = (intptr_t)&instrs[3];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "ab", NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "abab", NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, "ababab", NULL), 6);
    LIBCUT_TEST_EQ(rejit_match(m, "ababa", NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, "a", NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "", NULL), -1);
}

LIBCUT_TEST(test_lookahead) {
    // (?=ab)a(bc)?
    rejit_instruction instrs[] = {{RJ_ILAHEAD}, {RJ_IWORD, (intptr_t)"ab"},
                                  {RJ_IWORD, (intptr_t)"a"}, {RJ_IOPT},
                                  {RJ_IGROUP}, {RJ_IWORD, (intptr_t)"bc"},
                                  {RJ_INULL}};
    instrs[0].value = (intptr_t)&instrs[2];
    instrs[4].value = (intptr_t)&instrs[6];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "ab", NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "abc", NULL), 3);
    LIBCUT_TEST_EQ(rejit_match(m, "a", NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "", NULL), -1);
}

LIBCUT_TEST(test_negative_lookahead) {
    // (?!ab)[ab]*
    const char s[] = "ab\0  ";
    rejit_instruction instrs[] = {{RJ_INLAHEAD}, {RJ_IWORD, (intptr_t)"ab"},
                                  {RJ_ISTAR}, {RJ_ISET, (intptr_t)s}, {RJ_INULL}};
    instrs[0].value = (intptr_t)&instrs[2];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "bb", NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "a", NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "aa", NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "", NULL), 0);
    LIBCUT_TEST_EQ(rejit_match(m, "ab", NULL), -1);
}

LIBCUT_TEST(test_mplus) {
    // .+?b
    rejit_instruction instrs[] = {{RJ_IMPLUS}, {RJ_IDOT},
                                  {RJ_IWORD, (intptr_t)"b"}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "abcb", NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "b", NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "abb", NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "bb", NULL), 2);
}

LIBCUT_TEST(test_mstar) {
    // .*?b
    rejit_instruction instrs[] = {{RJ_IMSTAR}, {RJ_IDOT},
                                  {RJ_IWORD, (intptr_t)"b"}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "abcb", NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "b", NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "abb", NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "bb", NULL), 1);
}

LIBCUT_TEST(test_or_mixed) {
    // (b|a*)
    rejit_instruction instrs[] = {{RJ_IOR}, {RJ_IWORD, (intptr_t)"b"}, {RJ_ISTAR},
                                  {RJ_IWORD, (intptr_t)"a"}, {RJ_INULL}};
    instrs[0].value = (intptr_t)&instrs[2];
    instrs[0].value2 = (intptr_t)&instrs[4];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "aaaa", NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, "b", NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "", NULL), 0);
}

LIBCUT_TEST(test_search) {
    // a
    rejit_instruction instrs[] = {{RJ_IWORD, (intptr_t)"a"}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    const char* tgt;
    LIBCUT_TEST_EQ(rejit_search(m, "abc", &tgt, NULL), 1);
    LIBCUT_TEST_EQ(*tgt, 'c');
    LIBCUT_TEST_EQ(rejit_search(m, "babc", &tgt, NULL), 1);
    LIBCUT_TEST_EQ(*tgt, 'c');
    tgt = NULL;
    LIBCUT_TEST_EQ(rejit_search(m, "b", &tgt, NULL), -1);
    LIBCUT_TEST_EQ((void*)tgt, NULL);
}

LIBCUT_TEST(test_set_and_dot) {
    // [abc].
    const char s[] = "abc\0   ";
    rejit_instruction instrs[] = {{RJ_ISET, (intptr_t)s}, {RJ_IDOT},
                                  {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "ad", NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "bd", NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "cd", NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "c", NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "c\n", NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "\n", NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "", NULL), -1);
}

LIBCUT_TEST(test_or_group) {
    // (ab)|c
    rejit_instruction instrs[] = {{RJ_IOR}, {RJ_IGROUP},
                                  {RJ_IWORD, (intptr_t)"ab"},
                                  {RJ_IWORD, (intptr_t)"c"}, {RJ_INULL}};
    instrs[0].value = instrs[1].value = (intptr_t)&instrs[3];
    instrs[0].value2 = (intptr_t)&instrs[4];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "ab", NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "c", NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "ac", NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "b", NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "a", NULL), -1);
}

LIBCUT_TEST(test_misc) {
    rejit_matcher m;
    rejit_parse_error err;
    rejit_group group;

    group.begin = group.end = NULL;

    m = rejit_parse_compile("[Oo]rgani[sz]ation", &err);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_NONE);
    LIBCUT_TEST_EQ(rejit_match(m, "Organization", NULL), 12);
    LIBCUT_TEST_EQ(rejit_match(m, "Organisation", NULL), 12);
    LIBCUT_TEST_EQ(rejit_match(m, "organization", NULL), 12);
    LIBCUT_TEST_EQ(rejit_match(m, "organisation", NULL), 12);
    LIBCUT_TEST_EQ(rejit_match(m, "organizatio", NULL), -1);

    m = rejit_parse_compile("hono(?:u)?r(?:able)?", &err);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_NONE);
    LIBCUT_TEST_EQ(rejit_match(m, "honor", NULL), 5);
    LIBCUT_TEST_EQ(rejit_match(m, "honour", NULL), 6);
    LIBCUT_TEST_EQ(rejit_match(m, "honorable", NULL), 9);
    LIBCUT_TEST_EQ(rejit_match(m, "honourable", NULL), 10);

    m = rejit_parse_compile("a(b)?c", &err);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_NONE);
    LIBCUT_TEST_EQ(m->groups, 1);
    LIBCUT_TEST_EQ(rejit_match(m, "abc", &group), 3);
    LIBCUT_TEST_STREQ(group.begin, "bc");
    LIBCUT_TEST_STREQ(group.end, "c");
    group.begin = group.end = NULL;
    LIBCUT_TEST_EQ(rejit_match(m, "ac", &group), 2);
    LIBCUT_TEST_EQ(group.begin, NULL);
    LIBCUT_TEST_EQ(group.end, NULL);
}

LIBCUT_MAIN(
    test_tokenize,

    test_parse_word, test_parse_suffix, test_parse_group, test_parse_set,
    test_parse_pipe, test_parse_other,

    test_chr, test_dot, test_plus, test_star, test_opt, test_begin, test_end,
    test_set, test_or, test_group, test_cgroup, test_opt_group, test_star_group,
    test_plus_group, test_lookahead, test_negative_lookahead, test_mplus,
    test_mstar, test_or_mixed, test_set_and_dot, test_or_group,

    test_search,

    test_misc)
