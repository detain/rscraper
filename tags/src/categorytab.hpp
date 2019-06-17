/*
 * rscraper Copyright (C) 2019 Adam Gray
 * This program is licensed with GPLv3.0 and comes with absolutely no warranty.
 * This code may be copied, modified, distributed etc. with accordance to the GPLv3.0 license (a copy of which is in the root project directory) under the following conditions:
 *     This copyright notice must be included at the beginning of any copied/modified file originating from this project, or at the beginning of any section of code that originates from this project.
 */


#include <QWidget>
#include <QVBoxLayout>


class ClTagsTab : public QWidget{
    Q_OBJECT
  public:
    explicit ClTagsTab(const uint64_t id, QWidget* parent = 0);
    const uint64_t cat_id;
  public Q_SLOTS:
    void add_tag();
    uint64_t create_tag(QString& qs,  const char* s);
  private:
    QVBoxLayout* l;
};
