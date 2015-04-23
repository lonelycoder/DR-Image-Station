#include "studyregisterwidget.h"
#include "ui_studyregisterwidget.h"

#include "worklistitemmodel.h"
#include "../DicomService/wlistscuthread.h"
#include "../MainStation/mainwindow.h"
#include "../MainStation/studydbmanager.h"
#include "../share/studyrecord.h"

#include "dcmtk/dcmdata/dcuid.h"

#include <QSortFilterProxyModel>
#include <QMessageBox>
#include <QSettings>
#include <QDate>

StudyRegisterWidget::StudyRegisterWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StudyRegisterWidget)
{
    ui->setupUi(this);
    init();
}

StudyRegisterWidget::~StudyRegisterWidget()
{
    clearWlistScps();
    delete ui;
}

void StudyRegisterWidget::init()
{
    wlistModel = new WorklistItemModel(this);
    wlistProxyModel = new QSortFilterProxyModel(this);
    wlistProxyModel->setSourceModel(wlistModel);
    ui->wlistTableView->setModel(wlistProxyModel);
    ui->wlistTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    wlistThread = new WlistSCUThread(wlistModel, this);

    onWlistToday();
    createConnections();
    setPermissions();

    QSettings s;
    const CustomizedId &pidf = mainWindow->getPatientIdFormat();
    int start = s.value(PATIENTID_START).toInt();
    ui->newPatientIdEdit->setText(QString("%1%2%3").arg(pidf.prefix)
                               .arg(start, pidf.digits, 10, QChar('0'))
                               .arg(pidf.suffix));
    const CustomizedId &aidf = mainWindow->getAccNumFormat();
    start = s.value(ACCNUMBER_START).toInt();
    ui->newAccNumEdit->setText(QString("%1%2%3").arg(aidf.prefix)
                            .arg(start, aidf.digits, 10, QChar('0'))
                            .arg(aidf.suffix));

    QStringList phys = s.value(REQ_PHYSICIANS).toStringList();
    ui->newReqPhysicianCombo->addItems(phys);
    phys = s.value(PER_PHYSICIANS).toStringList();
    ui->newPerPhysicianCombo->addItems(phys);

    ui->newReqPhysicianCombo->setCurrentText(mainWindow->getCurrentUser().name);
    ui->newPerPhysicianCombo->setCurrentText(mainWindow->getCurrentUser().name);
    ui->newPatientBirthDateEdit->setDate(QDate::currentDate());
    ui->newPatientBirthDateEdit->setMaximumDate(QDate::currentDate());
}

void StudyRegisterWidget::clearWlistScps()
{
    for (int i = 0; i < ui->serverCombo->count(); ++i) {
        delete reinterpret_cast<DicomScp*>(ui->serverCombo->itemData(i).toULongLong());
    }
    ui->serverCombo->clear();
}

void StudyRegisterWidget::setPermissions()
{
    GroupPermissions perm = mainWindow->getCurrentGroup().permissions;
    ui->beginStudyButton->setEnabled(perm & GP_RegisterStudy);
}

void StudyRegisterWidget::onWlistScpUpdated(const QList<DicomScp *> &scps)
{
    clearWlistScps();

    foreach (DicomScp *scp, scps) {
        DicomScp *newScp = new DicomScp(*scp);
        ui->serverCombo->addItem(newScp->id, (qulonglong)newScp);
    }
}

