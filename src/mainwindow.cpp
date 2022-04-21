/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/***************************************************************************
 *            mainwindow.cpp
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

#include <stdio.h>
#include <fstream>

#include <QtWidgets>
#include <QSettings>
#include <QFile>
#include <QTransform>

#include "mainwindow.h"
#include "aboutbox.h"
#include "configdialog.h"

extern QSettings *settings;

constexpr int maxSize = 1000;

MainWindow::MainWindow()
{
  if(settings->contains("main/windowState")) {
    restoreGeometry(settings->value("main/windowState", "").toByteArray());
  } else {
    setMinimumWidth(640);
  }

  if(settings->allKeys().count() == 0) {
    showPreferences();
  }

  setWindowTitle("LithoMaker v" VERSION);

  createActions();
  createMenus();

  QLabel *minThicknessLabel = new QLabel(tr("Minimum thickness (mm):"));
  minThicknessSlider = new Slider("render", "minThickness", 8, 100, 8, 10);
  
  QLabel *totalThicknessLabel = new QLabel(tr("Total thickness (mm):"));
  totalThicknessSlider = new Slider("render", "totalThickness", 20, 150, 40, 10);

  QLabel *borderLabel = new QLabel(tr("Frame border (mm):"));
  borderSlider = new Slider("render", "frameBorder", 20, 500, 30, 10);

  QLabel *widthLabel = new QLabel(tr("Width, including frame borders (mm):"));
  widthSlider = new Slider("render", "width", 200, 4000, 2000, 10);

  QLabel *inputLabel = new QLabel(tr("Input PNG image filename:"));
  inputLineEdit = new QLineEdit(settings->value("main/inputFilePath", "examples/hummingbird.png").toString());
  inputButton = new QPushButton(tr("..."));
  connect(inputButton, &QPushButton::clicked, this, &MainWindow::inputSelect);
  QHBoxLayout *inputLayout = new QHBoxLayout();
  inputLayout->addWidget(inputLineEdit);
  inputLayout->addWidget(inputButton);

  QLabel *outputLabel = new QLabel(tr("Output STL filename:"));
  outputLineEdit = new QLineEdit(settings->value("main/outputFilePath", "lithophane.stl").toString());
  outputButton = new QPushButton(tr("..."));
  connect(outputButton, &QPushButton::clicked, this, &MainWindow::outputSelect);
  QHBoxLayout *outputLayout = new QHBoxLayout();
  outputLayout->addWidget(outputLineEdit);
  outputLayout->addWidget(outputButton);

  renderButton = new QPushButton(tr("Render"));
  connect(renderButton, &QPushButton::clicked, this, &MainWindow::render);
  exportButton = new QPushButton(tr("Export"));
  connect(exportButton, &QPushButton::clicked, this, &MainWindow::exportStl);
  
  renderProgress = new QProgressBar(this);
  renderProgress->setRange(0, 100);
  renderProgress->setValue(0);
  renderProgress->setFormat(tr("%p%"));

  statusMessage = new QLabel(tr("Ready"), this);
  
  QHBoxLayout *buttonsLayout = new QHBoxLayout();
  buttonsLayout->addWidget(renderButton);
  buttonsLayout->addWidget(exportButton);

  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(minThicknessLabel);
  layout->addWidget(minThicknessSlider);
  layout->addWidget(totalThicknessLabel);
  layout->addWidget(totalThicknessSlider);
  layout->addWidget(borderLabel);
  layout->addWidget(borderSlider);
  layout->addWidget(widthLabel);
  layout->addWidget(widthSlider);
  layout->addWidget(inputLabel);
  layout->addLayout(inputLayout);
  layout->addWidget(outputLabel);
  layout->addLayout(outputLayout);
  layout->addLayout(buttonsLayout);
  layout->addStretch();
  layout->addWidget(renderProgress);
  layout->addWidget(statusMessage);
  
  preview = new Preview(this);

  QHBoxLayout *hLayout = new QHBoxLayout();
  hLayout->addLayout(layout, 1);
  setCentralWidget(new QWidget());
  centralWidget()->setLayout(hLayout);

  connect(lithophane.get(), &Lithophane::progress, renderProgress, &QProgressBar::setValue);

  show();

  if(QFile::exists(outputLineEdit->text()))
    preview->loadStl(outputLineEdit->text());
}

MainWindow::~MainWindow()
{
  settings->setValue("main/windowState", saveGeometry());
  settings->setValue("main/inputFilePath", inputLineEdit->text());
  settings->setValue("main/outputFilePath", outputLineEdit->text());
}

void MainWindow::createActions()
{
  quitAct = new QAction("&Quit", this);
  quitAct->setIcon(QIcon(":quit.png"));
  connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);

  preferencesAct = new QAction(tr("Edit &Preferences..."), this);
  preferencesAct->setIcon(QIcon(":preferences.png"));
  connect(preferencesAct, &QAction::triggered, this, &MainWindow::showPreferences);

  aboutAct = new QAction(tr("&About..."), this);
  aboutAct->setIcon(QIcon(":about.png"));
  connect(aboutAct, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::createMenus()
{
  fileMenu = new QMenu(tr("&File"), this);
  fileMenu->addAction(quitAct);

  optionsMenu = new QMenu(tr("&Options"), this);
  optionsMenu->addAction(preferencesAct);

  helpMenu = new QMenu(tr("&Help"), this);
  helpMenu->addAction(aboutAct);

  menuBar = new QMenuBar();
  menuBar->addMenu(fileMenu);
  menuBar->addMenu(optionsMenu);
  menuBar->addMenu(helpMenu);

  setMenuBar(menuBar);
}

void MainWindow::showAbout()
{
  // Spawn about window
  AboutBox aboutBox(this);
  aboutBox.exec();
}

void MainWindow::showPreferences()
{
  // Spawn preferences dialog
  ConfigDialog preferences(this);
  preferences.exec();
}

const QImage MainWindow::getImage()
{
  QImageReader imgReader(inputLineEdit->text());
  imgReader.setAutoTransform(true);
  QImage image;
  imgReader.read(&image);

  if(image.width() > maxSize || image.height() > maxSize)
  {
    auto message = tr(
      "The input image is quite large. It is recommended to keep it at a resolution lower or equal to %1x%2 " 
      "pixels to avoid an unnecessarily complex 3D mesh. Do you want LithoMaker to resize the image before "
      "processing it?"
    ).arg(QString::number(maxSize), QString::number(maxSize));
    // auto response = QMessageBox::question(this, tr("Large image"), message);
    // if(response == QMessageBox::Yes)
    {
      if(image.width() > image.height()) {
        image = image.scaledToWidth(maxSize);
      } else {
        image = image.scaledToHeight(maxSize);
      }
    }
  }
  if(!image.isGrayscale()) {
    printf("Converting image to grayscale.\n");
    image = image.convertToFormat(QImage::Format_Grayscale8);
  }
  image.invertPixels();

  return image;
} 

void MainWindow::render()
{
  if(!QFileInfo::exists(inputLineEdit->text())) {
    QMessageBox::warning(
      this, tr("File not found"),
      tr("Input file doesn't exist. Please check filename and permissions.")
    );
    return;
  }

  if(settings->value("render/frameBorder").toFloat() * 2 > settings->value("render/width").toFloat()) {
    QMessageBox::warning(
      this, tr("Border too thick"),
      tr("The chosen frame border size exceeds the size of the total lithophane width. Please correct this.")
    );
    return;
  }

  disableUi();

  printf("Rendering STL...\n");
  const QImage image = getImage();

  const float width = settings->value("render/width").toFloat();
  const float frameBorder = settings->value("render/frameBorder").toFloat();
  const int noOfHangers = settings->value("render/enableHangers", true).toBool()? settings->value("render/hangers").toInt() : 0;
  const float stabilizerThreshold = settings->value("render/enableStabilizers", true).toBool()? settings->value("render/stabilizerThreshold", 60.0f).toFloat() : 0.0f;
  
  lithophane->reset();
  lithophane->configure(
    image,
    width,
    settings->value("render/totalThickness").toFloat(),
    settings->value("render/minThickness").toFloat(),
    frameBorder,
    settings->value("render/frameSlopeFactor", "0.75").toFloat(),
    settings->value("render/permanentStabilizers", "false").toBool(),
    settings->value("render/stabilizerHeightFactor", 0.15).toFloat(),
    stabilizerThreshold,
    noOfHangers
  );

  // Render Lithophane
  statusMessage->setText("Rendering...");
  // lithophane->generate();
  lithophane->uv_sphere(1000, 1000, (float) width / 2.0f);
  lithophane->uv_sphere(200, 200, (float) (width - 3.0f)  / 2.0f, true);
  
  printf("Rendering finished...\n");
  statusMessage->setText("Rendering finished"); 

  QList<QVector3D> data = lithophane->getVertices();
  preview->loadData(data);
  preview->setCameraPosition(QVector3D(0.0f, (float)lithophane->getHeight() * 0.45f, (float)width * 1.75f));

  enableUi();
}

void MainWindow::exportStl()
{ 
  disableUi();

  statusMessage->setText("Saving to file...");
  renderProgress->setValue(0);
  auto [ok, message] = lithophane->saveToStl(
    outputLineEdit->text(),
    settings->value("export/stlFormat", "binary").toString(),
    settings->value("export/alwaysOverwrite", false).toBool()
  );
  renderProgress->setValue(ok? 100 : 0);
  statusMessage->setText(message);

  enableUi();
}

void MainWindow::inputSelect()
{
  auto path = QFileInfo(inputLineEdit->text()).absolutePath();
  QString selectedFile = QFileDialog::getOpenFileName(this, tr("Select input file"), path, tr("PNG or JPG Images (*.png *.jpg)"));
  if(selectedFile != QByteArray()) {
    inputLineEdit->setText(selectedFile);
  }
}

void MainWindow::outputSelect()
{
  auto path = QFileInfo(outputLineEdit->text()).absolutePath();
  QString selectedFile = QFileDialog::getSaveFileName(this, tr("Enter output file"), path, "STL 3D Model (*.stl)");
  if(selectedFile != QByteArray()) {
    if(selectedFile.right(4).toLower() != ".stl") {
      selectedFile.append(".stl");
    }
    outputLineEdit->setText(selectedFile);
  }
}

void MainWindow::enableUi()
{
  inputButton->setEnabled(true);
  outputButton->setEnabled(true);
  renderButton->setEnabled(true);
}

void MainWindow::disableUi()
{
  inputButton->setEnabled(false);
  outputButton->setEnabled(false);
  renderButton->setEnabled(false);
}
