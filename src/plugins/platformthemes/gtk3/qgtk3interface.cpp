// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//


#include "qgtk3interface_p.h"
#include "qgtk3storage_p.h"
#include <QtCore/QMetaEnum>
#include <QtCore/QSettings>
#include <QtCore/QFileInfo>
#include <QtGui/QFontDatabase>

QT_BEGIN_NAMESPACE
Q_LOGGING_CATEGORY(lcQGtk3Interface, "qt.qpa.gtk");


// Callback for gnome event loop has to be static
static QGtk3Storage *m_storage = nullptr;

QGtk3Interface::QGtk3Interface(QGtk3Storage *s)
{
    initColorMap();

    if (!s) {
        qCDebug(lcQGtk3Interface) << "QGtk3Interface instantiated without QGtk3Storage."
                                  << "No reaction to runtime theme changes.";
        return;
    }

    // Connect to the GTK settings changed signal
    auto handleThemeChange = [] {
        if (m_storage)
            m_storage->handleThemeChange();
    };

    GtkSettings *settings = gtk_settings_get_default();
    const gboolean success = g_signal_connect(settings, "notify::gtk-theme-name",
                                        G_CALLBACK(handleThemeChange), nullptr);
    if (success == FALSE) {
        qCDebug(lcQGtk3Interface) << "Connection to theme change signal failed."
                                  << "No reaction to runtime theme changes.";
    } else {
        m_storage = s;
    }
}

QGtk3Interface::~QGtk3Interface()
{
    // Ignore theme changes when destructor is reached
    m_storage = nullptr;

    // QGtkWidgets have to be destroyed manually
    for (auto v : cache)
        gtk_widget_destroy(v.second);
}

