/*
 *  SPDX-FileCopyrightText: 2014 Sandro Knauß <knauss@kolabsys.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef INCIDENCEEDITOR_ATTENDEETABLEMODEL_H
#define INCIDENCEEDITOR_ATTENDEETABLEMODEL_H

#include <KCalendarCore/Attendee>

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QModelIndex>

namespace IncidenceEditorNG {
class AttendeeTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Roles {
        AttendeeRole = Qt::UserRole
    };

    enum Columns {
        CuType,
        Role,
        FullName,
        Name,
        Email,
        Available,
        Status,
        Response
    };

    enum AvailableStatus {
        Unknown,
        Free,
        Accepted,
        Busy,
        Tentative
    };

    explicit AttendeeTableModel(QObject *parent = nullptr);

    Q_REQUIRED_RESULT int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_REQUIRED_RESULT int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_REQUIRED_RESULT QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Q_REQUIRED_RESULT QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Q_REQUIRED_RESULT Qt::ItemFlags flags(const QModelIndex &index) const override;
    Q_REQUIRED_RESULT bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    Q_REQUIRED_RESULT bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;
    Q_REQUIRED_RESULT bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex()) override;

    Q_REQUIRED_RESULT bool insertAttendee(int position, const KCalendarCore::Attendee &attendee);

    void setAttendees(const KCalendarCore::Attendee::List &resources);
    Q_REQUIRED_RESULT KCalendarCore::Attendee::List attendees() const;

    void setKeepEmpty(bool keepEmpty);
    Q_REQUIRED_RESULT bool keepEmpty() const;

    void setRemoveEmptyLines(bool removeEmptyLines);
    Q_REQUIRED_RESULT bool removeEmptyLines() const;
private:
    void addEmptyAttendee();

    KCalendarCore::Attendee::List mAttendeeList;
    std::vector<AvailableStatus> mAttendeeAvailable;
    bool mKeepEmpty;
    bool mRemoveEmptyLines;
};

class ResourceFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit ResourceFilterProxyModel(QObject *parent = nullptr);
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};

class AttendeeFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit AttendeeFilterProxyModel(QObject *parent = nullptr);
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};
}

#endif
