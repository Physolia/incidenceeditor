/*
  SPDX-FileCopyrightText: 2004 Cornelius Schumacher <schumacher@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#ifndef INCIDENCEEDITOR_FREEBUSYURLDIALOG_H
#define INCIDENCEEDITOR_FREEBUSYURLDIALOG_H

#include "attendeedata.h"

#include <QDialog>

class KLineEdit;

namespace IncidenceEditorNG {
class FreeBusyUrlWidget;

class FreeBusyUrlDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FreeBusyUrlDialog(const AttendeeData::Ptr &, QWidget *parent = nullptr);

public Q_SLOTS:
    void slotOk();

private:
    FreeBusyUrlWidget *mWidget = nullptr;
};

class FreeBusyUrlWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FreeBusyUrlWidget(const AttendeeData::Ptr &, QWidget *parent = nullptr);
    ~FreeBusyUrlWidget();

    void loadConfig();
    void saveConfig();

private:
    KLineEdit *mUrlEdit = nullptr;
    AttendeeData::Ptr mAttendee;
};
}

#endif
