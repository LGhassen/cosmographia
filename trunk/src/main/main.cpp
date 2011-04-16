/*
 * $Revision: 224 $ $Date: 2010-03-30 05:50:58 -0700 (Tue, 30 Mar 2010) $
 *
 * Copyright by Astos Solutions GmbH, Germany
 *
 * This file is published under the Astos Solutions Free Public License.
 * For details on copyright and terms of use see 
 * http://www.astos.de/Astos_Solutions_Free_Public_License.html
 */

#include <QApplication>
#include <QGLFormat>
#include <QDir>
#include <QMessageBox>
#include <QDebug>

#ifdef Q_WS_MAC
#include <CoreFoundation/CFBundle.h>
#endif

#include "Cosmographia.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("Periapsis Visual Software");
    QCoreApplication::setOrganizationDomain("periapsisvisual.com");
    QCoreApplication::setApplicationName("Cosmographia");

    // Enable multisample antialiasing
    QGLFormat format;
    format.setSampleBuffers(true);
    format.setSamples(4);
    format.setSwapInterval(1); // sync to vertical retrace
    QGLFormat::setDefaultFormat(format);

    // Set current directory so that we find the needed data files. On the Mac, we
    // just look in the app bundle. On other platforms we make some guesses, since we
    // don't know exactly where the executable will be run from.
    QString dataPath;
    bool foundData = true;
#ifdef Q_WS_MAC
    // On the Mac, load resources from the app bundle
    CFURLRef appUrlRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
    CFStringRef macPath = CFURLCopyFileSystemPath(appUrlRef, kCFURLPOSIXPathStyle);
    const char *pathPtr = CFStringGetCStringPtr(macPath, CFStringGetSystemEncoding());
    dataPath = QString(pathPtr) + "/Contents/Resources/data";
    CFRelease(appUrlRef);
    CFRelease(macPath);
#else
    if (QDir("../data").exists())
    {
        dataPath = "../data";
    }
    else if (QDir("../../data").exists())
    {
        // QtCreator builds in the debug/ or release/ directory
        dataPath = "../../data";
    }
    else if (QDir("../../cosmographia/data").exists())
    {
        dataPath = "../../cosmographia/data";
    }
    else if (QDir("../../trunk/data").exists())
    {
        dataPath = "../../trunk/data";
    }
    else if (QDir("./data").exists())
    {
        dataPath = "./data";
    }
    else
    {
        foundData = false;
    }
#endif
    if (!foundData || !QDir::setCurrent(dataPath))
    {
        QMessageBox::warning(NULL, "Missing data", "Data files not found!");
        exit(0);
    }

    Cosmographia mainWindow;
    mainWindow.initialize();
    mainWindow.show();

    return app.exec();
}