#include "newservice.h"

#include <QDebug>

#if defined(QT_WEBKIT_LIB)
#include <QtWebKit>
#  if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#    include <QtWebKitWidgets>
#  endif
#endif


const QString METHOD_LOADED = "onLoaded";
const QString PARAM_STATUS = "status";
const QString PARAM_MESSGAGE = "message";

//method
const QString GET_SERVICE = "getservice";
const QString METHOD_NEW_METHOD = "ServiceMethod";

//properties
const QString NUM_RESULT ="num_result";



NewService::NewService(const QString& serviceName)
	:BaseService(serviceName)
{
	num_result = 0;
}

NewService::~NewService()
{

}

Service* createNewService()
{
//	qDebug()<<"createNewService";
	return new NewService(NEW_SERVICE_NAME);
}

#if defined (QT_WEBKIT_LIB)
void NewService::addWebFrame(QWebFrame* webFrame)
{
	qDebug()<<"NewService::addWebFrame";
    if (!m_javaScriptObjectHash.contains(webFrame))
    {
    	NewServiceJavascriptObject *object = new NewServiceJavascriptObject(this, webFrame);
        m_javaScriptObjectHash[webFrame] = static_cast<BaseServiceJavaScriptObject *>(object);
    }
}
#endif

bool NewService::registerForEvents(QList<QString> eventNames, ServiceListener* listener)
{
	return true;
}
bool NewService::unregisterEvents(ServiceListener* listener)
{
	return true;

}
bool NewService::unregisterEvents(QList<QString> eventNames, ServiceListener* listener)
{
	return true;

}
bool NewService::getProperties(const QVariantList& propertyNames, ServiceParams& returnProperties)
{
	QString property = propertyNames.first().toString();

	if(property.compare(NUM_RESULT,	Qt::CaseInsensitive) ==0)
	{
		 returnProperties[NUM_RESULT] = num_result;
		 return true;
	}
	else
	{
		return false;
	}
}

void NewService::notifyResult(const QString& eventName, ServiceParams params)
{
#if defined(QT_WEBKIT_LIB)
    if (m_javaScriptObjectHash.keys().count() > 0)
    {
        QHash<QWebFrame*, BaseServiceJavaScriptObject*>::iterator i;
        for (i = m_javaScriptObjectHash.begin(); i != m_javaScriptObjectHash.end(); i++)
        {
            if (i.value() != NULL)
            {
                NewServiceJavascriptObject* javaScriptObject = static_cast<NewServiceJavascriptObject *>(i.value());
                javaScriptObject->onEvent(eventName, params);
            }
        }
    }
#endif
}

bool NewService::setProperties(const ServiceParams& params)
{
	Q_UNUSED(params);

	return true;

}

ServiceParams NewService::callMethod(const QString& method,const ServiceParams& params)
{
	qDebug()<<"NewService::callMethod";

	ServiceParams returnResult;
	//Venu Begin
	if( method == METHOD_NEW_METHOD)
	{
		QString methodType;
		QVariantList list = params["params"].toList();

		if(list.length()>0)
		  methodType = list.first().toString();

		if(methodType.compare(GET_SERVICE,	Qt::CaseInsensitive) ==0)
			 returnResult[method] = QString("rdk20");

		ServiceParams eventParmas;
		QString message = "event";

		eventParmas[PARAM_STATUS] = QVariant(true);
		eventParmas[PARAM_MESSGAGE] = QVariant(message);

		notifyResult(METHOD_LOADED,eventParmas);

	}
	return returnResult;
}


QString NewService::ServiceMethod(QString info)
{
	qDebug()<<"NewService::method";

	QString methodName = METHOD_NEW_METHOD;
	ServiceParams params;
	QVariantList list;
	list.append(info);
	params["params"] = list;
	ServiceParams result = callMethod(methodName, params);
	return result[methodName].toString();
}

int NewService::doSums(int i, int j)
{
	num_result = i+j;
	return num_result;
}



#if defined(QT_WEBKIT_LIB)

NewServiceJavascriptObject::NewServiceJavascriptObject(NewService* NewService, QWebFrame *webFrame)
	:BaseServiceJavaScriptObject(NewService, webFrame)
{
}

QString NewServiceJavascriptObject::ServiceMethod(QString info)
{
    if (m_Service != NULL)
    {
    	NewService *service = dynamic_cast<NewService*>(m_Service);
        return  service->ServiceMethod(info);
    }
    return "unknown";
}

int NewServiceJavascriptObject::doSums( int a, int b )
{
	 if (m_Service != NULL)
    {
    	NewService *service = dynamic_cast<NewService*>(m_Service);
        return service->doSums(a, b);
    }
	return 0;
}

int NewServiceJavascriptObject::getNumResult() const
{
    if (m_Service != NULL)
    {
    	QVariantList list;
		ServiceParams result;
		list.append(NUM_RESULT);

        if(m_Service->getProperties(list,result) == true)
       	{
			return result[NUM_RESULT].toInt();
       	}
		else
			return 0;
    }

}
void NewServiceJavascriptObject::onEvent(const QString& eventName, ServiceParams params)
{
	// Let listeners in JS code know upload has finished.
	bool ok = params[PARAM_STATUS].toBool();
	QString errorMsg = params[PARAM_MESSGAGE].toString();
	emit onLoaded(ok, errorMsg);

}

#endif




