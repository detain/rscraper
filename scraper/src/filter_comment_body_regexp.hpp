/*
 * rscraper Copyright (C) 2019 Adam Gray
 * This program is licensed with GPLv3.0 and comes with absolutely no warranty.
 * This code may be copied, modified, distributed etc. with accordance to the GPLv3.0 license (a copy of which is in the root project directory) under the following conditions:
 *     This copyright notice must be included at the beginning of any copied/modified file originating from this project, or at the beginning of any section of code that originates from this project.
 */

#ifndef __FILTER_COMMENT_BODY_REGEXP_INIT_H__
#define __FILTER_COMMENT_BODY_REGEXP_INIT_H__

#include <inttypes.h>
#include <vector>


namespace filter_comment_body {

extern char* regexpr_str;

extern std::vector<std::vector<uint64_t>> SUBREDDIT_BLACKLISTS;

void init();

}


#endif