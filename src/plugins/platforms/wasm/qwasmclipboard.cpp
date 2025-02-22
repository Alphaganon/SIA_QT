// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmclipboard.h"
#include "qwasmwindow.h"
#include "qwasmstring.h"
#include <private/qstdweb_p.h>

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <QCoreApplication>
#include <qpa/qwindowsysteminterface.h>
#include <QBuffer>
#include <QString>

QT_BEGIN_NAMESPACE
using namespace emscripten;

static void commonCopyEvent(val event)
{
    QMimeData *_mimes = QWasmIntegration::get()->getWasmClipboard()->mimeData(QClipboard::Clipboard);
    if (!_mimes)
      return;

    // doing it this way seems to sanitize the text better that calling data() like down below
    if (_mimes->hasText()) {
        event["clipboardData"].call<void>("setData", val("text/plain")
                                          ,  QWasmString::fromQString(_mimes->text()));
    }
    if (_mimes->hasHtml()) {
        event["clipboardData"].call<void>("setData", val("text/html")
                                          ,   QWasmString::fromQString(_mimes->html()));
    }

    for (auto mimetype : _mimes->formats()) {
        if (mimetype.contains("text/"))
            continue;
        QByteArray ba = _mimes->data(mimetype);
        if (!ba.isEmpty())
            event["clipboardData"].call<void>("setData", QWasmString::fromQString(mimetype)
                                              , val(ba.constData()));
    }

    event.call<void>("preventDefault");
}

static void qClipboardCutTo(val event)
{
    if (!QWasmIntegration::get()->getWasmClipboard()->hasClipboardApi()) {
        // Send synthetic Ctrl+X to make the app cut data to Qt's clipboard
         QWindowSystemInterface::handleKeyEvent<QWindowSystemInterface::SynchronousDelivery>(
                     0, QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier, "X");
   }

    commonCopyEvent(event);
}

static void qClipboardCopyTo(val event)
{
    if (!QWasmIntegration::get()->getWasmClipboard()->hasClipboardApi()) {
        // Send synthetic Ctrl+C to make the app copy data to Qt's clipboard
            QWindowSystemInterface::handleKeyEvent<QWindowSystemInterface::SynchronousDelivery>(
                        0, QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier, "C");
    }
    commonCopyEvent(event);
}

static void qWasmClipboardPaste(QMimeData *mData)
{
    // Persist clipboard data so that the app can read it when handling the CTRL+V
    QWasmIntegration::get()->clipboard()->
        QPlatformClipboard::setMimeData(mData, QClipboard::Clipboard);

    QWindowSystemInterface::handleKeyEvent<QWindowSystemInterface::SynchronousDelivery>(
                0, QEvent::KeyPress, Qt::Key_V, Qt::ControlModifier, "V");
}

static void qClipboardPasteTo(val dataTransfer)
{
    enum class ItemKind {
        File,
        String,
    };

    struct Data
    {
        std::unique_ptr<QMimeData> data;
        int fileCount = 0;
        int doneCount = 0;
    };

    auto sharedData = std::make_shared<Data>();
    sharedData->data = std::make_unique<QMimeData>();

    auto continuation = [sharedData]() {
        Q_ASSERT(sharedData->doneCount <= sharedData->fileCount);
        if (sharedData->doneCount < sharedData->fileCount)
            return;

        if (!sharedData->data->formats().isEmpty())
            qWasmClipboardPaste(sharedData->data.release());
    };

    const val clipboardData = dataTransfer["clipboardData"];
    const val items = clipboardData["items"];
    for (int i = 0; i < items["length"].as<int>(); ++i) {
        const val item = items[i];
        const auto itemKind =
                item["kind"].as<std::string>() == "string" ? ItemKind::String : ItemKind::File;
        const auto itemMimeType = QString::fromStdString(item["type"].as<std::string>());

        switch (itemKind) {
        case ItemKind::File: {
            ++sharedData->fileCount;

            qstdweb::File file(item.call<emscripten::val>("getAsFile"));

            QByteArray fileContent(file.size(), Qt::Uninitialized);
            file.stream(fileContent.data(),
                        [continuation, itemMimeType, fileContent, sharedData]() {
                            if (!fileContent.isEmpty()) {
                                if (itemMimeType.startsWith("image/")) {
                                    QImage image;
                                    image.loadFromData(fileContent, nullptr);
                                    sharedData->data->setImageData(image);
                                } else {
                                    sharedData->data->setData(itemMimeType, fileContent.data());
                                }
                            }
                            ++sharedData->doneCount;
                            continuation();
                        });
            break;
        }
        case ItemKind::String:
            if (itemMimeType.contains("STRING", Qt::CaseSensitive)
                || itemMimeType.contains("TEXT", Qt::CaseSensitive)) {
                break;
            }
            const QString data = QWasmString::toQString(
                    clipboardData.call<val>("getData", val(itemMimeType.toStdString())));

            if (!data.isEmpty()) {
                if (itemMimeType == "text/html")
                    sharedData->data->setHtml(data);
                else if (itemMimeType.isEmpty() || itemMimeType == "text/plain")
                    sharedData->data->setText(data); // the type can be empty
                else
                    sharedData->data->setData(itemMimeType, data.toLocal8Bit());
            }
            break;
        }
    }
    continuation();
}

