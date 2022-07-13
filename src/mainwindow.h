/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/***************************************************************************
 *            mainwindow.h
 *
 *  Wed Nov 3 09:03:00 CEST 2021
 *  Copyright 2021 Lars Muldjord
 *  muldjordlars@gmail.com
 ****************************************************************************/
/*
 *  This file is part of LithoMaker.
 *
 *  LithoMaker is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  LithoMaker is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with LithoMaker; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 */

#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <memory>

#include <QMainWindow>
#include <QLineEdit>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QProgressBar>
#include <QPushButton>
#include <QLabel>
#include <QEntity>

#include "slider.h"
#include "lithophane.h"
#include "preview.h"


class MainWindow : public QMainWindow
{
  Q_OBJECT
    
public:
  MainWindow();
  ~MainWindow();
  
public slots:
  void render();
  void exportStl();

protected:
  
signals:

private slots:
  void showAbout();
  void showPreferences();
  void inputSelect();
  void outputSelect();
  
private:
  void enableUi();
  void disableUi();
  void createActions();
  void createMenus();

  const QImage getImage();

  //QByteArray stlString;
  Slider *minThicknessSlider;
  //QLineEdit *minThicknessLineEdit;
  Slider *totalThicknessSlider;
  //QLineEdit *totalThicknessLineEdit;
  Slider *borderSlider;
  //QLineEdit *borderLineEdit;
  Slider *widthSlider;
  //QLineEdit *widthLineEdit;
  QPushButton *inputButton;
  QLineEdit *inputLineEdit;
  QPushButton *outputButton;
  QPushButton *renderButton;
  QPushButton *exportButton;
  QLineEdit *outputLineEdit;
  QProgressBar *renderProgress;
  QLabel* statusMessage;
  QAction *quitAct;
  QAction *preferencesAct;
  QAction *aboutAct;
  QMenu *fileMenu;
  QMenu *optionsMenu;
  QMenu *helpMenu;
  QMenuBar *menuBar;

  Preview* preview;
  std::unique_ptr<Lithophane> lithophane = std::make_unique<Lithophane>();
};

#endif // __MAINWINDOW_H__
