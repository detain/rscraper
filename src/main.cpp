#include <b64/encode.h> // for 
#include <curl/curl.h>
#include "rapidjson/document.h" // for rapidjson::Document
#include "rapidjson/pointer.h" // for rapidjson::GetValueByPointer
#include <stdio.h> // for printf
#include <stdlib.h> // for free, malloc, realloc
#include <string.h> // for memcpy
#include <time.h> // for asctime
#include <unistd.h> // for sleep

#include <execinfo.h> // for printing stack trace

#include "rapidjson_utils.h" // for SET_DBG_* macros
#include "utils.h" // for PRINTF macro

#include "filter_comment_body.c" // for filter_comment_body::*

enum {
    SUCCESS,
    ERR,
    ERR_CANNOT_INIT_CURL,
    ERR_CANNOT_WRITE_RES,
    ERR_CURL_PERFORM,
    ERR_CANNOT_SET_PROXY,
    ERR_INVALID_PJ
};

#define REDDIT_REQUEST_DELAY 1


const char* USER_AGENT = "rscraper++:0.0.1-dev0 (by /u/Compsky)";
CURL* curl;
const char* PARAMS = "?limit=2048&sort=new&raw_json=1";
const int PARAMS_LEN = strlen(PARAMS);
const char* AUTH_HEADER_PREFIX = "Authorization: bearer ";
const char* TOKEN_FMT = "XXXXXXXX-XXXXXXXXXXXXXXXXXXXXXXXXXXX";
char* AUTH_HEADER;
const char* API_SUBMISSION_URL_PREFIX = "https://oauth.reddit.com/comments/";
const char* API_DUPLICATES_URL_PREFIX = "https://oauth.reddit.com/duplicates/";
const char* API_SUBREDDIT_URL_PREFIX = "https://oauth.reddit.com/r/";
const char* SUBMISSION_URL_PREFIX = "https://XXX.reddit.com/r/";
const char* API_ALLCOMMENTS_URL = "https://oauth.reddit.com/r/all/comments/?limit=100&raw_json=1";

const char* USR;
const char* PWD;
const char* KEY_AND_SECRET;


struct curl_slist* HEADERS;

struct MemoryStruct {
    char* memory;
    size_t size;
};

struct MemoryStruct MEMORY;

void handler(int n){
    void* arr[10];
    
    size_t size = backtrace(arr, 10);
    
    fprintf(stderr, "%s\n", MEMORY.memory);
    fprintf(stderr, "E(%d):\n", n);
    backtrace_symbols_fd(arr, size, STDERR_FILENO);
    
    
    free(MEMORY.memory);
    free(AUTH_HEADER);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    exit(n);
}

void megasleep(){
    sleep(5);
}

size_t write_res_to_mem(void* content, size_t size, size_t n, void* buf){
    size_t total_size = size * n;
    struct MemoryStruct* mem = (struct MemoryStruct*)buf;
    
    mem->memory = (char*)realloc(mem->memory,  mem->size + total_size + 1);
    // Larger requests are not written in just one call
    if (!mem->memory)
        handler(ERR_CANNOT_WRITE_RES);
    
    memcpy(mem->memory + mem->size,  content,  total_size);
    mem->size += total_size;
    mem->memory[mem->size] = 0;
    
    return total_size;
}


int request(const char* reqtype, const char* url){
    PRINTF("request(\"%s\", \"%s\")\n", reqtype, url);
    // Writes response contents to MEMORY
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, reqtype);
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_res_to_mem);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&MEMORY);
    
    
    if (curl_easy_perform(curl) != CURLE_OK)
        handler(ERR_CURL_PERFORM);
}

const char* BASIC_AUTH_PREFIX = "Authorization: Basic ";
const char* BASIC_AUTH_FMT = "base-64-encoded-client_key:client_secret----------------";

