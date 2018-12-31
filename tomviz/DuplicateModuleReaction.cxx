/* This source file is part of the Tomviz project, https://tomviz.org/.
   It is released under the 3-Clause BSD License, see "LICENSE". */

#include "DuplicateModuleReaction.h"

#include "ActiveObjects.h"
#include "Module.h"
#include "ModuleFactory.h"
#include "ModuleManager.h"

#include <QJsonObject>

namespace tomviz {

DuplicateModuleReaction::DuplicateModuleReaction(QAction* action)
  : pqReaction(action)
{
  connect(&ActiveObjects::instance(), &ActiveObjects::moduleChanged, this,
          &DuplicateModuleReaction::updateEnableState);
  updateEnableState();
}

void DuplicateModuleReaction::updateEnableState()
{
  parentAction()->setEnabled(ActiveObjects::instance().activeModule() !=
                             nullptr);
}

void DuplicateModuleReaction::onTriggered()
{
  auto module = ActiveObjects::instance().activeModule();
  auto dataSource = module->dataSource();
  auto operatorResult = module->operatorResult();
  auto moleculeSource = module->moleculeSource();
  auto view = ActiveObjects::instance().activeView();
  auto moduleType = ModuleFactory::moduleType(module);
  // Copy the module
  Module* copy;
  if (ModuleFactory::moduleApplicable(moduleType, dataSource, view)) {
    copy = ModuleFactory::createModule(moduleType, dataSource, view);
  } else if (ModuleFactory::moduleApplicable(moduleType, moleculeSource,
                                             view)) {
    copy = ModuleFactory::createModule(moduleType, moleculeSource, view);
  } else {
    copy = ModuleFactory::createModule(moduleType, operatorResult, view);
  }

  if (copy) {
    // Copy its settings
    QJsonObject json = module->serialize();
    copy->deserialize(json);
    ModuleManager::instance().addModule(copy);
  }
}

} // end namespace tomviz
