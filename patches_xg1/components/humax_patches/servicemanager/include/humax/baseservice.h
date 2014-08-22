#ifndef BASE_SERVICE_H
#define BASE_SERVICE_H

#include "service.h"

#include <QSettings>
#include <QObject>
#include <QHash>
#include <QVariantList>


#if defined(QT_WEBKIT_LIB)
class QGraphicsWebView;
class QWebView;
class QWebFrame;
class BaseServiceJavaScriptObject;
#endif


class BaseService : public QObject, public Service
{

Q_OBJECT
public:

    BaseService(const QString& serviceName);
	virtual ~BaseService();

    enum Status
    {
        StatusOk,
        StatusFailure,
        StatusInvalidArgument,
        StatusInvalidState,
        StatusMethodNotFound
    };


    QString getName();
    quint32 getApiVersionNumber();

    void setApiVersionNumber(quint32 version);

    virtual bool setProperties(const ServiceParams& params) =0;
    virtual bool getProperties(const QVariantList& propertyNames, ServiceParams& propertyValues)=0;

    virtual bool registerForEvents(QList<QString> eventNames, ServiceListener* listener);
    virtual bool unregisterEvents(ServiceListener* listener);
    virtual bool unregisterEvents(QList<QString> eventNames, ServiceListener* listener);

    virtual ServiceParams callMethod(const QString& method, const ServiceParams& params);


#if defined (QT_WEBKIT_LIB)
    virtual void addWebFrame(QWebFrame* webFrame)=0;
    void removeWebFrame(QWebFrame* webFrame);
    QObject* getJavaScriptObject(QWebFrame* webFrame);
#endif

    void setLastError(Status statusCode, const QString& statusMessage = QString());
    Status getLastError();
    QString getLastErrorDescription();

protected:

    void notifyEvent(const QString& eventName, const ServiceParams& serviceParameters);


protected:
    QString m_serviceName;
    quint32 m_serviceApiVersion;
    Status  m_lastError;
    QString m_lastErrorDescription;

    QHash< QString, QList<ServiceListener*> > m_serviceListeners;


#if  defined (QT_WEBKIT_LIB)
    QHash<QWebFrame*, BaseServiceJavaScriptObject*> m_javaScriptObjectHash;
#endif

};



#if defined(QT_WEBKIT_LIB)
class BaseServiceJavaScriptObject : public QObject
{
Q_OBJECT
public:
	BaseServiceJavaScriptObject(BaseService* baseService, QWebFrame* webFrame);

protected slots:
	void attachObject();

protected :
	BaseService *m_Service;
	QWebFrame* m_webFrame;
};
#endif //defined(QT_WEBKIT_LIB)

#endif //BASE_SERVICE_H

