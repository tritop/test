#include "baseservice.h"
#include "servicelistener.h"

#if defined(QT_WEBKIT_LIB)
#include <QtWebKit>
#  if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#    include <QtWebKitWidgets>
#  endif
#endif


#define MODULE_NAME "BaseService"

#define LOG_TRACE(FORMAT, ...)
#ifdef Q_CC_GNU
#define LOG_FUNCTION_NAME __PRETTY_FUNCTION__
#else
#define LOG_FUNCTION_NAME __FUNCTION__
#endif


BaseService::BaseService(const QString& serviceName)
    : m_serviceName(serviceName)
    , m_lastError(StatusOk)
#if defined(QT_WEBKIT_LIB)
	  , m_javaScriptObjectHash()
#endif

{

}

BaseService::~BaseService()
{
    foreach (QObject* jsObject, m_javaScriptObjectHash)
    {
        delete jsObject;
    }

}

QString BaseService::getName()
{
    LOG_TRACE(LOG_FUNCTION_NAME);
    return m_serviceName;
}

quint32 BaseService::getApiVersionNumber()
{
    LOG_TRACE(LOG_FUNCTION_NAME);
    return m_serviceApiVersion;
}

void BaseService::setApiVersionNumber(quint32 version)
{
    LOG_TRACE("%s(%u)", LOG_FUNCTION_NAME, version);
    m_serviceApiVersion = version;
}



void BaseService::setLastError(Status statusCode, const QString& statusMessage)
{
    m_lastError = statusCode;
    m_lastErrorDescription = statusMessage;
}

BaseService::Status BaseService::getLastError()
{
    return m_lastError;
}

QString BaseService::getLastErrorDescription()
{
    return m_lastErrorDescription;
}

bool BaseService::registerForEvents(QList<QString> eventNames, ServiceListener* listener)
{
    LOG_TRACE(LOG_FUNCTION_NAME);
    foreach(const QString& name, eventNames)
    {
        LOG_TRACE("\t%s", qPrintable(name));
        m_serviceListeners[name].append(listener);
    }
    return true;
}

bool BaseService::unregisterEvents(ServiceListener* listener)
{
    LOG_TRACE(LOG_FUNCTION_NAME);

    // http://qt-project.org/doc/qt-5.1/qtcore/qhash.html#erase
    QHash< QString, QList<ServiceListener*> >::Iterator begin = m_serviceListeners.begin();
    while (begin != m_serviceListeners.end())
    {
        QList<ServiceListener*> & listeners = begin.value();
        listeners.removeAll(listener);
        if (listeners.isEmpty())
            begin = m_serviceListeners.erase(begin);
        else
            ++begin;
    }
    return true;
}

bool BaseService::unregisterEvents(QList<QString> eventNames, ServiceListener* listener)
{
    LOG_TRACE(LOG_FUNCTION_NAME);
    foreach (const QString & name, eventNames)
    {
        QHash< QString, QList<ServiceListener *> >::Iterator itr = m_serviceListeners.find(name);
        if (itr != m_serviceListeners.end())
        {
            QList<ServiceListener*>& listeners = itr.value();
            listeners.removeAll(listener);
            if (listeners.isEmpty())
                m_serviceListeners.erase(itr);
        }
    }
    return true;
}


void BaseService::notifyEvent(const QString& eventName, const ServiceParams& serviceParameters)
{
    LOG_TRACE("%s: eventName=%s", LOG_FUNCTION_NAME, qPrintable(eventName));
    QHash< QString, QList<ServiceListener*> >::Iterator itr = m_serviceListeners.find(eventName);
    if (itr != m_serviceListeners.end())
    {
        foreach (ServiceListener* listener, itr.value())
        {
            listener->onServiceEvent(eventName, serviceParameters);
        }
    }
}

ServiceParams BaseService::callMethod(const QString& method, const ServiceParams& params)
{
	ServiceParams results;


    return results;
}

#if defined (QT_WEBKIT_LIB)
void BaseService::removeWebFrame(QWebFrame* webFrame)
{
    LOG_TRACE(LOG_FUNCTION_NAME);
    QHash<QWebFrame*, BaseServiceJavaScriptObject*>::Iterator itr = m_javaScriptObjectHash.find(webFrame);
    if (itr != m_javaScriptObjectHash.end())
    {
        delete itr.value();
        m_javaScriptObjectHash.erase(itr);
    }
}

QObject* BaseService::getJavaScriptObject(QWebFrame* webFrame)
{

    LOG_TRACE(LOG_FUNCTION_NAME);
    if (m_javaScriptObjectHash.keys().count() > 0 && m_javaScriptObjectHash.keys().contains(webFrame))
    {
        QObject *object =  m_javaScriptObjectHash[webFrame];
		return object;
    }
    return NULL;
}

BaseServiceJavaScriptObject::BaseServiceJavaScriptObject(BaseService* NewService, QWebFrame *webFrame)
    : m_Service(NewService), m_webFrame(webFrame)
{

    if (m_webFrame != NULL)
    {
        attachObject();
        connect( m_webFrame, SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(attachObject()) );
    }
}

void BaseServiceJavaScriptObject::attachObject()
{
    if (m_webFrame != NULL)
    {
        m_webFrame->addToJavaScriptWindowObject( m_Service->getName(), this );
    }
}


#endif




