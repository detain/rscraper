package main

/*
#cgo LDFLAGS: -L${SRCDIR}/../../build/tagger -lrscraper-tagger
#cgo CFLAGS: -O3

extern char* DST;
extern void init();
extern void exit_mysql();
extern const char* generate_id_list_string(const char* tblname,  const char** names);
extern void csv2cls(const char* csv,  const char* tagcondition,  const char* reasoncondition);
extern void subreddits_given_userid(const char* const tagfilter,  const char* user_id);
extern void comments_given_userid(const char* const reasonfilter,  const char* const id_str);
extern void comments_given_username(const char* reasonfilter,  const char* const name);
extern void comments_given_reason(const char* const reasonfilter,  const char* const reason_name);
extern void subreddits_given_reason(const char* const reasonfilter,  const char* const reason_name);
extern void subreddits_correlation_to_reasons(const char* const reasonfilter);
extern void get_all_reasons(const char* const reasonfilter);
extern void get_all_tags(const char* const tagfilter);
*/
import "C" // Pseudopackage
import "flag"
import "io"
import "net/http"
import "os"
import "syscall"
import "os/signal"


var tagfilter string
var reasonfilter string
var all_reasons string
var all_tags string
var JSON_subreddits_correlation_to_reasons string

// NOTE: To convert JS/HTML to human readable format,  ^(\t*)"|" \+$|\\(")  ->  \1\2


func js_utils(w http.ResponseWriter, r* http.Request){
	w.Header().Set("Cache-Control", "max-age=86400") // 24h
	const html = "" +
		"function timestamp2dt(t){" +
			"return new Date(t*1000).toISOString().slice(-24, -5)" +
		"}" +
		"function wipe_table(selector){" +
			"$(selector + \" tr\").remove();" +
		"}" +
		"function populate_table(url, selector, postfnct){" +
			"$.ajax({" +
				"dataType: \"json\"," +
				"url: url," +
				"success: function(data){" +
					"var s = \"\";" +
					"for (var row of data){" +
						"s += \"<tr>\";" +
						"for (var item of row){" +
							"s += \"<td>\" + item + \"</td>\";" +
						"}" +
						"s += \"</tr>\";" +
					"}" +
					"$(selector).html(s);" +
					"if (postfnct !== undefined){" +
						"postfnct();" +
					"}" +
				"}," +
				"error: function(){" +
					"alert(\"Error populating table\");" +
				"}" +
			"});" +
		"}" +
		"function column_to_permalink(selector, subreddit_indx, link_indx){" +
			"$(selector).find('tr').each(function (i, el){" +
				"var $tds = $(this).find('td');" +
				"var subreddit_name = $tds.eq(subreddit_indx).text();" +
				"var $link = $tds.eq(link_indx);" +
				"$link.html(\"<a href='https://www.reddit.com/r/\" + subreddit_name + \"/comments/\" + $link.text() + \"'>Link</a>\")" +
			"});" +
		"}" +
		"function column_from_timestamp(selector, timestamp_indx){" +
			"$(selector).find('tr').each(function (i, el){" +
				"var $tds = $(this).find('td');" +
				"var $link = $tds.eq(timestamp_indx);" +
				"$link.value = $link.text();" +
				"$link.text(timestamp2dt($link.value));" +
			"});" +
		"}" +
		"function populate_reasons(){" +
			"$.ajax({" +
				"dataType: \"json\"," +
				"url: \"/api/reasons.json\"," +
				"success: function(data){" +
					"var s = $(\"#m\").html();" +
					"for (const [reason_id, reason_name] of Object.entries(data)){" +
						"s += \"<option value='\" + reason_id + \"'>\" + reason_name + \"</option>\";" +
					"}" +
					"$(\"#m\").html(s);" +
				"}," +
				"error: function(){" +
					"alert(\"Error populating table\");" +
				"}" +
			"});" +
		"}" +
		"function column_id2name_reason(selector, col){" +
			"$.ajax({" +
				"dataType: \"json\"," +
				"url: \"/api/reasons.json\"," +
				"success: function(data){" +
					"$(selector).find('tr').each(function (i, el){" +
						"var $tds = $(this).find('td');" +
						"var $reason = $tds.eq(col);" +
						"$reason.value = $reason.text();" +
						"$reason.text(data[$reason.value]);" +
					"});" +
				"}," +
				"error: function(){" +
					"alert(\"Error populating table\");" +
				"}" +
			"});" +
		"}" +
		"function column_id2name_tag(selector, col){" +
			"$.ajax({" +
				"dataType: \"json\"," +
				"url: \"/api/tags.json\"," +
				"success: function(data){" +
					"$(selector).find('tr').each(function (i, el){" +
						"var $tds = $(this).find('td');" +
						"var $reason = $tds.eq(col);" +
						"$reason.value = $reason.text();" +
						"$reason.text(data[$reason.value]);" +
					"});" +
				"}," +
				"error: function(){" +
					"alert(\"Error populating table\");" +
				"}" +
			"});" +
		"}"
    io.WriteString(w, html)
}