void login(){
    int i;
    
    // TODO: Necessary to copy cookies to global curl object?
    
    
    base64::encoder base64_encoder;
    
    AUTH_HEADER = (char*)realloc(AUTH_HEADER,  strlen(BASIC_AUTH_PREFIX) + strlen(BASIC_AUTH_FMT) + 1);
    
    i = 0;
    
    memcpy(AUTH_HEADER + i,  BASIC_AUTH_PREFIX,  strlen(BASIC_AUTH_PREFIX));
    i += strlen(BASIC_AUTH_PREFIX);
    
    base64_encoder.encode(KEY_AND_SECRET,  strlen(KEY_AND_SECRET),  AUTH_HEADER + i);
    i += strlen(BASIC_AUTH_FMT);
    
    AUTH_HEADER[i] = 0;
    
    struct curl_slist* headers;
    headers = curl_slist_append(headers, AUTH_HEADER);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    
    const char* a = "grant_type=password&password=";
    const char* b = "&username=";
    
    char postdata[strlen(a) + strlen(PWD) + strlen(b) + strlen(USR) + 1];
    
    i = 0;
    
    memcpy(postdata + i,  a,  strlen(a));
    i += strlen(a);
    
    memcpy(postdata + i,  PWD,  strlen(PWD));
    i += strlen(PWD);
    
    memcpy(postdata + i,  b,  strlen(b));
    i += strlen(b);
    
    memcpy(postdata + i,  USR,  strlen(USR));
    i += strlen(USR);
    
    postdata[i] = 0;
    
    
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata);
    
    curl_easy_setopt(curl, CURLOPT_URL, "https://www.reddit.com/api/v1/access_token");
    
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_res_to_mem);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&MEMORY);
    
    auto rc = curl_easy_perform(curl);
    
    if (rc != CURLE_OK)
        handler(ERR_CURL_PERFORM);
    
    
    // Result is in format
    // {"access_token": "XXXXXXXX-XXXXXXXXXXXXXXXXXXXXXXXXXXX", "token_type": "bearer", "expires_in": 3600, "scope": "*"}
    
    AUTH_HEADER = (char*)realloc(AUTH_HEADER,  strlen(AUTH_HEADER_PREFIX) + strlen(TOKEN_FMT) + 1);
    
    i = 0;
    memcpy(AUTH_HEADER + i,  AUTH_HEADER_PREFIX,  strlen(AUTH_HEADER_PREFIX));
    i += strlen(AUTH_HEADER_PREFIX);
    
    memcpy(AUTH_HEADER + i,  MEMORY.memory + strlen("{\"access_token\": \""),  strlen(TOKEN_FMT));
    i += strlen(TOKEN_FMT);
    
    AUTH_HEADER[i] = 0;
    
    
    HEADERS = curl_slist_append(HEADERS, AUTH_HEADER);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, HEADERS);
    
    MEMORY.size = 0; // No longer need contents of request
}

int slashindx(const char* str){
    int i = 0;
    while (str[i] != '/')
        ++i;
    return i;
}


int id2n(const char* str){
    int n = 0;
    while (*str != 0){
        n *= 10 + 26 + 26;
        if (*str >= '0'  &&  *str <= '9')
            n += *str - '0';
        else if (*str >= 'a'  &&  *str <= 'z')
            n += *str - 'a';
        else
            n += *str - 'A';
        ++str;
    }
    return n;
}

