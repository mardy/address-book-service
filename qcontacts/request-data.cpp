/*
 * Copyright 2013 Canonical Ltd.
 *
 * This file is part of contact-service-app.
 *
 * contact-service-app is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * contact-service-app is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "request-data.h"

#include <QtContacts/QContactManagerEngine>

using namespace QtContacts;
namespace galera
{

RequestData::RequestData(QContactAbstractRequest *request,
                         QDBusInterface *view,
                         const FetchHint &hint,
                         QDBusPendingCallWatcher *watcher)
    : m_offset(0),
      m_hint(hint)
{
    init(request, view, watcher);
}

RequestData::RequestData(QtContacts::QContactAbstractRequest *request,
                         QDBusPendingCallWatcher *watcher)
    : m_offset(0)
{
    init(request, 0, watcher);
}

RequestData::~RequestData()
{
    m_request.clear();
}

void RequestData::init(QtContacts::QContactAbstractRequest *request,
                       QDBusInterface *view,
                       QDBusPendingCallWatcher *watcher)
{
    m_request = request;

    if (view) {
        m_view = QSharedPointer<QDBusInterface>(view, RequestData::deleteView);
    }

    if (watcher) {
        m_watcher = QSharedPointer<QDBusPendingCallWatcher>(watcher, RequestData::deleteWatcher);
    }

}

QContactAbstractRequest* RequestData::request() const
{
    return m_request.data();
}

int RequestData::offset() const
{
    return m_offset;
}

bool RequestData::isLive() const
{
    return !m_request.isNull();
}

QDBusInterface* RequestData::view() const
{
    return m_view.data();
}

QStringList RequestData::fields() const
{
    return m_hint.fields();
}

void RequestData::updateWatcher(QDBusPendingCallWatcher *watcher)
{
    m_watcher.clear();
    if (watcher) {
        m_watcher = QSharedPointer<QDBusPendingCallWatcher>(watcher, RequestData::deleteWatcher);
    }
}

void RequestData::updateOffset(int offset)
{
    m_offset += offset;
}

QList<QContact> RequestData::result() const
{
    return m_result;
}

void RequestData::setError(QContactManager::Error error)
{
    m_result.clear();
    update(QContactAbstractRequest::FinishedState, error);
}

void RequestData::update(QList<QContact> result,
                         QContactAbstractRequest::State state,
                         QContactManager::Error error,
                         QMap<int, QContactManager::Error> errorMap)
{
    m_result += result;
    update(state, error, errorMap);
}

void RequestData::update(QContactAbstractRequest::State state,
                         QContactManager::Error error,
                         QMap<int, QContactManager::Error> errorMap)
{
    if (!isLive()) {
        return;
    }

    switch (m_request->type()) {
        case QContactAbstractRequest::ContactFetchRequest:
            QContactManagerEngine::updateContactFetchRequest(static_cast<QContactFetchRequest*>(m_request.data()),
                                                             m_result,
                                                             error,
                                                             state);
            break;
        case QContactAbstractRequest::ContactFetchByIdRequest:
            QContactManagerEngine::updateContactFetchByIdRequest(static_cast<QContactFetchByIdRequest*>(m_request.data()),
                                                                 m_result,
                                                                 error,
                                                                 errorMap,
                                                                 state);
            break;
        case QContactAbstractRequest::ContactSaveRequest:
            QContactManagerEngine::updateContactSaveRequest(static_cast<QContactSaveRequest*>(m_request.data()),
                                                            m_result,
                                                            error,
                                                            QMap<int, QContactManager::Error>(),
                                                            state);
        case QContactAbstractRequest::ContactRemoveRequest:
            QContactManagerEngine::updateContactRemoveRequest(static_cast<QContactRemoveRequest*>(m_request.data()),
                                                              error,
                                                              errorMap,
                                                              state);
            break;
        default:
            break;
    }
}


void RequestData::registerMetaType()
{
    qRegisterMetaType<galera::RequestData*>();
}

void RequestData::setError(QContactAbstractRequest *request, QContactManager::Error error)
{
    RequestData r(request);
    r.setError(error);
}

void RequestData::deleteView(QDBusInterface *view)
{
    if (view) {
        view->call("close");
        view->deleteLater();
    }
}

void RequestData::deleteWatcher(QDBusPendingCallWatcher *watcher)
{
    if (watcher) {
        watcher->deleteLater();
    }
}

} //namespace
