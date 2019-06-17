/*
 * rscraper Copyright (C) 2019 Adam Gray
 * This program is licensed with GPLv3.0 and comes with absolutely no warranty.
 * This code may be copied, modified, distributed etc. with accordance to the GPLv3.0 license (a copy of which is in the root project directory) under the following conditions:
 *     This copyright notice must be included at the beginning of any copied/modified file originating from this project, or at the beginning of any section of code that originates from this project.
 */


#ifndef __MYRCU__
#define __MYRCU__

#ifdef _WIN32
# include <b64/cencode.h> // for base64_encode*
#else
# include <b64/encode.h> // for base64::*
#endif

#include <stdlib.h> // for free, malloc, realloc

#include "rapidjson/document.h" // for rapidjson::Document
#include "rapidjson/pointer.h" // for rapidjson::GetValueByPointer
// NOTE: These are to prefer local headers, as rapidjson is a header-only library. This allows easy use of any version of rapidjson, as those provided by repositories might be dated.

#include <compsky/asciify/asciify.hpp>

#include "error_codes.hpp" // for myerr:*
#include "curl_utils.hpp" // for mycu::*

namespace myrcu {


#ifdef DEBUG
# include <stdio.h> // for fprintf
# include <execinfo.h> // for printing stack trace
#else
# undef printf
# define printf(...)
#endif

constexpr size_t strlen_constexpr(const char* s){
    // GCC strlen is constexpr; this is apparently a bug
    return *s  ?  1 + strlen_constexpr(s + 1)  :  0;
}

constexpr int REDDIT_REQUEST_DELAY = 1;

constexpr const char* PARAMS = "?limit=2048&sort=new&raw_json=1";
constexpr const size_t PARAMS_LEN = strlen_constexpr(PARAMS);

constexpr const char* AUTH_HEADER_PREFIX = "Authorization: bearer ";
char AUTH_HEADER[strlen_constexpr(AUTH_HEADER_PREFIX) + 128 + 1] = "Authorization: bearer ";

constexpr const char* API_SUBMISSION_URL_PREFIX = "https://oauth.reddit.com/comments/";
constexpr const char* API_DUPLICATES_URL_PREFIX = "https://oauth.reddit.com/duplicates/";
constexpr const char* API_SUBREDDIT_URL_PREFIX = "https://oauth.reddit.com/r/";
constexpr const char* SUBMISSION_URL_PREFIX = "https://XXX.reddit.com/r/";
constexpr const char* API_ALLCOMMENTS_URL = "https://oauth.reddit.com/r/all/comments/?limit=100&raw_json=1";

constexpr const int BUF_SZ = 1024;
char BUF[BUF_SZ];
char* REDDIT_AUTH[6];
char* USR;
char* PWD;
char* KEY_AND_SECRET;
char* USER_AGENT;
char* PROXY_URL;


constexpr const char* BASIC_AUTH_PREFIX = "Authorization: Basic ";
constexpr const char* BASIC_AUTH_FMT = "base-64-encoded-client_key:client_secret----------------";
char BASIC_AUTH_HEADER[strlen_constexpr("Authorization: Basic ") + strlen_constexpr("base-64-encoded-client_key:client_secret----------------") + 1] = "Authorization: Basic ";


CURL* LOGIN_CURL;
struct curl_slist* LOGIN_HEADERS;
constexpr const char* LOGIN_POSTDATA_PREFIX = "grant_type=password&password=";
constexpr const char* LOGIN_POSTDATA_KEYNAME = "&username=";
char LOGIN_POSTDATA[strlen_constexpr(LOGIN_POSTDATA_PREFIX) + 128 + strlen_constexpr(LOGIN_POSTDATA_KEYNAME) + 128 + 1];



constexpr const char* URL__USER_MOD_OF__PRE  = "https://www.reddit.com/user/";
constexpr const char* URL__USER_MOD_OF__POST = "/moderated_subreddits.json";
char URL__USER_MOD_OF[strlen_constexpr(URL__USER_MOD_OF__PRE) + 128 + strlen_constexpr(URL__USER_MOD_OF__POST) + 1] = "https://www.reddit.com/user/";

CURL* BROWSER_CURL;



void handler(int n){
    void* arr[10];

#ifdef DEBUG
    size_t size = backtrace(arr, 10);
    
    fprintf(stderr, "%s\n", mycu::MEMORY.memory);
    fprintf(stderr, "E(%d):\n", n);
    backtrace_symbols_fd(arr, size, STDERR_FILENO);
#endif
    
    free(mycu::MEMORY.memory);
    curl_easy_cleanup(mycu::curl);
    curl_global_cleanup();
    exit(n);
}


void init_login(){
    int i;
    
    i = strlen_constexpr(BASIC_AUTH_PREFIX);
    
    {
  #ifdef _WIN32
    // Use C ABI rather than C++ to simplify Windows compatibility
    base64_encodestate state;
    base64_init_encodestate(&state);
    base64_encode_block(KEY_AND_SECRET,  strlen(KEY_AND_SECRET),  BASIC_AUTH_HEADER + i,  &state);
  #else
    // Using the C functions gives me linker errors on Ubuntu
    base64::encoder base64_encoder;
    base64_encoder.encode(KEY_AND_SECRET,  strlen(KEY_AND_SECRET),  BASIC_AUTH_HEADER + i);
  #endif
    }
    
    i += strlen_constexpr(BASIC_AUTH_FMT);
    
    BASIC_AUTH_HEADER[i] = 0;
    
    
    LOGIN_HEADERS = curl_slist_append(LOGIN_HEADERS, BASIC_AUTH_HEADER);
    
    
    LOGIN_CURL = curl_easy_init();
    if (!LOGIN_CURL)
        handler(myerr::CANNOT_INIT_CURL);
    
    curl_easy_setopt(LOGIN_CURL, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(LOGIN_CURL, CURLOPT_HTTPHEADER, LOGIN_HEADERS);
    curl_easy_setopt(LOGIN_CURL, CURLOPT_TIMEOUT, 20);
    
    i = 0;
    
    memcpy(LOGIN_POSTDATA + i,  LOGIN_POSTDATA_PREFIX,  strlen_constexpr(LOGIN_POSTDATA_PREFIX));
    i += strlen_constexpr(LOGIN_POSTDATA_PREFIX);
    
    memcpy(LOGIN_POSTDATA + i,  PWD,  strlen(PWD));
    i += strlen(PWD);
    
    memcpy(LOGIN_POSTDATA + i,  LOGIN_POSTDATA_KEYNAME,  strlen(LOGIN_POSTDATA_KEYNAME));
    i += strlen(LOGIN_POSTDATA_KEYNAME);
    
    memcpy(LOGIN_POSTDATA + i,  USR,  strlen(USR));
    i += strlen(USR);
    
    //constexpr static const compsky::asciify::flag::ChangeBuffer change_buffer;
    //compsky::asciify::asciify(change_buffer, LOGIN_POSTDATA, 0, LOGIN_POSTDATA_PREFIX, PWD, LOGIN_POSTDATA_KEYNAME, USR);
    
    LOGIN_POSTDATA[i] = 0;
    
    
    curl_easy_setopt(LOGIN_CURL, CURLOPT_POSTFIELDS, LOGIN_POSTDATA);
    
    curl_easy_setopt(LOGIN_CURL, CURLOPT_URL, "https://www.reddit.com/api/v1/access_token");
    
    curl_easy_setopt(LOGIN_CURL, CURLOPT_CUSTOMREQUEST, "POST");
    
    curl_easy_setopt(LOGIN_CURL, CURLOPT_WRITEFUNCTION, mycu::write_res_to_mem);
    curl_easy_setopt(LOGIN_CURL, CURLOPT_WRITEDATA, (void *)&mycu::MEMORY);
}


void login(){
    mycu::MEMORY.size = 0; // 'Clear' last request
    
    if (curl_easy_perform(LOGIN_CURL) != CURLE_OK)
        handler(myerr::CURL_PERFORM);
    
    // Result is in format
    // {"access_token": "XXXXXXXX-XXXXXXXXXXXXXXXXXXXXXXXXXXX", "token_type": "bearer", "expires_in": 3600, "scope": "*"}
    
    int i = strlen_constexpr(AUTH_HEADER_PREFIX);
    
    printf("MEMORY:         %s\n", mycu::MEMORY.memory);
    printf("LOGIN_POSTDATA: %s\n", LOGIN_POSTDATA);
    
    switch(mycu::MEMORY.size){
        case strlen_constexpr("{\"message\": \"Unauthorized\", \"error\": 401}"):
            handler(myerr::UNAUTHORISED);
        case strlen_constexpr("{\"error\": \"unsupported_grant_type\"}"):
            handler(myerr::UNSUPPORTED_GRANT_TYPE);
    }
    
    char* s = mycu::MEMORY.memory + strlen_constexpr("{\"access_token\": \"");
    while(*s != '"'){
        AUTH_HEADER[i++] = *s;
        ++s;
    }
    
    AUTH_HEADER[i] = 0;
    
    
    mycu::HEADERS = {};
    mycu::HEADERS = curl_slist_append(mycu::HEADERS, AUTH_HEADER);
    curl_easy_setopt(mycu::curl, CURLOPT_HTTPHEADER, mycu::HEADERS);
}


void init(const char* fp){
    FILE* f = fopen(fp, "r");
    fread(BUF, 1, BUF_SZ, f);
    
    /*
    reddit config file must have format:
    USERNAME: my_k00l_username
    PASSWORD: my_5up3r_53cur3_p455w0rd
    KEY_SCRT: something:somethingelse
    USERAGNT: my_program_name (by /u/my_k00l_username)
    PROXYURL: either the url of the proxy to use, or a single dash to indicate no proxy is used
    */
    
    int n_lines = 0;
    char* itr;
    REDDIT_AUTH[0] = BUF + 10;
    for (itr = REDDIT_AUTH[0];  n_lines < 5;  ++itr)
        if (*itr == '\n'){
            *itr = 0;
            itr += 11; // To skip "ABCD: "
            REDDIT_AUTH[++n_lines] = itr;
        }

    USR = REDDIT_AUTH[0];
    PWD = REDDIT_AUTH[1];
    KEY_AND_SECRET = REDDIT_AUTH[2];
    USER_AGENT = REDDIT_AUTH[3];
    PROXY_URL = REDDIT_AUTH[4];
    
    printf("%s \n%s \n%s \n%s \n%s \n",  REDDIT_AUTH[0],  REDDIT_AUTH[1],  REDDIT_AUTH[2],  REDDIT_AUTH[3],  REDDIT_AUTH[4]);
    
    mycu::MEMORY.memory = (char*)malloc(0);
    mycu::MEMORY.n_allocated = 0;
    
    
    curl_easy_setopt(mycu::curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(mycu::curl, CURLOPT_TIMEOUT, 20);
    
    curl_easy_setopt(mycu::curl, CURLOPT_CUSTOMREQUEST, "GET");
    
    curl_easy_setopt(mycu::curl, CURLOPT_WRITEFUNCTION, mycu::write_res_to_mem);
    curl_easy_setopt(mycu::curl, CURLOPT_WRITEDATA, (void *)&mycu::MEMORY);
    
    
    init_login();
    
    
    if (PROXY_URL[0] != '-'){
        // Greater than 1
        PROXY_URL[strlen(PROXY_URL)-1] = 0;
        curl_easy_setopt(LOGIN_CURL,   CURLOPT_PROXY, PROXY_URL);
        curl_easy_setopt(BROWSER_CURL, CURLOPT_PROXY, PROXY_URL);
        curl_easy_setopt(mycu::curl,   CURLOPT_PROXY, PROXY_URL);
    }
    
    
    login();
}

bool try_again(rapidjson::Document& d){
    if (d.Parse(mycu::MEMORY.memory).HasParseError())
        handler(myerr::JSON_PARSING);
    
    if (!d.HasMember("error"))
        return false;
    else {
        switch (d["error"].GetInt()){
            case 401:
                // Unauthorised
                PRINTF("Unauthorised. Logging in again with user:pass: %s:%s\n", USR, PWD);
                sleep(REDDIT_REQUEST_DELAY);
                login();
                break;
            default:
                handler(myerr::UNACCOUNTED_FOR_SERVER_CODE);
        }
        return true;
    }
}

void init_browser_curl(){
    BROWSER_CURL = curl_easy_init();
    if (!BROWSER_CURL)
        handler(myerr::CANNOT_INIT_CURL);
    curl_easy_setopt(BROWSER_CURL, CURLOPT_USERAGENT, USER_AGENT);
}

void get_user_moderated_subs(const char* username){
    auto i = strlen_constexpr(URL__USER_MOD_OF__PRE);
    memcpy(URL__USER_MOD_OF + i,  username,  strlen(username));
    i += strlen(username);
    memcpy(URL__USER_MOD_OF + i,  URL__USER_MOD_OF__POST,  strlen_constexpr(URL__USER_MOD_OF__POST));
    i += strlen_constexpr(URL__USER_MOD_OF__POST);
    URL__USER_MOD_OF[i] = 0;
    
    curl_easy_setopt(BROWSER_CURL, CURLOPT_WRITEFUNCTION, mycu::write_res_to_mem);
    curl_easy_setopt(BROWSER_CURL, CURLOPT_WRITEDATA, (void *)&mycu::MEMORY);
    
    //mycu::request(URL__USER_MOD_OF);
    PRINTF("GET %s\n", URL__USER_MOD_OF);
    curl_easy_setopt(BROWSER_CURL, CURLOPT_URL, URL__USER_MOD_OF);
    
    mycu::MEMORY.size = 0; // 'Clear' last request
    
    if (curl_easy_perform(BROWSER_CURL) != CURLE_OK)
        handler(myerr::CURL_PERFORM);
}

} // END namespace
#endif


/* Regex substitution to switch
((REDDIT_REQUEST_DELAY|USER_AGENT|PARAMS|PARAMS_LEN|AUTH_HEADER_PREFIX|TOKEN_FMT|AUTH_HEADER|API_SUBMISSION_URL_PREFIX|API_DUPLICATES_URL_PREFIX|SUBMISSION_URL_PREFIX|API_ALLCOMMENTS_URL|USR|PWD|KEY_AND_SECRET|BASIC_AUTH_PREFIX|BASIC_AUTH_FMT|BASIC_AUTH_HEADER|LOGIN_CURL|LOGIN_HEADERS|LOGIN_POSTDATA_PREFIX|LOGIN_POSTDATA_KEYNAME|LOGIN_POSTDATA|handler)[^_a-zA-Z])
myrcu::\1
*/



/* To convert id to name
curl 'https://oauth.reddit.com/api/info?id=tM_abcdef,tN_ghij' -H 'Authorization: bearer ...'
*/
