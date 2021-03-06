/*
 * rscraper Copyright (C) 2019 Adam Gray
 * This program is licensed with GPLv3.0 and comes with absolutely no warranty.
 * This code may be copied, modified, distributed etc. with accordance to the GPLv3.0 license (a copy of which is in the root project directory) under the following conditions:
 *     This copyright notice must be included at the beginning of any copied/modified file originating from this project, or at the beginning of any section of code that originates from this project.
 */

#include "filter_comment_body.hpp"

#include "filter_comment_body_regexp.hpp" // for regexpr_str, SUBREDDIT_BLACKLISTS


namespace filter_comment_body {

#ifndef USE_BOOST_REGEX
void init(){
}

unsigned int match(struct cmnt_meta metadata, const char* str, const int str_len){
	return 0;
}
#else
boost::match_results<const char*> what;


template<typename A,  typename B>
bool contains(A& ls,  B x){
	return (std::end(ls) != std::find(std::begin(ls), std::end(ls), x));
};


unsigned int match(struct cmnt_meta metadata, const char* str, const int str_len){
	// NOTE: metadata is not passed by const reference as sizeof(metadata) ~= 2*sizeof(void*)
	if (!boost::regex_search(str,  str + str_len,  what,  *regexpr))
		return 0;
	
	for (size_t i = 1;  i < what.size();  ++i){
		// Ignore first index - it is the entire match, not a regex group.
		if (!what[i].matched)
			continue;
		const unsigned int reason_id = groupindx2reason[i];
		if (SUBREDDIT_WHITELISTS[reason_id].size() != 0  &&  !contains(SUBREDDIT_WHITELISTS[reason_id], metadata.subreddit_id))
			continue;
		if (SUBREDDIT_BLACKLISTS[reason_id].size() != 0  &&  contains(SUBREDDIT_BLACKLISTS[reason_id], metadata.subreddit_id))
			continue; // Not return - might be later matches that are not blacklisted
		return reason_id;
	}
	
	return 0;
}
#endif


} // end namespace