func get_all_reasons(w http.ResponseWriter, r* http.Request){
    w.Header().Set("Content-Type", "application/json")
	w.Header().Set("Cache-Control", "max-age=86400") // 24h
    io.WriteString(w, all_reasons)
}

func get_all_tags(w http.ResponseWriter, r* http.Request){
    w.Header().Set("Content-Type", "application/json")
	w.Header().Set("Cache-Control", "max-age=86400") // 24h
    io.WriteString(w, all_tags)
}


func indexof_flairs_given_users(w http.ResponseWriter, r* http.Request){
	w.Header().Set("Cache-Control", "max-age=86400") // 24h
	const html = "" +
		"<!DOCTYPE html>" +
			"<body>" +
				"<h1>" +
					"RTagger Flairs" +
				"</h1>" +
				"<div>" +
					"<a href=\"https://104.197.15.19:8080/flairs/slurs/\">" +
						"Slurs Flair Server" +
					"</a>" +
					"<img src=\"https://user-images.githubusercontent.com/30552567/62826417-06416580-bbb3-11e9-88a7-ed035f82c2ab.png\"/>" +
				"</div>" +
				"<div>" +
					"<a href=\"https://104.197.15.19:8080/flairs/regions/\">" +
						"Regions Flair Server" +
					"</a>" +
					"Image pending" +
				"</div>" +
				"<a href=\"https://github.com/NotCompsky/rscraper/tree/master/tagger\">" +
					"Source code and installation instructions" +
				"</a>" +
			"</body>" +
		"</html>"
    io.WriteString(w, html)
}

func flairs_given_users(w http.ResponseWriter, r* http.Request){
    w.Header().Set("Content-Type", "application/json")
    C.csv2cls(C.CString(r.URL.Path[12:]), C.CString(tagfilter), C.CString(reasonfilter))
    io.WriteString(w, C.GoString(C.DST))
}


func subreddits_given_userid(w http.ResponseWriter, r* http.Request){
	w.Header().Set("Cache-Control", "max-age=86400") // 24h
	w.Header().Set("Content-Type", "application/json")
	C.subreddits_given_userid(C.CString(tagfilter), C.CString(r.URL.Path[29:]))
	io.WriteString(w, C.GoString(C.DST))
}


func html_comments_given_userid(w http.ResponseWriter, r* http.Request){
	w.Header().Set("Cache-Control", "max-age=60")
	html := "" +
		"<!DOCTYPE html>" +
			"<body>" +
				"<script src=\"https://code.jquery.com/jquery-3.4.1.min.js\"></script>" +
				"<script src=\"/static/utils.js\"></script>" +
				"<script>" +
					"function format_table(){" +
						"column_id2name_reason('#cmnt-tbl tbody',  0);" +
						"column_to_permalink('#cmnt-tbl tbody',  1,  3);" +
						"column_from_timestamp('#cmnt-tbl tbody',  2);" +
					"}" +
					"function format_sub_table(){" +
						"column_id2name_tag('#sub-tbl tbody',  0);" +
					"}" +
					"const user_id = \"" + r.URL.Path[3:] + "\";" +
					"populate_table('/api/u/' + user_id,  '#cmnt-tbl tbody',  format_table);" +
					"populate_table('/api/subreddits_given_userid/' + user_id,  '#sub-tbl tbody',  format_sub_table);" +
				"</script>" +
				"<h1>" +
					"User Summary" +
				"</h1>" +
				"<h2>" +
					"Subreddits" +
				"</h2>" +
				"<table id=\"sub-tbl\">" +
					"<thead>" +
						"<tr>" +
							"<th>" +
								"Tag" +
							"</th>" +
							"<th>" +
								"Subreddit" +
							"</th>" +
							"<th>" +
								"#Comments" +
							"</th>" +
						"</tr>" +
					"</thead>" +
					"<tbody></tbody>" +
				"</table>" +
				"<h2>" +
					"Comments" +
				"</h2>" +
				"<table id=\"cmnt-tbl\">" +
					"<thead>" +
						"<tr>" +
							"<th>" +
								"Reason" +
							"</th>" +
							"<th>" +
								"Subreddit" +
							"</th>" +
							"<th>" +
								"At" +
							"</th>" +
							"<th>" +
								"Link" +
							"</th>" +
						"</tr>" +
					"</thead>" +
					"<tbody></tbody>" +
				"</table>" +
			"</body>" +
		"</html>"
    io.WriteString(w, html)
}

