#ifndef NEWSERVICE_H
#define NEWSERVICE_H

#include "baseservice.h"

#include <QObject>
#include <QSettings>

const QString NEW_SERVICE_NAME = "NewService";



class NewService: public BaseService
{
Q_OBJECT
public:
	NewService(const QString& serviceName);
	~NewService();

#if defined (QT_WEBKIT_LIB)
    virtual void addWebFrame(QWebFrame* webFrame);
#endif
	virtual bool setProperties(const ServiceParams& params) ;
	virtual bool getProperties(const QVariantList& propertyNames, ServiceParams& returnProperties) ;
	virtual ServiceParams callMethod(const QString& method,const ServiceParams& params) ;
	virtual bool registerForEvents(QList<QString> eventNames, ServiceListener* listener) ;
	virtual bool unregisterEvents(ServiceListener* listener) ;
	virtual bool unregisterEvents(QList<QString> eventNames, ServiceListener* listener) ;


	QString ServiceMethod(QString info);

	int doSums(int i, int j);

	void notifyResult(const QString& eventName, ServiceParams params);
private:

	int num_result;

};


/**
 * @brief create service class object
 *
 *  *
 * @param None
 * @return  Service object
 * @retval   Service *
 */
Service* createNewService();


#if defined(QT_WEBKIT_LIB)
class NewServiceJavascriptObject : public BaseServiceJavaScriptObject
{
Q_OBJECT
Q_PROPERTY(int num_result READ getNumResult)
public:
	NewServiceJavascriptObject(NewService* newService, QWebFrame* webFrame);

public :
	int getNumResult() const;
    void onEvent(const QString& eventName, ServiceParams params);

public slots:
	QString ServiceMethod(QString info);
	int doSums( int a, int b );

signals:
	void onLoaded(bool ok, const QString& msg);


};
#endif //QT_WEBKIT_LIB

#endif // NEWSERVICE_H


