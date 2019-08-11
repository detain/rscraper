/*
 * rscraper Copyright (C) 2019 Adam Gray
 * This program is licensed with GPLv3.0 and comes with absolutely no warranty.
 * This code may be copied, modified, distributed etc. with accordance to the GPLv3.0 license (a copy of which is in the root project directory) under the following conditions:
 *     This copyright notice must be included at the beginning of any copied/modified file originating from this project, or at the beginning of any section of code that originates from this project.
 */

#include "view_matched_comments.hpp"

#include "init_regexp_from_file.hpp"

#include "id2str.hpp"

#define ASCIIFY_TIME
#include <compsky/asciify/asciify.hpp>
#include <compsky/mysql/query.hpp>

#include <boost/regex.hpp>
#include <ctime> // for localtime, time_t

#include <QCompleter>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRegExpValidator>
#include <QStringList>
#include <QVariantMap>
#include <QVBoxLayout>


extern QStringList tagslist;
extern QCompleter* reason_name_completer;

namespace filter_comment_body {
	extern boost::basic_regex<char, boost::cpp_regex_traits<char>>* regexpr;
}

namespace _f {
	constexpr static const compsky::asciify::flag::ChangeBufferTmp chbuf;
}

namespace details {
	static const QString sorting_column_titles[6] = {"Datetime", "Reason", "Subreddit", "User", "Submission", "Content Length"};
	constexpr static const char* sorting_columns[6] = {"c.id", "m.name", "subreddit_name", "u.name", "submission_id", "LENGTH(c.content)"};
}


constexpr const char* tag_a1 = 
	"SELECT S.name AS subreddit_name, S.id AS submission_id, c.id, c.created_at, c.content, u.name, m.name\n"
	"FROM reason_matched m, user u, comment c\n"
	"JOIN (\n"
	"	SELECT R.name, s.id\n"
	"	FROM submission s\n"
	"	JOIN (\n"
	"		SELECT r.id, r.name\n"
	"		FROM subreddit r\n";
constexpr const char* tag_b1 = 
	"		JOIN (\n"
	"			SELECT s2t.subreddit_id\n"
	"			FROM subreddit2tag s2t\n"
	"			JOIN (\n"
	"				SELECT t.id\n"
	"				FROM tag t\n"
	"				WHERE t.name=\"";

constexpr const char* tag_b2 = 
				"\""
	"			) T ON T.id = s2t.tag_id\n"
	"		) S2T on S2T.subreddit_id = r.id\n";
constexpr const char* tag_a2 = 
	"	) R on R.id = s.subreddit_id\n"
	") S on S.id = c.submission_id\n"
	"WHERE u.id=c.author_id\n"
	"  AND m.id=c.reason_matched\n";

constexpr const char* reason_a1 = 
	"SELECT r.name AS subreddit_name, s.id AS submission_id, c.id, c.created_at, c.content, u.name, m.name\n"
	"FROM subreddit r, submission s, comment c, user u, reason_matched m\n"
	"WHERE ";
constexpr const char* reason_b1 = 
	"m.name=\"";

constexpr const char* reason_b2 = 
	"\"\n"
	"  AND ";
constexpr const char* reason_a2 = 
	"c.reason_matched=m.id\n"
	"  AND s.id=c.submission_id\n"
	"  AND r.id=s.subreddit_id\n"
	"  AND u.id=c.author_id\n";


uint64_t indexof(const std::vector<uint64_t>& ms,  const uint64_t n){
	for (auto i = ms.size();  i != 0;  )
		if (ms[--i] == n)
			return i;
}


