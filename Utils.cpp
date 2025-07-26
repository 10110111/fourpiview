#include "Utils.hpp"
#include <QtGlobal>
#include <QPalette>
#include <QStyleHints>
#include <QGuiApplication>
#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

namespace Utils
{

bool isDarkMode()
{
#ifdef Q_OS_ANDROID
    // Source: https://forum.qt.io/post/777843
    const auto context = QJniObject(QNativeInterface::QAndroidApplication::context());
    // Implementation of the Javanese context.getResources().getConfiguration().uiMode
    const auto resources = context.callObjectMethod("getResources",
                                                    "()Landroid/content/res/Resources;");
    const auto config = resources.callObjectMethod("getConfiguration",
                                                   "()Landroid/content/res/Configuration;");
    const auto uiMode = config.getField<jint>("uiMode");
    constexpr int UI_MODE_NIGHT_MASK = 0x30;
    constexpr int UI_MODE_NIGHT_YES = 0x20;
    return (uiMode & UI_MODE_NIGHT_MASK) == UI_MODE_NIGHT_YES;
#else
    // Source: https://stackoverflow.com/a/78854851/673852
# if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const auto scheme = QGuiApplication::styleHints()->colorScheme();
    return scheme == Qt::ColorScheme::Dark;
# else
    const QPalette defaultPalette;
    const auto text = defaultPalette.color(QPalette::WindowText);
    const auto window = defaultPalette.color(QPalette::Window);
    return text.lightness() > window.lightness();
# endif // QT_VERSION
#endif // Q_OS_ANDROID
}

}
