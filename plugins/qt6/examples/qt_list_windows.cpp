// List all open IDA windows, docked panels, and toolbars.
//
// Run from the idacpp REPL:
//   C++> .x qt_list_windows.cpp

using namespace QT;

int main() {
    auto* app = qApp;
    if (!app) {
        msg("No QApplication instance\n");
        return 1;
    }

    auto allWidgets = app->allWidgets();

    // Find the main window
    QMainWindow* mainWin = nullptr;
    for (QWidget* w : app->topLevelWidgets()) {
        if (auto* mw = qobject_cast<QMainWindow*>(w)) {
            mainWin = mw;
            break;
        }
    }

    if (mainWin) {
        msg("=== Main Window ===\n");
        msg("  %s \"%s\"  %dx%d\n\n",
               mainWin->metaObject()->className(),
               qPrintable(mainWin->windowTitle()),
               mainWin->width(), mainWin->height());
    }

    // Collect visible docked windows (deduplicated by title)
    QMap<QString, QString> dockWindows;  // title -> className
    for (QWidget* w : allWidgets) {
        if (auto* dock = qobject_cast<QDockWidget*>(w)) {
            if (dock->isVisible() && !dock->windowTitle().isEmpty())
                dockWindows.insert(dock->windowTitle(), dock->metaObject()->className());
        }
    }

    msg("=== Visible Docked Windows (%d) ===\n", dockWindows.size());
    for (auto it = dockWindows.constBegin(); it != dockWindows.constEnd(); ++it)
        msg("  %-40s  [%s]\n", qPrintable(it.key()), qPrintable(it.value()));

    // Collect visible toolbars
    QStringList toolbars;
    for (QWidget* w : allWidgets) {
        if (auto* tb = qobject_cast<QToolBar*>(w)) {
            if (tb->isVisible()) {
                QString name = tb->windowTitle();
                if (name.isEmpty())
                    name = tb->objectName();
                if (!name.isEmpty())
                    toolbars.append(name);
            }
        }
    }
    toolbars.sort();

    msg("\n=== Visible Toolbars (%d) ===\n", toolbars.size());
    for (const QString& name : toolbars)
        msg("  %s\n", qPrintable(name));

    // Summary
    int totalVisible = 0;
    for (QWidget* w : allWidgets)
        if (w->isVisible())
            ++totalVisible;

    msg("\n=== Summary ===\n");
    msg("  Total widgets: %d\n", allWidgets.size());
    msg("  Visible: %d\n", totalVisible);
    msg("  Dock windows: %d\n", dockWindows.size());
    msg("  Toolbars: %d\n", toolbars.size());

    return 0;
}