ViewMatchedComments::ViewMatchedComments(QWidget* parent)
: QWidget(parent)
, cmnt_body(nullptr)
, res1(0)
, is_ascending(false)
{
	QVBoxLayout* l = new QVBoxLayout;
	
	
	{
		QHBoxLayout* box = new QHBoxLayout;
		box->addWidget(new QLabel("Tag Name:", this));
		this->tagname_input = new QLineEdit(this);
		QCompleter* tagcompleter = new QCompleter(tagslist);
		this->tagname_input->setCompleter(tagcompleter);
		box->addWidget(this->tagname_input);
		
		box->addWidget(new QLabel("Reason Matched:", this));
		this->reasonname_input = new QLineEdit(this);
		this->reasonname_input->setCompleter(reason_name_completer);
		box->addWidget(this->reasonname_input);
		
		box->addWidget(new QLabel("Limit:", this));
		this->limit_input = new QLineEdit("10", this);
		this->limit_input->setValidator(new QRegExpValidator(QRegExp("\\d*"), this));
		box->addWidget(this->limit_input);
		
		l->addLayout(box);
	}
	
	
	{
		QGroupBox* group_box = new QGroupBox("Order By:");
		QHBoxLayout* box = new QHBoxLayout;
		for (auto i = 0;  i < 6;  ++i){
			this->sorting_column_btns[i] = new QRadioButton(details::sorting_column_titles[i]);
			box->addWidget(this->sorting_column_btns[i]);
		}
		this->sorting_column_btns[0]->setChecked(true);
		box->addStretch(1);
		group_box->setLayout(box);
		l->addWidget(group_box);
	}
	
	{
		QGroupBox* group_box = new QGroupBox("Order:");
		QRadioButton* asc   = new QRadioButton("Ascending");
		QRadioButton* desc  = new QRadioButton("Descending");
		connect(asc,  &QRadioButton::toggled, this, &ViewMatchedComments::toggle_order_btns);
		desc->setChecked(true);
		QHBoxLayout* box = new QHBoxLayout;
		box->addWidget(asc);
		box->addWidget(desc);
		box->addStretch(1);
		group_box->setLayout(box);
		l->addWidget(group_box);
	}
	
	{
		QHBoxLayout* box = new QHBoxLayout;
		
		this->get_empty_comments = new QCheckBox("View empty comments");
		box->addWidget(this->get_empty_comments);
		this->is_content_from_remote = new QCheckBox("Is comment from remote?");
		this->is_content_from_remote->setEnabled(false);
		box->addWidget(this->is_content_from_remote);
		
		l->addLayout(box);
	}
	
	this->query_text = new QPlainTextEdit;
	l->addWidget(this->query_text);
	
	{
		QHBoxLayout* box = new QHBoxLayout;
		QPushButton* next_btn = new QPushButton("Generate", this);
		box->addWidget(next_btn);
		connect(next_btn, &QPushButton::clicked, this, &ViewMatchedComments::generate_query);
		
		QPushButton* exec_btn = new QPushButton("Execute", this);
		box->addWidget(exec_btn);
		connect(exec_btn, &QPushButton::clicked, this, &ViewMatchedComments::execute_query);
		
		QPushButton* init_btn = new QPushButton("Next", this);
		box->addWidget(init_btn);
		connect(init_btn, &QPushButton::clicked, this, &ViewMatchedComments::next);
		
		l->addLayout(box);
	}
	
	
	this->subname   = new QLabel(this);
	this->username  = new QLabel(this);
	this->reasonname = new QLabel(this);
	this->datetime  = new QLabel(this);
	this->permalink = new QLineEdit(this);
	this->permalink->setReadOnly(true);
	l->addWidget(this->subname);
	l->addWidget(this->username);
	l->addWidget(this->reasonname);
	l->addWidget(this->datetime);
	l->addWidget(this->permalink);
	
	this->textarea = new QPlainTextEdit(this);
	this->textarea->setReadOnly(true);
	
	l->addWidget(this->textarea);
	
	{
		QHBoxLayout* box = new QHBoxLayout;
		
		QPushButton* details_cmnt = new QPushButton("Details", this);
		connect(details_cmnt, &QPushButton::clicked, this, &ViewMatchedComments::view_matches);
		box->addWidget(details_cmnt);
		
		QPushButton* del_cmnt = new QPushButton("Delete", this);
		connect(del_cmnt, &QPushButton::clicked, this, &ViewMatchedComments::del_cmnt);
		QPalette palette = this->palette();
		palette.setColor(QPalette::Button, QColor(Qt::red));
		del_cmnt->setAutoFillBackground(true);
		del_cmnt->setPalette(palette);
		del_cmnt->setFlat(true);
		del_cmnt->update();
		box->addWidget(del_cmnt);
		
		l->addLayout(box);
	}
	
	this->setLayout(l);
}