EMSCRIPTEN_BINDINGS(qtClipboardModule) {
    function("qtClipboardCutTo", &qClipboardCutTo);
    function("qtClipboardCopyTo", &qClipboardCopyTo);
    function("qtClipboardPasteTo", &qClipboardPasteTo);
}

QWasmClipboard::QWasmClipboard()
{
    val clipboard = val::global("navigator")["clipboard"];

    const bool hasPermissionsApi = !val::global("navigator")["permissions"].isUndefined();
    m_hasClipboardApi = !clipboard.isUndefined() && !clipboard["readText"].isUndefined();

    if (m_hasClipboardApi && hasPermissionsApi)
        initClipboardPermissions();
}

QWasmClipboard::~QWasmClipboard()
{
}

QMimeData *QWasmClipboard::mimeData(QClipboard::Mode mode)
{
    if (mode != QClipboard::Clipboard)
        return nullptr;

    return QPlatformClipboard::mimeData(mode);
}

void QWasmClipboard::setMimeData(QMimeData *mimeData, QClipboard::Mode mode)
{
    // handle setText/ setData programmatically
    QPlatformClipboard::setMimeData(mimeData, mode);
    if (m_hasClipboardApi)
        writeToClipboardApi();
    else
        writeToClipboard();
}

QWasmClipboard::ProcessKeyboardResult
QWasmClipboard::processKeyboard(const QWasmEventTranslator::TranslatedEvent &event,
                                const QFlags<Qt::KeyboardModifier> &modifiers)
{
    if (event.type != QEvent::KeyPress || !modifiers.testFlag(Qt::ControlModifier))
        return ProcessKeyboardResult::Ignored;

    if (event.key != Qt::Key_C && event.key != Qt::Key_V && event.key != Qt::Key_X)
        return ProcessKeyboardResult::Ignored;

    const bool isPaste = event.key == Qt::Key_V;

    return m_hasClipboardApi && !isPaste
            ? ProcessKeyboardResult::NativeClipboardEventAndCopiedDataNeeded
            : ProcessKeyboardResult::NativeClipboardEventNeeded;
}

bool QWasmClipboard::supportsMode(QClipboard::Mode mode) const
{
    return mode == QClipboard::Clipboard;
}

bool QWasmClipboard::ownsMode(QClipboard::Mode mode) const
{
    Q_UNUSED(mode);
    return false;
}

void QWasmClipboard::initClipboardPermissions()
{
    val permissions = val::global("navigator")["permissions"];

    qstdweb::Promise::make(permissions, "query", { .catchFunc = [](emscripten::val) {} }, ([]() {
                               val readPermissionsMap = val::object();
                               readPermissionsMap.set("name", val("clipboard-read"));
                               return readPermissionsMap;
                           })());
    qstdweb::Promise::make(permissions, "query", { .catchFunc = [](emscripten::val) {} }, ([]() {
                               val readPermissionsMap = val::object();
                               readPermissionsMap.set("name", val("clipboard-write"));
                               return readPermissionsMap;
                           })());
}

