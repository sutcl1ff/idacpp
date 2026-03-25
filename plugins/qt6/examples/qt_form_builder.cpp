// Dynamically build a Qt form from the REPL.
//
// Creates a non-modal QDialog populated with live IDA window information.
// Demonstrates dynamic widget creation, layouts, and lambda signal
// connections (no MOC needed). Run from the idacpp REPL:
//   C++> .x qt_form_builder.cpp

using namespace QT;

static void populateTree(QTreeWidget* tree)
{
    tree->clear();
    auto* app = qApp;
    if (!app)
        return;

    for (QWidget* w : app->topLevelWidgets()) {
        if (!w->isVisible())
            continue;

        auto* item = new QTreeWidgetItem(tree);
        item->setText(0, w->metaObject()->className());
        item->setText(1, w->windowTitle().isEmpty()
                        ? w->objectName() : w->windowTitle());

        // Size
        auto* sizeItem = new QTreeWidgetItem(item);
        sizeItem->setText(0, "Size");
        sizeItem->setText(1, QString("%1x%2").arg(w->width()).arg(w->height()));

        // Position
        auto* posItem = new QTreeWidgetItem(item);
        posItem->setText(0, "Position");
        posItem->setText(1, QString("(%1, %2)").arg(w->x()).arg(w->y()));

        // Children count
        int childCount = 0;
        for (QObject* c : w->children())
            if (qobject_cast<QWidget*>(c))
                ++childCount;
        auto* childItem = new QTreeWidgetItem(item);
        childItem->setText(0, "Child widgets");
        childItem->setText(1, QString::number(childCount));

        // Meta-object info
        const QMetaObject* meta = w->metaObject();
        auto* metaItem = new QTreeWidgetItem(item);
        metaItem->setText(0, "Properties");
        metaItem->setText(1, QString::number(meta->propertyCount()));

        auto* methodItem = new QTreeWidgetItem(item);
        methodItem->setText(0, "Methods");
        methodItem->setText(1, QString::number(meta->methodCount()));
    }

    tree->expandAll();
}

int main() {
    auto* app = qApp;
    if (!app) {
        msg("No QApplication instance\n");
        return 1;
    }

    // Create a non-modal dialog
    auto* dlg = new QDialog(nullptr);
    dlg->setWindowTitle("idacpp Form Builder");
    dlg->resize(480, 400);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    auto* layout = new QVBoxLayout(dlg);

    // Header
    auto* header = new QLabel("IDA C++ REPL — Dynamic Form", dlg);
    header->setAlignment(Qt::AlignCenter);
    QFont headerFont = header->font();
    headerFont.setPointSize(headerFont.pointSize() + 2);
    headerFont.setBold(true);
    header->setFont(headerFont);
    layout->addWidget(header);

    // Tree
    auto* tree = new QTreeWidget(dlg);
    tree->setHeaderLabels({"Property", "Value"});
    tree->setColumnCount(2);
    tree->header()->setStretchLastSection(true);
    tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    layout->addWidget(tree);

    // Populate
    populateTree(tree);

    // Status label
    auto* status = new QLabel(dlg);
    int widgetCount = 0;
    for (QWidget* w : app->allWidgets())
        ++widgetCount;
    status->setText(QString("Total widgets in application: %1").arg(widgetCount));
    layout->addWidget(status);

    // Buttons
    auto* btnLayout = new QHBoxLayout();
    auto* refreshBtn = new QPushButton("Refresh", dlg);
    auto* closeBtn = new QPushButton("Close", dlg);
    btnLayout->addStretch();
    btnLayout->addWidget(refreshBtn);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    // Signal connections (lambda style — no MOC needed)
    QObject::connect(refreshBtn, &QPushButton::clicked, [tree, status, app]() {
        populateTree(tree);
        int count = 0;
        for (QWidget* w : app->allWidgets())
            ++count;
        status->setText(QString("Total widgets in application: %1 (refreshed)").arg(count));
    });
    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::close);

    // Show non-modal — dialog stays alive after script returns
    dlg->show();
    msg("Form created. Close the dialog when done.\n");
    return 0;
}
