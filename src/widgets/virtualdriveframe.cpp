#include "virtualdriveframe.h"
#include "ui_virtualdriveframe.h"

#include <QFile>
#include <QFileDialog>
#include <QStandardItemModel>

VirtualDriveFrame::VirtualDriveFrame(
    const QDir prefix,
    const QString letter,
    const QString path,
    QWidget *parent
)
    : QFrame(parent)
    , ui(new Ui::VirtualDriveFrame)
{
    ui->setupUi(this);

    m_prefix = prefix;
    m_letter = letter;

    for (const auto& let : m_letters) {
        ui->dirLetter->addItem(let.toUpper(), let);
    }
    ui->dirLetter->setCurrentIndex(
        ui->dirLetter->findData(letter, Qt::ItemDataRole::UserRole, Qt::MatchFlag::MatchFixedString)
    );
    auto letterFont = this->font();
    letterFont.setPointSize(10);
    letterFont.setBold(true);
    ui->dirLetter->setFont(letterFont);
    ui->dirLetter->setFrame(false);
    connect( ui->dirLetter, qOverload<int>(&QComboBox::currentIndexChanged),
             this, &VirtualDriveFrame::onDirLetterCurrentIndexChanged );

    auto mainLabelFont = this->font();
    mainLabelFont.setPointSize(10);
    mainLabelFont.setItalic(true);
    ui->dirPath->setFont(mainLabelFont);
    ui->dirPath->setText(path);

    QFile windowsLabel( QString(path).append("/.windows-label") ) ;
    if( windowsLabel.open( QIODevice::ReadWrite ) ) {
        ui->dirWinLabel->setText( windowsLabel.readLine().trimmed() );
    }
    ui->dirWinLabel->setPlaceholderText("Drive label");
    auto lineEditFont = this->font();
    lineEditFont.setPointSize(9);
    ui->dirWinLabel->setFont(lineEditFont);
    ui->dirWinLabel->setFrame( false );
    ui->dirWinLabel->setMaxLength(16);
    ui->dirWinLabel->setStyleSheet("QLineEdit { background-color: transparent } ");
    connect( ui->dirWinLabel, &QLineEdit::textEdited, this, &VirtualDriveFrame::onDirWinLaberEdited );

    ui->dirChange->setText("");
    ui->dirChange->setIcon(QIcon::fromTheme("document-open"));
    ui->dirChange->setIconSize( QSize( 23, 23 ) );
    ui->dirChange->setToolTip("Change symlink to new directory.");
    connect( ui->dirChange, &QPushButton::clicked, this, &VirtualDriveFrame::onDirChangeClicked );

    ui->dirDelete->setText("");
    ui->dirDelete->setIcon(QIcon::fromTheme("edit-delete"));
    ui->dirDelete->setIconSize( QSize( 23, 23 ) );
    ui->dirDelete->setFlat( true );
    ui->dirDelete->setToolTip( "Delete this symlink." );
    connect( ui->dirDelete, &QPushButton::clicked, this, &VirtualDriveFrame::onDirDeleteClicked );

    if(letter == "c:") {
        ui->dirLetter->setEnabled(false);
        ui->dirPath->setEnabled(false);
        ui->dirWinLabel->setEnabled(false);
        ui->dirChange->setVisible(false);
        ui->dirDelete->setVisible(false);
    }
}

VirtualDriveFrame::~VirtualDriveFrame()
{
    delete ui;
}

void VirtualDriveFrame::update()
{
    for ( int index = 0; index < ui->dirLetter->count(); ++index ) {
        auto data = ui->dirLetter->itemData(index, Qt::ItemDataRole::UserRole);
        QFileInfo parser(m_prefix.path().append("/").append(data.toString()));

        auto * model = qobject_cast<QStandardItemModel*>(ui->dirLetter->model());
        auto * item = model->item(index);
        item->setEnabled( !parser.isDir() );
    }
}

void VirtualDriveFrame::onDirLetterCurrentIndexChanged(int index)
{
    auto data = ui->dirLetter->itemData(index, Qt::ItemDataRole::UserRole).toString();

    QFile renamer;
    renamer.rename(
        m_prefix.path().append("/").append(m_letter),
        m_prefix.path().append("/").append(data)
    );

    emit letterChanged(m_letter, data);
    m_letter = data;
}

void VirtualDriveFrame::onDirWinLaberEdited(const QString text)
{
    QDir label_dir = m_prefix.absoluteFilePath(m_letter);

    if ( label_dir.exists() ) {
        label_dir.remove(".windows-label");

        auto label = text;
        label = label.remove('&').trimmed();

        if ( !label.isEmpty() ) {
            QFile label_file(label_dir.canonicalPath().append("/.windows-label"));
            if( label_file.open(QFile::ReadWrite) ) {
                label_file.write(label.toLocal8Bit());
            }
        }
    }
}

void VirtualDriveFrame::onDirChangeClicked(bool checked)
{
    auto new_path = QDir();
    new_path.setPath(
        QFileDialog::getExistingDirectory(this, "Select a directory", qEnvironmentVariable("HOME"))
    );

    if ( !new_path.path().isEmpty() ) {
        QFile file;
        if ( file.remove( m_prefix.path().append("/").append(m_letter) ) ) {
            if ( file.link( new_path.canonicalPath(), m_prefix.path().append("/").append(m_letter) ) ) {
                ui->dirPath->setText(new_path.path());
            }
        }
    }
}

void VirtualDriveFrame::onDirDeleteClicked(bool)
{
    m_prefix.remove(m_letter);
    emit driveDeleted(m_letter);
}

