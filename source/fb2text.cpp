#include "fb2text.hpp"

#include <QBoxLayout>
#include <QDockWidget>
#include <QFileDialog>
#include <QInputDialog>
#include <QMainWindow>
#include <QMenu>
#include <QToolBar>
#include <QWebInspector>
#include <QWebFrame>
#include <QWebPage>
#include <QtDebug>

#include "fb2dlgs.hpp"
#include "fb2page.hpp"
#include "fb2save.hpp"
#include "fb2tree.hpp"
#include "fb2utils.h"

//---------------------------------------------------------------------------
//  FbTextAction
//---------------------------------------------------------------------------

FbTextAction::FbTextAction(const QString &text, QWebPage::WebAction action, FbTextEdit *parent)
    : QAction(text, parent)
    , m_action(action)
    , m_parent(parent)
{
}

FbTextAction::FbTextAction(const QIcon &icon, const QString &text, QWebPage::WebAction action, FbTextEdit* parent)
    : QAction(icon, text, parent)
    , m_action(action)
    , m_parent(parent)
{
}

QAction * FbTextAction::action(QWebPage::WebAction action)
{
    return m_parent->pageAction(m_action);
}

void FbTextAction::updateAction()
{
    if (QAction * act = action(m_action)) {
        if (isCheckable()) setChecked(act->isChecked());
        setEnabled(act->isEnabled());
    }
}

void FbTextAction::connectAction()
{
    if (QAction * act = action(m_action)) {
        connect(this, SIGNAL(triggered(bool)), act, SIGNAL(triggered(bool)));
        connect(act, SIGNAL(changed()), this, SLOT(updateAction()));
        if (isCheckable()) setChecked(act->isChecked());
        setEnabled(act->isEnabled());
    } else {
        if (isCheckable()) setChecked(false);
        setEnabled(false);
    }
}

void FbTextAction::disconnectAction()
{
    QAction * act = action(m_action);
    disconnect(act, 0, this, 0);
    disconnect(this, 0, act, 0);
}

//---------------------------------------------------------------------------
//  FbDockWidget
//---------------------------------------------------------------------------

FbDockWidget::FbDockWidget(const QString &title, QWidget *parent, Qt::WindowFlags flags)
    : QDockWidget(title, parent, flags)
{
    setFeatures(QDockWidget::AllDockWidgetFeatures);
    setAttribute(Qt::WA_DeleteOnClose);
}

//---------------------------------------------------------------------------
//  FbNoteView
//---------------------------------------------------------------------------

class FbNoteView : public QWebView
{
public:
    explicit FbNoteView(QWidget *parent, const QUrl &url);
    void hint(const QWebElement element, const QRect &rect);
protected:
    void paintEvent(QPaintEvent *event);
    const QUrl m_url;
};

FbNoteView::FbNoteView(QWidget *parent, const QUrl &url)
    : QWebView(parent)
    , m_url(url)
{
}

void FbNoteView::paintEvent(QPaintEvent *event)
{
    QWebView::paintEvent(event);
    QPainter painter(this);
    painter.setPen(Qt::black);
    QSize size = geometry().size() - QSize(1, 1);
    painter.drawRect( QRect(QPoint(0, 0), size) );
}

void FbNoteView::hint(const QWebElement element, const QRect &rect)
{
    QString html = element.toOuterXml();
    html.prepend(
        "<body bgcolor=lightyellow style='overflow:hidden;padding:0;margin:0;margin-top:2;'>"
        "<fb:body name=notes style='padding:0;margin:0;'>"
    );
    html.append("</fb:body></body>");
    setGeometry(rect);
    setHtml(html, m_url);
    show();
}

//---------------------------------------------------------------------------
//  FbTextBase
//---------------------------------------------------------------------------

