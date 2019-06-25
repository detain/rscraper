/*
 * rscraper Copyright (C) 2019 Adam Gray
 * This program is licensed with GPLv3.0 and comes with absolutely no warranty.
 * This code may be copied, modified, distributed etc. with accordance to the GPLv3.0 license (a copy of which is in the root project directory) under the following conditions:
 *     This copyright notice must be included at the beginning of any copied/modified file originating from this project, or at the beginning of any section of code that originates from this project.
 */


#include "mainwindow.hpp"

#include <QColorDialog>
#include <QCompleter>
#include <QMessageBox>
#include <QVBoxLayout>

#include <compsky/asciify/flags.hpp>
#include <compsky/mysql/query.hpp>

#include "categorytab.hpp"
#include "maintab.hpp"
#include "name_dialog.hpp"
#include "view_matched_comments.hpp"

#define DIGITS_IN_UINT64 19


namespace compsky {
    namespace asciify {
        char* BUF = (char*)malloc(4096);
    }
}

MYSQL_RES* RES1;
MYSQL_ROW ROW1;

MYSQL_RES* RES2;
MYSQL_ROW ROW2;


std::map<QString, uint64_t> tag_name2id;
QStringList tagslist;
QCompleter* tagcompleter;
QStringList subreddit_names;
QCompleter* subreddit_name_completer;

constexpr static const compsky::asciify::flag::Escape f_esc;


MainWindow::MainWindow(QWidget* parent){
    compsky::mysql::init(getenv("RSCRAPER_MYSQL_CFG"));
    
    this->tab_widget = new QTabWidget(this);
    connect(this->tab_widget, &QTabWidget::tabBarDoubleClicked, this, &MainWindow::rename_category);
    
    this->tab_widget->addTab(new MainTab(this->tab_widget), "__MAIN__");
    
    this->tab_widget->addTab(new ViewMatchedComments(this->tab_widget), "__CMNTS__");
    
    tag_name2id.clear();
    compsky::mysql::query_buffer(&RES1, "SELECT id, name FROM tag");
    {
    uint64_t id;
    char* name;
    while (compsky::mysql::assign_next_row(RES1, &ROW1, &id, &name)){
        tag_name2id[name] = id;
        tagslist << name;
    }
    }
    
    compsky::mysql::query_buffer(&RES1, "SELECT id, name FROM category");
    
    {
    uint64_t id;
    char* name;
    while (compsky::mysql::assign_next_row(RES1, &ROW1, &id, &name)){
        this->tab_widget->addTab(new ClTagsTab(id, this->tab_widget), tr(name));
    }
    }
    
    compsky::mysql::query_buffer(&RES1, "SELECT name FROM subreddit");
    {
    char* name;
    while (compsky::mysql::assign_next_row(RES1, &ROW1, &name)){
        subreddit_names << name;
    }
    }
    subreddit_name_completer = new QCompleter(subreddit_names);
    
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    
    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(this->tab_widget);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
    
    setWindowTitle("RScraper Hub");
}

MainWindow::~MainWindow(){
    compsky::mysql::exit_mysql();
}

void MainWindow::rename_category(int indx){
    if (indx == 0)
        // Only category tabs can be renamed
        return;
    
    NameDialog* dialog = new NameDialog("Rename Category", this->tab_widget->tabText(indx));
    dialog->name_edit->setCompleter(tagcompleter);
    if (dialog->exec() != QDialog::Accepted)
        return;
    QString qstr = dialog->name_edit->text();
    if (qstr.isEmpty())
        return;
    
    this->tab_widget->setTabText(indx, qstr);
    
    const uint64_t cat_id = static_cast<ClTagsTab*>(this->tab_widget->widget(indx))->cat_id;
    
    compsky::mysql::exec("UPDATE category SET name=\"", f_esc, '"', qstr, "\" WHERE id=", cat_id);
}