void process_live_cmnt(rapidjson::Value& cmnt, const int cmnt_id){
    SET_DBG_STR(body,           cmnt["data"]["body"])
    SET_DBG_STR(subreddit,      cmnt["data"]["subreddit"])
    SET_DBG_STR(author,         cmnt["data"]["author"])
    
    
    
    if (filter_comment_body::wl::match(body, strlen(body))){
        printf("MATCHED: %s\n", filter_comment_body::wl::what[0].str().c_str());
        goto goto__do_process_this_live_cmnt;
    }
    // if filter_comment_body::bl: return;
    
    
    
    return;
    
    goto__do_process_this_live_cmnt:
    
    SET_DBG_STR(author_id,      cmnt["data"]["author_fullname"])
    author_id += 3; // Skip "t2_" prefix
    // TODO: Maybe use author_id to filter users, instead of raw usernames.
    
    SET_DBG_STR(parent_id,      cmnt["data"]["parent_id"])
    parent_id += 3; // Skip "t3_" or "t1_" prefix - we can use 'depth' attribute to see if it is a root comment or not
    
    SET_DBG_STR(permalink,      cmnt["data"]["permalink"])
    
    const time_t RSET_DBG_FLT(created_at,     cmnt["data"]["created_utc"])
    
    
    printf("/r/%s\t/u/%s\t@%shttps://old.reddit.com%s\n%s\n\n\n", subreddit, author, asctime(localtime(&created_at)), permalink, body); // asctime introduces a newline
}

int process_live_replies(rapidjson::Value& replies, int last_processed_cmnt_id){
    /*
    'replies' object is the 'replies' JSON object which has property 'kind' of value 'Listing'
    */
    int cmnt_id;
    int i = 0;
    for (rapidjson::Value::ValueIterator itr = replies["data"]["children"].Begin();  itr != replies["data"]["children"].End();  ++itr){
        cmnt_id = id2n((*itr)["data"]["id"].GetString()); // No "t1_" prefix
        if (cmnt_id == last_processed_cmnt_id)
            break;
        PRINTF("[%d] ", i++);
        process_live_cmnt(*itr, cmnt_id);
    }
    return id2n(replies["data"]["children"][0]["data"]["id"].GetString());
}

void process_all_comments_live(){
    int last_processed_cmnt_id = 0;
    
    while (true){
        goto procallcmntslivectnloop:
        
        sleep(REDDIT_REQUEST_DELAY);
        
        
        request("GET", API_ALLCOMMENTS_URL);
        
        rapidjson::Document d;
        if (d.Parse(MEMORY.memory).HasParseError()){
            printf("ERROR: HasParseError\n%s\n", MEMORY.memory);
            megasleep();
            MEMORY.size = 0; // 'Clear' contents of request
            continue;
        }
        
        if (d.HasMember("error")){
            switch (d["error"].GetInt()){
                case 401:
                    // Unauthorised
                    sleep(REDDIT_REQUEST_DELAY);
                    login();
                    goto procallcmntslivectnloop;
                default:
                    printf("%s\n", MEMORY.memory);
                    handler(ERR);
            }
        }
        
        if (d.IsNull()  ||  !d.HasMember("data")){
            megasleep();
            printf("ERROR: d or d[\"data\"] NOT OBJECT\n%s\n", MEMORY.memory);
            MEMORY.size = 0; // 'Clear' contents of request
            continue;
        }
        
        MEMORY.size = 0; // 'Clear' contents of request
        
        last_processed_cmnt_id = process_live_replies(d, last_processed_cmnt_id);
    }
}

void process_moderator(rapidjson::Value& user){
    SET_DBG_STR(user_id,    user["id"])
    user_id += 3; // Skip prefix "t2_"
    SET_DBG_STR(user_name,  user["name"])
    
    const size_t RSET_DBG(added_on, user["date"], GetFloat, "%lu")
    
    // TODO: process mod_permissions, converting array of strings like "all" to integer of bits
}

void process_moderators(const char* subreddit, const int subreddit_len){
    const char* a = "/about/moderators/?raw_json=1";
    char api_url[strlen(API_SUBREDDIT_URL_PREFIX) + subreddit_len + strlen(a) + 1];
    int i = 0;
    
    
    memcpy(api_url + i,  API_SUBREDDIT_URL_PREFIX,  strlen(API_SUBREDDIT_URL_PREFIX));
    i += strlen(API_SUBREDDIT_URL_PREFIX);
    
    memcpy(api_url + i,  subreddit,  subreddit_len);
    i += subreddit_len;
    
    memcpy(api_url + i,  a,  strlen(a));
    i += strlen(a);
    
    api_url[i] = 0;
    
    
    request("GET", api_url);
    
    rapidjson::Document d;
    if (d.Parse(MEMORY.memory).HasParseError())
        handler(ERR_INVALID_PJ);
    
    MEMORY.size = 0; // 'Clear' contents of request
    
    
    for (rapidjson::Value::ValueIterator itr = d["data"]["children"].Begin();  itr != d["data"]["children"].End();  ++itr)
        process_moderator(*itr);
    
    
}