void FbTextBase::addTools(QToolBar *tool)
{
    QAction *act;

    act = pageAction(QWebPage::Undo);
    act->setIcon(FbIcon("edit-undo"));
    act->setText(QObject::tr("&Undo"));
    act->setPriority(QAction::LowPriority);
    act->setShortcut(QKeySequence::Undo);
    tool->addAction(act);

    act = pageAction(QWebPage::Redo);
    act->setIcon(FbIcon("edit-redo"));
    act->setText(QObject::tr("&Redo"));
    act->setPriority(QAction::LowPriority);
    act->setShortcut(QKeySequence::Redo);
    tool->addAction(act);

    tool->addSeparator();

    act = pageAction(QWebPage::Cut);
    act->setIcon(FbIcon("edit-cut"));
    act->setText(QObject::tr("Cu&t"));
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Cut);
    act->setStatusTip(QObject::tr("Cut the current selection's contents to the clipboard"));
    tool->addAction(act);

    act = pageAction(QWebPage::Copy);
    act->setIcon(FbIcon("edit-copy"));
    act->setText(QObject::tr("&Copy"));
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Copy);
    act->setStatusTip(QObject::tr("Copy the current selection's contents to the clipboard"));
    tool->addAction(act);

    act = pageAction(QWebPage::Paste);
    act->setIcon(FbIcon("edit-paste"));
    act->setText(QObject::tr("&Paste"));
    act->setPriority(QAction::LowPriority);
    act->setShortcuts(QKeySequence::Paste);
    act->setStatusTip(QObject::tr("Paste the clipboard's contents into the current selection"));
    tool->addAction(act);

    tool->addSeparator();

    act = pageAction(QWebPage::RemoveFormat);
    act->setText(QObject::tr("Clear format"));
    act->setIcon(FbIcon("edit-clear"));

    act = pageAction(QWebPage::ToggleBold);
    act->setIcon(FbIcon("format-text-bold"));
    act->setText(QObject::tr("&Bold"));
    tool->addAction(act);

    act = pageAction(QWebPage::ToggleItalic);
    act->setIcon(FbIcon("format-text-italic"));
    act->setText(QObject::tr("&Italic"));
    tool->addAction(act);

    act = pageAction(QWebPage::ToggleStrikethrough);
    act->setIcon(FbIcon("format-text-strikethrough"));
    act->setText(QObject::tr("&Strikethrough"));
    tool->addAction(act);

    act = pageAction(QWebPage::ToggleSuperscript);
    act->setIcon(FbIcon("format-text-superscript"));
    act->setText(QObject::tr("Su&perscript"));
    tool->addAction(act);

    act = pageAction(QWebPage::ToggleSubscript);
    act->setIcon(FbIcon("format-text-subscript"));
    act->setText(QObject::tr("Su&bscript"));
    tool->addAction(act);
}

//---------------------------------------------------------------------------
//  FbTextEdit
//---------------------------------------------------------------------------

FbTextEdit::FbTextEdit(QWidget *parent, QObject *owner)
    : FbTextBase(parent)
    , m_owner(qobject_cast<QMainWindow*>(owner))
    , m_noteView(0)
    , m_thread(0)
    , dockTree(0)
    , dockImgs(0)
    , dockInsp(0)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(contextMenu(QPoint)));
    setPage(new FbTextPage(this));
}

FbTextEdit::~FbTextEdit()
{
    if (m_noteView) delete m_noteView;
}

FbTextPage *FbTextEdit::page()
{
    return qobject_cast<FbTextPage*>(FbTextBase::page());
}

FbNetworkAccessManager *FbTextEdit::files()
{
    return page()->temp();
}

QAction * FbTextEdit::act(Fb::Actions index) const
{
    return m_actions[index];
}

QAction * FbTextEdit::pAct(QWebPage::WebAction index) const
{
    return pageAction(index);
}

void FbTextEdit::setAction(Fb::Actions index, QAction *action)
{
    m_actions[index] = action;
}

