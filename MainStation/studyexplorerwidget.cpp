#include "studyexplorerwidget.h"
#include "ui_studyexplorerwidget.h"
#include "sqlstudymodel.h"
#include "sqlstudyview.h"
#include "sqlimagemodel.h"
#include "sqlimageview.h"
#include "sqlreportmodel.h"
#include "sqlreportview.h"
#include "../share/global.h"
#include "../share/dicomscp.h"
#include "sendstudydialog.h"
#include "exportimagedialog.h"
#include "../share/studyrecord.h"
#include "studydbmanager.h"
#include "../ImageAcquisitor/newstudydialog.h"
#include "../MainStation/mainwindow.h"
#include "../MainStation/studydbmanager.h"

#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDebug>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>
#include <QHBoxLayout>

StudyExplorerWidget::StudyExplorerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StudyExplorerWidget)
{
    ui->setupUi(this);
    init();
}

StudyExplorerWidget::~StudyExplorerWidget()
{
    delete ui;
}

void StudyExplorerWidget::onToday()
{
    ui->fromCheckBox->setChecked(true);
    ui->fromDateTimeEdit->setDate(QDate::currentDate());
    ui->fromDateTimeEdit->setTime(QTime(0, 0));

    ui->toCheckBox->setChecked(true);
    ui->toDateTimeEdit->setDate(QDate::currentDate());
    ui->toDateTimeEdit->setTime(QTime(23, 59, 59, 999));

    onStudySearch();
}

void StudyExplorerWidget::onLatestWeek()
{
    ui->fromCheckBox->setChecked(true);
    ui->fromDateTimeEdit->setDate(QDate::currentDate().addDays(-6));
    ui->fromDateTimeEdit->setTime(QTime(0, 0));

    ui->toCheckBox->setChecked(true);
    ui->toDateTimeEdit->setDate(QDate::currentDate());
    ui->toDateTimeEdit->setTime(QTime(23, 59, 59, 999));

    onStudySearch();
}

void StudyExplorerWidget::onLatestMonth()
{
    ui->fromCheckBox->setChecked(true);
    ui->fromDateTimeEdit->setDate(QDate::currentDate().addDays(-30));
    ui->fromDateTimeEdit->setTime(QTime(0, 0));

    ui->toCheckBox->setChecked(true);
    ui->toDateTimeEdit->setDate(QDate::currentDate());
    ui->toDateTimeEdit->setTime(QTime(23, 59, 59, 999));

    onStudySearch();
}

void StudyExplorerWidget::onClear()
{
    onToday();
    ui->modalityCombo->setCurrentIndex(0);
    ui->patientIDEdit->clear();
    ui->patientNameEdit->clear();
    ui->accNumberEdit->clear();
}

void StudyExplorerWidget::onStudySearch()
{
    QString filter;
    if (ui->fromCheckBox->isChecked()) {
        filter = QString("StudyTime>\'%1\'").arg(ui->fromDateTimeEdit->dateTime().toString("yyyy-MM-dd hh:mm:ss"));
    }
    if (ui->toCheckBox->isChecked()) {
        if (!filter.isEmpty()) filter.append(" and ");
        filter.append(QString("StudyTime<\'%1\'").arg(ui->toDateTimeEdit->dateTime().toString("yyyy-MM-dd hh:mm:ss")));
    }
    if (!ui->modalityCombo->currentText().isEmpty()) {
        if (!filter.isEmpty()) filter.append(" and ");
        filter.append(QString("Modality=\'%1\'").arg(ui->modalityCombo->currentText()));
    }
    if (!ui->patientIDEdit->text().isEmpty()) {
        if (!filter.isEmpty()) filter.append(" and ");
        filter.append(QString("PatientId LIKE \'%%1%\'").arg(ui->patientIDEdit->text()));
    }
    if (!ui->patientNameEdit->text().isEmpty()) {
        if (!filter.isEmpty()) filter.append(" and ");
        filter.append(QString("PatientName LIKE \'%%1%\'").arg(ui->patientNameEdit->text()));
    }
    if (!ui->accNumberEdit->text().isEmpty()) {
        if (!filter.isEmpty()) filter.append(" and ");
        filter.append(QString("AccNumber LIKE \'%%1%\'").arg(ui->accNumberEdit->text()));
    }

    studyModel->setFilter(filter);
}

