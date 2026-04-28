#include "page_parser.h" 


struct test_case {
    u8 *filepath;
    u8 *name;
    b32 (*test)(u8*);
};
#define make_test(x) {"PageParser\\tests\\" #x ".md", #x, test_##x}


internal b32 test_empty_title(u8 *filepath) 
{
    return 0;
}


internal b32 test_equation(u8 *filepath) 
{
    return 0;
}


internal b32 test_empty(u8 *filepath)
{
    return 0;
}


internal b32 test_figure(u8 *filepath)
{
    return 0;
}


internal b32 test_link(u8 *filepath)
{
    return 0;
}


internal b32 test_list(u8 *filepath) 
{
    return 0;
}


internal b32 test_paragraph(u8 *filepath) 
{
    return 0;
}


internal b32 test_title(u8 *filepath) 
{
    return 0;
}


internal b32 test_unordered_list(u8 *filepath) 
{
    return 0;
}


struct test_case tests[] = {
    make_test(empty_title),
    make_test(equation),
    make_test(empty),
    make_test(figure),
    make_test(link),
    make_test(list),
    make_test(paragraph),
    make_test(title),
    make_test(unordered_list),
};


int main(int argc, char **argv) 
{
    puts("Start test suite");
    puts("========================================");
    for (int i = 0; i < ARRAY_LEN(tests); i++) {
        b32 test_result = tests[i].test(tests[i].filepath);
        if (test_result) {
            printf("\t%ld: [PASSED] %s\n", i + 1, tests[i].name);
        } else { 
            printf("\t%ld: [FAILED] %s\n", i + 1, tests[i].name);
        }
    }
    puts("========================================");
    puts("End test suite");
    return 0;
}