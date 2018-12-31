/* This source file is part of the Tomviz project, https://tomviz.org/.
   It is released under the 3-Clause BSD License, see "LICENSE". */

#ifndef tomvizAlignWidget_h
#define tomvizAlignWidget_h

#include "EditOperatorWidget.h"

#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkVector.h>

#include <QPointer>
#include <QVector>

class QLabel;
class QComboBox;
class QFileDialog;
class QSpinBox;
class QTimer;
class QKeyEvent;
class QButtonGroup;
class QPushButton;
class QRadioButton;
class QTableWidget;

class vtkImageData;
class vtkImageSlice;
class vtkImageSliceMapper;
class vtkInteractorStyleRubberBand2D;
class vtkInteractorStyleRubberBandZoom;
class vtkRenderer;

namespace tomviz {

class DataSource;
class SpinBox;
class TranslateAlignOperator;
class ViewMode;
class QVTKGLWidget;

class AlignWidget : public EditOperatorWidget
{
  Q_OBJECT

public:
  AlignWidget(TranslateAlignOperator* op, vtkSmartPointer<vtkImageData> data,
              QWidget* parent = nullptr);
  ~AlignWidget() override;

  // This will filter the QVTKGLWidget events
  bool eventFilter(QObject* object, QEvent* event) override;

  void applyChangesToOperator() override;

protected:
  void changeSlice(int delta);
  void setSlice(int slice, bool resetInc = true);
  void widgetKeyPress(QKeyEvent* key);
  void applySliceOffset(int sliceNumber = -1);

  void zoomToSelectionFinished();

protected slots:
  void changeMode(int mode);
  void updateReference();
  void setFrameRate(int rate);
  void currentSliceEdited();

  void startAlign();
  void stopAlign();
  void onTimeout();

  void zoomToSelectionStart();

  void resetCamera();

  void onPresetClicked();
  void applyCurrentPreset();

  void sliceOffsetEdited(int slice, int offsetComponent);

  void onSaveClicked();
  void onLoadClicked();

protected:
  vtkNew<vtkRenderer> m_renderer;
  vtkNew<vtkInteractorStyleRubberBand2D> m_defaultInteractorStyle;
  vtkNew<vtkInteractorStyleRubberBandZoom> m_zoomToBoxInteractorStyle;
  vtkSmartPointer<vtkImageData> m_inputData;
  QVTKGLWidget* m_widget;

  QComboBox* m_modeSelect;
  QTimer* m_timer;
  SpinBox* m_currentSlice;
  QLabel* m_currentSliceOffset;
  QButtonGroup* m_referenceSliceMode;
  QRadioButton* m_prevButton;
  QRadioButton* m_nextButton;
  QRadioButton* m_statButton;
  QSpinBox* m_refNum;
  QSpinBox* m_fpsSpin;
  QPushButton* m_startButton;
  QPushButton* m_stopButton;
  QTableWidget* m_offsetTable;

  int m_frameRate = 5;
  int m_referenceSlice = 0;
  int m_observerId = 0;

  int m_maxSliceNum = 1;
  int m_minSliceNum = 0;

  QVector<ViewMode*> m_modes;
  int m_currentMode = 0;

  QVector<vtkVector2i> m_offsets;
  QPointer<TranslateAlignOperator> m_operator;

private:
  int restoreDraftDialog() const;
  QString dialogToFileName(QFileDialog*) const;
  void loadAlignError(QString) const;
};
} // namespace tomviz

#endif // tomvizAlignWidget
