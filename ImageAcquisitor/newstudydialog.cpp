#include "newstudydialog.h"
#include "ui_newstudydialog.h"
#include "../share/global.h"
#include "../MainStation/mainwindow.h"

#include "dcmtk/dcmdata/dcuid.h"

#include <QSettings>
#include <QMessageBox>

NewStudyDialog::NewStudyDialog(const StudyRecord &studyRec, QWidget *parent) :
    study(studyRec),
    QDialog(parent),
    ui(new Ui::NewStudyDialog)
{
    ui->setupUi(this);
    init();
}

NewStudyDialog::~NewStudyDialog()
{
    delete ui;
}

void NewStudyDialog::init()
{
    ui->patientBirthDateEdit->setMaximumDate(QDate::currentDate());
    connect(ui->okButton, SIGNAL(clicked()), this, SLOT(onOk()));
    connect(ui->patientAgeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateBirth()));
    connect(ui->patientAgeSpin, SIGNAL(valueChanged(int)), this, SLOT(updateBirth()));
    connect(ui->patientBirthDateEdit, SIGNAL(dateChanged(QDate)), this, SLOT(updateAge(QDate)));

    QSettings s;
    /*
    const CustomizedId &pidf = mainWindow->getPatientIdFormat();
    int start = s.value(PATIENTID_START).toInt();
    ui->patientIdEdit->setText(QString("%1%2%3").arg(pidf.prefix)
                               .arg(start, pidf.digits, 10, QChar('0'))
                               .arg(pidf.suffix));
                               */
    const CustomizedId &aidf = mainWindow->getAccNumFormat();
    int start = s.value(ACCNUMBER_START).toInt();
    ui->accNumEdit->setText(QString("%1%2%3").arg(aidf.prefix)
                            .arg(start, aidf.digits, 10, QChar('0'))
                            .arg(aidf.suffix));

    QStringList phys = s.value(REQ_PHYSICIANS).toStringList();
    ui->reqPhysicianCombo->addItems(phys);
    phys = s.value(PER_PHYSICIANS).toStringList();
    ui->perPhysicianCombo->addItems(phys);

    ui->reqPhysicianCombo->setCurrentText(mainWindow->getCurrentUser().name);
    ui->perPhysicianCombo->setCurrentText(mainWindow->getCurrentUser().name);

    ui->patientIdEdit->setText(study.patientId);
    ui->patientSexCombo->setCurrentText(study.patientSex);
    ui->patientBirthDateEdit->setDate(study.patientBirth);
    updateAge(study.patientBirth);
    ui->patientNameEdit->setText(study.patientName);
    ui->patientAddrEdit->setText(study.patientAddr);
    ui->patientPhoneEdit->setText(study.patientPhone);
    ui->patientSizeDSpin->setValue(study.patientSize.toDouble());
    ui->patientWeightDSpin->setValue(study.patientWeight.toDouble());
    ui->medicalAlertEdit->setText(study.medicalAlert);
    ui->modalityCombo->setCurrentText(study.modality);
    ui->studyDescEdit->setText(study.studyDesc);
    if (!study.studyUid.isEmpty()) {
        ui->accNumEdit->setText(study.accNumber);
        ui->reqPhysicianCombo->setCurrentText(study.reqPhysician);
        ui->perPhysicianCombo->setCurrentText(study.perPhysician);
    }
}

void NewStudyDialog::updateAge(const QDate &date)
{
    if (ui->patientBirthDateEdit->hasFocus()) {
        QDate curDate = QDate::currentDate();
        if (curDate.year() - date.year() > 1) {
            ui->patientAgeSpin->setValue(curDate.year()-date.year());
            ui->patientAgeCombo->setCurrentIndex(0);
        } else if ((curDate.year() > date.year()) || (curDate.month() - date.month() > 1)) {
            ui->patientAgeSpin->setValue(curDate.month()-date.month()+(curDate.year()-date.year())*12);
            ui->patientAgeCombo->setCurrentIndex(1);
        } else if ((curDate.month() > date.month()) || (curDate.weekNumber()-date.weekNumber()>1)) {
            ui->patientAgeSpin->setValue(date.daysTo(curDate)/7);
            ui->patientAgeCombo->setCurrentIndex(2);
        } else {
            ui->patientAgeSpin->setValue(date.daysTo(curDate));
            ui->patientAgeCombo->setCurrentIndex(3);
        }
    }
}

void NewStudyDialog::updateBirth()
{
    if (ui->patientAgeCombo->hasFocus() || ui->patientAgeSpin->hasFocus()) {
        int value = ui->patientAgeSpin->value();
        QDate curDate = QDate::currentDate();
        switch (ui->patientAgeCombo->currentIndex()) {
        case 0:
            ui->patientBirthDateEdit->setDate(curDate.addYears(-value));
            break;
        case 1:
            ui->patientBirthDateEdit->setDate(curDate.addMonths(-value));
            break;
        case 2:
            ui->patientBirthDateEdit->setDate(curDate.addDays(-(value*7)));
            break;
        case 3:
            ui->patientBirthDateEdit->setDate(curDate.addDays(-value));
            break;
        }
    }
}

void NewStudyDialog::onOk()
{
    study.accNumber = ui->accNumEdit->text();
    study.patientId = ui->patientIdEdit->text();
    study.patientName = ui->patientNameEdit->text();
    if (study.accNumber.isEmpty() || study.patientId.isEmpty() ||
            study.patientName.isEmpty() || (ui->patientAgeSpin->value()==0)) {
        QMessageBox::critical(this, tr("Register Study"),
                              tr("Mandatory fields empty."));
        return;
    }

    QString ageUnit;
    switch (ui->patientAgeCombo->currentIndex()) {
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
    study.patientAge = QString("%1%2").arg(ui->patientAgeSpin->value()).arg(ageUnit);

    study.patientSex = trSex2Sex(ui->patientSexCombo->currentText());
    study.patientBirth = ui->patientBirthDateEdit->date();
    study.patientAddr = ui->patientAddrEdit->text();
    study.patientPhone = ui->patientPhoneEdit->text();
    study.patientSize = QString::number(ui->patientSizeDSpin->value());
    study.patientWeight = QString::number(ui->patientWeightDSpin->value());
    study.medicalAlert = ui->medicalAlertEdit->text();

    study.reqPhysician = ui->reqPhysicianCombo->currentText();
    study.perPhysician = ui->perPhysicianCombo->currentText();
    study.modality = ui->modalityCombo->currentText();
    study.studyDesc = ui->studyDescEdit->text();

    QSettings s;
    if (study.studyUid.isEmpty()) {
        char uid[128];
        study.studyUid = QString::fromLatin1(dcmGenerateUniqueIdentifier(uid, SITE_STUDY_UID_ROOT));
        study.studyTime = QDateTime::currentDateTime();
        s.setValue(ACCNUMBER_START, s.value(ACCNUMBER_START).toInt()+1);
    }

    QStringList phys = s.value(REQ_PHYSICIANS).toStringList();
    phys.removeOne(study.reqPhysician);
    phys.prepend(study.reqPhysician);
    s.setValue(REQ_PHYSICIANS, phys);
    phys = s.value(PER_PHYSICIANS).toStringList();
    phys.removeOne(study.perPhysician);
    phys.prepend(study.perPhysician);
    s.setValue(PER_PHYSICIANS, phys);

    accept();
}