ViewMatchedComments::~ViewMatchedComments(){
	delete this->textarea;
	delete this->datetime;
	delete this->reasonname;
	delete this->username;
	delete this->subname;
}

const char* ViewMatchedComments::get_sort_column(){
	for (auto i = 0;  i < 6;  ++i)
		if (this->sorting_column_btns[i]->isChecked())
			return details::sorting_columns[i];
}

void ViewMatchedComments::generate_query(){
	if (this->res1 != nullptr)
		mysql_free_result(this->res1);
	
	const QString tag    = this->tagname_input->text();
	const QString reason = this->reasonname_input->text();
	const QString limit_str = this->limit_input->text();
	
	compsky::asciify::reset_index();
	
	if (!tag.isEmpty())
		if (!reason.isEmpty())
			// TODO: Make different
			compsky::asciify::asciify(tag_a1, tag_b1, tag, tag_b2, tag_a2);
		else
			compsky::asciify::asciify(tag_a1, tag_b1, tag, tag_b2, tag_a2);
	else if (!reason.isEmpty())
		compsky::asciify::asciify(reason_a1, reason_b1, reason, reason_b2, reason_a2);
	else
		compsky::asciify::asciify(reason_a1, reason_a2);
	
	compsky::asciify::asciify(" ORDER BY ", this->get_sort_column());
	compsky::asciify::asciify((this->is_ascending) ? " asc" : " desc");
	
	if (!limit_str.isEmpty()) 
		compsky::asciify::asciify("\nLIMIT ",  limit_str.toInt());
	
	compsky::asciify::asciify('\0');
	
	this->query_text->setPlainText(compsky::asciify::BUF);
}

void ViewMatchedComments::execute_query(){
	compsky::mysql::query(&this->res1, this->query_text->toPlainText());
	this->query_indx = 0;
	this->cached_cmnt_indx = 0;
	this->cmnt_contents_from_remote.clear();
	if (this->get_empty_comments->isChecked()){
		compsky::asciify::reset_index();
		compsky::asciify::asciify("https://api.pushshift.io/reddit/comment/search/?ids=");
		char* _subname;
		char* _post_id;
		uint64_t _cmnt_id;
		char* _t;
		char* _cmnt_body;
		
		std::vector<uint64_t> cmnt_id_2_result_indx;
		// Pushshift automatically orders the results, and there does not appear to be an option for retaining the order of IDs passed in.
		
		while(compsky::mysql::assign_next_row__no_free(this->res1, &this->row1, &_subname, &_post_id, &_cmnt_id, &_t, &_cmnt_body)){
			if(_cmnt_body[0] == 0){
				compsky::asciify::ITR += id2str(_cmnt_id, compsky::asciify::ITR);
				compsky::asciify::asciify(',');
				cmnt_id_2_result_indx.push_back(_cmnt_id);
			}
		}
		
		this->cmnt_contents_from_remote.reserve(cmnt_id_2_result_indx.size());
		for (auto i = 0;  i < cmnt_id_2_result_indx.size();  ++i)
			// Initialise members, otherwise assert fails when setting values via arbitrary indexes
			this->cmnt_contents_from_remote.append(0);
		
		mysql_data_seek(this->res1, 0); // Return to start of results set
		--compsky::asciify::ITR; // Overwrite trailing comma
		compsky::asciify::asciify("&filter=id,body", '\0');
		
		QProcess proc;
		proc.start("curl",  {compsky::asciify::BUF});
		if (!proc.waitForFinished()){
			QMessageBox::information(this,  "Cannot get comment contents",  QString("Comment failed: curl $1\nIs CURL installed?").arg(compsky::asciify::BUF));
			goto goto__init_first_cmnt;
		}
		const QByteArray json_str = proc.readAllStandardOutput();
		proc.close();
		
		const QJsonDocument jdoc = QJsonDocument::fromJson(json_str);
		const QJsonObject   jobj = jdoc.object();
		const QVariantMap   jmap = jobj.toVariantMap();
		const QVariantList jdata = jmap["data"].toList();
		
		for (auto i = 0;  i < jdata.size();  ++i){
			const QVariantMap cmnt   = jdata.at(i).toMap();
			const QByteArray id_ba   = cmnt["id"].toByteArray();
			const char* const id_str = id_ba.data();
			const uint64_t id = str2id(id_str);
			this->cmnt_contents_from_remote[indexof(cmnt_id_2_result_indx, id)] = cmnt["body"].toString();
		}
	}
	goto__init_first_cmnt:
	this->next();
}

