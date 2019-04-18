#include <rapidjson/document.h> // for rapidjson::Document
#include "rapidjson/pointer.h" // for rapidjson::GetValueByPointer
#include <stdio.h> // for fread, printf

#define REDDIT_REQUEST_DELAY 1

#define SET_DBG_STR(a, b) const char* a = b; printf(#a ":\t%s\n", b);
#define SET_DBG_INT(a, b) const int a = b; printf(#a ":\t%d\n", b);


int main(const int argc, const char* argv[]){
    FILE* f = fopen("/tmp/b", "r");
    char buf[19980];
    fread(buf, 1, 19980, f);
    rapidjson::Document d;
    if (d.Parse(buf).HasParseError())
        return 1;
    
    SET_DBG_STR(title, d[0]["data"]["children"][0]["data"]["title"].GetString())
    //const char* title = d[0]["data"]["children"][0]["data"]["title"].GetString();
    
    SET_DBG_STR(author_id, d[0]["data"]["children"][0]["data"]["author_fullname"].GetString()); // t2_<ID>
    SET_DBG_STR(author, d[0]["data"]["children"][0]["data"]["author"].GetString());
    SET_DBG_INT(score, d[0]["data"]["children"][0]["data"]["score"].GetInt());
   // const time_t created_at = d[0]["data"]["children"][0]["data"]["created"].GetInt64();
    SET_DBG_STR(link_domain, d[0]["data"]["children"][0]["data"]["domain"].GetString());
    SET_DBG_STR(link_url, d[0]["data"]["children"][0]["data"]["url"].GetString());
    
    //printf("d[0][\"kind\"] = %s\n", d[0]["kind"].GetString());
    
    
    rapidjson::Value* cmts = &d[1]["data"]["children"];
    rapidjson::Value* cmt;
    printf("cmts->Size():\t%d\n", cmts->Size());
    while (true){
        for (auto i = 0;  i < cmts->Size();  ++i){
            cmt = &cmts[i];
        }
    }
}
