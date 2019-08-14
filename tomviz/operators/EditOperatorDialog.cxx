/* This source file is part of the Tomviz project, https://tomviz.org/.
   It is released under the 3-Clause BSD License, see "LICENSE". */

#include "EditOperatorDialog.h"

#include "ActiveObjects.h"
#include "DataSource.h"
#include "EditOperatorWidget.h"
#include "ModuleManager.h"
#include "Operator.h"
#include "Pipeline.h"
#include "Utilities.h"

#include <pqApplicationCore.h>
#include <pqPythonSyntaxHighlighter.h>
#include <pqSettings.h>

#include <QDebug>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>

namespace tomviz {

class EditOperatorDialog::EODInternals
{
public:
  QPointer<Operator> Op;
  EditOperatorWidget* Widget;
  bool needsToBeAdded;
  DataSource* dataSource;

  void saveGeometry(const QRect& geometry)
  {
    if (Op.isNull()) {
      return;
    }
    QSettings* settings = pqApplicationCore::instance()->settings();
    QString settingName =
      QString("Edit%1OperatorDialogGeometry").arg(Op->label());
    settings->setValue(settingName, QVariant(geometry));
  }

  QVariant loadGeometry()
  {
    if (Op.isNull()) {
      return QVariant();
    }
    QSettings* settings = pqApplicationCore::instance()->settings();
    QString settingName =
      QString("Edit%1OperatorDialogGeometry").arg(Op->label());
    return settings->value(settingName);
  }
};

EditOperatorDialog::EditOperatorDialog(Operator* op, DataSource* dataSource,
                                       bool needToAddOperator, QWidget* p)
  : Superclass(p), Internals(new EditOperatorDialog::EODInternals())
{
  Q_ASSERT(op);
  this->Internals->Op = op;
  this->Internals->dataSource = dataSource;
  this->Internals->needsToBeAdded = needToAddOperator;
  if (this->Internals->dataSource->pipeline()->isRunning()) {
    auto result = QMessageBox::question(
      this, "Cancel running operation?",
      "Editing or adding an operator that is part of a running pipeline "
      "will cancel the current running operation and restart the pipeline "
      "Proceed anyway?");
    if (result == QMessageBox::No) {
      QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
      return;
    } else {
      auto whenCanceled = []() {};
      this->Internals->dataSource->pipeline()->cancel(whenCanceled);
    }
  }
  // Connect to the finished signal on the pipeline to handle the UI after
  // pressing apply
  connect(this, &EditOperatorDialog::editStarted,
          this->Internals->dataSource->pipeline(), &Pipeline::startedEditingOp);
  connect(this, &EditOperatorDialog::editEnded,
          this->Internals->dataSource->pipeline(),
          &Pipeline::finishedEditingOp);
  emit editStarted(this->Internals->Op);
  // If editing an existing operator, still emit the signal to disable
  // menubar buttons to add new operators to the current source
  if (!needToAddOperator) {
    ActiveObjects::instance().setActiveDataSource(this->Internals->dataSource);
  }

  if (needToAddOperator) {
    op->setParent(this);
    this->Internals->dataSource->addOperator(this->Internals->Op);
  }

  QVariant geometry = this->Internals->loadGeometry();
  if (!geometry.isNull()) {
    this->setGeometry(geometry.toRect());
  }

  if (op->hasCustomUI()) {
    EditOperatorWidget* opWidget = op->getEditorContents(this);
    if (opWidget != nullptr) {
      this->setupUI(opWidget);
    }
    // We need the image data for call the datasource to run the pipeline
    else {
      auto operators = op->dataSource()->operators();
      auto future =
        dataSource->pipeline()->execute(op->dataSource(), nullptr, op);
      connect(future, &Pipeline::Future::finished, this,
              &EditOperatorDialog::getCopyOfImagePriorToFinished);
    }
  } else {
    this->setupUI();
  }
  op->setCustomDialog(this);
}

EditOperatorDialog::~EditOperatorDialog() {}

void EditOperatorDialog::setViewMode(const QString& mode)
{
  if (this->Internals->Widget) {
    this->Internals->Widget->setViewMode(mode);
  }
}

Operator* EditOperatorDialog::op()
{
  return this->Internals->Op;
}

void EditOperatorDialog::applyChanges()
{
  if (this->Internals->Op.isNull()) {
    return;
  }

  if (this->Internals->Widget) {
    // If we are modifying an operator that is already part of a pipeline and
    // the pipeline is running it has to cancel the currently running pipeline
    // first. Warn the user rather that just canceling potentially long-running
    // operations.
    if (this->Internals->dataSource->pipeline()->isRunning()) {
      auto result = QMessageBox::question(
        this, "Cancel running operation?",
        "Applying changes to an operator that is part of a running pipeline "
        "will cancel the current running operator and restart the pipeline "
        "run.  Proceed anyway?");
      // FIXME There is still a concurrency issue here if the background thread
      // running the operator finishes and the finished event is queued behind
      // the question() return event above.  If that happens then we will not
      // get a canceled() event and the pipeline will stay paused.
      if (result == QMessageBox::No) {
        return;
      } else {
        auto op = this->Internals->Op;
        auto dataSource = this->Internals->dataSource;
        auto whenCanceled = [op, dataSource]() {
          // Resume the pipeline and emit transformModified
          dataSource->pipeline()->resume(false);
          emit op->transformModified();
        };
        if (this->Internals->needsToBeAdded) {
          this->Internals->needsToBeAdded = false;
        }
        emit editEnded(this->Internals->Op);
        // We do this before causing cancel so the values are in place for when
        // whenCanceled cause the pipeline to be re-executed.
        this->Internals->Widget->applyChangesToOperator();
        if (dataSource->pipeline()->isRunning()) {
          this->Internals->dataSource->pipeline()->cancel(whenCanceled);
        } else {
          whenCanceled();
        }
      }
    } else {
      this->Internals->Widget->applyChangesToOperator();
      if (this->Internals->needsToBeAdded) {
        this->Internals->needsToBeAdded = false;
      }
      // Emit edit ended, so that the pipeline can decide whether to execute
      // if there are no other operators being edited at the moment
      emit editEnded(this->Internals->Op);
    }
  }
}

void EditOperatorDialog::closeDialog()
{
  this->Internals->saveGeometry(this->geometry());
  ActiveObjects::instance().setActiveDataSource(this->Internals->dataSource);
}

void EditOperatorDialog::onApply()
{
  applyChanges();
  // Changes are applied, but the dialog is still open.
  // Let's fire the editStarted signal.
  emit editStarted(this->Internals->Op);
}

void EditOperatorDialog::onOkay()
{
  applyChanges();
  closeDialog();
}

void EditOperatorDialog::onCancel()
{
  if (this->Internals->Op == nullptr) {
    return;
  }
  if (this->Internals->needsToBeAdded) {
    // Since for now operators can't be programmatically removed
    // (i.e. all removals are assumed to be initialized
    // from the gui in PipelineModel), we need to do a workaround and have the
    // ModuleManaged emit a signal that is captured by the PipelineModel,
    // which eventually will lead to the removal of the operator.
    ModuleManager::instance().removeOperator(this->Internals->Op);
  }
  emit editEnded(this->Internals->Op);
  closeDialog();
}

void EditOperatorDialog::setupUI(EditOperatorWidget* opWidget)
{
  if (this->Internals->Op.isNull()) {
    return;
  }

  QVBoxLayout* vLayout = new QVBoxLayout(this);
  vLayout->setContentsMargins(5, 5, 5, 5);
  vLayout->setSpacing(5);
  if (this->Internals->Op->hasCustomUI()) {
    vLayout->addWidget(opWidget);
    this->Internals->Widget = opWidget;
    const double* dsPosition = this->Internals->dataSource->displayPosition();
    opWidget->dataSourceMoved(dsPosition[0], dsPosition[1], dsPosition[2]);
    QObject::connect(this->Internals->dataSource,
                     &DataSource::displayPositionChanged, opWidget,
                     &EditOperatorWidget::dataSourceMoved);
  } else {
    this->Internals->Widget = nullptr;
  }
  QDialogButtonBox* dialogButtons = new QDialogButtonBox(
    QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::Ok,
    Qt::Horizontal, this);
  vLayout->addWidget(dialogButtons);
  dialogButtons->button(QDialogButtonBox::Ok)->setDefault(false);

  this->setLayout(vLayout);

  connect(dialogButtons, &QDialogButtonBox::accepted, this,
          &EditOperatorDialog::accept);
  connect(dialogButtons, &QDialogButtonBox::rejected, this,
          &EditOperatorDialog::reject);
  connect(dialogButtons->button(QDialogButtonBox::Apply), &QPushButton::clicked,
          this, &EditOperatorDialog::onApply);

  connect(this, &EditOperatorDialog::rejected, this,
          &EditOperatorDialog::onCancel);

  connect(this, &EditOperatorDialog::accepted, this,
          &EditOperatorDialog::onOkay);
}

void EditOperatorDialog::getCopyOfImagePriorToFinished()
{
  if (this->Internals->Op.isNull()) {
    return;
  }

  auto future = qobject_cast<Pipeline::Future*>(this->sender());

  auto opWidget =
    this->Internals->Op->getEditorContentsWithData(this, future->result());
  this->setupUI(opWidget);

  future->deleteLater();
}

void EditOperatorDialog::showDialogForOperator(Operator* op,
                                               const QString& viewMode)
{
  if (!op) {
    return;
  }

  if (op->hasCustomUI()) {
    // See if we already have a dialog open for this operator
    auto dialog = op->customDialog();
    if (dialog) {
      dialog->setViewMode(viewMode);
      dialog->show();
      dialog->raise();
      dialog->activateWindow();
    } else {
      // Create a non-modal dialog, delete it once it has been closed.
      QString dialogTitle("Edit - ");
      dialogTitle.append(op->label());
      dialog = new EditOperatorDialog(op, op->dataSource(), false,
                                      tomviz::mainWidget());

      dialog->setViewMode(viewMode);
      dialog->setAttribute(Qt::WA_DeleteOnClose, true);
      dialog->setWindowTitle(dialogTitle);
      dialog->show();

      // Close the dialog if the Operator is destroyed.
      connect(op, SIGNAL(destroyed()), dialog, SLOT(reject()));
    }
  }
}
} // namespace tomviz
