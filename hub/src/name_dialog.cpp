/*
 * rscraper Copyright (C) 2019 Adam Gray
 * This program is licensed with GPLv3.0 and comes with absolutely no warranty.
 * This code may be copied, modified, distributed etc. with accordance to the GPLv3.0 license (a copy of which is in the root project directory) under the following conditions:
 *     This copyright notice must be included at the beginning of any copied/modified file originating from this project, or at the beginning of any section of code that originates from this project.
 */


#include "name_dialog.hpp"

#include <QTimer>


NameDialog::NameDialog(QString title,  QString str,  QString checkbox_title,  QWidget* parent) : QDialog(parent){
	// If the functions are implemented in the header file you have to declare the definitions of the functions with inline to prevent having multiple definitions of the functions.
	this->btn_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(this->btn_box, &QDialogButtonBox::accepted, this, &NameDialog::accept);
	connect(this->btn_box, &QDialogButtonBox::rejected, this, &NameDialog::reject);
	l = new QVBoxLayout;
	l->addWidget(this->btn_box);
	this->name_edit = new QLineEdit(str, this);
	l->addWidget(this->name_edit);
	if (!checkbox_title.isEmpty()){
		this->checkbox = new QCheckBox(checkbox_title,  this);
		l->addWidget(this->checkbox);
	}
	this->setLayout(l);
	this->setWindowTitle(title);
	QTimer::singleShot(0, this->name_edit, SLOT(setFocus())); // Set focus after NameDialog instance is visible
}

NameDialog::~NameDialog(){
	delete this->name_edit;
	delete this->l;
	delete this->btn_box;
}
