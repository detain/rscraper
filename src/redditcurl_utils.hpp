#ifndef __MYRCU__
#define __MYRCU__
#include <b64/encode.h> // for base64::encode
#include <stdlib.h> // for free, malloc, realloc

#include "rapidjson/document.h" // for rapidjson::Document
#include "rapidjson/pointer.h" // for rapidjson::GetValueByPointer
// NOTE: These are to prefer local headers, as rapidjson is a header-only library. This allows easy use of any version of rapidjson, as those provided by repositories might be dated.

#include "error_codes.hpp" // for myerr:*
#include "curl_utils.hpp" // for mycu::*

namespace myrcu {


#ifdef DEBUG
    #include <stdio.h> // for fprintf
    #include <execinfo.h> // for printing stack trace
#endif


constexpr int REDDIT_REQUEST_DELAY = 1;

const char* USER_AGENT;
constexpr const char* PARAMS = "?limit=2048&sort=new&raw_json=1";
constexpr const int PARAMS_LEN = strlen(PARAMS);

constexpr const char* AUTH_HEADER_PREFIX = "Authorization: bearer ";
constexpr const char* TOKEN_FMT = "XXXXXXXX-XXXXXXXXXXXXXXXXXXXXXXXXXXX";
char AUTH_HEADER[strlen("Authorization: bearer ") + strlen("XXXXXXXX-XXXXXXXXXXXXXXXXXXXXXXXXXXX") + 1] = "Authorization: bearer ";

constexpr const char* API_SUBMISSION_URL_PREFIX = "https://oauth.reddit.com/comments/";
constexpr const char* API_DUPLICATES_URL_PREFIX = "https://oauth.reddit.com/duplicates/";
constexpr const char* API_SUBREDDIT_URL_PREFIX = "https://oauth.reddit.com/r/";
constexpr const char* SUBMISSION_URL_PREFIX = "https://XXX.reddit.com/r/";
constexpr const char* API_ALLCOMMENTS_URL = "https://oauth.reddit.com/r/all/comments/?limit=100&raw_json=1";

const char* USR;
const char* PWD;
const char* KEY_AND_SECRET;


constexpr const char* BASIC_AUTH_PREFIX = "Authorization: Basic ";
constexpr const char* BASIC_AUTH_FMT = "base-64-encoded-client_key:client_secret----------------";
char BASIC_AUTH_HEADER[strlen("Authorization: Basic ") + strlen("base-64-encoded-client_key:client_secret----------------") + 1] = "Authorization: Basic ";


CURL* LOGIN_CURL;
struct curl_slist* LOGIN_HEADERS;
constexpr const char* LOGIN_POSTDATA_PREFIX = "grant_type=password&password=";
constexpr const char* LOGIN_POSTDATA_KEYNAME = "&username=";
char* LOGIN_POSTDATA;



void handler(int n){
    void* arr[10];

#ifdef DEBUG
    size_t size = backtrace(arr, 10);
    
    fprintf(stderr, "%s\n", mycu::MEMORY.memory);
    fprintf(stderr, "E(%d):\n", n);
    backtrace_symbols_fd(arr, size, STDERR_FILENO);
#endif
    
    free(mycu::MEMORY.memory);
    free(LOGIN_POSTDATA);
    curl_easy_cleanup(mycu::curl);
    curl_global_cleanup();
    exit(n);
}


void init_login(){
    int i;
    
    
    base64::encoder base64_encoder;
    
    i = strlen(BASIC_AUTH_PREFIX);
    
    base64_encoder.encode(KEY_AND_SECRET,  strlen(KEY_AND_SECRET),  BASIC_AUTH_HEADER + i);
    i += strlen(BASIC_AUTH_FMT);
    
    BASIC_AUTH_HEADER[i] = 0;
    
    
    LOGIN_HEADERS = curl_slist_append(LOGIN_HEADERS, BASIC_AUTH_HEADER);
    
    
    LOGIN_CURL = curl_easy_init();
    if (!LOGIN_CURL)
        handler(myerr::CANNOT_INIT_CURL);
    
    curl_easy_setopt(LOGIN_CURL, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(LOGIN_CURL, CURLOPT_HTTPHEADER, LOGIN_HEADERS);
    curl_easy_setopt(LOGIN_CURL, CURLOPT_TIMEOUT, 20);
    
    i = 0;
    
    memcpy(LOGIN_POSTDATA + i,  LOGIN_POSTDATA_PREFIX,  strlen(LOGIN_POSTDATA_PREFIX));
    i += strlen(LOGIN_POSTDATA_PREFIX);
    
    memcpy(LOGIN_POSTDATA + i,  PWD,  strlen(PWD));
    i += strlen(PWD);
    
    memcpy(LOGIN_POSTDATA + i,  LOGIN_POSTDATA_KEYNAME,  strlen(LOGIN_POSTDATA_KEYNAME));
    i += strlen(LOGIN_POSTDATA_KEYNAME);
    
    memcpy(LOGIN_POSTDATA + i,  USR,  strlen(USR));
    i += strlen(USR);
    
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
    
    int i = strlen(AUTH_HEADER_PREFIX);
    
    memcpy(AUTH_HEADER + i,  mycu::MEMORY.memory + strlen("{\"access_token\": \""),  strlen(TOKEN_FMT));
    i += strlen(TOKEN_FMT);
    
    AUTH_HEADER[i] = 0;
    
    
    mycu::HEADERS = {};
    mycu::HEADERS = curl_slist_append(mycu::HEADERS, AUTH_HEADER);
    curl_easy_setopt(mycu::curl, CURLOPT_HTTPHEADER, mycu::HEADERS);
}


void init(const char* usr,  const char* pwd,  const char* key_n_secret,  const char* user_agent){
    mycu::MEMORY.memory = (char*)malloc(0);
    mycu::MEMORY.n_allocated = 0;
    
    
    USER_AGENT = user_agent;
    
    USR = usr;
    PWD = pwd;
    KEY_AND_SECRET = key_n_secret;
    
    LOGIN_POSTDATA = (char*)malloc(strlen(LOGIN_POSTDATA_PREFIX) + strlen(PWD) + strlen(LOGIN_POSTDATA_KEYNAME) + strlen(USR) + 1);
    
    
    curl_global_init(CURL_GLOBAL_ALL);
    mycu::curl = curl_easy_init();
    if (!mycu::curl)
        handler(myerr::CANNOT_INIT_CURL);
    
    curl_easy_setopt(mycu::curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(mycu::curl, CURLOPT_TIMEOUT, 20);
    
    curl_easy_setopt(mycu::curl, CURLOPT_CUSTOMREQUEST, "GET");
    
    curl_easy_setopt(mycu::curl, CURLOPT_WRITEFUNCTION, mycu::write_res_to_mem);
    curl_easy_setopt(mycu::curl, CURLOPT_WRITEDATA, (void *)&mycu::MEMORY);
    
    
    init_login();
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
                PRINTF("Unauthorised. Logging in again.\n");
                sleep(REDDIT_REQUEST_DELAY);
                login();
                break;
            default:
                handler(myerr::UNKNOWN);
        }
        return true;
    }
}


} // END namespace
#endif


/* Regex substitution to switch
((REDDIT_REQUEST_DELAY|USER_AGENT|curl|PARAMS|PARAMS_LEN|AUTH_HEADER_PREFIX|TOKEN_FMT|AUTH_HEADER|API_SUBMISSION_URL_PREFIX|API_DUPLICATES_URL_PREFIX|SUBMISSION_URL_PREFIX|API_ALLCOMMENTS_URL|USR|PWD|KEY_AND_SECRET|BASIC_AUTH_PREFIX|BASIC_AUTH_FMT|BASIC_AUTH_HEADER|LOGIN_CURL|LOGIN_HEADERS|LOGIN_POSTDATA_PREFIX|LOGIN_POSTDATA_KEYNAME|LOGIN_POSTDATA|handler)[^_a-zA-Z])
myrcu::\1
*/