func comments_given_userid(w http.ResponseWriter, r* http.Request){
    C.comments_given_userid(C.CString(reasonfilter), C.CString(r.URL.Path[7:]))
    io.WriteString(w, C.GoString(C.DST))
}


func html_comments_given_username(w http.ResponseWriter, r* http.Request){
	w.Header().Set("Cache-Control", "max-age=86400") // 24h
	const html = "" +
		"<!DOCTYPE html>" +
			"<body>" +
				"<script src=\"https://code.jquery.com/jquery-3.4.1.min.js\"></script>" +
				"<script src=\"/static/utils.js\"></script>" +
				"<script>" +
					"function format_table(){" +
						"column_id2name_reason('#tbl tbody',  0);" +
						"column_to_permalink('#tbl tbody',  1,  3);" +
						"column_from_timestamp('#tbl tbody',  2);" +
					"}" +
				"</script>" +
				"<h1>" +
					"Comments given user" +
				"</h1>" +
				"<div>" +
					"<input type=\"text\" id=\"u\" placeholder=\"Username\"/>" +
					"<button onclick=\"wipe_table('#tbl tbody'); populate_table('/api/user/' + $('#u')[0].value,  '#tbl tbody',  format_table)\">" +
						"Go" +
					"</button>" +
				"</div>" +
				"<table id=\"tbl\">" +
					"<thead>" +
						"<tr>" +
							"<th>" +
								"Reason" +
							"</th>" +
							"<th>" +
								"Subreddit" +
							"</th>" +
							"<th>" +
								"At" +
							"</th>" +
							"<th>" +
								"Link" +
							"</th>" +
						"</tr>" +
					"</thead>" +
					"<tbody></tbody>" +
				"</table>" +
			"</body>" +
		"</html>"
    io.WriteString(w, html)
}

func comments_given_username(w http.ResponseWriter, r* http.Request){
    C.comments_given_username(C.CString(reasonfilter), C.CString(r.URL.Path[10:]))
    io.WriteString(w, C.GoString(C.DST))
}


func html_subreddits_given_reason(w http.ResponseWriter, r* http.Request){
	w.Header().Set("Cache-Control", "max-age=86400") // 24h
	const html = "" +
		"<!DOCTYPE html>" +
			"<body>" +
				"<script src=\"https://code.jquery.com/jquery-3.4.1.min.js\"></script>" +
				"<script src=\"/static/utils.js\"></script>" +
				"<script>window.onload=populate_reasons;</script>" +
				"<h1>" +
					"Subreddits given reason" +
				"</h1>" +
				"<div>" +
					"<select id=\"m\">" +
						"<option value=\"all\">All</option>" +
					"</select>" +
					"<button onclick=\"wipe_table('#tbl tbody'); populate_table('/api/reason/subreddits/' + $('#m')[0].value,  '#tbl tbody')\">" +
						"Go" +
					"</button>" +
				"</div>" +
				"<table id=\"tbl\">" +
					"<thead>" +
						"<tr>" +
							"<th>" +
								"Subreddit" +
							"</th>" +
							"<th>" +
								"Proportion" +
							"</th>" +
						"</tr>" +
					"</thead>" +
					"<tbody></tbody>" +
				"</table>" +
			"</body>" +
		"</html>"
    io.WriteString(w, html)
}

func subreddits_given_reason(w http.ResponseWriter, r* http.Request){
	w.Header().Set("Cache-Control", "max-age=86400") // 24h
    C.subreddits_given_reason(C.CString(reasonfilter), C.CString(r.URL.Path[23:]))
    io.WriteString(w, C.GoString(C.DST))
}

func subreddits_correlation_to_reasons(w http.ResponseWriter, r* http.Request){
	w.Header().Set("Cache-Control", "max-age=86400") // 24h
	io.WriteString(w, JSON_subreddits_correlation_to_reasons)
}


func html_comments_given_reason(w http.ResponseWriter, r* http.Request){
	w.Header().Set("Cache-Control", "max-age=86400") // 24h
	const html = "" +
		"<!DOCTYPE html>" +
			"<body>" +
				"<script src=\"https://code.jquery.com/jquery-3.4.1.min.js\"></script>" +
				"<script src=\"/static/utils.js\"></script>" +
				"<script>window.onload=populate_reasons;</script>" +
				"<script>" +
					"function format_table(){" +
						"column_to_permalink('#tbl tbody',  0,  2);" +
						"column_from_timestamp('#tbl tbody',  1);" +
					"}" +
				"</script>" +
				"<h1>" +
					"Comments given reason" +
				"</h1>" +
				"<div>" +
					"<select id=\"m\"></select>" +
					"<button onclick=\"wipe_table('#tbl tbody'); populate_table('/api/reason/comments/' + $('#m')[0].value,  '#tbl tbody',  format_table);  \">" +
						"Go" +
					"</button>" +
				"</div>" +
				"<table id=\"tbl\">" +
					"<thead>" +
						"<tr>" +
							"<th>" +
								"Subreddit" +
							"</th>" +
							"<th>" +
								"At" +
							"</th>" +
							"<th>" +
								"Link" +
							"</th>" +
						"</tr>" +
					"</thead>" +
					"<tbody></tbody>" +
				"</table>" +
			"</body>" +
		"</html>"
    io.WriteString(w, html)
}

