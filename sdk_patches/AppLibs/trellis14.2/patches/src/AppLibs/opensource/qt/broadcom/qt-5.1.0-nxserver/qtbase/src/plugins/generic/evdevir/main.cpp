/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui/qgenericplugin.h>
#include <QtPlatformSupport/private/qevdevkeyboardmanager_p.h>

#include <sys/time.h>
#include <qpa/qwindowsysteminterface.h>
#include <linux/input.h>

#include "nexus_platform.h"
#include "nexus_display.h"
#include "nexus_core_utils.h"
#include "nxclient.h"
#include "default_nexus.h"
#include "nexus_ir_input.h"

/* [11/18/2013]
 * Initial implementation has been done with NEXUS Input Router in RDK1.3-B3.9.
 * Now we move to NEXUS_IrInput implementation for the two reasons.
 * 1. Both RDK and Trellis don't use NEXUS Input Router.
 * 2. RDK (cpc) modifies nxserverlib_input.c to not enable NEXUS Input Router for XRE/IARM.
 *    Current nocpc build don't modify it.  Migrating to use Nexus_IrInput in nocpc can avoid
 *    this difference.
 *
 * [11/19/2013]
 * Added IARM support with 'evdevir_iarm' in evdevir.pro
 * Try to open NEXUS_IrInput first, if it fails (as IARM Manager already opened it),
 * then try to use IARM.
 *
 * [11/25/2013]
 * Added NEXUS Input Router back.  It tries to open NEXUS_IrInput then try NEXUS_InputClient.
 */
static NEXUS_IrInputHandle irHandle = 0;

#define EVDEVIR_NEXUS_INPUT_ROUTER 1
#if EVDEVIR_NEXUS_INPUT_ROUTER
#include "nexus_input_client.h"
static NEXUS_InputClientHandle icHandle = 0;
#endif