int QGtk3Interface::toGtkState(const QString &state)
{
#define CASE(x) \
    if (QLatin1String(QByteArray(state.toLatin1())) == #x ##_L1) \
        return GTK_STATE_FLAG_ ##x

#define CONVERT\
    CASE(NORMAL);\
    CASE(ACTIVE);\
    CASE(PRELIGHT);\
    CASE(SELECTED);\
    CASE(INSENSITIVE);\
    CASE(INCONSISTENT);\
    CASE(FOCUSED);\
    CASE(BACKDROP);\
    CASE(DIR_LTR);\
    CASE(DIR_RTL);\
    CASE(LINK);\
    CASE(VISITED);\
    CASE(CHECKED);\
    CASE(DROP_ACTIVE)

    CONVERT;
    return -1;
#undef CASE
}

const QLatin1String QGtk3Interface::fromGtkState(GtkStateFlags state)
{
#define CASE(x) case GTK_STATE_FLAG_ ##x: return QLatin1String(#x)
    switch (state) {
    CONVERT;
    }
    Q_UNREACHABLE();
#undef CASE
#undef CONVERT
}

void QGtk3Interface::initColorMap()
{
    // Populate map with default values
 #define SAVE(src, state, prop, def)\
    {ColorKey({QGtkColorSource::src, GTK_STATE_FLAG_ ##state}), ColorValue({#prop ##_L1, QGtkColorDefault::def})}

    gtkColorMap = ColorMap {
        SAVE(Foreground, NORMAL, theme_fg_color, Foreground),
        SAVE(Foreground, BACKDROP, theme_unfocused_selected_fg_color, Foreground),
        SAVE(Foreground, INSENSITIVE, insensitive_fg_color, Foreground),
        SAVE(Foreground, SELECTED, theme_selected_fg_color, Foreground),
        SAVE(Foreground, ACTIVE, theme_unfocused_fg_color, Foreground),
        SAVE(Text, NORMAL, theme_text_color, Foreground),
        SAVE(Text, ACTIVE, theme_unfocused_text_color, Foreground),
        SAVE(Base, NORMAL, theme_base_color, Background),
        SAVE(Base, INSENSITIVE, insensitive_base_color, Background),
        SAVE(Background, NORMAL, theme_bg_color, Background),
        SAVE(Background, SELECTED, theme_selected_bg_color, Background),
        SAVE(Background, INSENSITIVE, insensitive_bg_color, Background),
        SAVE(Background, ACTIVE, theme_unfocused_bg_color, Background),
        SAVE(Background, BACKDROP, theme_unfocused_selected_bg_color, Background),
        SAVE(Border, NORMAL, borders, Border),
        SAVE(Border, ACTIVE, unfocused_borders, Border)
    };
#undef SAVE

    qCDebug(lcQGtk3Interface) << "Color map populated from defaults.";
}

// Return an image rather than an icon or a pixmap:
// Image can be cached and re-scaled to different sizes if requested multiple times
QImage QGtk3Interface::standardPixmap(QPlatformTheme::StandardPixmap standardPixmap) const
{
    switch (standardPixmap) {
    case QPlatformTheme::DialogDiscardButton:
        return qt_gtk_get_icon(GTK_STOCK_DELETE);
    case QPlatformTheme::DialogOkButton:
        return qt_gtk_get_icon(GTK_STOCK_OK);
    case QPlatformTheme::DialogCancelButton:
        return qt_gtk_get_icon(GTK_STOCK_CANCEL);
    case QPlatformTheme::DialogYesButton:
        return qt_gtk_get_icon(GTK_STOCK_YES);
    case QPlatformTheme::DialogNoButton:
        return qt_gtk_get_icon(GTK_STOCK_NO);
    case QPlatformTheme::DialogOpenButton:
        return qt_gtk_get_icon(GTK_STOCK_OPEN);
    case QPlatformTheme::DialogCloseButton:
        return qt_gtk_get_icon(GTK_STOCK_CLOSE);
    case QPlatformTheme::DialogApplyButton:
        return qt_gtk_get_icon(GTK_STOCK_APPLY);
    case QPlatformTheme::DialogSaveButton:
        return qt_gtk_get_icon(GTK_STOCK_SAVE);
    case QPlatformTheme::MessageBoxWarning:
        return qt_gtk_get_icon(GTK_STOCK_DIALOG_WARNING);
    case QPlatformTheme::MessageBoxQuestion:
        return qt_gtk_get_icon(GTK_STOCK_DIALOG_QUESTION);
    case QPlatformTheme::MessageBoxInformation:
        return qt_gtk_get_icon(GTK_STOCK_DIALOG_INFO);
    case QPlatformTheme::MessageBoxCritical:
        return qt_gtk_get_icon(GTK_STOCK_DIALOG_ERROR);
    case QPlatformTheme::CustomBase:
    case QPlatformTheme::TitleBarMenuButton:
    case QPlatformTheme::TitleBarMinButton:
    case QPlatformTheme::TitleBarMaxButton:
    case QPlatformTheme::TitleBarCloseButton:
    case QPlatformTheme::TitleBarNormalButton:
    case QPlatformTheme::TitleBarShadeButton:
    case QPlatformTheme::TitleBarUnshadeButton:
    case QPlatformTheme::TitleBarContextHelpButton:
    case QPlatformTheme::DockWidgetCloseButton:
    case QPlatformTheme::DesktopIcon:
    case QPlatformTheme::TrashIcon:
    case QPlatformTheme::ComputerIcon:
    case QPlatformTheme::DriveFDIcon:
    case QPlatformTheme::DriveHDIcon:
    case QPlatformTheme::DriveCDIcon:
    case QPlatformTheme::DriveDVDIcon:
    case QPlatformTheme::DriveNetIcon:
    case QPlatformTheme::DirOpenIcon:
    case QPlatformTheme::DirClosedIcon:
    case QPlatformTheme::DirLinkIcon:
    case QPlatformTheme::DirLinkOpenIcon:
    case QPlatformTheme::FileIcon:
    case QPlatformTheme::FileLinkIcon:
    case QPlatformTheme::ToolBarHorizontalExtensionButton:
    case QPlatformTheme::ToolBarVerticalExtensionButton:
    case QPlatformTheme::FileDialogStart:
    case QPlatformTheme::FileDialogEnd:
    case QPlatformTheme::FileDialogToParent:
    case QPlatformTheme::FileDialogNewFolder:
    case QPlatformTheme::FileDialogDetailedView:
    case QPlatformTheme::FileDialogInfoView:
    case QPlatformTheme::FileDialogContentsView:
    case QPlatformTheme::FileDialogListView:
    case QPlatformTheme::FileDialogBack:
    case QPlatformTheme::DirIcon:
    case QPlatformTheme::DialogHelpButton:
    case QPlatformTheme::DialogResetButton:
    case QPlatformTheme::ArrowUp:
    case QPlatformTheme::ArrowDown:
    case QPlatformTheme::ArrowLeft:
    case QPlatformTheme::ArrowRight:
    case QPlatformTheme::ArrowBack:
    case QPlatformTheme::ArrowForward:
    case QPlatformTheme::DirHomeIcon:
    case QPlatformTheme::CommandLink:
    case QPlatformTheme::VistaShield:
    case QPlatformTheme::BrowserReload:
    case QPlatformTheme::BrowserStop:
    case QPlatformTheme::MediaPlay:
    case QPlatformTheme::MediaStop:
    case QPlatformTheme::MediaPause:
    case QPlatformTheme::MediaSkipForward:
    case QPlatformTheme::MediaSkipBackward:
    case QPlatformTheme::MediaSeekForward:
    case QPlatformTheme::MediaSeekBackward:
    case QPlatformTheme::MediaVolume:
    case QPlatformTheme::MediaVolumeMuted:
    case QPlatformTheme::LineEditClearButton:
    case QPlatformTheme::DialogYesToAllButton:
    case QPlatformTheme::DialogNoToAllButton:
    case QPlatformTheme::DialogSaveAllButton:
    case QPlatformTheme::DialogAbortButton:
    case QPlatformTheme::DialogRetryButton:
    case QPlatformTheme::DialogIgnoreButton:
    case QPlatformTheme::RestoreDefaultsButton:
    case QPlatformTheme::TabCloseButton:
    case QPlatformTheme::NStandardPixmap:
        return QImage();
    }
    Q_UNREACHABLE();
}

QImage QGtk3Interface::qt_gtk_get_icon(const char* iconName) const
{
    GtkIconSet* iconSet  = gtk_icon_factory_lookup_default (iconName);
    GdkPixbuf* icon = gtk_icon_set_render_icon_pixbuf(iconSet, context(), GTK_ICON_SIZE_DIALOG);
    return qt_convert_gdk_pixbuf(icon);
}

QImage QGtk3Interface::qt_convert_gdk_pixbuf(GdkPixbuf *buf) const
{
    if (!buf)
        return QImage();

    // Ability to convert GdkPixbuf to QImage relies on the assumptions, that
    // - QImage uses uchar as a data container
    // - the types guint8 and uchar are identical
    const guint8 *gdata = gdk_pixbuf_read_pixels(buf);
    static_assert(std::is_same<decltype(gdata), const uchar *>::value,
            "guint8 has diverted from uchar. Code needs fixing.");
    Q_ASSUME(gdk_pixbuf_get_bits_per_sample(buf) == 8);
    Q_ASSUME(gdk_pixbuf_get_n_channels(buf) == 4);
    const uchar *data = static_cast<const uchar *>(gdata);

    const int width = gdk_pixbuf_get_width(buf);
    const int height = gdk_pixbuf_get_height(buf);
    const int bpl = gdk_pixbuf_get_rowstride(buf);
    QImage converted(data, width, height, bpl, QImage::Format_ARGB32);
    return converted.copy(); // detatch to survive lifetime of buf
}

GtkWidget *QGtk3Interface::qt_new_gtkWidget(QGtkWidget type) const
{
#define CASE(Type)\
    case QGtkWidget::Type: return Type ##_new();
#define CASEN(Type)\
    case QGtkWidget::Type: return Type ##_new(nullptr);

    switch (type) {
    CASE(gtk_menu_bar)
    CASE(gtk_menu)
    CASE(gtk_button)
    case QGtkWidget::gtk_button_box: return gtk_button_box_new(GtkOrientation::GTK_ORIENTATION_HORIZONTAL);
    CASE(gtk_check_button)
    CASEN(gtk_radio_button)
    CASEN(gtk_frame)
    CASE(gtk_statusbar)
    CASE(gtk_entry)
    case QGtkWidget::gtk_popup: return gtk_window_new(GTK_WINDOW_POPUP);
    CASE(gtk_notebook)
    CASE(gtk_toolbar)
    CASE(gtk_tree_view)
    CASE(gtk_combo_box)
    CASE(gtk_combo_box_text)
    CASE(gtk_progress_bar)
    CASE(gtk_fixed)
    CASE(gtk_separator_menu_item)
    CASE(gtk_offscreen_window)
    case QGtkWidget::gtk_Default: return nullptr;
    }
#undef CASE
#undef CASEN
    Q_UNREACHABLE();
}

GdkRGBA QGtk3Interface::genericColor(GtkStyleContext *con, GtkStateFlags state, QGtkColorDefault def) const
{
    GdkRGBA color;

#define CASE(def, call)\
    case QGtkColorDefault::def:\
        gtk_style_context_get_ ##call(con, state, &color);\
        break;

    switch (def) {
    CASE(Foreground, color)
    CASE(Background, background_color)
    CASE(Border, border_color)
    }
    return color;
#undef CASE
}

// Deliver a QColor from a GTK widget, a source type and a GTK widget state
// Fall back to the generic color getter of source/state if the property name does not exist
// Fall back to a hard coded generic color getter of source/state are not mapped
QColor QGtk3Interface::color(GtkWidget *widget, QGtkColorSource source, GtkStateFlags state) const
{
    GdkRGBA col;
    GtkStyleContext *con = context(widget);

#define CASE(src, def)\
    case QGtkColorSource::src: {\
        const ColorKey key = ColorKey({QGtkColorSource::src, state});\
        if (gtkColorMap.contains(key)) {\
            const ColorValue val = gtkColorMap.value(key);\
            if (!gtk_style_context_lookup_color(con, val.propertyName.toUtf8().constData(), &col)) {\
                col = genericColor(con, state, val.genericSource);\
                qCDebug(lcQGtk3Interface) << "Property name" << val.propertyName << "not found.\n"\
                                          << "Falling back to " << val.genericSource;\
            }\
         } else {\
           col = genericColor(con, state, QGtkColorDefault::def);\
           qCDebug(lcQGtk3Interface) << "No color source found for" << QGtkColorSource::src\
                                     << fromGtkState(state) << "\n Falling back to"\
                                     << QGtkColorDefault::def;\
         }\
    }\
         break;

    switch (source) {
    CASE(Foreground, Foreground)
    CASE(Background, Background)
    CASE(Text, Foreground)
    CASE(Base, Background)
    CASE(Border, Border)
    }

    return fromGdkColor(col);
#undef CASE
}

// Deliver a widget pointer
GtkWidget *QGtk3Interface::widget(QGtkWidget type) const
{
    if (type == QGtkWidget::gtk_Default)
        return nullptr;

    // Return from cache
    if (GtkWidget *w = cache.value(type))
        return w;

    // Create new item and cache it
    GtkWidget *w = qt_new_gtkWidget(type);
    cache.insert(type, w);
    return w;
}

// Return widget syle context or default style
GtkStyleContext *QGtk3Interface::context(GtkWidget *w) const
{
    if (w)
        return gtk_widget_get_style_context(w);

    return gtk_widget_get_style_context(widget(QGtkWidget::gtk_entry));
}

// FIXME
// Brush assets (e.g. 9-patches) can't be accessed by the GTK API.
// => brush height and width from GTK will be ignored for the time being,
// because it is unknown if they relate to a plain brush or an image brush.
QBrush QGtk3Interface::brush(QGtkWidget wtype, QGtkColorSource source, GtkStateFlags state) const
{
    return QBrush(color(widget(wtype), source, state));
}

const QString QGtk3Interface::themeName() const
{
    gchar *theme_name;
    GtkSettings *settings = gtk_settings_get_default();
    if (!settings)
        return QString();

    g_object_get(settings, "gtk-theme-name", &theme_name, nullptr);
    return QLatin1StringView(theme_name);
}

inline constexpr QGtk3Interface::QGtkWidget QGtk3Interface::toWidgetType(QPlatformTheme::Font type)
{
    switch (type) {
    case QPlatformTheme::SystemFont: return QGtkWidget::gtk_Default;
    case QPlatformTheme::MenuFont: return QGtkWidget::gtk_menu;
    case QPlatformTheme::MenuBarFont: return QGtkWidget::gtk_menu_bar;
    case QPlatformTheme::MenuItemFont: return QGtkWidget::gtk_menu;
    case QPlatformTheme::MessageBoxFont: return QGtkWidget::gtk_popup;
    case QPlatformTheme::LabelFont: return QGtkWidget::gtk_popup;
    case QPlatformTheme::TipLabelFont: return QGtkWidget::gtk_Default;
    case QPlatformTheme::StatusBarFont: return QGtkWidget::gtk_statusbar;
    case QPlatformTheme::TitleBarFont: return QGtkWidget::gtk_Default;
    case QPlatformTheme::MdiSubWindowTitleFont: return QGtkWidget::gtk_Default;
    case QPlatformTheme::DockWidgetTitleFont: return QGtkWidget::gtk_Default;
    case QPlatformTheme::PushButtonFont: return QGtkWidget::gtk_button;
    case QPlatformTheme::CheckBoxFont: return QGtkWidget::gtk_check_button;
    case QPlatformTheme::RadioButtonFont: return QGtkWidget::gtk_radio_button;
    case QPlatformTheme::ToolButtonFont: return QGtkWidget::gtk_button;
    case QPlatformTheme::ItemViewFont: return QGtkWidget::gtk_entry;
    case QPlatformTheme::ListViewFont: return QGtkWidget::gtk_tree_view;
    case QPlatformTheme::HeaderViewFont: return QGtkWidget::gtk_fixed;
    case QPlatformTheme::ListBoxFont: return QGtkWidget::gtk_Default;
    case QPlatformTheme::ComboMenuItemFont: return QGtkWidget::gtk_combo_box;
    case QPlatformTheme::ComboLineEditFont: return QGtkWidget::gtk_combo_box_text;
    case QPlatformTheme::SmallFont: return QGtkWidget::gtk_Default;
    case QPlatformTheme::MiniFont: return QGtkWidget::gtk_Default;
    case QPlatformTheme::FixedFont: return QGtkWidget::gtk_fixed;
    case QPlatformTheme::GroupBoxTitleFont: return QGtkWidget::gtk_Default;
    case QPlatformTheme::TabButtonFont: return QGtkWidget::gtk_button;
    case QPlatformTheme::EditorFont: return QGtkWidget::gtk_entry;
    case QPlatformTheme::NFonts: return QGtkWidget::gtk_Default;
    }
    Q_UNREACHABLE();
}

inline constexpr QFont::Style QGtk3Interface::toFontStyle(PangoStyle style)
{
     switch (style) {
     case PANGO_STYLE_ITALIC: return QFont::StyleItalic;
     case PANGO_STYLE_OBLIQUE: return QFont::StyleOblique;
     case PANGO_STYLE_NORMAL: return QFont::StyleNormal;
     }
     // This is reached when GTK has introduced a new font style
     Q_UNREACHABLE();
}

inline constexpr int QGtk3Interface::toFontWeight(PangoWeight weight)
{
    // GTK PangoWeight can be directly converted to QFont::Weight
    // unless one of the enums changes.
    static_assert(PANGO_WEIGHT_THIN == 100 && PANGO_WEIGHT_ULTRAHEAVY == 1000,
                  "Pango font weight enum changed. Fix conversion.");

    static_assert(QFont::Thin == 100 && QFont::Black == 900,
                  "QFont::Weight enum changed. Fix conversion.");

    return qBound(1, static_cast<int>(weight), 1000);
}

QFont QGtk3Interface::font(QPlatformTheme::Font type) const
{
    GtkStyleContext *con = context(widget(toWidgetType(type)));
    if (!con)
        return QFont();

    const PangoFontDescription *gtkFont = gtk_style_context_get_font(con, GTK_STATE_FLAG_NORMAL);
    if (!gtkFont)
        return QFont();

    const QString family = QString::fromLatin1(pango_font_description_get_family(gtkFont));
    if (family.isEmpty())
        return QFont();

    const int weight = toFontWeight(pango_font_description_get_weight(gtkFont));

    // Creating a QFont() creates a futex lockup on a theme change
    // QFont doesn't have a constructor with float point size
    // => create a dummy point size and set it later.
    QFont font(family, 1, weight);
    font.setPointSizeF(static_cast<float>(pango_font_description_get_size(gtkFont)/PANGO_SCALE));
    font.setStyle(toFontStyle(pango_font_description_get_style(gtkFont)));

    // fix pixel pitch if fixed font is requested
    // NOTE: GTK allows to specify a non fixed font as the system's fixed font.
    // => the returned font may still not be a fixed font.
    if (type == QPlatformTheme::FixedFont) {
        font.setFixedPitch(true);
        if (!QFontInfo(font).fixedPitch()) {
            qCDebug(lcQGtk3Interface) << "No fixed pitch font found in font family"
                                      << font.family() << ". falling back to a default"
                                      << "fixed pitch font";
            font.setFamily("monospace"_L1);
        }
    }
    return font;
}

QIcon QGtk3Interface::fileIcon(const QFileInfo &fileInfo) const
{
    GFile *file = g_file_new_for_path(fileInfo.absoluteFilePath().toLatin1().constData());
    if (!file)
        return QIcon();

    GFileInfo *info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                         G_FILE_QUERY_INFO_NONE, nullptr, nullptr);
    if (!info) {
        g_object_unref(file);
        return QIcon();
    }

    GIcon *icon = g_file_info_get_icon(info);
    if (!icon) {
        g_object_unref(file);
        g_object_unref(info);
        return QIcon();
    }

    GtkIconTheme *theme = gtk_icon_theme_get_default();
    GtkIconInfo *iconInfo = gtk_icon_theme_lookup_by_gicon(theme, icon, GTK_ICON_SIZE_BUTTON,
                                                                  GTK_ICON_LOOKUP_FORCE_SIZE);
    if (!iconInfo) {
        g_object_unref(file);
        g_object_unref(info);
        g_object_unref(icon);
        return QIcon();
    }

    GdkPixbuf *buf = gtk_icon_info_load_icon(iconInfo, nullptr);
    QImage image = qt_convert_gdk_pixbuf(buf);
    g_object_unref(file);
    g_object_unref(info);
    g_object_unref(icon);
    g_object_unref(buf);
    return QIcon(QPixmap::fromImage(image));
}

QT_END_NAMESPACE
