/*
 * rscraper Copyright (C) 2019 Adam Gray
 * This program is licensed with GPLv3.0 and comes with absolutely no warranty.
 * This code may be copied, modified, distributed etc. with accordance to the GPLv3.0 license (a copy of which is in the root project directory) under the following conditions:
 *     This copyright notice must be included at the beginning of any copied/modified file originating from this project, or at the beginning of any section of code that originates from this project.
 */

#ifndef RSCRAPER_HUB_REGEX_EDITOR_HPP
#define RSCRAPER_HUB_REGEX_EDITOR_HPP

#include <QDialog>
#include <QPlainTextEdit>


class RegexEditorVarsMenu;


class RegexEditor : public QDialog {
    Q_OBJECT
  public:
    RegexEditor(const QString& human_fp,  const QString& raw_fp,  QWidget* parent = 0);
  private Q_SLOTS:
    void test_regex();
    void save_to_file();
  private:
    void find_text();
    bool to_final_format(QString& buf,  int j = 0);
    void display_help();
    void load_file();
    RegexEditorVarsMenu* vars_menu;
    QString f_human_fp;
    QString f_raw_fp;
    QPlainTextEdit* text_editor;
};


#endif
