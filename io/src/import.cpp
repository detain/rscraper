/*
 * rscraper Copyright (C) 2019 Adam Gray
 * This program is licensed with GPLv3.0 and comes with absolutely no warranty.
 * This code may be copied, modified, distributed etc. with accordance to the GPLv3.0 license (a copy of which is in the root project directory) under the following conditions:
 *     This copyright notice must be included at the beginning of any copied/modified file originating from this project, or at the beginning of any section of code that originates from this project.
 */


#include <stdio.h> // for fwrite
#include <stdlib.h> // for malloc

#include <compsky/mysql/query.hpp>


MYSQL_RES* RES;
MYSQL_ROW ROW;

bool force = false;

char str_a[1024];
char str_b[1024];
char uint64_a[20]; // 19 digits + 1 null byte
char uint64_b[20];
char uint64_c[20];


constexpr size_t strlen_constexpr(const char* s){
    // GCC strlen is constexpr; this is apparently a bug
    return  *s  ?  1 + strlen_constexpr(s + 1)  :  0;
}


namespace compsky {
    namespace asciify {
        constexpr static const size_t BUF_SZ = 4 * 1024 * 1024;
        char* BUF = (char*)malloc(4 * 1024 * 1024); // 4 MiB
            
        void ensure_buf_can_fit(const char* pre,  size_t n,  size_t trim_trailing_bytes = 1){
            if (unlikely(BUF_INDX + n  >  BUF_SZ)){
                compsky::mysql::exec_buffer(BUF,  BUF_INDX - trim_trailing_bytes);
                // Usually a trailing comma
                BUF_INDX = 0;
                asciify(pre);
            }
        }
        
        void ensure_buf_can_fit(const char* pre,  size_t n,  const char* post,  size_t post_len){
            if (unlikely(BUF_INDX + n + strlen(post)  >  BUF_SZ)){
                --BUF_INDX; // Overwrite trailing comma
                memcpy(BUF + BUF_INDX,  post,  post_len);
                BUF_INDX += post_len;
                compsky::mysql::exec_buffer(BUF,  BUF_INDX);
                BUF_INDX = 0;
                asciify(pre);
            }
        }
    }
}

namespace _f {
    constexpr static const compsky::asciify::flag::Escape esc;
}

bool contains(const char** ls,  const int n,  const char* s){
    const size_t slen = strlen_constexpr(s);
    for (auto i = 0;  i < n;  ++i)
        if (strncmp(ls[i], s, slen) == 0)
            return true;
    return false;
}

void import_user_table(FILE* f){
    compsky::asciify::BUF_INDX = 0;
    constexpr static const char* pre = "INSERT IGNORE INTO user (id,name) VALUES ";
    compsky::asciify::asciify(pre);
    while(fscanf(f, "%[^\t\n]\t%[^\n]\n", uint64_a, str_a) != EOF){
        compsky::asciify::asciify('(', uint64_a, ',', '"', _f::esc, '"', str_a, '"', ')', ',');
        compsky::asciify::ensure_buf_can_fit(pre,  1 + 19 + 1 + 1 + 1 + 2*128 + 1 + 1 + 1);
    }
    if (compsky::asciify::BUF_INDX != strlen_constexpr(pre))
        compsky::mysql::exec_buffer(compsky::asciify::BUF,  compsky::asciify::BUF_INDX - 1); // Overwrite trailing comma
    fclose(f);
}

void import_subreddit_table(FILE* f){
    compsky::asciify::BUF_INDX = 0;
    constexpr static const char* pre = "INSERT IGNORE INTO subreddit (id,name) VALUES ";
    compsky::asciify::asciify(pre);
    while(fscanf(f, "%[^\t\n]\t%[^\n]\n", uint64_a, str_a) != EOF){
        compsky::asciify::asciify('(', uint64_a, ',', '"', _f::esc, '"', str_a, '"', ')', ',');
        compsky::asciify::ensure_buf_can_fit(pre,  1 + 19 + 1 + 1 + 1 + 2*128 + 1 + 1 + 1);
    }
    if (compsky::asciify::BUF_INDX != strlen_constexpr(pre))
        compsky::mysql::exec_buffer(compsky::asciify::BUF,  compsky::asciify::BUF_INDX - 1); // Overwrite trailing comma
    fclose(f);
}