void StudyRegisterWidget::createConnections()
{
    connect(ui->beginStudyButton, SIGNAL(clicked()), this, SLOT(onBeginNewStudy()));
    connect(ui->newPatientAgeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateBirth()));
    connect(ui->newPatientAgeSpin, SIGNAL(valueChanged(int)), this, SLOT(updateBirth()));
    connect(ui->newPatientBirthDateEdit, SIGNAL(dateChanged(QDate)), this, SLOT(updateAge(QDate)));
    connect(ui->todayButton, SIGNAL(clicked()), this, SLOT(onWlistToday()));
    connect(ui->thisWeekButton, SIGNAL(clicked()), this, SLOT(onWlistThisWeek()));
    connect(ui->thisMonthButton, SIGNAL(clicked()), this, SLOT(onWlistThisMonth()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(onWlistClear()));
    connect(ui->searchButton, SIGNAL(clicked(bool)), this, SLOT(onWlistSearch(bool)));
    connect(ui->wlistTableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onWlistDoubleClicked(QModelIndex)));
    connect(wlistThread, SIGNAL(finished()), this, SLOT(onWlistScuFinished()));
}

void StudyRegisterWidget::updateAge(const QDate &date)
{
    if (ui->newPatientBirthDateEdit->hasFocus()) {
        QDate curDate = QDate::currentDate();
        if (curDate.year() - date.year() > 1) {
            ui->newPatientAgeSpin->setValue(curDate.year()-date.year());
            ui->newPatientAgeCombo->setCurrentIndex(0);
        } else if ((curDate.year() > date.year()) || (curDate.month() - date.month() > 1)) {
            ui->newPatientAgeSpin->setValue(curDate.month()-date.month()+(curDate.year()-date.year())*12);
            ui->newPatientAgeCombo->setCurrentIndex(1);
        } else if ((curDate.month() > date.month()) || (curDate.weekNumber()-date.weekNumber()>1)) {
            ui->newPatientAgeSpin->setValue(date.daysTo(curDate)/7);
            ui->newPatientAgeCombo->setCurrentIndex(2);
        } else {
            ui->newPatientAgeSpin->setValue(date.daysTo(curDate));
            ui->newPatientAgeCombo->setCurrentIndex(3);
        }
    }
}

void StudyRegisterWidget::updateBirth()
{
    if (ui->newPatientAgeCombo->hasFocus() || ui->newPatientAgeSpin->hasFocus()) {
        int value = ui->newPatientAgeSpin->value();
        QDate curDate = QDate::currentDate();
        ui->newPatientBirthDateEdit->setMaximumDate(curDate);
        switch (ui->newPatientAgeCombo->currentIndex()) {
        case 0:
            ui->newPatientBirthDateEdit->setDate(curDate.addYears(-value));
            break;
        case 1:
            ui->newPatientBirthDateEdit->setDate(curDate.addMonths(-value));
            break;
        case 2:
            ui->newPatientBirthDateEdit->setDate(curDate.addDays(-(value*7)));
            break;
        case 3:
            ui->newPatientBirthDateEdit->setDate(curDate.addDays(-value));
            break;
        }
    }
}

