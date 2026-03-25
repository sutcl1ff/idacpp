// Qt6 introspection via QMetaObject.
//
// Enumerates top-level widgets and prints their properties, signals, and
// slots using Qt's meta-object system. Run from the idacpp REPL:
//   C++> .x qt_introspect.cpp

using namespace QT;

int main() {
    auto* app = qApp;
    if (!app) {
        msg("No QApplication instance\n");
        return 1;
    }

    auto topLevel = app->topLevelWidgets();
    msg("=== Qt Introspection: %d top-level widgets ===\n\n", topLevel.size());

    for (QWidget* w : topLevel) {
        if (!w->isVisible())
            continue;

        const QMetaObject* meta = w->metaObject();
        msg("--- %s", meta->className());
        if (!w->objectName().isEmpty())
            msg(" [%s]", qPrintable(w->objectName()));
        if (!w->windowTitle().isEmpty())
            msg(" \"%s\"", qPrintable(w->windowTitle()));
        msg(" ---\n");
        msg("  Size: %dx%d  Pos: (%d,%d)  Visible: %s\n",
               w->width(), w->height(), w->x(), w->y(),
               w->isVisible() ? "yes" : "no");

        // Properties (own class only, skip inherited)
        int ownPropStart = meta->superClass() ? meta->superClass()->propertyCount() : 0;
        int ownPropCount = meta->propertyCount() - ownPropStart;
        if (ownPropCount > 0) {
            msg("  Properties (%d own):\n", ownPropCount);
            for (int i = ownPropStart; i < meta->propertyCount(); ++i) {
                QMetaProperty prop = meta->property(i);
                QVariant val = prop.read(w);
                QString valStr = val.toString();
                if (valStr.length() > 60)
                    valStr = valStr.left(57) + "...";
                msg("    %s %s = %s\n",
                       prop.typeName(), prop.name(),
                       qPrintable(valStr));
            }
        }

        // Signals (own class only)
        int ownMethodStart = meta->superClass() ? meta->superClass()->methodCount() : 0;
        bool hasSignals = false;
        for (int i = ownMethodStart; i < meta->methodCount(); ++i) {
            QMetaMethod m = meta->method(i);
            if (m.methodType() == QMetaMethod::Signal) {
                if (!hasSignals) {
                    msg("  Signals:\n");
                    hasSignals = true;
                }
                msg("    %s\n", qPrintable(QString::fromLatin1(m.methodSignature())));
            }
        }

        // Slots (own class only)
        bool hasSlots = false;
        for (int i = ownMethodStart; i < meta->methodCount(); ++i) {
            QMetaMethod m = meta->method(i);
            if (m.methodType() == QMetaMethod::Slot) {
                if (!hasSlots) {
                    msg("  Slots:\n");
                    hasSlots = true;
                }
                msg("    %s\n", qPrintable(QString::fromLatin1(m.methodSignature())));
            }
        }

        msg("\n");
    }

    // Summary of all properties across inheritance chain for the main window
    for (QWidget* w : topLevel) {
        if (qobject_cast<QMainWindow*>(w)) {
            const QMetaObject* meta = w->metaObject();
            msg("=== Full meta-object chain for %s ===\n", meta->className());
            const QMetaObject* m = meta;
            while (m) {
                msg("  %s (%d methods, %d properties)\n",
                       m->className(), m->methodCount(), m->propertyCount());
                m = m->superClass();
            }
            break;
        }
    }

    return 0;
}
