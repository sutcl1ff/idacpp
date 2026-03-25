// Persistent dockable widget hierarchy explorer.
//
// Creates an IDA-dockable panel with a tree view of the Qt widget hierarchy.
//
// Uses IDA's create_empty_widget()/display_widget() for a persistent tab
// that survives REPL session changes. Run from the idacpp REPL:
//   C++> .x qt_widget_explorer.cpp
//
// Running again brings the existing panel to front. Close via IDA's tab close.

using namespace QT;

// Forward declarations
static void populateCombo(QComboBox* combo);
static void buildTree(QTreeWidget* tree, QWidget* root, int maxDepth = 10);

// Custom tree item that stores a widget pointer
class WidgetTreeItem : public QTreeWidgetItem {
public:
    QWidget* widget;
    WidgetTreeItem(QWidget* w, QTreeWidgetItem* parent = nullptr)
        : QTreeWidgetItem(parent), widget(w) {}
    WidgetTreeItem(QWidget* w, QTreeWidget* tree)
        : QTreeWidgetItem(tree), widget(w) {}
};

static void addChildren(WidgetTreeItem* parentItem, QWidget* parentWidget,
                         int depth, int maxDepth)
{
    if (depth >= maxDepth)
        return;

    for (QObject* child : parentWidget->children()) {
        QWidget* cw = qobject_cast<QWidget*>(child);
        if (!cw)
            continue;

        auto* item = new WidgetTreeItem(cw, parentItem);
        QString label = QString("%1").arg(cw->metaObject()->className());
        if (!cw->objectName().isEmpty())
            label += QString(" [%1]").arg(cw->objectName());
        if (!cw->windowTitle().isEmpty())
            label += QString(" \"%1\"").arg(cw->windowTitle());

        item->setText(0, label);
        item->setText(1, cw->isVisible() ? "visible" : "hidden");
        item->setText(2, QString("%1x%2").arg(cw->width()).arg(cw->height()));

        addChildren(item, cw, depth + 1, maxDepth);
    }
}

static void buildTree(QTreeWidget* tree, QWidget* root, int maxDepth)
{
    tree->clear();
    if (!root)
        return;

    auto* rootItem = new WidgetTreeItem(root, tree);
    QString label = QString("%1").arg(root->metaObject()->className());
    if (!root->objectName().isEmpty())
        label += QString(" [%1]").arg(root->objectName());
    if (!root->windowTitle().isEmpty())
        label += QString(" \"%1\"").arg(root->windowTitle());

    rootItem->setText(0, label);
    rootItem->setText(1, root->isVisible() ? "visible" : "hidden");
    rootItem->setText(2, QString("%1x%2").arg(root->width()).arg(root->height()));

    addChildren(rootItem, root, 0, maxDepth);
    tree->expandToDepth(2);
}

static void populateCombo(QComboBox* combo)
{
    combo->clear();
    auto* app = qApp;
    if (!app)
        return;

    for (QWidget* w : app->topLevelWidgets()) {
        QString display = QString("[%1]").arg(w->metaObject()->className());
        if (!w->windowTitle().isEmpty())
            display += QString(" %1").arg(w->windowTitle());
        else if (!w->objectName().isEmpty())
            display += QString(" %1").arg(w->objectName());
        combo->addItem(display, QVariant::fromValue(static_cast<void*>(w)));
    }
}

int main() {
    const char* widgetName = "C++ Widget Explorer";

    // Idempotent: find existing or create new
    TWidget* tw = find_widget(widgetName);
    if (tw) {
        activate_widget(tw, true);
        msg("Widget Explorer already open — activated\n");
        return 0;
    }

    tw = create_empty_widget(widgetName);
    display_widget(tw, WOPN_DP_TAB | WOPN_RESTORE);

    // Cast TWidget* to QWidget* — same pattern as IDA SDK's qwindow/qproject.
    QWidget* container = (QWidget*)tw;
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(4, 4, 4, 4);

    // Selector
    auto* selectorLayout = new QHBoxLayout();
    auto* selectorLabel = new QLabel("Widget:", container);
    auto* combo = new QComboBox(container);
    combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    selectorLayout->addWidget(selectorLabel);
    selectorLayout->addWidget(combo);
    layout->addLayout(selectorLayout);

    // Tree
    auto* tree = new QTreeWidget(container);
    tree->setHeaderLabels({"Widget", "Visibility", "Size"});
    tree->setColumnCount(3);
    tree->header()->setStretchLastSection(false);
    tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    layout->addWidget(tree);

    // Buttons
    auto* btnLayout = new QHBoxLayout();
    auto* expandBtn = new QPushButton("Expand All", container);
    auto* collapseBtn = new QPushButton("Collapse All", container);
    auto* refreshBtn = new QPushButton("Refresh", container);
    btnLayout->addWidget(expandBtn);
    btnLayout->addWidget(collapseBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(refreshBtn);
    layout->addLayout(btnLayout);

    // Populate combo
    populateCombo(combo);

    // Combo selection → rebuild tree
    QObject::connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     [combo, tree](int index) {
        if (index < 0)
            return;
        void* ptr = combo->itemData(index).value<void*>();
        auto* w = static_cast<QWidget*>(ptr);
        buildTree(tree, w);
    });

    // Click tree item → raise the widget
    QObject::connect(tree, &QTreeWidget::itemClicked,
                     [](QTreeWidgetItem* item, int) {
        auto* wti = static_cast<WidgetTreeItem*>(item);
        if (wti && wti->widget) {
            wti->widget->raise();
            wti->widget->activateWindow();
        }
    });

    // Buttons
    QObject::connect(expandBtn, &QPushButton::clicked, tree, &QTreeWidget::expandAll);
    QObject::connect(collapseBtn, &QPushButton::clicked, tree, &QTreeWidget::collapseAll);
    QObject::connect(refreshBtn, &QPushButton::clicked, [combo, tree]() {
        int idx = combo->currentIndex();
        populateCombo(combo);
        if (idx >= 0 && idx < combo->count())
            combo->setCurrentIndex(idx);
    });

    // Auto-select first entry
    if (combo->count() > 0)
        combo->setCurrentIndex(0);

    msg("Widget Explorer opened as dockable tab\n");
    return 0;
}