void StudyRegisterWidget::onBeginNewStudy()
{
    StudyRecord study;
    study.accNumber = ui->newAccNumEdit->text();
    study.patientId = ui->newPatientIdEdit->text();
    study.patientName = ui->newPatientNameEdit->text();
    if (study.accNumber.isEmpty() || study.patientId.isEmpty() ||
            study.patientName.isEmpty() || (ui->newPatientAgeSpin->value()==0)) {
        QMessageBox::critical(this, tr("Register Study"),
                              tr("Mandatory fields empty."));
    } else {
        study.patientSex = trSex2Sex(ui->newPatientSexCombo->currentText());
        study.patientBirth = ui->newPatientBirthDateEdit->date();

        QString ageUnit;
        switch (ui->newPatientAgeCombo->currentIndex()) {
        case 0:
            ageUnit = "Y";
            break;
        case 1:
            ageUnit = "M";
            break;
        case 2:
            ageUnit = "W";
            break;
        case 3:
            ageUnit = "D";
            break;
        }
        study.patientAge = QString("%1%2").arg(ui->newPatientAgeSpin->value()).arg(ageUnit);

        study.medicalAlert = ui->newMedicalAlertEdit->text();
        study.patientAddr = ui->newPatientAddrEdit->text();
        study.patientPhone = ui->newPatientPhoneEdit->text();
        if (ui->newPatientWeightDSpin->value() > 0)
            study.patientWeight = QString::number(ui->newPatientWeightDSpin->value());
        if (ui->newPatientSizeDSpin->value() > 0)
            study.patientSize = QString::number(ui->newPatientSizeDSpin->value());
        study.reqPhysician = ui->newReqPhysicianCombo->currentText();
        study.perPhysician = ui->newPerPhysicianCombo->currentText();
        study.modality = ui->newModalityCombo->currentText();
        study.studyDesc = ui->newStudyDescEdit->text();
        study.studyTime = QDateTime::currentDateTime();
        char uid[128];
        study.studyUid = QString::fromLatin1(dcmGenerateUniqueIdentifier(uid, SITE_STUDY_UID_ROOT));

        if (StudyDbManager::insertStudyToDb(study)) {
            QSettings s;
            s.setValue(PATIENTID_START, s.value(PATIENTID_START).toInt()+1);
            s.setValue(ACCNUMBER_START, s.value(ACCNUMBER_START).toInt()+1);

            QStringList phys = s.value(REQ_PHYSICIANS).toStringList();
            phys.removeOne(study.reqPhysician);
            phys.prepend(study.reqPhysician);
            s.setValue(REQ_PHYSICIANS, phys);
            phys = s.value(PER_PHYSICIANS).toStringList();
            phys.removeOne(study.perPhysician);
            phys.prepend(study.perPhysician);
            s.setValue(PER_PHYSICIANS, phys);

            emit startAcq(study);
        } else {
            QMessageBox::critical(this, tr("New Study"),
                                  tr("Insert study to Database failed: %1.")
                                  .arg(StudyDbManager::lastError));
        }
    }
}

void StudyRegisterWidget::showEvent(QShowEvent *e)
{
    QSettings s;
    const CustomizedId &pidf = mainWindow->getPatientIdFormat();
    int start = s.value(PATIENTID_START).toInt();
    ui->newPatientIdEdit->setText(QString("%1%2%3").arg(pidf.prefix)
                               .arg(start, pidf.digits, 10, QChar('0'))
                               .arg(pidf.suffix));
    const CustomizedId &aidf = mainWindow->getAccNumFormat();
    start = s.value(ACCNUMBER_START).toInt();
    ui->newAccNumEdit->setText(QString("%1%2%3").arg(aidf.prefix)
                            .arg(start, aidf.digits, 10, QChar('0'))
                            .arg(aidf.suffix));
    ui->newPatientBirthDateEdit->setDate(QDate::currentDate());
    ui->newPatientAgeSpin->setValue(0);
    ui->newPatientNameEdit->clear();
    ui->newMedicalAlertEdit->clear();
    ui->newPatientAddrEdit->clear();
    ui->newPatientPhoneEdit->clear();
    ui->newPatientSizeDSpin->setValue(0);
    ui->newPatientWeightDSpin->setValue(0);

    QWidget::showEvent(e);
}

void StudyRegisterWidget::onWlistToday()
{
    ui->fromCheck->setChecked(true);
    ui->toCheck->setChecked(true);
    ui->fromDateTimeEdit->setDate(QDate::currentDate());
    ui->fromDateTimeEdit->setTime(QTime(0, 0));
    ui->toDateTimeEdit->setDate(QDate::currentDate());
    ui->toDateTimeEdit->setTime(QTime(23, 59, 59, 999));
    emit ui->searchButton->clicked(true);
}

void StudyRegisterWidget::onWlistThisWeek()
{
    ui->fromCheck->setChecked(true);
    ui->toCheck->setChecked(true);
    ui->fromDateTimeEdit->setDate(QDate::currentDate());
    ui->fromDateTimeEdit->setTime(QTime(0, 0));
    ui->toDateTimeEdit->setDate(QDate::currentDate().addDays(6));
    ui->toDateTimeEdit->setTime(QTime(23, 59, 59, 999));
    emit ui->searchButton->clicked(true);
}