static void irCallback(void *pParam, int iParam)
{
    static struct timeval tv0 = {0, 0};
    struct timeval tv, tv_delta;

    while (1) {
        unsigned ir_code;
        unsigned num;
        int rc;
        if (irHandle)
        {
            bool overflow;
            NEXUS_IrInputEvent irEvent;
            rc = NEXUS_IrInput_GetEvents(irHandle, &irEvent, 1, &num, &overflow);
            ir_code = irEvent.code;
        }
#if EVDEVIR_NEXUS_INPUT_ROUTER
        else if (icHandle)
        {
            NEXUS_InputRouterCode code;
            rc = NEXUS_InputClient_GetCodes(icHandle, &code, 1, &num);
            if (code.deviceType == NEXUS_InputRouterDevice_eIrInput)
                ir_code = code.data.irInput.code;
            else
                break;
        }
#endif
        if (rc || !num) break;

        // Ignore pseudo IR event that follows.  This typically happen within 49034 usec.
        if (gettimeofday(&tv, NULL) == 0)
        {
            timersub(&tv, &tv0, &tv_delta);
            tv0 = tv;

            if (tv_delta.tv_sec == 0 && tv_delta.tv_usec < 70000)
                break;
        }
        printf("IR: %#x\n", ir_code);

        int key = 0;
        int nativecode = 0;
        int unicode = 65535;
        /* Map: IR code (Broadcom Silver Remote) to {Qt key, native code}
         *    Qt::Key_* is defined in qtbase/src/corelib/global/qnamespace.h
         *    KEY_* is defined in linux/input.h
         */
        switch (ir_code) {
#if 1	//HUMAX RM-L01
	case /* Up     */ 0xee111000:	key = Qt::Key_Up;			nativecode = 38;	break;
	case /* Down   */ 0xea151000:	key = Qt::Key_Down;			nativecode = 40;	break;
	case /* Right  */ 0xeb141000:	key = Qt::Key_Right;			nativecode = 39;	break;
	case /* Left   */ 0xed121000:	key = Qt::Key_Left;			nativecode = 37;	break;
	case /* OK     */ 0xec131000:	key = Qt::Key_Enter;			nativecode = 13;	break;
	                             	
	case /* 1      */ 0xfc031000:	key = Qt::Key_1;			nativecode = 49;	break;
	case /* 2      */ 0xfb041000:	key = Qt::Key_2;			nativecode = 50;	break;
	case /* 3      */ 0xfa051000:	key = Qt::Key_3;			nativecode = 51;	break;
	case /* 4      */ 0xf9061000:	key = Qt::Key_4;			nativecode = 52;	break;
	case /* 5      */ 0xf8071000:	key = Qt::Key_5;			nativecode = 53;	break;
	case /* 6      */ 0xf7081000:	key = Qt::Key_6;			nativecode = 54;	break;
	case /* 7      */ 0xf6091000:	key = Qt::Key_7;			nativecode = 55;	break;
	case /* 8      */ 0xf50a1000:	key = Qt::Key_8;			nativecode = 56;	break;
	case /* 9      */ 0xf40b1000:	key = Qt::Key_9;			nativecode = 57;	break;
	case /* 0      */ 0xf30c1000:	key = Qt::Key_0;			nativecode = 48;	break;

	case /* Menu   */ 0xf10e1000:key = Qt::Key_Home;			nativecode = 36;	break;
	case /* exit   */ 0xe9161000:	key = Qt::Key_Escape;		nativecode = 27;	break;
	case /* Clear  */ 0xbe411000:	key = 461;					nativecode = 461;		break;

 // [02014.08.14.][khyeo][rdk] :
	case /* Power  */ 0xff001000:key = 409;				nativecode = 409;	break; //VK_POWER
	case /* Guide  */ 0xe41b1000:	key = 458;				nativecode = 458;	break; //VK_GUIDE
	case /* Text   */ 0x916e1000:	key = 459;				nativecode = 459;	break; //VK_TELETEXT
	case /* Sub    */ 0xb9461000:	key = 460;				nativecode = 460;	break; //VK_SUBTITLE
	case /* Search */ 0xb8471000:key = 9008;				nativecode = 9008;	break;
	case /* +      */ 0xbd421000:	key = 82;				nativecode = 82;	break;
	case /* MUTE   */ 0xe7181000:key = 449;				nativecode = 449;	break; //VK_MUTE
	case /* WIDE   */ 0xb14e1000:key = 445;				nativecode = 445;	break; //VK_SCREEN_MODE_NEXT

	case /* >>     */ 0x9b641000:	key = 417;				nativecode = 417;	break; //VK_FAST_FWD
	case /* <<     */ 0x9a651000:	key = 412;				nativecode = 412;	break; //VK_REWIND
	case /* Pause  */ 0x9d621000:key = 19;				nativecode = 19;	break; //VK_PAUSE
	case /* Play   */ 0x97681000:	key = 415;				nativecode = 415;	break; //VK_PLAY
	case /* Stop   */ 0x9c631000:	key = 413;				nativecode = 413;	break; //VK_STOP
	case /* Record */ 0x9e611000:key = 416;				nativecode = 416;	break; //VK_RECORD

	case /* Vol Up */ 0xe01f1000:	key = 447;				nativecode = 447;	break; //VK_VOLUME_UP
	case /* Vol Dn */ 0xbf401000:	key = 448;				nativecode = 448;	break; //VK_VOLUME_DOWN
	case /* Ch Up  */ 0xef101000:key = 427;				nativecode = 427;	break; //VK_CHANNEL_UP
	case /* Ch Dn  */ 0xf00f1000:key = 428;				nativecode = 428;	break; //VK_CHANNEL_DOWN

	case /* RED    */ 0xe31c1000:	key = 403;				nativecode = 403;	break; //VK_RED
	case /* GREEN  */ 0xe21d1000:key = 404;				nativecode = 404;	break; //VK_GREEN
	case /* YELLOW */ 0xe51a1000:key = 405;				nativecode = 405;	break; //VK_YELLOW
	case /* BLUE   */ 0xe11e1000:	key = 406;				nativecode = 406;	break; //VK_BLUE
 // [02014.08.14.][khyeo][rdk] :
#if 0
	case /* Power  */ 0xff001000:	key = 297;				nativecode = 297;	break;
	case /* Guide  */ 0xe41b1000:	key = 260;				nativecode = 260;	break;
	case /* Text   */ 0x916e1000:	key = 459;				nativecode = 459;	break;
	case /* Sub    */ 0xb9461000:	key = 9002;				nativecode = 9002;	break;
	case /* Search */ 0xb8471000:	key = 9008;				nativecode = 9008;	break;
	case /* +      */ 0xbd421000:	key = 262;				nativecode = 262;	break;				                             	
	case /* MUTE   */ 0xe7181000:	key = 449;				nativecode = 449;	break;
	case /* WIDE   */ 0xb14e1000:	key = 445;				nativecode = 445;	break;

	case /* >>     */ 0x9b641000:	key = 473;				nativecode = 473;	break;
	case /* <<     */ 0x9a651000:	key = 412;				nativecode = 412;	break;
	case /* Pause  */ 0x9d621000:	key = 9011;				nativecode = 9011;	break;
	case /* Play   */ 0x97681000:	key = 415;				nativecode = 415;	break;
	case /* Stop   */ 0x9c631000:	key = 413;				nativecode = 413;	break;
	case /* Record */ 0x9e611000:	key = 9009;				nativecode = 9009;	break;                        	

	case /* Vol Up */ 0xe01f1000:	key = 503;				nativecode = 503;	break;
	case /* Vol Dn */ 0xbf401000:	key = 448;				nativecode = 448;	break;
	case /* Ch Up  */ 0xef101000:	key = 268;				nativecode = 268;	break;
	case /* Ch Dn  */ 0xf00f1000:	key = 269;				nativecode = 269;	break;

	case /* RED    */ 0xe31c1000:	key = 57893;				nativecode = 57893;	break;
	case /* GREEN  */ 0xe21d1000:	key = 57894;				nativecode = 57894;	break;
	case /* YELLOW */ 0xe51a1000:	key = 57895;				nativecode = 57895;	break;
	case /* BLUE   */ 0xe11e1000:	key = 57896;				nativecode = 57896;	break;
#endif
#else
        case /* Up    */ 0xb14eff00:    key = Qt::Key_Up;       nativecode = KEY_UP;    break;
        case /* Down  */ 0xf30cff00:    key = Qt::Key_Down;     nativecode = KEY_DOWN;  break;
        case /* Right */ 0xb649ff00:    key = Qt::Key_Right;    nativecode = KEY_RIGHT; break;
        case /* Left  */ 0xf40bff00:    key = Qt::Key_Left;     nativecode = KEY_LEFT;  break;
        case /* OK    */ 0xf708ff00:    key = Qt::Key_Enter;    nativecode = KEY_ENTER; break;
            
        case /* Menu  */ 0xb04fff00:    key = Qt::Key_Home;     nativecode = KEY_HOME;  break;
        case /* Guide */ 0xf10eff00:    key = Qt::Key_Escape;   nativecode = KEY_ESC;   break;
        case /* Clear */ 0xb24dff00:    key = Qt::Key_F1;       nativecode = KEY_F1;    break;
        case /* >>    */ 0xa659ff00:    key = Qt::Key_F2;       nativecode = KEY_F2;    break;
        case /* <<    */ 0xe619ff00:    key = Qt::Key_F3;       nativecode = KEY_F3;    break;
        case /* Pause */ 0xe31cff00:    key = Qt::Key_F4;       nativecode = KEY_F4;    break;
        case /* Play  */ 0xe21dff00:    key = Qt::Key_F5;       nativecode = KEY_F5;    break;
        case /* Ch+   */ 0xf609ff00:    key = Qt::Key_PageUp;   nativecode = KEY_PAGEUP;   break;
        case /* Ch-   */ 0xf20dff00:    key = Qt::Key_PageDown; nativecode = KEY_PAGEDOWN; break;
#endif
        }

        if (key != 0)
        {
            QWindowSystemInterface::handleExtendedKeyEvent(0, QEvent::KeyPress,
                                                           key, Qt::NoModifier, nativecode + 8, 0, int(Qt::NoModifier),
                                                           QString(unicode), false);

            QWindowSystemInterface::handleExtendedKeyEvent(0, QEvent::KeyRelease,
                                                           key, Qt::NoModifier, nativecode + 8, 0, int(Qt::NoModifier),
                                                           QString(unicode), false);
        }
    }    
}

