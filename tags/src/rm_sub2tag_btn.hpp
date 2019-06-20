/*
 * rscraper Copyright (C) 2019 Adam Gray
 * This program is licensed with GPLv3.0 and comes with absolutely no warranty.
 * This code may be copied, modified, distributed etc. with accordance to the GPLv3.0 license (a copy of which is in the root project directory) under the following conditions:
 *     This copyright notice must be included at the beginning of any copied/modified file originating from this project, or at the beginning of any section of code that originates from this project.
 */

#include <QMouseEvent>
#include <QPushButton>


class RmSub2TagBtn : public QPushButton{
    Q_OBJECT
  private Q_SLOTS:
    void mousePressEvent(QMouseEvent* e);
  public:
    const uint64_t tag_id;
    explicit RmSub2TagBtn(const uint64_t id,  QWidget* parent);
  public Q_SLOTS:
    void rm_subreddit();
};
