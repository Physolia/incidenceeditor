/*
  SPDX-FileCopyrightText: 2010 Bertjan Broeksema <broeksema@kde.org>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "editoritemmanager.h"
#include "individualmailcomponentfactory.h"

#include <CalendarSupport/KCalPrefs>
#include <CalendarSupport/Utils>

#include <Akonadi/Item>
#include <Akonadi/ItemDeleteJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemMoveJob>
#include <Akonadi/Monitor>
#include <Akonadi/Session>
#include <Akonadi/TagFetchScope>

#include "incidenceeditor_debug.h"
#include <KJob>
#include <KLocalizedString>

#include <QMessageBox>
#include <QPointer>

/// ItemEditorPrivate

namespace IncidenceEditorNG
{
class ItemEditorPrivate
{
    EditorItemManager *q_ptr;
    Q_DECLARE_PUBLIC(EditorItemManager)

public:
    Akonadi::Item mItem;
    Akonadi::Item mPrevItem;
    Akonadi::ItemFetchScope mFetchScope;
    Akonadi::Monitor *mItemMonitor = nullptr;
    ItemEditorUi *mItemUi = nullptr;
    bool mIsCounterProposal = false;
    EditorItemManager::SaveAction currentAction;
    Akonadi::IncidenceChanger *mChanger = nullptr;

public:
    ItemEditorPrivate(Akonadi::IncidenceChanger *changer, EditorItemManager *qq);
    void itemChanged(const Akonadi::Item &, const QSet<QByteArray> &);
    void itemFetchResult(KJob *job);
    void itemMoveResult(KJob *job);
    void onModifyFinished(int changeId, const Akonadi::Item &item, Akonadi::IncidenceChanger::ResultCode resultCode, const QString &errorString);

    void onCreateFinished(int changeId, const Akonadi::Item &item, Akonadi::IncidenceChanger::ResultCode resultCode, const QString &errorString);

    void setupMonitor();
    void moveJobFinished(KJob *job);
    void setItem(const Akonadi::Item &item);
};

ItemEditorPrivate::ItemEditorPrivate(Akonadi::IncidenceChanger *changer, EditorItemManager *qq)
    : q_ptr(qq)
    , currentAction(EditorItemManager::None)
{
    mFetchScope.fetchFullPayload();
    mFetchScope.setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    mFetchScope.setFetchTags(true);
    mFetchScope.tagFetchScope().setFetchIdOnly(false);
    mFetchScope.setFetchRemoteIdentification(false);

    mChanger = changer ? changer : new Akonadi::IncidenceChanger(new IndividualMailComponentFactory(qq), qq);

    // clang-format off
    qq->connect(mChanger,
                SIGNAL(modifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
                qq,
                SLOT(onModifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)));

    qq->connect(mChanger,
                SIGNAL(createFinished(int, Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
                qq,
                SLOT(onCreateFinished(int, Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)));
    // clang-format on
}

void ItemEditorPrivate::moveJobFinished(KJob *job)
{
    Q_Q(EditorItemManager);
    if (job->error()) {
        qCCritical(INCIDENCEEDITOR_LOG) << "Error while moving and modifying " << job->errorString();
        mItemUi->reject(ItemEditorUi::ItemMoveFailed, job->errorString());
    } else {
        Akonadi::Item item(mItem.id());
        currentAction = EditorItemManager::MoveAndModify;
        q->load(item);
    }
}

void ItemEditorPrivate::itemFetchResult(KJob *job)
{
    Q_ASSERT(job);
    Q_Q(EditorItemManager);

    EditorItemManager::SaveAction action = currentAction;
    currentAction = EditorItemManager::None;

    if (job->error()) {
        mItemUi->reject(ItemEditorUi::ItemFetchFailed, job->errorString());
        return;
    }

    auto fetchJob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    if (fetchJob->items().isEmpty()) {
        mItemUi->reject(ItemEditorUi::ItemFetchFailed);
        return;
    }

    Akonadi::Item item = fetchJob->items().at(0);
    if (mItemUi->hasSupportedPayload(item)) {
        setItem(item);
        if (action != EditorItemManager::None) {
            // Finally enable ok/apply buttons, we've finished loading
            Q_EMIT q->itemSaveFinished(action);
        }
    } else {
        mItemUi->reject(ItemEditorUi::ItemHasInvalidPayload);
    }
}

void ItemEditorPrivate::setItem(const Akonadi::Item &item)
{
    Q_ASSERT(item.hasPayload());
    mPrevItem = item;
    mItem = item;
    mItemUi->load(item);
    setupMonitor();
}

void ItemEditorPrivate::itemMoveResult(KJob *job)
{
    Q_ASSERT(job);
    Q_Q(EditorItemManager);

    if (job->error()) {
        auto moveJob = qobject_cast<Akonadi::ItemMoveJob *>(job);
        Q_ASSERT(moveJob);
        Q_UNUSED(moveJob)
        // Q_ASSERT(!moveJob->items().isEmpty());
        // TODO: What is reasonable behavior at this point?
        qCCritical(INCIDENCEEDITOR_LOG) << "Error while moving item "; // << moveJob->items().first().id() << " to collection "
        //<< moveJob->destinationCollection() << job->errorString();
        Q_EMIT q->itemSaveFailed(EditorItemManager::Move, job->errorString());
    } else {
        // Fetch the item again, we want a new mItem, which has an updated parentCollection
        Akonadi::Item item(mItem.id());
        // set currentAction, so the fetchResult slot emits itemSavedFinished(Move);
        // We could emit it here, but we should only enable ok/apply buttons after the loading
        // is complete
        currentAction = EditorItemManager::Move;
        q->load(item);
    }
}

void ItemEditorPrivate::onModifyFinished(int, const Akonadi::Item &item, Akonadi::IncidenceChanger::ResultCode resultCode, const QString &errorString)
{
    Q_Q(EditorItemManager);
    if (resultCode == Akonadi::IncidenceChanger::ResultCodeSuccess) {
        if (mItem.parentCollection() == mItemUi->selectedCollection() || mItem.storageCollectionId() == mItemUi->selectedCollection().id()) {
            mItem = item;
            Q_EMIT q->itemSaveFinished(EditorItemManager::Modify);
            setupMonitor();
        } else { // There's a collection move too.
            auto moveJob = new Akonadi::ItemMoveJob(mItem, mItemUi->selectedCollection());
            q->connect(moveJob, SIGNAL(result(KJob *)), SLOT(moveJobFinished(KJob *)));
        }
    } else if (resultCode == Akonadi::IncidenceChanger::ResultCodeUserCanceled) {
        Q_EMIT q->itemSaveFailed(EditorItemManager::Modify, QString());
        q->load(Akonadi::Item(mItem.id()));
    } else {
        qCCritical(INCIDENCEEDITOR_LOG) << "Modify failed " << errorString;
        Q_EMIT q->itemSaveFailed(EditorItemManager::Modify, errorString);
    }
}

void ItemEditorPrivate::onCreateFinished(int, const Akonadi::Item &item, Akonadi::IncidenceChanger::ResultCode resultCode, const QString &errorString)
{
    Q_Q(EditorItemManager);
    if (resultCode == Akonadi::IncidenceChanger::ResultCodeSuccess) {
        currentAction = EditorItemManager::Create;
        q->load(item);
        setupMonitor();
    } else {
        qCCritical(INCIDENCEEDITOR_LOG) << "Creation failed " << errorString;
        Q_EMIT q->itemSaveFailed(EditorItemManager::Create, errorString);
    }
}

void ItemEditorPrivate::setupMonitor()
{
    // Q_Q(EditorItemManager);
    delete mItemMonitor;
    mItemMonitor = new Akonadi::Monitor;
    mItemMonitor->setObjectName(QStringLiteral("EditorItemManagerMonitor"));
    mItemMonitor->ignoreSession(Akonadi::Session::defaultSession());
    mItemMonitor->itemFetchScope().fetchFullPayload();
    if (mItem.isValid()) {
        mItemMonitor->setItemMonitored(mItem);
    }

    //   q->connect(mItemMonitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)),
    //               SLOT(itemChanged(Akonadi::Item,QSet<QByteArray>)));
}

void ItemEditorPrivate::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers)
{
    Q_Q(EditorItemManager);
    if (mItemUi->containsPayloadIdentifiers(partIdentifiers)) {
        QPointer<QMessageBox> dlg = new QMessageBox; // krazy:exclude=qclasses
        dlg->setIcon(QMessageBox::Question);
        dlg->setInformativeText(
            i18n("The item has been changed by another application.\n"
                 "What should be done?"));
        dlg->addButton(i18n("Take over changes"), QMessageBox::AcceptRole);
        dlg->addButton(i18n("Ignore and Overwrite changes"), QMessageBox::RejectRole);

        if (dlg->exec() == QMessageBox::AcceptRole) {
            auto job = new Akonadi::ItemFetchJob(mItem);
            job->setFetchScope(mFetchScope);

            mItem = item;

            q->load(mItem);
        } else {
            mItem.setRevision(item.revision());
            q->save();
        }

        delete dlg;
    }

    // Overwrite or not, we need to update the revision and the remote id to be able
    // to store item later on.
    mItem.setRevision(item.revision());
}

/// ItemEditor

EditorItemManager::EditorItemManager(ItemEditorUi *ui, Akonadi::IncidenceChanger *changer)
    : d_ptr(new ItemEditorPrivate(changer, this))
{
    Q_D(ItemEditor);
    d->mItemUi = ui;
}

EditorItemManager::~EditorItemManager()
{
    delete d_ptr;
}

Akonadi::Item EditorItemManager::item(ItemState state) const
{
    Q_D(const ItemEditor);

    switch (state) {
    case EditorItemManager::AfterSave:
        if (d->mItem.hasPayload()) {
            return d->mItem;
        } else {
            qCDebug(INCIDENCEEDITOR_LOG) << "Won't return mItem because isValid = " << d->mItem.isValid() << "; and haPayload is " << d->mItem.hasPayload();
        }
        break;
    case EditorItemManager::BeforeSave:
        if (d->mPrevItem.hasPayload()) {
            return d->mPrevItem;
        } else {
            qCDebug(INCIDENCEEDITOR_LOG) << "Won't return mPrevItem because isValid = " << d->mPrevItem.isValid() << "; and haPayload is "
                                         << d->mPrevItem.hasPayload();
        }
        break;
    }
    qCDebug(INCIDENCEEDITOR_LOG) << "state = " << state;
    Q_ASSERT_X(false, "EditorItemManager::item", "Unknown enum value");
    return Akonadi::Item();
}

void EditorItemManager::load(const Akonadi::Item &item)
{
    Q_D(ItemEditor);

    // We fetch anyways to make sure we have everything required including tags
    auto job = new Akonadi::ItemFetchJob(item, this);
    job->setFetchScope(d->mFetchScope);
    connect(job, SIGNAL(result(KJob *)), SLOT(itemFetchResult(KJob *)));
}

void EditorItemManager::save()
{
    Q_D(ItemEditor);

    if (!d->mItemUi->isValid()) {
        Q_EMIT itemSaveFailed(d->mItem.isValid() ? Modify : Create, QString());
        return;
    }

    if (!d->mItemUi->isDirty() && d->mItemUi->selectedCollection() == d->mItem.parentCollection()) {
        // Item did not change and was not moved
        Q_EMIT itemSaveFinished(None);
        return;
    }

    d->mChanger->setGroupwareCommunication(CalendarSupport::KCalPrefs::instance()->useGroupwareCommunication());

    Akonadi::Item updateItem = d->mItemUi->save(d->mItem);
    Q_ASSERT(updateItem.id() == d->mItem.id());
    d->mItem = updateItem;

    if (d->mItem.isValid()) { // A valid item. Means we're modifying.
        Q_ASSERT(d->mItem.parentCollection().isValid());
        KCalendarCore::Incidence::Ptr oldPayload = CalendarSupport::incidence(d->mPrevItem);
        if (d->mItem.parentCollection() == d->mItemUi->selectedCollection() || d->mItem.storageCollectionId() == d->mItemUi->selectedCollection().id()) {
            (void) d->mChanger->modifyIncidence(d->mItem, oldPayload);
        } else {
            Q_ASSERT(d->mItemUi->selectedCollection().isValid());
            Q_ASSERT(d->mItem.parentCollection().isValid());

            qCDebug(INCIDENCEEDITOR_LOG) << "Moving from" << d->mItem.parentCollection().id() << "to" << d->mItemUi->selectedCollection().id();

            if (d->mItemUi->isDirty()) {
                (void) d->mChanger->modifyIncidence(d->mItem, oldPayload);
            } else {
                auto itemMoveJob = new Akonadi::ItemMoveJob(d->mItem, d->mItemUi->selectedCollection());
                connect(itemMoveJob, SIGNAL(result(KJob *)), SLOT(itemMoveResult(KJob *)));
            }
        }
    } else { // An invalid item. Means we're creating.
        if (d->mIsCounterProposal) {
            // We don't write back to akonadi, that will be done in ITipHandler.
            Q_EMIT itemSaveFinished(EditorItemManager::Modify);
        } else {
            Q_ASSERT(d->mItemUi->selectedCollection().isValid());
            (void) d->mChanger->createFromItem(d->mItem, d->mItemUi->selectedCollection());
        }
    }
}

void EditorItemManager::setIsCounterProposal(bool isCounterProposal)
{
    Q_D(ItemEditor);
    d->mIsCounterProposal = isCounterProposal;
}

ItemEditorUi::~ItemEditorUi()
{
}

bool ItemEditorUi::isValid() const
{
    return true;
}
} // namespace

#include "moc_editoritemmanager.cpp"