void StudyExplorerWidget::init()
{
    if (!StudyDbManager::createStudyDb()) {
        QMessageBox::critical(this, tr("Create Study Database"),
                              tr("Create study database failed: %1.")
                              .arg(StudyDbManager::lastError));
    }

    setupComponents();
    createConnections();
}

void StudyExplorerWidget::createConnections()
{
    connect(ui->todayButton, SIGNAL(clicked()), this, SLOT(onToday()));
    connect(ui->latestWeekButton, SIGNAL(clicked()), this, SLOT(onLatestWeek()));
    connect(ui->latestMonthButton, SIGNAL(clicked()), this, SLOT(onLatestMonth()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(onClear()));
    connect(ui->searchButton, SIGNAL(clicked()), this, SLOT(onStudySearch()));

    connect(this, SIGNAL(studyModified(QSqlRecord)), studyModel, SLOT(onStudyModified(QSqlRecord)));

    connect(studyModel, SIGNAL(studySelectionChanged(QStringList)), imageModel, SLOT(onStudySelected(QStringList)));
    connect(studyModel, SIGNAL(studySelectionChanged(QStringList)), reportModel, SLOT(onStudySelected(QStringList)));
    connect(studyModel, SIGNAL(createReport(QString)), this, SIGNAL(createReport(QString)));
    connect(studyModel, SIGNAL(modifyStudy(QSqlRecord&)), this, SLOT(onModifyStudy(QSqlRecord&)));
    connect(studyModel, SIGNAL(newStudy(QSqlRecord)), this, SLOT(onNewStudyAndAcquisit(QSqlRecord)));
    connect(studyModel, SIGNAL(newImage(QSqlRecord)), this, SLOT(onStudyAcquisit(QSqlRecord)));
    connect(studyModel, SIGNAL(removeFinished()), imageModel, SLOT(select()));
    connect(studyModel, SIGNAL(removeFinished()), reportModel, SLOT(select()));
    connect(studyModel, SIGNAL(selectItems(QModelIndex)), studyView, SLOT(setItemSelected(QModelIndex)));

    connect(studyView, SIGNAL(studySelectionChanged(QModelIndexList)), studyModel, SLOT(onSelectionChanged(QModelIndexList)));
    connect(studyView, SIGNAL(viewImages()), imageModel, SLOT(onViewAllImages()));
    connect(studyView, SIGNAL(viewReports()), reportModel, SLOT(onViewAllReports()));
    connect(studyView, SIGNAL(createReport(QModelIndex)), studyModel, SLOT(onCreateReport(QModelIndex)));
    connect(studyView, SIGNAL(sendStudies()), imageModel, SLOT(onSendAllImages()));
    connect(studyView, SIGNAL(exportStudies()), imageModel, SLOT(onExportAllImages()));
    connect(studyView, SIGNAL(removeStudies()), studyModel, SLOT(onRemoveStudies()));
    connect(studyView, SIGNAL(modifyStudy(QModelIndex)), studyModel, SLOT(onModifyStudy(QModelIndex)));
    connect(studyView, SIGNAL(newImage(QModelIndex)), studyModel, SLOT(onNewImage(QModelIndex)));
    connect(studyView, SIGNAL(newStudy(QModelIndex)), studyModel, SLOT(onNewStudy(QModelIndex)));

    connect(imageView, SIGNAL(viewImage(QModelIndex)), imageModel, SLOT(onViewImage(QModelIndex)));
    connect(imageView, SIGNAL(sendImage(QModelIndex)), imageModel, SLOT(onSendImage(QModelIndex)));
    connect(imageView, SIGNAL(exportImage(QModelIndex)), imageModel, SLOT(onSendImage(QModelIndex)));
    connect(imageView, SIGNAL(removeImage(QModelIndex)), imageModel, SLOT(onRemoveImage(QModelIndex)));

    connect(reportView, SIGNAL(viewReport(QModelIndex)), reportModel, SLOT(onViewReport(QModelIndex)));
    connect(reportView, SIGNAL(exportReport(QModelIndex)), reportModel, SLOT(onExportReport(QModelIndex)));
    connect(reportView, SIGNAL(removeReport(QModelIndex)), reportModel, SLOT(onRemoveReport(QModelIndex)));

    connect(imageModel, SIGNAL(viewImages(QStringList)), this, SIGNAL(viewImages(QStringList)));
    connect(imageModel, SIGNAL(sendImages(QStringList)), this, SLOT(onSendImages(QStringList)));
    connect(imageModel, SIGNAL(exportImages(QStringList)), this, SLOT(onExportImages(QStringList)));

    connect(reportModel, SIGNAL(viewReports(QStringList)), this, SIGNAL(viewReports(QStringList)));
    connect(reportModel, SIGNAL(exportReports(QStringList)), this, SLOT(onExportReports(QStringList)));
}

void StudyExplorerWidget::setupComponents()
{
    QSqlDatabase db = QSqlDatabase::database(STUDY_DB_CONNECTION_NAME);
    studyModel = new SqlStudyModel(this, db);
    studyView = new SqlStudyView;
    studyView->setModel(studyModel);
    studyView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    studyView->hideColumn(SqlStudyModel::StudyUid);
    ui->verticalLayout->insertWidget(2, studyView, 1);

    imageModel = new SqlImageModel(this, db);
    imageView = new SqlImageView;
    imageView->setModel(imageModel);
    imageView->hideColumn(SqlImageModel::ImageUid);
    imageView->hideColumn(SqlImageModel::SopClassUid);
    imageView->hideColumn(SqlImageModel::SeriesUid);
    imageView->hideColumn(SqlImageModel::StudyUid);
    imageView->hideColumn(SqlImageModel::RefImageUid);
    imageView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    reportModel = new SqlReportModel(this, db);
    reportView = new SqlReportView;
    reportView->setModel(reportModel);
    reportView->hideColumn(SqlReportModel::ReportUid);
    reportView->hideColumn(SqlReportModel::SeriesUid);
    reportView->hideColumn(SqlReportModel::StudyUid);
    reportView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget(imageView);
    hlayout->addWidget(reportView);
    ui->verticalLayout->insertLayout(4, hlayout);

    studyModel->setFilter("StudyUid IS NULL");
    studyModel->select();
    onToday();

    imageModel->setFilter("ImageUid IS NULL");
    imageModel->select();
    reportModel->setFilter("ReportUid IS NULL");
    reportModel->select();
}

void StudyExplorerWidget::openOrCreateReport(const QString &studyUid)
{
    if (!reportModel->onViewReport(studyUid))
        emit createReport(studyUid);
}

void StudyExplorerWidget::onSendImages(const QStringList &imageFiles)
{
    SendStudyDialog dialog(this);
    dialog.setImageFiles(imageFiles);
    dialog.exec();
    studyModel->select();
}

void StudyExplorerWidget::onExportImages(const QStringList &imageFiles)
{
    ExportImageDialog dialog(this);
    dialog.setImageFiles(imageFiles);
    dialog.exec();
}

void StudyExplorerWidget::onExportReports(const QStringList &reportFiles)
{

}

void StudyExplorerWidget::onNewStudyAndAcquisit(const QSqlRecord &studyRec)
{
    StudyRecord study;
    //study.studyUid = studyRec.value(SqlStudyModel::StudyUid).toString();
    //study.studyTime = studyRec.value(SqlStudyModel::StudyTime).toDateTime();
    study.patientId = studyRec.value(SqlStudyModel::PatientId).toString();
    study.patientName = studyRec.value(SqlStudyModel::PatientName).toString();
    study.patientSex = studyRec.value(SqlStudyModel::PatientSex).toString();
    study.patientBirth = studyRec.value(SqlStudyModel::PatientBirth).toDate();
    study.medicalAlert = studyRec.value(SqlStudyModel::MedicalAlert).toString();
    study.patientAddr = studyRec.value(SqlStudyModel::PatientAddr).toString();
    study.patientPhone = studyRec.value(SqlStudyModel::PatientPhone).toString();
    study.patientSize = studyRec.value(SqlStudyModel::PatientSize).toString();
    study.patientWeight = studyRec.value(SqlStudyModel::PatientWeight).toString();
    //study.accNumber = studyRec.value(SqlStudyModel::AccNumber).toString();
    study.reqPhysician = studyRec.value(SqlStudyModel::ReqPhysician).toString();
    study.perPhysician = studyRec.value(SqlStudyModel::PerPhysician).toString();
    study.modality = studyRec.value(SqlStudyModel::Modality).toString();
    study.studyDesc = studyRec.value(SqlStudyModel::StudyDesc).toString();
    NewStudyDialog dialog(study, this);
    if (QDialog::Accepted == dialog.exec()) {
        if (StudyDbManager::insertStudyToDb(study)) {
            study = dialog.getStudyRecord();
            //emit studyCreated(study);
            emit startAcq(study);
        } else {
            QMessageBox::critical(this, tr("New Study"),
                                  tr("Insert study to Database failed: %1.")
                                  .arg(StudyDbManager::lastError));
        }
    }
}

void StudyExplorerWidget::onModifyStudy(QSqlRecord &studyRec)
{
    StudyRecord study;
    study.studyUid = studyRec.value(SqlStudyModel::StudyUid).toString();
    study.patientId = studyRec.value(SqlStudyModel::PatientId).toString();
    study.patientName = studyRec.value(SqlStudyModel::PatientName).toString();
    study.patientSex = studyRec.value(SqlStudyModel::PatientSex).toString();
    study.patientBirth = studyRec.value(SqlStudyModel::PatientBirth).toDate();
    study.patientAddr = studyRec.value(SqlStudyModel::PatientAddr).toString();
    study.patientPhone = studyRec.value(SqlStudyModel::PatientPhone).toString();
    study.patientSize = studyRec.value(SqlStudyModel::PatientSize).toString();
    study.patientWeight = studyRec.value(SqlStudyModel::PatientWeight).toString();
    study.medicalAlert = studyRec.value(SqlStudyModel::MedicalAlert).toString();
    study.accNumber = studyRec.value(SqlStudyModel::AccNumber).toString();
    study.reqPhysician = studyRec.value(SqlStudyModel::ReqPhysician).toString();
    study.perPhysician = studyRec.value(SqlStudyModel::PerPhysician).toString();
    study.modality = studyRec.value(SqlStudyModel::Modality).toString();
    study.studyDesc = studyRec.value(SqlStudyModel::StudyDesc).toString();
    NewStudyDialog dialog(study, this);
    if (QDialog::Accepted == dialog.exec()) {
        study = dialog.getStudyRecord();
        studyRec.setValue(SqlStudyModel::AccNumber, study.accNumber);
        studyRec.setValue(SqlStudyModel::PatientId, study.patientId);
        studyRec.setValue(SqlStudyModel::PatientName, study.patientName);
        studyRec.setValue(SqlStudyModel::PatientSex, study.patientSex);
        studyRec.setValue(SqlStudyModel::PatientBirth, study.patientBirth);
        studyRec.setValue(SqlStudyModel::PatientAge, study.patientAge);
        studyRec.setValue(SqlStudyModel::PatientAddr, study.patientAddr);
        studyRec.setValue(SqlStudyModel::PatientPhone, study.patientPhone);
        studyRec.setValue(SqlStudyModel::PatientSize, study.patientSize);
        studyRec.setValue(SqlStudyModel::PatientWeight, study.patientWeight);
        studyRec.setValue(SqlStudyModel::MedicalAlert, study.medicalAlert);
        studyRec.setValue(SqlStudyModel::ReqPhysician, study.reqPhysician);
        studyRec.setValue(SqlStudyModel::PerPhysician, study.perPhysician);
        studyRec.setValue(SqlStudyModel::Modality, study.modality);
        studyRec.setValue(SqlStudyModel::StudyDesc, study.studyDesc);
        emit studyModified(studyRec);
    }
}

void StudyExplorerWidget::studyAcquisit()
{
    studyView->onNewImage();
}

void StudyExplorerWidget::onStudyAcquisit(const QSqlRecord &studyRec)
{
    StudyRecord study;
    study.studyUid = studyRec.value(SqlStudyModel::StudyUid).toString();
    study.studyTime = studyRec.value(SqlStudyModel::StudyTime).toDateTime();
    study.patientId = studyRec.value(SqlStudyModel::PatientId).toString();
    study.patientName = studyRec.value(SqlStudyModel::PatientName).toString();
    study.patientSex = studyRec.value(SqlStudyModel::PatientSex).toString();
    study.patientBirth = studyRec.value(SqlStudyModel::PatientBirth).toDate();
    study.accNumber = studyRec.value(SqlStudyModel::AccNumber).toString();
    study.reqPhysician = studyRec.value(SqlStudyModel::ReqPhysician).toString();
    study.perPhysician = studyRec.value(SqlStudyModel::PerPhysician).toString();
    study.modality = studyRec.value(SqlStudyModel::Modality).toString();
    study.studyDesc = studyRec.value(SqlStudyModel::StudyDesc).toString();
    emit startAcq(study);
}

void StudyExplorerWidget::showEvent(QShowEvent *e)
{
    studyModel->select();
    QWidget::showEvent(e);
}
