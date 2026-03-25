# qt6 — Extension Plugin

Makes Qt6 (Core, Gui, Widgets) available in the idacpp C++ REPL. Discovers
an existing Qt6 installation built with `QT_NAMESPACE=QT`, loads its shared
libraries for JIT resolution, and preloads Qt headers.

## QT_NAMESPACE

IDA's Qt is **always** built with `QT_NAMESPACE=QT`. All Qt classes live in
the `QT` namespace. Standard Qt installations will not work — you must use a
Qt built with `-DQT_NAMESPACE=QT`.

The plugin adds `using namespace QT;` automatically, so you can write
`QWidget*` directly instead of `QT::QWidget*`.

## Enable

### Option 1: Build Qt from the IDA SDK (recommended)

```bash
cmake .. -DPLUGIN_QT6=ON              # includes QtSupport.cmake, creates build_qt target
cmake --build . --target build_qt     # one-time build (~1-2 hours)
# build_qt auto-reconfigures — just rebuild:
cmake --build .
```

Qt 6.8.2 is downloaded, built with `QT_NAMESPACE=QT`, and installed into
`<build-dir>/qt-install/`. Subsequent configures auto-detect it.

### Option 2: Point to existing Qt installation

```bash
cmake .. -DPLUGIN_QT6=ON -DPLUGIN_QT6_DIR=/path/to/qt-install
```

The directory must contain `lib/cmake/Qt6/` from a Qt build with
`QT_NAMESPACE=QT`.

### Option 3: Auto-detect

If Qt6 is already on `CMAKE_PREFIX_PATH` or was built by the SDK in the
same build tree, it's found automatically:

```bash
cmake .. -DPLUGIN_QT6=ON
```

## What It Does

1. **Build time**: Generates a PCH bridge header with `QT_NAMESPACE=QT` and
   Qt umbrella includes. Extends the shared PCH with Qt6 headers.
2. **Runtime**: Loads Qt6Core/Gui/Widgets shared libraries, adds include
   paths, defines `QT_NAMESPACE=QT`, includes Qt headers, and adds
   `using namespace QT;`.

## REPL Usage

```cpp
// QApplication is already running in IDA — access it directly
auto app = qApp;
auto widgets = app->topLevelWidgets();
printf("%d top-level widgets\n", widgets.size());

// QMetaObject introspection
for (auto* w : widgets) {
    auto* meta = w->metaObject();
    printf("%s: %d properties, %d methods\n",
           meta->className(), meta->propertyCount(), meta->methodCount());
}

// Create a widget dynamically
auto* dlg = new QDialog(nullptr);
dlg->setWindowTitle("Hello from C++");
dlg->resize(300, 200);
dlg->show();
```

## MOC Limitation

The REPL has no MOC (Meta-Object Compiler) step, so you cannot define
`Q_OBJECT` classes or use `signals:`/`slots:` keywords. Instead:

- Use **lambda-based `QObject::connect()`** for signal/slot connections
- Use **`QMetaObject`** API for runtime reflection
- Access **existing IDA widgets** and their properties

```cpp
// Lambda connect — no MOC needed
auto* btn = new QPushButton("Click me");
QObject::connect(btn, &QPushButton::clicked, []() {
    msg("Button clicked!\n");
});
```

## Persistent Forms

Use IDA's widget API for dockable panels that survive REPL sessions:

```cpp
auto* tw = find_widget("My Panel");
if (!tw) {
    tw = create_empty_widget("My Panel");
    display_widget(tw, WOPN_DP_TAB | WOPN_RESTORE);
}
auto* w = reinterpret_cast<QWidget*>(tw);
// Populate w with Qt layouts and widgets...
```

See `examples/qt_widget_explorer.cpp` for a complete implementation.

## Examples

| File | Description |
|------|-------------|
| `qt_introspect.cpp` | QMetaObject reflection — properties, signals, slots |
| `qt_list_windows.cpp` | Enumerate docked windows and toolbars |
| `qt_widget_explorer.cpp` | Persistent dockable widget hierarchy explorer |
| `qt_form_builder.cpp` | Dynamic QDialog with tree view and controls |