void import_tag_table(FILE* f){
    compsky::asciify::BUF_INDX = 0;
    constexpr static const char* pre = "INSERT IGNORE INTO tag (name, r, g, b, a) VALUES ";
    compsky::asciify::asciify(pre);
    while(fscanf(f, "%[^\t\n]\t%[^\n]\n", str_a, str_b) != EOF){
        // `str_b` itself is of the format %lf,%lf,%lf,%lf
        compsky::asciify::asciify("(\"", _f::esc, '"', str_a, "\",", str_b, "),");
        compsky::asciify::ensure_buf_can_fit(pre,  2 + 2*128 + 2 + 4*7 + 2);
    }
    if (compsky::asciify::BUF_INDX != strlen_constexpr(pre))
        compsky::mysql::exec_buffer(compsky::asciify::BUF,  compsky::asciify::BUF_INDX - 1); // Overwrite trailing comma
    fclose(f);
}

void import_category_table(FILE* f){
    compsky::asciify::BUF_INDX = 0;
    constexpr static const char* pre = "INSERT IGNORE INTO category (name) VALUES ";
    compsky::asciify::asciify(pre);
    while(fscanf(f, "%[^\n]\n", str_a) != EOF){
        compsky::asciify::asciify("(\"", _f::esc, '"', str_a, "\"),");
        compsky::asciify::ensure_buf_can_fit(pre,  2 + 2*128 + 3);
    }
    if (compsky::asciify::BUF_INDX != strlen_constexpr(pre))
        compsky::mysql::exec_buffer(compsky::asciify::BUF,  compsky::asciify::BUF_INDX - 1); // Overwrite trailing comma
    fclose(f);
}

void import_u2scc_table(FILE* f){
    compsky::asciify::BUF_INDX = 0;
    char* pre;
    char* post;
    size_t post_strlen;
    if (force){
        pre  = "INSERT INTO user2subreddit_cmnt_count (user_id,subreddit_id,count) VALUES ";
        post = " ON DUPLICATE KEY SET count=VALUES(count)";
        post_strlen = strlen_constexpr("INSERT INTO user2subreddit_cmnt_count (user_id,subreddit_id,count) VALUES ");
    } else {
        pre = "INSERT IGNORE INTO user2subreddit_cmnt_count (user_id,subreddit_id,count) VALUES ";
        post = " ";
        post_strlen = 0;
    }
    compsky::asciify::asciify(pre);
    while(fscanf(f, "%[^\t\n]\t%[^\t\n]\t%[^\n]\n", uint64_a, uint64_b, uint64_c) != EOF){
        compsky::asciify::asciify("(", uint64_a, ',', uint64_b, ',', uint64_c, "),");
        compsky::asciify::ensure_buf_can_fit(pre,  2 + 2*128 + 3,  post, post_strlen);
    }
    if (compsky::asciify::BUF_INDX != strlen(pre))
        compsky::mysql::exec_buffer(compsky::asciify::BUF,  compsky::asciify::BUF_INDX - 1); // Overwrite trailing comma
    fclose(f);
}

void import_subreddit2tag_table(FILE* f){
    compsky::asciify::BUF_INDX = 0;
    constexpr static const char* pre = "INSERT IGNORE INTO subreddit2tag (subreddit_id,tag_id) SELECT s.id,t.id FROM subreddit s, tag t WHERE ";
    compsky::asciify::asciify(pre);
    while(fscanf(f, "%[^\t\n]\t%[^\n]\n", str_a, str_b) != EOF){
        compsky::asciify::asciify("(s.name=\"", _f::esc, '"', str_a, "\" AND t.name=\"", _f::esc, '"', str_b, "\") OR ");
        compsky::asciify::ensure_buf_can_fit(pre,  9 + 2*128 + 14 + 2*128 + 6,  4);
        // 6 being strlen_constexpr(" OR ")
    }
    if (compsky::asciify::BUF_INDX != strlen_constexpr(pre))
        compsky::mysql::exec_buffer(compsky::asciify::BUF,  compsky::asciify::BUF_INDX - 4);
    fclose(f);
}