void ViewMatchedComments::next(){
	if (this->res1 == nullptr){
		this->subname->setText("");
		this->username->setText("");
		this->reasonname->setText("");
		this->datetime->setText("");
		this->textarea->setPlainText("");
		return;
	}
	char* subname_;
	constexpr static const compsky::asciify::flag::StrLen f;
	uint64_t post_id;
	uint64_t t;
	char* username_;
	char* reason;
	if (compsky::mysql::assign_next_row(this->res1, &this->row1, &subname_, &post_id, &this->cmnt_id, &t, f, &this->cmnt_body_sz, &this->cmnt_body, &username_, &reason)){
		post_id_str[id2str(post_id,         post_id_str)] = 0;
		cmnt_id_str[id2str(this->cmnt_id,   cmnt_id_str)] = 0;
		
		this->permalink->setText(QString("https://www.reddit.com/r/" + QString(subname_) + QString("/comments/") + QString(post_id_str) + QString("/_/") + QString(cmnt_id_str)));
		
		const time_t tt = t;
		const struct tm* dt = localtime(&tt);
		char* const dt_buf = compsky::asciify::BUF;
		compsky::asciify::reset_index();
		compsky::asciify::asciify(dt);
		compsky::asciify::append(0);
		
		this->subname->setText(subname_);
		this->username->setText(username_);
		this->reasonname->setText(reason);
		this->datetime->setText(dt_buf);
		
		if (cmnt_body[0] == 0  &&  this->get_empty_comments->isChecked()){
			if (this->cmnt_contents_from_remote.isEmpty()){
				QMessageBox::warning(this,  "Error",  "Contents can only be grabbed at the start of a query.\nRun query again in order to capture empties' contents");
			} else {
				this->is_content_from_remote->setChecked(true);
				this->textarea->setPlainText(this->cmnt_contents_from_remote.at(this->cached_cmnt_indx++));
			}
		} else {
			this->is_content_from_remote->setChecked(false);
			this->textarea->setPlainText(this->cmnt_body);
		}
		
		QPalette palette = this->textarea->palette();
		palette.setColor(this->textarea->backgroundRole(), Qt::black);
		palette.setColor(this->textarea->foregroundRole(), Qt::yellow);
		this->textarea->setPalette(palette);
		
		++this->query_indx;
	} else this->res1 = nullptr;
}

void ViewMatchedComments::toggle_order_btns(){
	this->is_ascending = !this->is_ascending;
}

void ViewMatchedComments::del_cmnt(){
	compsky::mysql::exec("DELETE FROM comment WHERE id=", this->cmnt_id);
	this->next();
}

void ViewMatchedComments::view_matches(){
	/*
	 * NOTE: The current regexp is run on the string, notably not necessarily the one that originally matched the comment.
	 */
	
	std::vector<char*> reason_name2id;
	std::vector<int> groupindx2reason;
	std::vector<bool> record_contents;
	
	QString report = ""; 
	
	if (filter_comment_body::regexpr == nullptr)
		filter_comment_body::init_regexp_from_file(reason_name2id, groupindx2reason, record_contents);
	
	boost::match_results<const char*> what;
	
	const char* str = this->cmnt_body;
	
	if (!boost::regex_search(str,  str + this->cmnt_body_sz,  what,  *filter_comment_body::regexpr))
		report += "No matches";
	
	for (size_t i = 1;  i < what.size();  ++i){
		// Ignore first index - it is the entire match, not a regex group.
		if (what[i].matched)
			report += QString("\nMatched group ") + QString::number(i) + QString("\n\t") + QString::fromLocal8Bit(what[i].first,  (uintptr_t)what[i].second - (uintptr_t)what[i].first);
	}
	
	QMessageBox::information(this, "Report", report);
}