void StudyRegisterWidget::onWlistThisMonth()
{
    ui->fromCheck->setChecked(true);
    ui->toCheck->setChecked(true);
    ui->fromDateTimeEdit->setDate(QDate::currentDate());
    ui->fromDateTimeEdit->setTime(QTime(0, 0));
    ui->toDateTimeEdit->setDate(QDate::currentDate().addDays(30));
    ui->toDateTimeEdit->setTime(QTime(23, 59, 59, 999));
    emit ui->searchButton->clicked(true);
}

void StudyRegisterWidget::onWlistSearch(bool checked)
{
    if (checked) {
        ui->searchButton->setChecked(true);
        DicomScp *scp = reinterpret_cast<DicomScp*>(ui->serverCombo->currentData().toULongLong());
        if (!scp) {
            ui->searchButton->setChecked(false);
            return;
        }
        wlistThread->setWorklistScp(*scp);
        wlistThread->setAccNumber(ui->accNumberEdit->text());
        wlistThread->setPatientId(ui->patientIdEdit->text());
        wlistThread->setPatientName(ui->patientNameEdit->text());
        wlistThread->setFromTime(ui->fromCheck->isChecked()?
                                    ui->fromDateTimeEdit->dateTime():
                                    QDateTime());
        wlistThread->setToTime(ui->toCheck->isChecked()?
                                  ui->toDateTimeEdit->dateTime():
                                  QDateTime());
        wlistThread->setModality(ui->modalityCombo->currentText());
        wlistModel->clearAllItems();
        ui->searchButton->setText(tr("Abort"));
        wlistThread->start();
    } else {
        ui->searchButton->setChecked(false);
        wlistThread->setAbort(true);
    }
}

void StudyRegisterWidget::onWlistScuFinished()
{
    ui->searchButton->setChecked(false);
    ui->searchButton->setText(tr("Search"));
}

void StudyRegisterWidget::onWlistDoubleClicked(const QModelIndex &index)
{
    if (mainWindow->getCurrentGroup().permissions & GP_RegisterStudy) {
        QModelIndex idx = wlistProxyModel->mapToSource(index);
        if (idx.isValid()) {
            WorklistItem *item = static_cast<WorklistItem*>(idx.internalPointer());
            StudyRecord study;
            study.studyUid = item->studyUid;
            study.studyUid = item->studyUid;
            study.accNumber = item->accNumber;
            study.patientId = item->patientId;
            study.patientName = item->patientName;
            study.patientSex = item->patientSex;
            study.patientBirth = item->patientBirth;
            study.patientAge = item->patientAge;
            study.medicalAlert = item->medicalAlert;
            study.patientAddr = item->patientAddr;
            study.patientPhone = item->patientPhone;
            study.patientWeight = item->patientWeight;
            study.patientSize = item->patientSize;
            study.studyTime = QDateTime::currentDateTime();
            study.studyDesc = item->studyDesc;
            study.reqPhysician = item->reqPhysician;
            study.perPhysician = item->schPhysician;
            study.procId = item->reqProcId;
            if (StudyDbManager::insertStudyToDb(study)) {
                emit startAcq(study);
                wlistModel->removeRow(idx.row());
            } else {
                QMessageBox::critical(this, tr("New Study"),
                                      tr("Insert study to Database failed: %1.")
                                      .arg(StudyDbManager::lastError));
            }

        }
    }
}

void StudyRegisterWidget::onWlistClear()
{
    ui->patientIdEdit->clear();
    ui->accNumberEdit->clear();
    ui->patientNameEdit->clear();
    ui->modalityCombo->setCurrentIndex(0);
    ui->fromCheck->setChecked(false);
    ui->toCheck->setChecked(false);
}