void import_tag2category_table(FILE* f){
    while(fscanf(f, "%[^\t\n]\t%[^\n]\n", str_a, str_b) != EOF)
        compsky::mysql::exec("INSERT IGNORE INTO tag2category (tag_id,category_id) SELECT t.id,c.id FROM tag t, category c WHERE t.name=\"", _f::esc, '"', str_a, "\" AND c.name=\"", _f::esc, '"', str_b, "\"");
    fclose(f);
}

void import_subredditsbasic_table(FILE* f){
    compsky::asciify::BUF_INDX = 0;
    constexpr static const char* pre = "INSERT IGNORE INTO subreddit2meta (id,subscribers,created_at) VALUES ";
    compsky::asciify::asciify(pre);
    while(fscanf(f, "%[^,],%[^,],%[^,],%[^,],%[^,\n]\n", uint64_a, uint64_b, uint64_c, str_a, str_b) != EOF){
        // Note that str_b stores 'subscribers' column, so only needs 20 char
        if (str_b[0] == 'N')
            // == None
            continue;
        compsky::asciify::asciify("(", uint64_a, ',', str_b, ',', uint64_c, "),");
        compsky::asciify::ensure_buf_can_fit(pre,  1 + 19 + 1 + 19 + 1 + 19 + 2);
    }
    if (compsky::asciify::BUF_INDX != strlen_constexpr(pre))
        compsky::mysql::exec_buffer(compsky::asciify::BUF,  compsky::asciify::BUF_INDX - 1); // Overwrite trailing comma
    fclose(f);
}

int main(int argc,  const char** argv){
    compsky::mysql::init(getenv("RSCRAPER_MYSQL_CFG"));  // Init SQL
    
    FILE* f;
    
    ++argv;
    --argc;
    
    
    // Arg Parser
    while(argc != 0){
        const char* s = *argv;
        if (s[2] != 0  ||  s[0] != '-')
            break;
        switch(s[1]){
            case 'f':
                force = true;
                break;
            default:
                fprintf(stderr, "Unrecognised option: %s\n", s);
                exit(1);
        }
        ++argv;
        --argc;
    }
    
    
    /* No dependencies on other tables */
    
    if (argc == 0  ||  contains(argv, argc, "user.csv"))
        if ((f = fopen("user.csv", "rb")))
            import_user_table(f);
    
    if (argc == 0  ||  contains(argv, argc, "subreddit.csv"))
        if ((f = fopen("subreddit.csv", "rb")))
            import_subreddit_table(f);
    
    if (argc == 0  ||  contains(argv, argc, "tag.csv"))
        if ((f = fopen("tag.csv", "rb")))
            import_tag_table(f);
    
    if (argc == 0  ||  contains(argv, argc, "category.csv"))
        if ((f = fopen("category.csv", "rb")))
            import_category_table(f);
    
    /* No dependencies on previous import data (i.e. just using absolute IDs) */
    
    if (argc == 0  ||  contains(argv, argc, "user2subreddit_cmnt_count.csv"))
        if ((f = fopen("user2subreddit_cmnt_count.csv", "rb")))
            import_u2scc_table(f);
    
    /* Name-to-name tables */
    
    if (argc == 0  ||  contains(argv, argc, "subreddit2tag.csv"))
        if ((f = fopen("subreddit2tag.csv", "rb")))
            import_subreddit2tag_table(f);
    
    if (argc == 0  ||  contains(argv, argc, "tag2category.csv"))
        if ((f = fopen("tag2category.csv", "rb")))
            import_tag2category_table(f);
    
    /* 3rd party tables */
    
    // Specifically for importing https://files.pushshift.io/reddit/subreddits/subreddits_basic.csv
    if (argc == 0  ||  contains(argv, argc, "subreddits_basic.csv"))
        if ((f = fopen("subreddits_basic.csv", "rb")))
            import_subredditsbasic_table(f);
    
    compsky::mysql::exit_mysql();
}
