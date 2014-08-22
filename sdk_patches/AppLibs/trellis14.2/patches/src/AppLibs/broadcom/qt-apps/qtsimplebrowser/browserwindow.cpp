/******************************************************************************
 *   Copyright 2013 Broadcom Corporation
 *
 * This program is the proprietary software of Broadcom Corporation and/or its
 * licensors, and may only be used, duplicated, modified or distributed
 * pursuant to the terms and conditions of a separate, written license
 * agreement executed between you and Broadcom (an "Authorized License").
 * Except as set forth in an Authorized License, Broadcom grants no license
 * (express or implied), right to use, or waiver of any kind with respect to
 * the Software, and Broadcom expressly reserves all rights in and to the
 * Software and all intellectual property rights therein.  IF YOU HAVE NO
 * AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY WAY,
 * AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF THE
 * SOFTWARE.
 *
 * Except as expressly set forth in the Authorized License,
 *
 * 1.     This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use all
 * reasonable efforts to protect the confidentiality thereof, and to use this
 * information only in connection with your use of Broadcom integrated circuit
 * products.
 *
 * 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 * "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS
 * OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
 * RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND ALL
 * IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR
 * A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR COMPLETENESS, QUIET
 * ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE
 * ENTIRE RISK ARISING OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 * 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 * ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
 * INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY
 * RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 * HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN
 * EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR U.S. $1,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY
 * FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *****************************************************************************/

//#include "config.h"

#include "browserwindow.h"
#include "cookiejar.h"

#include <QLayout>
#if !defined(QT_NO_NETWORKDISKCACHE) && !defined(QT_NO_DESKTOPSERVICES)
#include <QStandardPaths>
#include <QtNetwork/QNetworkDiskCache>
#endif

#include <QWebSecurityOrigin>
#include "servicemanager.h"
#include "services/devicesettingservice.h"

#include "humax/newservice.h"


extern BrowserOptions browserOptions;

static TestBrowserCookieJar* testBrowserCookieJarInstance()
{
    static TestBrowserCookieJar* cookieJar = new TestBrowserCookieJar(qApp);
    return cookieJar;
}

BrowserWindow::BrowserWindow(BrowserOptions* data)
    : QWidget()
    , m_view(0)
    , m_page(0)
{

    if (data)
        m_windowOptions = *data;

    init();
}

BrowserWindow::~BrowserWindow()
{
}

void BrowserWindow::init()
{
    resize(1280, 720);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);

    if (browserOptions.startMaximized)
        toggleFullScreenMode(true);

    initializeView();
}

void BrowserWindow::initializeView()
{
    if (m_view) {
        delete m_view;
        m_view = 0;
    }

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->setMargin(0);
    setLayout(mainLayout);

    WebPage* webPage = new WebPage(this);
    setPage(webPage);
    setDiskCache(m_windowOptions.useDiskCache);
    setUseDiskCookies(m_windowOptions.useDiskCookies);

    // We reuse the same cookieJar on multiple QNAMs, which is OK.
    QObject* cookieJarParent = testBrowserCookieJarInstance()->parent();
    page()->networkAccessManager()->setCookieJar(testBrowserCookieJarInstance());
    testBrowserCookieJarInstance()->setParent(cookieJarParent);

    if (!m_windowOptions.useGraphicsView) {
        WebViewTraditional* view = new WebViewTraditional(this);
        mainLayout->addWidget((QWidget*)view, 0, 0);
        m_view = view;
        view->setPage(page());
    } else {
        WebViewGraphicsBased* view = new WebViewGraphicsBased(this);
        mainLayout->addWidget((QWidget*)view, 0, 0);
        m_view = view;

#ifndef QT_NO_OPENGL
        bool enable = m_windowOptions.useQGLWidgetViewport;
        if (enable && isGraphicsBased()) {
            WebViewGraphicsBased* view = static_cast<WebViewGraphicsBased*>(m_view);
            view->setViewport(enable ? new QGLWidget() : 0);
        }
#endif
        view->setPage(page());
        connect(view, SIGNAL(currentFPSUpdated(int)), this, SLOT(updateFPS(int)));
    }

    connect(page(), SIGNAL(loadStarted()), this, SLOT(loadStarted()));
    connect(page(), SIGNAL(loadFinished(bool)), this, SLOT(loadFinished()));
    connect(this, SIGNAL(enteredFullScreenMode(bool)), this, SLOT(toggleFullScreenMode(bool)));

    applyPrefs();
}