void process_submission_duplicates(const char* submission_id, const int submission_id_len){
    int i = 0;
    char api_url[strlen(API_DUPLICATES_URL_PREFIX) + submission_id_len + 1 + PARAMS_LEN + 1];
    
    memcpy(api_url + i,  API_DUPLICATES_URL_PREFIX,  strlen(API_DUPLICATES_URL_PREFIX));
    i += strlen(API_DUPLICATES_URL_PREFIX);
    
    memcpy(api_url + i,  submission_id,  submission_id_len);
    i += submission_id_len;
    
    api_url[i++] = '/';
    
    // We only need "?limit=1000&raw_json=1", but the additional parameter "&sort=best" has no effect
    memcpy(api_url + i,  PARAMS,  PARAMS_LEN);
    i += PARAMS_LEN;
    
    api_url[i] = 0;
    
    
    request("GET", api_url);
    
    PRINTF("%s\n", MEMORY.memory);
    
    MEMORY.size = 0;
}

void process_submission(const char* url){
    int i = strlen(SUBMISSION_URL_PREFIX);
    
    const int subreddit_len = slashindx(url + i);
    char subreddit[subreddit_len + 1];
    memcpy(subreddit,  url + strlen(SUBMISSION_URL_PREFIX),  subreddit_len);
    subreddit[subreddit_len] = 0;
    i += subreddit_len + 1;
    i += slashindx(url + i) + 1; // Skip the /comments/ section
    
    const int submission_id_len = slashindx(url + i);
    char submission_id[submission_id_len + 1];
    memcpy(submission_id,  url + i,  submission_id_len);
    submission_id[submission_id_len] = 0;
    i += submission_id_len + 1;
    
    
    char api_url[strlen(API_SUBMISSION_URL_PREFIX) + submission_id_len + 1 + PARAMS_LEN + 1];
    int api_url_indx = 0;
    memcpy(api_url + api_url_indx,  API_SUBMISSION_URL_PREFIX,  strlen(API_SUBMISSION_URL_PREFIX));
    api_url_indx += strlen(API_SUBMISSION_URL_PREFIX);
    memcpy(api_url + api_url_indx,  submission_id,  submission_id_len);
    api_url_indx += submission_id_len;
    api_url[api_url_indx++] = '/';
    memcpy(api_url + api_url_indx,  PARAMS,  PARAMS_LEN);
    api_url_indx += PARAMS_LEN;
    api_url[api_url_indx] = 0;
    
    request("GET", api_url);
    
    
    rapidjson::Document d;
    if (d.Parse(MEMORY.memory).HasParseError())
        handler(ERR_INVALID_PJ);
    
    MEMORY.size = 0; // 'Clear' contents of request
    
    SET_DBG_STR(id,             d[0]["data"]["children"][0]["data"]["id"])
    // No prefix to ignore
}

int main(const int argc, const char* argv[]){
    MEMORY.memory = (char*)malloc(0);
    AUTH_HEADER = (char*)malloc(0);
    
    int i = 0;
    
    USR = argv[++i];
    PWD = argv[++i];
    KEY_AND_SECRET = argv[++i];
    
    
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (!curl)
        handler(ERR_CANNOT_INIT_CURL);
    
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    
    login();
    
    while (++i < argc){
        sleep(REDDIT_REQUEST_DELAY);
        process_submission(argv[i]);
    }
    
    process_all_comments_live();
    
    handler(0);
}
