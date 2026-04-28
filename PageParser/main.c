#include "page_parser.h"


int main(int argc, char **argv) 
{
#define TEST_FILE "PageParser/tests/unordered_list.md"
    struct markdown_tree doc = parse_markdown(TEST_FILE);
//    emit_tex();
//    emit_html();

    free_arena(&global_arena);
    return 0;
}