void QWasmClipboard::installEventHandlers(const emscripten::val &screenElement)
{
    emscripten::val cContext = val::undefined();
    emscripten::val isChromium = val::global("window")["chrome"];
   if (!isChromium.isUndefined()) {
        cContext = val::global("document");
   } else {
       cContext = screenElement;
   }
    // Fallback path for browsers which do not support direct clipboard access
    cContext.call<void>("addEventListener", val("cut"),
                        val::module_property("qtClipboardCutTo"), true);
    cContext.call<void>("addEventListener", val("copy"),
                        val::module_property("qtClipboardCopyTo"), true);
    cContext.call<void>("addEventListener", val("paste"),
                        val::module_property("qtClipboardPasteTo"), true);
}

bool QWasmClipboard::hasClipboardApi()
{
    return m_hasClipboardApi;
}

void QWasmClipboard::writeToClipboardApi()
{
    Q_ASSERT(m_hasClipboardApi);

    // copy event
    // browser event handler detected ctrl c if clipboard API
    // or Qt call from keyboard event handler

    QMimeData *_mimes = mimeData(QClipboard::Clipboard);
    if (!_mimes)
        return;

    emscripten::val clipboardWriteArray = emscripten::val::array();
    QByteArray ba;

    for (auto mimetype : _mimes->formats()) {
        // we need to treat binary and text differently, as the blob method below
        // fails for text mimetypes
        // ignore text types

        if (mimetype.contains("STRING", Qt::CaseSensitive) || mimetype.contains("TEXT", Qt::CaseSensitive))
            continue;

        if (_mimes->hasHtml()) { // prefer html over text
            ba = _mimes->html().toLocal8Bit();
            // force this mime
            mimetype = "text/html";
        } else if (mimetype.contains("text/plain")) {
            ba = _mimes->text().toLocal8Bit();
        } else if (mimetype.contains("image")) {
            QImage img = qvariant_cast<QImage>( _mimes->imageData());
            QBuffer buffer(&ba);
            buffer.open(QIODevice::WriteOnly);
            img.save(&buffer, "PNG");
            mimetype = "image/png"; // chrome only allows png
            // clipboard error "NotAllowedError" "Type application/x-qt-image not supported on write."
            // safari silently fails
            // so we use png internally for now
        } else {
            // DATA
            ba = _mimes->data(mimetype);
        }
        // Create file data Blob

        const char *content = ba.data();
        int dataLength = ba.length();
        if (dataLength < 1) {
            qDebug() << "no content found";
            return;
        }

        emscripten::val document = emscripten::val::global("document");
        emscripten::val window = emscripten::val::global("window");

        emscripten::val fileContentView =
                emscripten::val(emscripten::typed_memory_view(dataLength, content));
        emscripten::val fileContentCopy = emscripten::val::global("ArrayBuffer").new_(dataLength);
        emscripten::val fileContentCopyView =
                emscripten::val::global("Uint8Array").new_(fileContentCopy);
        fileContentCopyView.call<void>("set", fileContentView);

        emscripten::val contentArray = emscripten::val::array();
        contentArray.call<void>("push", fileContentCopyView);

        // we have a blob, now create a ClipboardItem
        emscripten::val type = emscripten::val::array();
        type.set("type", val(QWasmString::fromQString(mimetype)));

        emscripten::val contentBlob = emscripten::val::global("Blob").new_(contentArray, type);

        emscripten::val clipboardItemObject = emscripten::val::object();
        clipboardItemObject.set(val(QWasmString::fromQString(mimetype)), contentBlob);

        val clipboardItemData = val::global("ClipboardItem").new_(clipboardItemObject);

        clipboardWriteArray.call<void>("push", clipboardItemData);

        // Clipboard write is only supported with one ClipboardItem at the moment
        // but somehow this still works?
        // break;
    }

    val navigator = val::global("navigator");

    qstdweb::Promise::make(
        navigator["clipboard"], "write",
        {
            .catchFunc = [](emscripten::val error) {
                qWarning() << "clipboard error"
                    << QString::fromStdString(error["name"].as<std::string>())
                    << QString::fromStdString(error["message"].as<std::string>());
            }
        },
        clipboardWriteArray);
}

void QWasmClipboard::writeToClipboard()
{
    // this works for firefox, chrome by generating
    // copy event, but not safari
    // execCommand has been deemed deprecated in the docs, but browsers do not seem
    // interested in removing it. There is no replacement, so we use it here.
    val document = val::global("document");
    document.call<val>("execCommand", val("copy"));
}
QT_END_NAMESPACE