void FbTextEdit::connectActions(QToolBar *tool)
{
    m_actions.connect();

    foreach (QAction *action, m_actions) {
        if (FbTextAction *a = qobject_cast<FbTextAction*>(action)) {
            a->connectAction();
        }
    }

    connect(act(Fb::ViewContents), SIGNAL(triggered(bool)), this, SLOT(viewContents(bool)));
    connect(act(Fb::ViewPictures), SIGNAL(triggered(bool)), this, SLOT(viewPictures(bool)));
    connect(act(Fb::ViewInspector), SIGNAL(triggered(bool)), this, SLOT(viewInspector(bool)));

    connect(act(Fb::ZoomIn), SIGNAL(triggered()), SLOT(zoomIn()));
    connect(act(Fb::ZoomOut), SIGNAL(triggered()), SLOT(zoomOut()));
    connect(act(Fb::ZoomReset), SIGNAL(triggered()), SLOT(zoomReset()));

    /*
    connect(textPage, SIGNAL(selectionChanged()), SLOT(selectionChanged()));

    connect(actionFind, SIGNAL(triggered()), textEdit, SLOT(find()));
    connect(actionImage, SIGNAL(triggered()), textEdit, SLOT(insertImage()));
    connect(actionNote, SIGNAL(triggered()), textEdit, SLOT(insertNote()));
    connect(actionLink, SIGNAL(triggered()), textEdit, SLOT(insertLink()));

    connect(actionTitle, SIGNAL(triggered()), textPage, SLOT(insertTitle()));
    connect(actionAnnot, SIGNAL(triggered()), textPage, SLOT(insertAnnot()));
    connect(actionAuthor, SIGNAL(triggered()), textPage, SLOT(insertAuthor()));
    connect(actionEpigraph, SIGNAL(triggered()), textPage, SLOT(insertEpigraph()));
    connect(actionSubtitle, SIGNAL(triggered()), textPage, SLOT(insertSubtitle()));
    connect(actionSection, SIGNAL(triggered()), textPage, SLOT(insertSection()));
    connect(actionStanza, SIGNAL(triggered()), textPage, SLOT(insertStanza()));
    connect(actionPoem, SIGNAL(triggered()), textPage, SLOT(insertPoem()));
    connect(actionDate, SIGNAL(triggered()), textPage, SLOT(insertDate()));
    connect(actionBody, SIGNAL(triggered()), textPage, SLOT(insertBody()));

    connect(actionSimpleText, SIGNAL(triggered()), textPage, SLOT(insertText()));
    connect(actionParaSeparator, SIGNAL(triggered()), textEdit->pageAction(QWebPage::InsertParagraphSeparator), SIGNAL(triggered()));
    connect(actionLineSeparator, SIGNAL(triggered()), textEdit->pageAction(QWebPage::InsertLineSeparator), SIGNAL(triggered()));

    connect(actionSectionAdd, SIGNAL(triggered()), textPage, SLOT(createSection()));
    connect(actionSectionDel, SIGNAL(triggered()), textPage, SLOT(deleteSection()));
    connect(actionTextTitle, SIGNAL(triggered()), textPage, SLOT(createTitle()));

*/

    tool->clear();

    tool->addSeparator();
    tool->addAction(act(Fb::EditUndo));
    tool->addAction(act(Fb::EditRedo));

    tool->addSeparator();
    tool->addAction(act(Fb::EditCut));
    tool->addAction(act(Fb::EditCopy));
    tool->addAction(act(Fb::EditPaste));

    tool->addSeparator();
    tool->addAction(act(Fb::TextBold));
    tool->addAction(act(Fb::TextItalic));
    tool->addAction(act(Fb::TextStrike));
    tool->addAction(act(Fb::TextSup));
    tool->addAction(act(Fb::TextSub));

    tool->addSeparator();
    tool->addAction(act(Fb::SectionAdd));
    tool->addAction(act(Fb::SectionDel));
    tool->addAction(act(Fb::TextTitle));

    tool->addSeparator();
    tool->addAction(act(Fb::InsertImage));
    tool->addAction(act(Fb::InsertNote));
    tool->addAction(act(Fb::InsertLink));
    tool->addAction(act(Fb::InsertSection));
}

void FbTextEdit::disconnectActions()
{
    m_actions.disconnect();
    foreach (QAction *action, m_actions) {
        if (FbTextAction *a = qobject_cast<FbTextAction*>(action)) {
            a->disconnectAction();
        }
    }
}

void FbTextEdit::viewContents(bool show)
{
    if (show) {
        if (dockTree) { dockTree->show(); return; }
        dockTree = new FbDockWidget(tr("Contents"), this);
        dockTree->setWidget(new FbTreeWidget(this, m_owner));
        connect(dockTree, SIGNAL(visibilityChanged(bool)), act(Fb::ViewContents), SLOT(setChecked(bool)));
        connect(dockTree, SIGNAL(destroyed()), SLOT(treeDestroyed()));
        m_owner->addDockWidget(Qt::LeftDockWidgetArea, dockTree);
    } else {
        dockTree->deleteLater();
        dockTree = 0;
    }
}

void FbTextEdit::viewPictures(bool show)
{
    if (show) {
        if (dockImgs) { dockImgs->show(); return; }
        dockImgs = new FbDockWidget(tr("Pictures"), this);
        dockImgs->setWidget(new FbListWidget(this, m_owner));
        connect(dockImgs, SIGNAL(visibilityChanged(bool)), act(Fb::ViewPictures), SLOT(setChecked(bool)));
        connect(dockImgs, SIGNAL(destroyed()), SLOT(imgsDestroyed()));
        m_owner->addDockWidget(Qt::RightDockWidgetArea, dockImgs);
    } else {
        dockImgs->deleteLater();
        dockImgs = 0;
    }
}