void BrowserWindow::applyPrefs()
{
    QWebSettings* settings = page()->settings();
    settings->setAttribute(QWebSettings::AcceleratedCompositingEnabled, m_windowOptions.useCompositing);
    settings->setAttribute(QWebSettings::TiledBackingStoreEnabled, m_windowOptions.useTiledBackingStore);
    settings->setAttribute(QWebSettings::FrameFlatteningEnabled, m_windowOptions.useFrameFlattening);
    settings->setAttribute(QWebSettings::WebGLEnabled, m_windowOptions.useWebGL);
    settings->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, true); // [02014.08.08.][khyeo][rdk] :
   // qDebug("BrowserWindow::applyPrefs");

   registerServices();

    if (!isGraphicsBased())
        return;

    WebViewGraphicsBased* view = static_cast<WebViewGraphicsBased*>(m_view);
    view->setViewportUpdateMode(m_windowOptions.viewportUpdateMode);
    view->setFrameRateMeasurementEnabled(m_windowOptions.showFrameRate);
    view->setItemCacheMode(m_windowOptions.cacheWebView ? QGraphicsItem::DeviceCoordinateCache : QGraphicsItem::NoCache);

 // [02014.08.08.][khyeo][rdk] :
	QWebFrame *frame = page()->mainFrame();
 	if(frame)
 		{
 				QWebSecurityOrigin security = frame->securityOrigin();
		//if(security)
			{
				//QString original = security.scheme();
				//qDebug() << "BrowserWindow::applyPrefs original scheme : " << original;
				//security.addAccessWhitelistEntry("http","test.humaxtvportal.com",SubdomainSetting::AllowSubdomains);
				security.addLocalScheme("http");
			}
		}
    if (m_windowOptions.resizesToContents)
        toggleResizesToContents(m_windowOptions.resizesToContents);
}

void BrowserWindow::registerServices(void)
{

	qDebug("BrowserWindow::registerServices");
#if 0 //def USE_DISPLAY_SETTINGS
       ServiceStruct serviceStruct;
       serviceStruct.createFunction = &createDisplaySettingsService;
       serviceStruct.serviceName = DISPLAY_SETTINGS_SERVICE_NAME;
       ServiceManager::getInstance()->registerService(DISPLAY_SETTINGS_SERVICE_NAME, serviceStruct);
#endif

#if 0
       ServiceStruct hnServiceStruct;
       hnServiceStruct.createFunction = &createHomeNetworkingService;
       hnServiceStruct.serviceName = HOME_NETWORKING_SERVICE_NAME;
       ServiceManager::getInstance()->registerService(HOME_NETWORKING_SERVICE_NAME, hnServiceStruct);

       ServiceStruct scrCapServiceStruct;
       scrCapServiceStruct.createFunction = &ScreenCaptureService::create;
       scrCapServiceStruct.serviceName = ScreenCaptureService::NAME;
       ServiceManager::getInstance()->registerService(ScreenCaptureService::NAME, scrCapServiceStruct);
#endif
       /*ServiceStruct deviceServiceStruct;
       deviceServiceStruct.createFunction = &createDeviceSettingService;
       deviceServiceStruct.serviceName = DEVICE_SETTING_SERVICE_NAME;
       ServiceManager::getInstance()->registerService(DEVICE_SETTING_SERVICE_NAME, deviceServiceStruct);*/

	ServiceStruct newServiceStruct;
	newServiceStruct.createFunction = &createNewService;
	newServiceStruct.serviceName = NEW_SERVICE_NAME;
	ServiceManager::getInstance()->registerService(NEW_SERVICE_NAME, newServiceStruct);
	if (!m_windowOptions.useGraphicsView)
	{
		 QWebView* view = static_cast<QWebView*>(m_view);
		ServiceManager::getInstance()->addWebView(view);
		//ServiceManager::getInstance()->setGraphicsView(this);
	}
	else
	{
		WebViewGraphicsBased *graphic = static_cast<WebViewGraphicsBased*>(m_view);
		 QGraphicsWebView* webview = static_cast<QGraphicsWebView*>(graphic->graphicsWebView());
		ServiceManager::getInstance()->addGraphicsWebView(webview);
		QGraphicsView *view = static_cast<QGraphicsView*>(m_view);
		ServiceManager::getInstance()->setGraphicsView(view);
	}

}