static int enable_nexus_ir(void)
{
    NEXUS_IrInputSettings irSettings;
    int rc = NxClient_Join(NULL);
    if (rc) {
        printf("cannot join: %d\n", rc);
        return -1;
    }

    NEXUS_IrInput_GetDefaultSettings(&irSettings);
    //irSettings.mode = NEXUS_IrInputMode_eRemoteA;	// Motorola/GI type IR
    irSettings.mode = NEXUS_IrInputMode_eCirNec;	// Broadcom Silver IR
    irSettings.dataReady.callback = irCallback;
    irSettings.dataReady.context = &irHandle;
    irHandle = NEXUS_IrInput_Open(0, &irSettings);
    if (irHandle)
    {
        printf("Enabled IR with Nexus. handle=0x%08x\n", irHandle);
        return 0;
    }

#if EVDEVIR_NEXUS_INPUT_ROUTER
    NxClient_AllocSettings allocSettings;
    NxClient_AllocResults allocResults;

    NxClient_GetDefaultAllocSettings(&allocSettings);
    allocSettings.inputClient = 1;
    NxClient_Alloc(&allocSettings, &allocResults);
    if (allocResults.inputClient[0].id)
        icHandle = NEXUS_InputClient_Acquire(allocResults.inputClient[0].id);

    if (icHandle)
    {
        printf("Enabled IR with Nexus Inpur Router. handle=0x%08x\n", icHandle);
        NEXUS_InputClientSettings settings;
        NEXUS_InputClient_GetSettings(icHandle, &settings);
        settings.filterMask = 0xFFFFFFFF; // everything
        settings.codeAvailable.callback = irCallback;
        settings.codeAvailable.context = NULL;
        NEXUS_InputClient_SetSettings(icHandle, &settings);
        return 0;
    }
#endif

    return -1;
}


