/* This source file is part of the Tomviz project, https://tomviz.org/.
   It is released under the 3-Clause BSD License, see "LICENSE". */

#include "DoubleSpinBox.h"

#include <QMouseEvent>
#include <QStyle>
#include <QStyleOptionSpinBox>

namespace tomviz {

DoubleSpinBox::DoubleSpinBox(QWidget* p)
  : QDoubleSpinBox(p), pressInUp(false), pressInDown(false)
{}

void DoubleSpinBox::mousePressEvent(QMouseEvent* event)
{
  QDoubleSpinBox::mousePressEvent(event);

  QStyleOptionSpinBox opt;
  this->initStyleOption(&opt);

  if (this->style()
        ->subControlRect(QStyle::CC_SpinBox, &opt, QStyle::SC_SpinBoxUp)
        .contains(event->pos())) {
    this->pressInUp = true;
  } else if (this->style()
               ->subControlRect(QStyle::CC_SpinBox, &opt,
                                QStyle::SC_SpinBoxDown)
               .contains(event->pos())) {
    this->pressInDown = true;
  } else {
    this->pressInUp = this->pressInDown = false;
  }
  if (this->pressInUp || this->pressInDown) {
    this->connect(this, SIGNAL(valueChanged(double)), this,
                  SIGNAL(editingFinished()));
  }
}

void DoubleSpinBox::mouseReleaseEvent(QMouseEvent* event)
{
  QDoubleSpinBox::mouseReleaseEvent(event);

  if (this->pressInUp || this->pressInDown) {
    this->disconnect(this, SIGNAL(valueChanged(double)), this,
                     SIGNAL(editingFinished()));
  }

  QStyleOptionSpinBox opt;
  this->initStyleOption(&opt);

  if (this->style()
        ->subControlRect(QStyle::CC_SpinBox, &opt, QStyle::SC_SpinBoxUp)
        .contains(event->pos())) {
    if (this->pressInUp) {
      emit this->editingFinished();
    }
  } else if (this->style()
               ->subControlRect(QStyle::CC_SpinBox, &opt,
                                QStyle::SC_SpinBoxDown)
               .contains(event->pos())) {
    if (this->pressInDown) {
      emit this->editingFinished();
    }
  }
  this->pressInUp = this->pressInDown = false;
}
} // namespace tomviz