void BrowserWindow::setPage(WebPage* page)
{
    if (page && m_page)
    {
        if (browserOptions.userAgentForUrl.isEmpty())
            page->setUserAgent(m_page->userAgentForUrl(QUrl()));
        else
            page->setUserAgent(browserOptions.userAgentForUrl);
    }

//Crash on Linux
//    if (m_page)
//        delete m_page;
    m_page = page;
}

WebPage* BrowserWindow::page() const
{
    return m_page;
}

void BrowserWindow::load(const QUrl& url)
{
    if (!url.isValid())
        return;
    page()->mainFrame()->load(url);
}

bool BrowserWindow::isGraphicsBased() const
{
    return bool(qobject_cast<QGraphicsView*>(m_view));
}

bool BrowserWindow::isGLWidgetBased() const
{
    bool ret = false;
    if (isGraphicsBased())  {
        QGraphicsView* view = static_cast<QGraphicsView*>(m_view);
        ret = qobject_cast<QGLWidget*>(view->viewport());
    }
    return ret;
}

void BrowserWindow::loadStarted()
{
    m_view->setFocus(Qt::OtherFocusReason);
	qDebug("BrowserWindow::loadStarted");
}

void BrowserWindow::loadFinished()
{
    qDebug("BrowserWindow::loadFinished");
}

void BrowserWindow::dumpHtml()
{
    qDebug("HTML: %s", qPrintable(m_page->mainFrame()->toHtml()));
}

void BrowserWindow::setDiskCache(bool enable)
{
#if !defined(QT_NO_NETWORKDISKCACHE) && !defined(QT_NO_DESKTOPSERVICES)
    m_windowOptions.useDiskCache = enable;
    QNetworkDiskCache* cache = 0;
    if (enable) {
        cache = new QNetworkDiskCache();
        QString cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        cache->setCacheDirectory(cacheLocation);
    }
    page()->networkAccessManager()->setCache(cache);
#endif
}

void BrowserWindow::toggleWebView(bool graphicsBased)
{
    m_windowOptions.useGraphicsView = graphicsBased;
    initializeView();
}

void BrowserWindow::toggleAcceleratedCompositing(bool toggle)
{
    m_windowOptions.useCompositing = toggle;
    page()->settings()->setAttribute(QWebSettings::AcceleratedCompositingEnabled, toggle);
}

void BrowserWindow::toggleTiledBackingStore(bool toggle)
{
    page()->settings()->setAttribute(QWebSettings::TiledBackingStoreEnabled, toggle);
}

void BrowserWindow::toggleResizesToContents(bool toggle)
{
    m_windowOptions.resizesToContents = toggle;
    static_cast<WebViewGraphicsBased*>(m_view)->setResizesToContents(toggle);
}

void BrowserWindow::toggleWebGL(bool toggle)
{
    m_windowOptions.useWebGL = toggle;
    page()->settings()->setAttribute(QWebSettings::WebGLEnabled, toggle);
}

