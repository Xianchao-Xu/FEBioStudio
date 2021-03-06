/*This file is part of the FEBio Studio source code and is licensed under the MIT license
listed below.

See Copyright-FEBio-Studio.txt for details.

Copyright (c) 2020 University of Utah, The Trustees of Columbia University in 
the City of New York, and others.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#include "stdafx.h"
#include <QWidget>
#include <QFrame>
#include <QAction>
#include <QLineEdit>
#include <QCompleter>
#include <QPlainTextEdit>
#include <QFormLayout>
#include <QBoxLayout>
#include <QListWidget>
#include <QToolButton>
#include <QDialogButtonBox>
#include <QLabel>
#include <QFrame>
#include "DlgUpload.h"
#include "PublicationWidgetView.h"

//ClickableLabel::ClickableLabel(QWidget* parent, Qt::WindowFlags f)
//    : QLabel(parent) {
//
//}
//
//ClickableLabel::~ClickableLabel() {}
//
//void ClickableLabel::mousePressEvent(QMouseEvent* event) {
//    emit clicked();
//}
//
//
//TagLabel::TagLabel(QString text, QWidget* parent)
//	: QFrame(parent)
//{
//	QHBoxLayout* layout = new QHBoxLayout;
//	layout->setContentsMargins(3, 0, 3, 0);
//	layout->setAlignment(Qt::AlignLeft);
//
//	layout->addWidget(label = new QLabel(text));
//
//	remove = new ClickableLabel;
//	remove->setText("x");
//
//	layout->addWidget(remove);
//	layout->setSizeConstraint(QLayout::SetFixedSize);
//
//	setLayout(layout);
//	setFrameStyle(QFrame::Box);
//
////
////	setStyleSheet("background-color : white; border: black;");
//
//
//	QObject::connect(remove, SIGNAL(clicked()), this, SLOT(deleteThis()));
//}
//
//void TagLabel::deleteThis()
//{
//	delete this;
//}

class Ui::CDlgUpload
{
public:
	QLineEdit* name;
	QPlainTextEdit* description;
	QLabel* owner;
	QLabel* version;
	
	QLineEdit* newTag;
	QCompleter* completer;
	QListWidget* tags;
	::CPublicationWidgetView* pubs;

public:
	void setup(QDialog* dlg)
	{
		QFormLayout* form = new QFormLayout;
		form->addRow("Name: ", name = new QLineEdit);
		form->addRow("Description: ", description = new QPlainTextEdit);
		form->addRow("Owner: ", owner = new QLabel);
		form->addRow("Version: ", version = new QLabel);


		QHBoxLayout* tagLayout = new QHBoxLayout;
		QVBoxLayout* v1 = new QVBoxLayout;
		QVBoxLayout* v2 = new QVBoxLayout;
		v2->setAlignment(Qt::AlignTop);

		newTag = new QLineEdit;
		completer = new QCompleter;
		completer->setCaseSensitivity(Qt::CaseInsensitive);
		newTag->setCompleter(completer);
		v1->addWidget(newTag);

		tags = new QListWidget;
		tags->setSelectionMode(QAbstractItemView::ExtendedSelection);
		v1->addWidget(tags);

		QAction* addTag = new QAction;
		addTag->setIcon(QIcon(":/icons/selectAdd.png"));
		QToolButton* addTagBtn = new QToolButton;
		addTagBtn->setDefaultAction(addTag);
		addTagBtn->setObjectName("addTagBtn");
		v2->addWidget(addTagBtn);

		QAction* delTag= new QAction;
		delTag->setIcon(QIcon(":/icons/selectSub.png"));
		QToolButton* delTagBtn = new QToolButton;
		delTagBtn->setDefaultAction(delTag);
		delTagBtn->setObjectName("delTagBtn");
		v2->addWidget(delTagBtn);

		tagLayout->addLayout(v1);
		tagLayout->addLayout(v2);

		QVBoxLayout* layout = new QVBoxLayout;

		layout->addLayout(form);
		layout->addWidget(new QLabel("Tags:"));
		layout->addLayout(tagLayout);

		layout->addWidget(new QLabel("Publications:"));

		pubs = new ::CPublicationWidgetView(::CPublicationWidgetView::EDITABLE);
		layout->addWidget(pubs);


		QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		layout->addWidget(bb);

		dlg->setLayout(layout);

		QObject::connect(bb, SIGNAL(accepted()), dlg, SLOT(accept()));
		QObject::connect(bb, SIGNAL(rejected()), dlg, SLOT(reject()));

	}
};

CDlgUpload::CDlgUpload(QWidget* parent) : QDialog(parent), ui(new Ui::CDlgUpload)
{
	ui->setup(this);
	setWindowTitle("Upload Project");

	QMetaObject::connectSlotsByName(this);
}

void CDlgUpload::setName(QString name)
{
	ui->name->setText(name);
}

void CDlgUpload::setDescription(QString desc)
{
	ui->description->document()->setPlainText(desc);
}

void CDlgUpload::setOwner(QString owner)
{
	ui->owner->setText(owner);
}

void CDlgUpload::setVersion(QString version)
{
	ui->version->setText(version);
}

void CDlgUpload::setTags(QStringList& tags)
{
	for(auto tag : tags)
	{
		ui->tags->addItem(tag);
	}
}

void CDlgUpload::setPublications(const std::vector<CPublicationWidget*>& pubs)
{
	for(auto pub : pubs)
	{
		ui->pubs->addPublicationCopy(*pub);
	}
}

void CDlgUpload::setTagList(QStringList& tags)
{
	delete ui->completer;
	ui->completer = new QCompleter(tags);
	ui->completer->setCaseSensitivity(Qt::CaseInsensitive);

	ui->newTag->setCompleter(ui->completer);
}

QString CDlgUpload::getName()
{
	return ui->name->text();
}

QString CDlgUpload::getDescription()
{
	return ui->description->document()->toPlainText();
}

QString CDlgUpload::getOwner()
{
	return ui->owner->text();
}

QString CDlgUpload::getVersion()
{
	return ui->version->text();
}


QStringList CDlgUpload::getTags()
{
	QStringList tagList;

	for(int tag = 0; tag < ui->tags->count(); ++tag)
	{
		QString tagText = ui->tags->item(tag)->text().trimmed();

		if(!tagText.isEmpty())
		{
			if(tagList.filter(tagText, Qt::CaseInsensitive).count() == 0)
			{
				tagList.append(tagText);
			}
		}
	}

	return tagList;
}

QList<QVariant> CDlgUpload::getPublicationInfo()
{
	return ui->pubs->getPublicationInfo();
}

void CDlgUpload::on_addTagBtn_clicked()
{
	if(!ui->newTag->text().isEmpty())
	{
		ui->tags->addItem(ui->newTag->text());
	}
	ui->newTag->clear();

}

void CDlgUpload::on_delTagBtn_clicked()
{
	QList<QListWidgetItem*> items = ui->tags->selectedItems();

	for(QListWidgetItem* item : items)
	{
		delete item;
	}
}