void FbTextEdit::viewInspector(bool show)
{
    if (show) {
        if (dockInsp) { dockInsp->show(); return; }
        QWebInspector *inspector = new QWebInspector(this);
        inspector->setPage(page());
        dockInsp = new QDockWidget(tr("Web inspector"), this);
        dockInsp->setFeatures(QDockWidget::AllDockWidgetFeatures);
        dockInsp->setWidget(inspector);
        connect(dockInsp, SIGNAL(visibilityChanged(bool)), act(Fb::ViewInspector), SLOT(setChecked(bool)));
        m_owner->addDockWidget(Qt::BottomDockWidgetArea, dockInsp);
    } else {
        dockImgs->hide();
    }
}

void FbTextEdit::hideDocks()
{
    if (dockTree) {
        dockTree->deleteLater();
        dockTree = 0;
    }
    if (dockImgs) {
        dockImgs->deleteLater();
        dockImgs = 0;
    }
    if (dockInsp) {
        dockInsp->hide();
    }
}

void FbTextEdit::treeDestroyed()
{
    m_actions[Fb::ViewContents]->setChecked(false);
    dockTree = 0;
}

void FbTextEdit::imgsDestroyed()
{
    m_actions[Fb::ViewPictures]->setChecked(false);
    dockImgs = 0;
}

FbNoteView & FbTextEdit::noteView()
{
    if (m_noteView) return *m_noteView;
    m_noteView = new FbNoteView(qobject_cast<QWidget*>(parent()), url());
    m_noteView->setPage(new FbTextPage(this));
    m_noteView->page()->setNetworkAccessManager(page()->networkAccessManager());
    m_noteView->page()->setContentEditable(false);
    m_noteView->setGeometry(QRect(100, 100, 400, 200));
    return *m_noteView;
}

QWebElement FbTextEdit::body()
{
    return doc().findFirst("body");
}

QWebElement FbTextEdit::doc()
{
    return page()->mainFrame()->documentElement();
}

void FbTextEdit::mouseMoveEvent(QMouseEvent *event)
{
    m_point = event->pos();
    QWebView::mouseMoveEvent(event);
}

void FbTextEdit::contextMenu(const QPoint &pos)
{
    QMenu menu, *submenu;

    submenu = menu.addMenu(tr("Fo&rmat"));
    submenu->addAction(pageAction(QWebPage::RemoveFormat));
    submenu->addSeparator();
    submenu->addAction(pageAction(QWebPage::ToggleBold));
    submenu->addAction(pageAction(QWebPage::ToggleItalic));
    submenu->addAction(pageAction(QWebPage::ToggleStrikethrough));
    submenu->addAction(pageAction(QWebPage::ToggleSuperscript));
    submenu->addAction(pageAction(QWebPage::ToggleSubscript));

    menu.addSeparator();

    menu.addAction(pageAction(QWebPage::Cut));
    menu.addAction(pageAction(QWebPage::Copy));
    menu.addAction(pageAction(QWebPage::Paste));
    menu.addAction(pageAction(QWebPage::PasteAndMatchStyle));

    menu.exec(mapToGlobal(pos));
}

void FbTextEdit::linkHovered(const QString &link, const QString &title, const QString &textContent)
{
    Q_UNUSED(title);
    Q_UNUSED(textContent);

    const QString href = QUrl(link).fragment();
    if (href.isEmpty()) {
        if (m_noteView) m_noteView->hide();
        return;
    }

    const QString query = QString("fb\\:section#%1").arg(href);
    const QWebElement element = doc().findFirst(query);
    if (element.isNull()) {
        if (m_noteView) m_noteView->hide();
        return;
    }

    QRect rect = geometry();
    QSize size = element.geometry().size() + QSize(2, 4);
    int center = rect.size().height() / 2;
    int h = size.height();
    if (h > center) size.setHeight(center - 10);
    int x = (rect.size().width() - size.width()) / 2;
    int y = m_point.y();
    if ( y > h ) y = y - h - 10; else y = y + 10;
    QPoint point = QPoint(x, y) + rect.topLeft();
    noteView().hint(element, QRect(point, size));
}