#if EVDEVIR_IARM

extern "C" {
#include "libIBus.h"
#include "libIBusDaemon.h"
#include "irMgr.h"
}

static void _evtHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
    if (strcmp(owner, IARM_BUS_IRMGR_NAME)  == 0) {
        if (eventId == IARM_BUS_IRMGR_EVENT_IRKEY)
        {
            IARM_Bus_IRMgr_EventData_t *irEventData = (IARM_Bus_IRMgr_EventData_t*)data;
            int keyCode = irEventData->data.irkey.keyCode;
            int keyType = irEventData->data.irkey.keyType;
            printf("IR: %#x\n", keyCode);

            int key = 0;
            int nativecode = 0;
            int unicode = 65535;
            switch (keyCode) {
            case /* Up    */ 0x81:    key = Qt::Key_Up;       nativecode = KEY_UP;    break;
            case /* Down  */ 0x82:    key = Qt::Key_Down;     nativecode = KEY_DOWN;  break;
            case /* Right */ 0x84:    key = Qt::Key_Right;    nativecode = KEY_RIGHT; break;
            case /* Left  */ 0x83:    key = Qt::Key_Left;     nativecode = KEY_LEFT;  break;
            case /* OK    */ 0x85:    key = Qt::Key_Enter;    nativecode = KEY_ENTER; break;

            case /* Menu  */ 0xC0:    key = Qt::Key_Home;     nativecode = KEY_HOME;  break;
            case /* Guide */ 0x8D:    key = Qt::Key_Escape;   nativecode = KEY_ESC;   break;
            case /* Clear */ 0x87:    key = Qt::Key_F1;       nativecode = KEY_F1;    break;
            case /* >>    */ 0x98:    key = Qt::Key_F2;       nativecode = KEY_F2;    break;
            case /* <<    */ 0x97:    key = Qt::Key_F3;       nativecode = KEY_F3;    break;
            case /* Pause */ 0x9b:    key = Qt::Key_F4;       nativecode = KEY_F4;    break;
            }

            if (key)
            {
                if (keyType == 0x8000)
                    QWindowSystemInterface::handleExtendedKeyEvent(0, QEvent::KeyPress,
                                                                   key, Qt::NoModifier, nativecode + 8, 0, int(Qt::NoModifier),
                                                                   QString(unicode), false);
                else if (keyType == 0x8100)
                    QWindowSystemInterface::handleExtendedKeyEvent(0, QEvent::KeyRelease,
                                                                   key, Qt::NoModifier, nativecode + 8, 0, int(Qt::NoModifier),
                                                                   QString(unicode), false);
            }
        }
    }
}

static void enable_iarm_ir(void)
{
    printf("Enabling IR with IARM\n");
    IARM_Bus_Init("XRE_native_receiver");
    IARM_Bus_Connect();
    IARM_Bus_RegisterEventHandler(IARM_BUS_IRMGR_NAME, IARM_BUS_IRMGR_EVENT_IRKEY, _evtHandler);
    IARM_BusDaemon_RequestOwnership(IARM_BUS_RESOURCE_FOCUS);

}

static void disable_iarm_ir(void)
{
    IARM_BusDaemon_ReleaseOwnership(IARM_BUS_RESOURCE_FOCUS);
    IARM_Bus_Disconnect();
    IARM_Bus_Term();
}
#endif // EVDEVIR_IARM


QT_BEGIN_NAMESPACE

class QEvdevIrPlugin : public QGenericPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QGenericPluginFactoryInterface" FILE "evdevir.json")

public:
    QEvdevIrPlugin();

    QObject* create(const QString &key, const QString &specification);
};

QEvdevIrPlugin::QEvdevIrPlugin()
    : QGenericPlugin()
{
}

QObject* QEvdevIrPlugin::create(const QString &key, const QString &specification)
{
    if (!key.compare(QLatin1String("EvdevIr"), Qt::CaseInsensitive))
    {
        int rc = enable_nexus_ir();
#if EVDEVIR_IARM
        if (rc)
            enable_iarm_ir();
#endif
    }
    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
