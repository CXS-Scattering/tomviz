/* This source file is part of the Tomviz project, https://tomviz.org/.
   It is released under the 3-Clause BSD License, see "LICENSE". */

#include "StartServerDialog.h"
#include "ui_StartServerDialog.h"

#include <pqApplicationCore.h>
#include <pqSettings.h>

#include <QFileDialog>
#include <QFontMetrics>

const char* PYTHON_PATH_DEFAULT = "Enter Python path ...";

namespace tomviz {

StartServerDialog::StartServerDialog(QWidget* parent)
  : QDialog(parent), m_ui(new Ui::StartServerDialog)
{
  m_ui->setupUi(this);

  connect(m_ui->pythonPathLineEdit, &QLineEdit::textChanged, [this]() {
    auto currentText = m_ui->pythonPathLineEdit->text();
    auto valid = !currentText.isEmpty() && currentText != PYTHON_PATH_DEFAULT;
    m_ui->startButton->setEnabled(valid);
    if (valid) {
      setPythonExecutablePath(currentText);
    }
  });

  readSettings();

  connect(m_ui->browseButton, &QPushButton::clicked, [this]() {
    QFileInfo info(m_pythonExecutablePath);
    auto pythonExecutablePath = QFileDialog::getOpenFileName(
      this, "Select Python Executable", info.dir().path());

    if (!pythonExecutablePath.isEmpty()) {
      setPythonExecutablePath(pythonExecutablePath);
      writeSettings();
    }
  });

  connect(m_ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
  connect(m_ui->startButton, &QPushButton::clicked, this, &QDialog::accept);
}

StartServerDialog::~StartServerDialog() = default;

void StartServerDialog::readSettings()
{
  auto settings = pqApplicationCore::instance()->settings();
  settings->beginGroup("acquisition");
  if (!settings->contains("pythonExecutablePath")) {
    setPythonExecutablePath(PYTHON_PATH_DEFAULT);
  } else {
    setPythonExecutablePath(settings->value("pythonExecutablePath").toString());
  }
  settings->endGroup();
}

void StartServerDialog::writeSettings()
{
  auto settings = pqApplicationCore::instance()->settings();
  settings->beginGroup("acquisition");
  settings->setValue("pythonExecutablePath", m_pythonExecutablePath);
  settings->endGroup();
}

void StartServerDialog::setPythonExecutablePath(const QString& path)
{
  m_pythonExecutablePath = path;
  m_ui->pythonPathLineEdit->setText(path);
}
} // namespace tomviz