func comments_given_reason(w http.ResponseWriter, r* http.Request){
    C.comments_given_reason(C.CString(reasonfilter), C.CString(r.URL.Path[21:]))
    io.WriteString(w, C.GoString(C.DST))
}


func indexof_reason(w http.ResponseWriter, r* http.Request){
	w.Header().Set("Cache-Control", "max-age=86400") // 24h
	const html = "" +
		"<!DOCTYPE html>" +
			"<body>" +
				"<h1>" +
					"Statistics or links for a given reason" +
				"</h1>" +
				"<a href=\"subreddits/\">" +
					"Subreddits" +
				"</a>" +
				"<br/>" +
				"<a href=\"comments/\">" +
					"Comments" +
				"</a>" +
			"</body>" +
		"</html>"
    io.WriteString(w, html)
}

func indexof_root(w http.ResponseWriter, r* http.Request){
	w.Header().Set("Cache-Control", "max-age=86400") // 24h
	const html = "" +
		"<!DOCTYPE html>" +
			"<body>" +
				"<h1>" +
					"Index" +
				"</h1>" +
				"<a href=\"flairs/\">" +
					"User Flairing" +
				"</a>" +
				"<br/>" +
				"<a href=\"reason/\">" +
					"Reason Statistics" +
				"</a>" +
				"<br/>" +
				"<a href=\"user/\">" +
					"User Statistics" +
				"</a>" +
			"</body>" +
		"</html>"
    io.WriteString(w, html)
}

func main(){
    var portN string
    flag.StringVar(&portN, "p", "8080", "Port number")
    flag.StringVar(&tagfilter,    "t", "", "SQL condition that t.id (tag ID) must fulfil. If non-empty, must begin with 'AND'. E.g. 'AND t.id=3'")
    flag.StringVar(&reasonfilter, "m", "", "SQL condition that m.id (reason_matched ID) must fulfil. If non-empty, must begin with 'AND'. E.g. 'AND m.id=3'")
    flag.Parse()
    
    /* Exit MySQL on interrupt signals */
    sgnl := make(chan os.Signal)
    signal.Notify(sgnl, os.Interrupt, syscall.SIGTERM)
    go func(){
        <-sgnl
        C.exit_mysql()
        os.Exit(1)
    }()
    
    C.init()
	
	C.get_all_reasons(C.CString(reasonfilter))
	all_reasons = C.GoString(C.DST) // Deep copied
	
	C.get_all_tags(C.CString(tagfilter))
	all_tags = C.GoString(C.DST) // Deep copied
	
	C.subreddits_correlation_to_reasons(C.CString(reasonfilter))
	JSON_subreddits_correlation_to_reasons = C.GoString(C.DST)
	
	
    mux := http.NewServeMux()
	
	/* NOTE: 
		API is unstable.
		When it is stable, the following (/js and /api) may be versioned.
	*/
	
	mux.HandleFunc("/", indexof_root)
	mux.HandleFunc("/reason/", indexof_reason)
	
	mux.HandleFunc("/flairs/", indexof_flairs_given_users)
    mux.HandleFunc("/api/flairs/", flairs_given_users)
	
	mux.HandleFunc("/static/utils.js", js_utils)
	
	mux.HandleFunc("/reason/subreddits/", html_subreddits_given_reason)
	mux.HandleFunc("/api/reason/subreddits/", subreddits_given_reason)
	mux.HandleFunc("/api/reason/subreddits/all", subreddits_correlation_to_reasons)
	mux.HandleFunc("/reason/comments/",   html_comments_given_reason)
	mux.HandleFunc("/api/reason/comments/",   comments_given_reason)
	
	mux.HandleFunc("/api/subreddits_given_userid/", subreddits_given_userid)
	
	mux.HandleFunc("/user/", html_comments_given_username)
	mux.HandleFunc("/api/user/", comments_given_username)
	mux.HandleFunc("/u/",     html_comments_given_userid)
	mux.HandleFunc("/api/u/", comments_given_userid)
	
	mux.HandleFunc("/api/reasons.json",  get_all_reasons)
	mux.HandleFunc("/api/tags.json",  get_all_tags)
	
    http.ListenAndServe(":" + portN,  mux)
}

/*
ISSUES:
    Exits normally (returns 0) immediately if the port is unusable - either bad format (should be colon preceding number, e.g. :12345) or already in use.
*/