void BrowserWindow::toggleSpatialNavigation(bool b)
{
    page()->settings()->setAttribute(QWebSettings::SpatialNavigationEnabled, b);
}

void BrowserWindow::toggleFullScreenMode(bool enable)
{
    bool alreadyEnabled = windowState() & Qt::WindowFullScreen;
    if (enable ^ alreadyEnabled)
        setWindowState(windowState() ^ Qt::WindowFullScreen);
}

void BrowserWindow::toggleFrameFlattening(bool toggle)
{
    m_windowOptions.useFrameFlattening = toggle;
    page()->settings()->setAttribute(QWebSettings::FrameFlatteningEnabled, toggle);
}

void BrowserWindow::toggleInterruptingJavaScriptEnabled(bool enable)
{
    page()->setInterruptingJavaScriptEnabled(enable);
}

void BrowserWindow::toggleJavascriptCanOpenWindows(bool enable)
{
    page()->settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, enable);
}

void BrowserWindow::setUseDiskCookies(bool enable)
{
    testBrowserCookieJarInstance()->setDiskStorageEnabled(enable);
}

void BrowserWindow::clearCookies()
{
    testBrowserCookieJarInstance()->reset();
}

void BrowserWindow::toggleAutoLoadImages(bool enable)
{
    page()->settings()->setAttribute(QWebSettings::AutoLoadImages, !enable);
}

void BrowserWindow::togglePlugins(bool enable)
{
    page()->settings()->setAttribute(QWebSettings::PluginsEnabled, !enable);
}

void BrowserWindow::changeViewportUpdateMode(int mode)
{
    m_windowOptions.viewportUpdateMode = QGraphicsView::ViewportUpdateMode(mode);

    if (!isGraphicsBased())
        return;

    WebViewGraphicsBased* view = static_cast<WebViewGraphicsBased*>(m_view);
    view->setViewportUpdateMode(m_windowOptions.viewportUpdateMode);
}

void BrowserWindow::showFPS(bool enable)
{
    if (!isGraphicsBased())
        return;

    m_windowOptions.showFrameRate = enable;
    WebViewGraphicsBased* view = static_cast<WebViewGraphicsBased*>(m_view);
    view->setFrameRateMeasurementEnabled(enable);
}

void BrowserWindow::toggleLocalStorage(bool toggle)
{
    m_windowOptions.useLocalStorage = toggle;
    page()->settings()->setAttribute(QWebSettings::LocalStorageEnabled, toggle);
}

void BrowserWindow::toggleOfflineStorageDatabase(bool toggle)
{
    m_windowOptions.useOfflineStorageDatabase = toggle;
    page()->settings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, toggle);
}

void BrowserWindow::toggleOfflineWebApplicationCache(bool toggle)
{
    m_windowOptions.useOfflineWebApplicationCache = toggle;
    page()->settings()->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, toggle);
}

void BrowserWindow::setOfflineStorageDefaultQuota()
{
    // For command line execution, quota size is taken from command line.
    if (m_windowOptions.offlineStorageDefaultQuotaSize)
        page()->settings()->setOfflineStorageDefaultQuota(m_windowOptions.offlineStorageDefaultQuotaSize);
}

void BrowserWindow::toggleScrollAnimator(bool toggle)
{
    m_windowOptions.enableScrollAnimator = toggle;
    page()->settings()->setAttribute(QWebSettings::ScrollAnimatorEnabled, toggle);
}

void BrowserWindow::updateFPS(int fps)
{
    QString fpsStatusText = QString("Current FPS: %1").arg(fps);
    qDebug("%s", qPrintable(fpsStatusText));
}

void BrowserWindow::printSettings()
{
    qDebug("-graphicsbased       : WebPage Graphics based - %s", isGraphicsBased() ? "enabled" : "disabled");
    qDebug("-tiled-backing-store : Tiled backing store - %s", page()->settings()->testAttribute(QWebSettings::TiledBackingStoreEnabled) ? "enabled" : "disabled");
}

