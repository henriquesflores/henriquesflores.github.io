#include "page_parser.h" 


struct test_case {
    u8 *filepath;
    u8 *name;
    b32 (*test)(u8*);
};
#define make_test(x) {"PageParser\\tests\\" #x ".md", #x, test_##x}


internal b32 compare_nodes(
    struct markdown_node *n1,
    struct markdown_node *n2
) {
    b32 result = 0;
    if (n1 && n2) {
        result += compare_tokens(n1, n2);
        struct markdown_node *sibling1 = n1->next_sibling;
        struct markdown_node *sibling2 = n2->next_sibling;
        while (sibling1 && sibling2) {
            result += compare_nodes(sibling1, sibling2);
            sibling1 = sibling1->next_sibling;
            sibling2 = sibling2->next_sibling;
        } 
    }
    return result == 0;
}


internal b32 compare_trees(
    struct markdown_tree t1,
    struct markdown_tree t2
) {
    struct markdown_node *n1 = t1.head;
    struct markdown_node *n2 = t2.head;
    compare_nodes(n1, n2);
    return 0
}


internal b32 test_empty_title(u8 *filepath) 
{
    b32 result = 0;
    struct markdown_tree doc = parse_markdown(filepath);

    struct markdown_tree *e = 0;
    result += !addc_node(e, 
        (struct markdown_token) {
            .new_line_space=0,
            .kind=TOKEN_TITLE,
            .item=(struct strbuffer){0, ""}
    });
    return result && compare_trees(doc, *e);
}


internal b32 test_equation(u8 *filepath) 
{
    struct markdown_tree doc = parse_markdown(filepath);
    return 0;
}


internal b32 test_empty(u8 *filepath)
{
    struct markdown_tree doc = parse_markdown(filepath);
    return 0;
}


internal b32 test_figure(u8 *filepath)
{
    struct markdown_tree doc = parse_markdown(filepath);
    return 0;
}


internal b32 test_link(u8 *filepath)
{
    struct markdown_tree doc = parse_markdown(filepath);
    return 0;
}


internal b32 test_list(u8 *filepath) 
{
    struct markdown_tree doc = parse_markdown(filepath);
    return 0;
}


internal b32 test_paragraph(u8 *filepath) 
{
    struct markdown_tree doc = parse_markdown(filepath);
    return 0;
}


internal b32 test_title(u8 *filepath) 
{
    struct markdown_tree doc = parse_markdown(filepath);
    return 0;
}


internal b32 test_unordered_list(u8 *filepath) 
{
    struct markdown_tree doc = parse_markdown(filepath);
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
        if (tests[i].test(tests[i].filepath)) {
            printf("\t%ld: [PASSED] %s\n", i + 1, tests[i].name);
        } else { 
            printf("\t%ld: [FAILED] %s\n", i + 1, tests[i].name);
        }
    }
    puts("========================================");
    puts("End test suite");
    return 0;
}