void FbTextEdit::html(QString html)
{
/*
    if (!m_thread) return;
    static int number = 0;
    QWebSettings::clearMemoryCaches();
    QUrl url(QString("fb2:/%1/").arg(number++));
    FbTextPage *page = m_thread->page();
    setPage(page);
    page->setParent(this);
    page->temp()->setPath(url.path());
    setHtml(html, url);
    connect(page, SIGNAL(linkHovered(QString,QString,QString)), SLOT(linkHovered(QString,QString,QString)));
    m_thread->deleteLater();
    m_thread = 0;
*/
}

bool FbTextEdit::save(QIODevice *device, const QString &codec)
{
    FbSaveWriter writer(*this, device);
    if (!codec.isEmpty()) writer.setCodec(codec.toLatin1());
    bool ok = FbSaveHandler(writer).save();
    if (ok) page()->undoStack()->setClean();
    return ok;
}

bool FbTextEdit::save(QByteArray *array)
{
    FbSaveWriter writer(*this, array);
    return FbSaveHandler(writer).save();
}

bool FbTextEdit::save(QString *string)
{
    // Use class QByteArray instead QString
    // to store information about encoding.
    QByteArray data;
    bool ok = save(&data);
    if (ok) *string = QString::fromUtf8(data.data());
    return ok;
}

QString FbTextEdit::toHtml()
{
    return page()->mainFrame()->toHtml();
}

void FbTextEdit::data(QString name, QByteArray data)
{
    files()->data(name, data);
}

void FbTextEdit::zoomIn()
{
    qreal zoom = zoomFactor();
    setZoomFactor(zoom * 1.1);
}

void FbTextEdit::zoomOut()
{
    qreal zoom = zoomFactor();
    setZoomFactor(zoom * 0.9);
}

void FbTextEdit::zoomReset()
{
    setZoomFactor(1);
}

bool FbTextEdit::actionEnabled(QWebPage::WebAction action)
{
    QAction *act = pageAction(action);
    return act ? act->isEnabled() : false;
}

bool FbTextEdit::actionChecked(QWebPage::WebAction action)
{
    QAction *act = pageAction(action);
    return act ? act->isChecked() : false;
}

bool FbTextEdit::BoldChecked()
{
    return pageAction(QWebPage::ToggleBold)->isChecked();
}

bool FbTextEdit::ItalicChecked()
{
    return pageAction(QWebPage::ToggleItalic)->isChecked();
}

bool FbTextEdit::StrikeChecked()
{
    return pageAction(QWebPage::ToggleStrikethrough)->isChecked();
}

bool FbTextEdit::SubChecked()
{
    return pageAction(QWebPage::ToggleSubscript)->isChecked();
}

bool FbTextEdit::SupChecked()
{
    return pageAction(QWebPage::ToggleSuperscript)->isChecked();
}

void FbTextEdit::find()
{
    FbTextFindDlg dlg(*this);
    dlg.exec();
}

void FbTextEdit::insertImage()
{
    QString filters;
    filters += tr("Common Graphics (*.png *.jpg *.jpeg *.gif);;");
    filters += tr("Portable Network Graphics (PNG) (*.png);;");
    filters += tr("JPEG (*.jpg *.jpeg);;");
    filters += tr("Graphics Interchange Format (*.gif);;");
    filters += tr("All Files (*)");

    QString path = QFileDialog::getOpenFileName(this, tr("Insert image..."), QString(), filters);
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QByteArray data = file.readAll();
    QString name = files()->add(path, data);
    execCommand("insertImage", name.prepend("#"));
}

void FbTextEdit::insertNote()
{
    FbNoteDlg dlg(*this);
    dlg.exec();
}

void FbTextEdit::insertLink()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Insert hyperlink"), tr("URL:"), QLineEdit::Normal, QString(), &ok);
    if (ok && !text.isEmpty()) execCommand("CreateLink", text);
}

void FbTextEdit::execCommand(const QString &cmd, const QString &arg)
{
    QString javascript = QString("document.execCommand(\"%1\",false,\"%2\")").arg(cmd).arg(arg);
    page()->mainFrame()->evaluateJavaScript(javascript);
}

//---------------------------------------------------------------------------
//  FbTextFrame
//---------------------------------------------------------------------------

FbTextFrame::FbTextFrame(QWidget *parent)
    : QFrame(parent)
{
    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Sunken);

    QLayout * layout = new QBoxLayout(QBoxLayout::LeftToRight, this);
    layout->setSpacing(0);
    layout->setMargin(0);
    setLayout(layout);
}

