#include "page_parser.h"


int main(int argc, char **argv) 
{
#define TEST_FILE "PageParser/tests/unordered_list.md"
<<<<<<< HEAD
    struct markdown_tree doc = parse_markdown(TEST_FILE);
//    emit_tex();
//    emit_html();
=======
    struct strbuffer contents = read_entire_file(TEST_FILE);
    if (contents.size == 0) {
        fprintf(stderr, "[ERROR] Input file is empty. Nothing to do.");
        exit(0);
    }

    global_arena = arena_init(KILO(5));

    struct markdown_parser parser = {
        .contents  = contents.buffer, 
        .has_error = 0,
        .length    = contents.size, 
        .cursor    = 0
    };
    
    struct markdown_tree doc = alloc_markdown_tree();
    do {
        struct markdown_token t = parse_token(&parser);
        parser.has_error += !add_node(&doc, t); 
    } while (is_parsing(parser));
>>>>>>> 67f3a2d (Adding some tests but I still need to create code that compares two tress.)

    free_arena(&global_arena);
    return 0;
}