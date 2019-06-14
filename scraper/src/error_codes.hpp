#ifndef __MYERR__
#define __MYERR__

namespace myerr {
enum {
    NONE,
    UNKNOWN,
    CANNOT_INIT_CURL,
    CANNOT_WRITE_RES,
    CURL_PERFORM,
    CANNOT_SET_PROXY,
    INVALID_PJ,
    JSON_PARSING,
    BAD_ARGUMENT,
    UNACCOUNTED_FOR_SERVER_CODE,
    SUBREDDIT_NOT_IN_DB,
    IMPOSSIBLE // Should never happen
};
}
#endif