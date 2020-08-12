/*
 * SPDX-FileCopyrightText: 2014 Sandro Knauß <knauss@kolabsys.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
 */

#ifndef INCIDENCEEDITOR_INDIVIDUALMAILDIALOG_H
#define INCIDENCEEDITOR_INDIVIDUALMAILDIALOG_H

#include <KCalendarCore/Attendee>
#include <QDialog>

#include <QComboBox>
#include <QDialogButtonBox>

class KGuiItem;

class TestIndividualMailDialog;

namespace IncidenceEditorNG {
// Shows a dialog with a question and the option to select which attendee should get the mail or to open a composer for him.
// Used to get individual mails for attendees of an event.
class IndividualMailDialog : public QDialog
{
    Q_OBJECT
    friend class ::TestIndividualMailDialog;
public:
    enum Decisions {
        Update,         /**< send automatic mail to attendee */
        NoUpdate,       /**< do not send mail to attendee */
        Edit            /**< open composer for attendee */
    };
    explicit IndividualMailDialog(const QString &question, const KCalendarCore::Attendee::List &attendees, const KGuiItem &buttonYes, const KGuiItem &buttonNo, QWidget *parent = nullptr);
    ~IndividualMailDialog() override;

    Q_REQUIRED_RESULT KCalendarCore::Attendee::List editAttendees() const;
    Q_REQUIRED_RESULT KCalendarCore::Attendee::List updateAttendees() const;

private:
    void updateButtonState();

    std::vector<std::pair<KCalendarCore::Attendee, QComboBox *> > mAttendeeDecision;
    QDialogButtonBox *m_buttons = nullptr;
    QWidget *m_detailsWidget = nullptr;
};
}

#endif
