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

#include "mainwindow.h"
#include "aboutbox.h"
#include "configdialog.h"

extern QSettings *settings;

constexpr int maxSize = 2000;

MainWindow::MainWindow()
{
  if(settings->contains("main/windowState")) {
    restoreGeometry(settings->value("main/windowState", "").toByteArray());
  } else {
    setMinimumWidth(640);
  }

  setWindowTitle("LithoMaker v" VERSION);

  createActions();
  createMenus();

  QDoubleValidator *thicknessValidator = new QDoubleValidator(0.8, 10.0, 1);
  QDoubleValidator *borderValidator = new QDoubleValidator(2.0, 50.0, 1);
  QDoubleValidator *widthValidator = new QDoubleValidator(20.0, 400.0, 1);

  QLabel *minThicknessLabel = new QLabel(tr("Minimum thickness (mm):"));
  minThicknessLineEdit = new QLineEdit(settings->value("main/minThickness", "0.8").toString());
  minThicknessLineEdit->setToolTip(tr("Allowed range: 0.8 to 10.0, 1 decimal"));
  minThicknessLineEdit->setValidator(thicknessValidator);
  
  QLabel *totalThicknessLabel = new QLabel(tr("Total thickness (mm):"));
  totalThicknessLineEdit = new QLineEdit(settings->value("main/totalThickness", "3.0").toString());
  totalThicknessLineEdit->setToolTip(tr("Allowed range: 0.8 to 10.0, 1 decimal"));
  totalThicknessLineEdit->setValidator(thicknessValidator);

  QLabel *borderLabel = new QLabel(tr("Frame border (mm):"));
  borderLineEdit = new QLineEdit(settings->value("main/frameBorder", "3.0").toString());
  borderLineEdit->setToolTip(tr("Allowed range: 2.0 to 50.0, 1 decimal"));
  borderLineEdit->setValidator(borderValidator);

  QLabel *widthLabel = new QLabel(tr("Width, including frame borders (mm):"));
  widthLineEdit = new QLineEdit(settings->value("main/width", "200.0").toString());
  widthLineEdit->setToolTip(tr("Allowed range: 20.0 to 400.0, 1 decimal"));
  widthLineEdit->setValidator(widthValidator);

  QLabel *inputLabel = new QLabel(tr("Input image filename:"));
  inputLineEdit = new QLineEdit(settings->value("main/inputFilePath", "example.png").toString());
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

  renderButton = new QPushButton(tr("Render and export"));
  connect(renderButton, &QPushButton::clicked, this, &MainWindow::renderStl);
  renderProgress = new QProgressBar(this);
  renderProgress->setMinimum(0);
  
  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(minThicknessLabel);
  layout->addWidget(minThicknessLineEdit);
  layout->addWidget(totalThicknessLabel);
  layout->addWidget(totalThicknessLineEdit);
  layout->addWidget(borderLabel);
  layout->addWidget(borderLineEdit);
  layout->addWidget(widthLabel);
  layout->addWidget(widthLineEdit);
  layout->addWidget(inputLabel);
  layout->addLayout(inputLayout);
  layout->addWidget(outputLabel);
  layout->addLayout(outputLayout);
  layout->addWidget(renderProgress);
  layout->addWidget(renderButton);

  setCentralWidget(new QWidget());
  centralWidget()->setLayout(layout);

  show();

}

MainWindow::~MainWindow()
{
  settings->setValue("main/windowState", saveGeometry());
  settings->setValue("main/minThickness", minThicknessLineEdit->text());
  settings->setValue("main/totalThickness", totalThicknessLineEdit->text());
  settings->setValue("main/frameBorder", borderLineEdit->text());
  settings->setValue("main/width", widthLineEdit->text());
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

void MainWindow::renderStl()
{
  if(!QFileInfo::exists(inputLineEdit->text())) {
    QMessageBox::warning(this, tr("File not found"), tr("Input file doesn't exist. Please check filename and permissions."));
    return;
  }
  if(!minThicknessLineEdit->hasAcceptableInput() ||
     !totalThicknessLineEdit->hasAcceptableInput() ||
     !borderLineEdit->hasAcceptableInput() ||
     !widthLineEdit->hasAcceptableInput()) {
    QMessageBox::critical(this, tr("Invalid format"), tr("One or more of the variables are invalid. Hold the mouse pointer over the input fields to see valid ranges and formats."));
    return;
  }

  if(borderLineEdit->text().toFloat() * 2 > widthLineEdit->text().toFloat()) {
    QMessageBox::warning(this, tr("Border too thick"), tr("The chosen frame border size exceeds the size of the total lithophane width. Please correct this."));
    return;
  }

  disableUi();
  
  printf("Rendering STL...\n");
  polygons.clear();
  QImage image(inputLineEdit->text());
  if((image.width() > maxSize || image.height() > maxSize) &&
     QMessageBox::question(this, tr("Large image"), tr("The input image is quite large. It is recommended to keep it at a resolution lower or equal to ") + QString::number(maxSize) + " x " + QString::number(maxSize) + tr(" pixels to avoid an unnecessarily complex 3D mesh. Do you want LithoMaker to resize the image before processing it?")) == QMessageBox::Yes) {
    if(image.width() > image.height()) {
      image = image.scaledToWidth(maxSize);
    } else {
      image = image.scaledToHeight(maxSize);
    }
  }
  if(image.format() != QImage::Format_Grayscale8 &&
     image.format() != QImage::Format_Grayscale16 &&
     QMessageBox::question(this, tr("Not grayscale"), tr("The image is reported to not be grayscale. Do you want LithoMaker to convert it to grayscale before processing it?")) == QMessageBox::Yes) {
    image = image.convertToFormat(QImage::Format_Grayscale16);
  }
  image.invertPixels();
  border = borderLineEdit->text().toFloat();
  depthFactor = (totalThicknessLineEdit->text().toFloat() - minThicknessLineEdit->text().toFloat()) / 255.0;
  widthFactor = (widthLineEdit->text().toFloat() - (border * 2)) / image.width();
  float minThickness = minThicknessLineEdit->text().toFloat() * -1;
  renderProgress->setMaximum(image.height() - 1);
  renderProgress->setValue(0);
  renderProgress->setFormat(tr("Rendering %p%"));
  for(int y = 0; y < image.height() - 1; ++y) {
    // Close left side
    polygons.append(getVertex(0, y, minThickness, true));
    polygons.append(getVertex(0, y, getPixel(image, 0, y) * depthFactor, true));
    polygons.append(getVertex(0, y + 1, getPixel(image, 0, y + 1) * depthFactor, true));
    
    polygons.append(getVertex(0, y + 1, getPixel(image, 0, y + 1) * depthFactor, true));
    polygons.append(getVertex(0, y + 1, minThickness, true));
    polygons.append(getVertex(0, y, minThickness, true));
    for(int x = 0; x < image.width() - 1; ++x) {
      if(y == 0) {
        // Close top
        polygons.append(getVertex(x + 1, 0, getPixel(image, x + 1, 0) * depthFactor, true));
        polygons.append(getVertex(x, 0, getPixel(image, x, 0) * depthFactor, true));
        polygons.append(getVertex(x, 0, minThickness, true));
    	
        polygons.append(getVertex(x, 0, minThickness, true));
        polygons.append(getVertex(x + 1, 0, minThickness, true));
        polygons.append(getVertex(x + 1, 0, getPixel(image, x + 1, 0) * depthFactor, true));
    
        // Close bottom
        polygons.append(getVertex(x, image.height() - 1, minThickness, true));
        polygons.append(getVertex(x, image.height() - 1, getPixel(image, x, image.height() - 1) * depthFactor, true));
        polygons.append(getVertex(x + 1, image.height() - 1, getPixel(image, x + 1, image.height() - 1) * depthFactor, true));
    	
        polygons.append(getVertex(x + 1, image.height() - 1, getPixel(image, x + 1, image.height() - 1) * depthFactor, true));
        polygons.append(getVertex(x + 1, image.height() - 1, minThickness, true));
        polygons.append(getVertex(x, image.height() - 1, minThickness, true));
          }
      // The lithophane heightmap
      polygons.append(getVertex(x, y, getPixel(image, x, y) * depthFactor, true));
      polygons.append(getVertex(x + 1, y + 1, getPixel(image, x + 1, y + 1) * depthFactor, true));
      polygons.append(getVertex(x, y + 1, getPixel(image, x, y + 1) * depthFactor, true));
  
      polygons.append(getVertex(x, y, getPixel(image, x, y) * depthFactor, true));
      polygons.append(getVertex(x + 1, y, getPixel(image, x + 1, y) * depthFactor, true));
      polygons.append(getVertex(x + 1, y + 1, getPixel(image, x + 1, y + 1) * depthFactor, true));
      }
    // Close right side
    polygons.append(getVertex(image.width() - 1, y + 1, getPixel(image, image.width() - 1, y + 1) * depthFactor, true));
    polygons.append(getVertex(image.width() - 1, y, getPixel(image, image.width() - 1, y) * depthFactor, true));
    polygons.append(getVertex(image.width() - 1, y, minThickness, true));
    
    polygons.append(getVertex(image.width() - 1, y, minThickness, true));
    polygons.append(getVertex(image.width() - 1, y + 1, minThickness, true));
    polygons.append(getVertex(image.width() - 1, y + 1, getPixel(image, image.width() - 1, y + 1) * depthFactor, true));
    renderProgress->setValue(renderProgress->value() + 1);
  }
  renderProgress->setValue(renderProgress->maximum());
  renderProgress->setFormat("Ready!");

  // Backside
  polygons.append(getVertex(0, 0, minThickness, true));
  polygons.append(getVertex(image.width() - 1, image.height() - 1, minThickness, true));
  polygons.append(getVertex(0, image.height() - 1, minThickness, true));

  polygons.append(getVertex(0, 0, minThickness, true));
  polygons.append(getVertex(image.width() - 1, 0, minThickness, true));
  polygons.append(getVertex(image.width() - 1, image.height() - 1, minThickness, true));

  // Stabilizers
  if(settings->value("render/enableStabilizers", true).toBool()) {
    polygons.append(addStabilizer(0, ((border * 2) + (image.height() * widthFactor)) * 0.15));
    polygons.append(addStabilizer(widthLineEdit->text().toFloat() - (border < 4?border:4), ((border * 2) + (image.height() * widthFactor)) * 0.15));
  }

  // Frame
  polygons.append(addFrame(widthLineEdit->text().toFloat(), (border * 2) + (image.height() * widthFactor)));
  
  //polygons.append("endsolid\n");
  printf("Rendering finished...\n");
  exportStl();
}

void MainWindow::exportStl()
{
  if(polygons.isEmpty()) {
    QMessageBox::warning(this, tr("Empty STL buffer"), tr("There is currently no rendered lithophane in the STL buffer. You need to render one before you can export it."));
    enableUi();
    return;
  }
  if(QFileInfo::exists(outputLineEdit->text()) && !settings->value("export/alwaysOverwrite", false).toBool() && QMessageBox::question(this, tr("Overwrite file?"), tr("The output STL file already exists. Do you want to overwrite it?")) != QMessageBox::Yes) {
    enableUi();
    return;
  }

  printf("Exporting to file: '%s'... ", outputLineEdit->text().toStdString().c_str());
  
  if(settings->value("export/stlFormat", "binary").toString() == "binary") {
    // Export as binary
    std::ofstream out(outputLineEdit->text().toStdString(), std::ios::binary);
    if(out.good()) {
      char title[80];
      memset(title, 0, 80);
      strcpy(title, "lithophane");
      out.write((char *)&title, 80);
      quint32 polCount = polygons.length() / 3;
      out.write((char *)&polCount, sizeof(quint32));;
      quint16 attrByteCount = 0;
      for(int a = 0; a < polygons.length(); a += 3) {
        float normal = 0.0;
        out.write((char *)&normal, sizeof(float));
        out.write((char *)&normal, sizeof(float));
        out.write((char *)&normal, sizeof(float));
        float x, y, z;
        x = polygons.at(a).x();
        y = polygons.at(a).y();
        z = polygons.at(a).z();
        out.write((char *)&x, sizeof(float));
        out.write((char *)&y, sizeof(float));
        out.write((char *)&z, sizeof(float));
        x = polygons.at(a + 1).x();
        y = polygons.at(a + 1).y();
        z = polygons.at(a + 1).z();
        out.write((char *)&x, sizeof(float));
        out.write((char *)&y, sizeof(float));
        out.write((char *)&z, sizeof(float));
        x = polygons.at(a + 2).x();
        y = polygons.at(a + 2).y();
        z = polygons.at(a + 2).z();
        out.write((char *)&x, sizeof(float));
        out.write((char *)&y, sizeof(float));
        out.write((char *)&z, sizeof(float));
        out.write((char *)&attrByteCount, sizeof(quint16));
      }
      out.close();
      printf("Success!\n");
      QMessageBox::information(this, tr("Export succeeded"), tr("The binary STL was successfully exported. You can now import it in your preferred 3D printing slicer."));
    } else {
      printf("Failed!\n");
      QMessageBox::warning(this, tr("Export failed"), tr("File could not be opened for writing. Please check export filename and try again."));
    }
  } else if(settings->value("export/stlFormat", "binary").toString() == "ascii") {
    // Export as ascii
    QFile stlFile(outputLineEdit->text());
    if(stlFile.open(QIODevice::WriteOnly)) {
      stlFile.write("solid lithophane\n");
      for(int a = 0; a < polygons.length(); a += 3) {
        stlFile.write("facet normal 0.0 0.0 0.0\n");
        stlFile.write("\touter loop\n");
        stlFile.write("\t\tvertex " + QByteArray::number(polygons.at(a).x(), 'g') + " " + QByteArray::number(polygons.at(a).y(), 'g') + " " + QByteArray::number(polygons.at(a).z(), 'g') + "\n");
        stlFile.write("\t\tvertex " + QByteArray::number(polygons.at(a + 1).x(), 'g') + " " + QByteArray::number(polygons.at(a + 1).y(), 'g') + " " + QByteArray::number(polygons.at(a + 1).z(), 'g') + "\n");
        stlFile.write("\t\tvertex " + QByteArray::number(polygons.at(a + 2).x(), 'g') + " " + QByteArray::number(polygons.at(a + 2).y(), 'g') + " " + QByteArray::number(polygons.at(a + 2).z(), 'g') + "\n");
        stlFile.write("\tendloop\n");
        stlFile.write("endfacet\n");
      }
      stlFile.write("endsolid\n");
      stlFile.close();
      printf("Success!\n");
      QMessageBox::information(this, tr("Export succeeded"), tr("The ascii STL was successfully exported. You can now import it in your preferred 3D printing slicer."));
    } else {
      printf("Failed!\n");
      QMessageBox::warning(this, tr("Export failed"), tr("File could not be opened for writing. Please check export filename and try again."));
    }
  }
  enableUi();
}

QList<QVector3D> MainWindow::addStabilizer(const float &x, const float &height)
{
  float depth = height * 0.5;
  float z;

  QList<QVector3D> stabilizer;
  
  z = totalThicknessLineEdit->text().toFloat() - minThicknessLineEdit->text().toFloat() + 1;
  stabilizer.append(getVertex(x, 0.000000, z + 1));
  stabilizer.append(getVertex(x, 0.000000, z + depth));
  stabilizer.append(getVertex(x, height, z + 3));
                    
  stabilizer.append(getVertex(x, height, z + 3));
  stabilizer.append(getVertex(x, height, z + 1));
  stabilizer.append(getVertex(x, height - 1, z + 1));

  stabilizer.append(getVertex(x, height, z + 3));
  stabilizer.append(getVertex(x, height - 1, z + 1));
  stabilizer.append(getVertex(x, 0.000000, z + 1));

  stabilizer.append(getVertex(x + (border < 4?border:4), height, z + 3));
  stabilizer.append(getVertex(x + (border < 4?border:4), 0.000000, z + depth));
  stabilizer.append(getVertex(x + (border < 4?border:4), 0.000000, z + 1));

  stabilizer.append(getVertex(x + (border < 4?border:4), height - 1, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4), height, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4), height, z + 3));

  stabilizer.append(getVertex(x + (border < 4?border:4), 0.000000, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4), height - 1, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4), height, z + 3));

  stabilizer.append(getVertex(x + 1, height, z + 1));
  stabilizer.append(getVertex(x, height, z + 1));
  stabilizer.append(getVertex(x, height, z + 3));

  stabilizer.append(getVertex(x, height, z + 3));
  stabilizer.append(getVertex(x + (border < 4?border:4), height, z + 3));
  stabilizer.append(getVertex(x + (border < 4?border:4), height, z + 1));

  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height, z + 1));
  stabilizer.append(getVertex(x + 1, height, z + 1));
  stabilizer.append(getVertex(x, height, z + 3));

  stabilizer.append(getVertex(x, height, z + 3));
  stabilizer.append(getVertex(x + (border < 4?border:4), height, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height, z + 1));

  stabilizer.append(getVertex(x, 0.000000, z + depth));
  stabilizer.append(getVertex(x, 0.000000, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4), 0.000000, z + 1));

  stabilizer.append(getVertex(x, 0.000000, z + depth));
  stabilizer.append(getVertex(x + (border < 4?border:4), 0.000000, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4), 0.000000, z + depth));

  stabilizer.append(getVertex(x, height, z + 3));
  stabilizer.append(getVertex(x, 0.000000, z + depth));
  stabilizer.append(getVertex(x + (border < 4?border:4), 0.000000, z + depth));

  stabilizer.append(getVertex(x, height, z + 3));
  stabilizer.append(getVertex(x + (border < 4?border:4), 0.000000, z + depth));
  stabilizer.append(getVertex(x + (border < 4?border:4), height, z + 3));

  stabilizer.append(getVertex(x + 1, height - 1, z + 1));
  stabilizer.append(getVertex(x + 1, height, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height, z + 1));

  stabilizer.append(getVertex(x + 1, height - 1, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height - 1, z + 1));

  stabilizer.append(getVertex(x + 1, height - 1, 0.000000));
  stabilizer.append(getVertex(x + 1, height, 0.000000));
  stabilizer.append(getVertex(x, height, 0.000000));

  stabilizer.append(getVertex(x + 1, height - 1, 0.000000));
  stabilizer.append(getVertex(x, height, 0.000000));
  stabilizer.append(getVertex(x, height - 1, 0.000000));

  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height - 1, 0.000000));
  stabilizer.append(getVertex(x + (border < 4?border:4), height - 1, 0.000000));
  stabilizer.append(getVertex(x + (border < 4?border:4), height, 0.000000));

  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height - 1, 0.000000));
  stabilizer.append(getVertex(x + (border < 4?border:4), height, 0.000000));
  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height, 0.000000));

  stabilizer.append(getVertex(x + (border < 4?border:4), height, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4), height - 1, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4), height - 1, 0.000000));

  stabilizer.append(getVertex(x + (border < 4?border:4), height, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4), height - 1, 0.000000));
  stabilizer.append(getVertex(x + (border < 4?border:4), height, 0.000000));

  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4), height, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4), height, 0.000000));

  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4), height, 0.000000));
  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height, 0.000000));

  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height - 1, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height, 0.000000));

  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height - 1, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height, 0.000000));
  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height - 1, 0.000000));

  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height - 1, 0.000000));
  stabilizer.append(getVertex(x + (border < 4?border:4), height - 1, 0.000000));
  stabilizer.append(getVertex(x + (border < 4?border:4), height - 1, z + 1));

  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height - 1, 0.000000));
  stabilizer.append(getVertex(x + (border < 4?border:4), height - 1, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height - 1, z + 1));

  stabilizer.append(getVertex(x, height, z + 1));
  stabilizer.append(getVertex(x + 1, height, z + 1));
  stabilizer.append(getVertex(x + 1, height, 0.000000));

  stabilizer.append(getVertex(x, height, z + 1));
  stabilizer.append(getVertex(x + 1, height, 0.000000));
  stabilizer.append(getVertex(x, height, 0.000000));

  stabilizer.append(getVertex(x, height - 1, z + 1));
  stabilizer.append(getVertex(x, height, z + 1));
  stabilizer.append(getVertex(x, height, 0.000000));

  stabilizer.append(getVertex(x, height - 1, z + 1));
  stabilizer.append(getVertex(x, height, 0.000000));
  stabilizer.append(getVertex(x, height - 1, 0.000000));

  stabilizer.append(getVertex(x + 1, height, z + 1));
  stabilizer.append(getVertex(x + 1, height - 1, z + 1));
  stabilizer.append(getVertex(x + 1, height - 1, 0.000000));

  stabilizer.append(getVertex(x + 1, height, z + 1));
  stabilizer.append(getVertex(x + 1, height - 1, 0.000000));
  stabilizer.append(getVertex(x + 1, height, 0.000000));

  stabilizer.append(getVertex(x + 1, height - 1, 0.000000));
  stabilizer.append(getVertex(x + 1, height - 1, z + 1));
  stabilizer.append(getVertex(x, height - 1, z + 1));

  stabilizer.append(getVertex(x + 1, height - 1, 0.000000));
  stabilizer.append(getVertex(x, height - 1, z + 1));
  stabilizer.append(getVertex(x, height - 1, 0.000000));

  stabilizer.append(getVertex(x + (border < 4?border:4), 0.000000, z + 1));
  stabilizer.append(getVertex(x, 0.000000, z + 1));
  stabilizer.append(getVertex(x, height - 1, z + 1));

  stabilizer.append(getVertex(x, height - 1, z + 1));
  stabilizer.append(getVertex(x + 1, height - 1, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height - 1, z + 1));

  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height - 1, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4), height - 1, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4), 0.000000, z + 1));

  stabilizer.append(getVertex(x, height - 1, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4) - 1, height - 1, z + 1));
  stabilizer.append(getVertex(x + (border < 4?border:4), 0.000000, z + 1));

  // Front
  /*
  stabilizer.append(getVertex(x + (border < 4?border:4), 0, z));
  stabilizer.append(getVertex(x + (border < 4?border:4), height, z));
  stabilizer.append(getVertex(x, height, z));

  stabilizer.append(getVertex(x, height, z));
  stabilizer.append(getVertex(x, 0, z));
  stabilizer.append(getVertex(x + (border < 4?border:4), 0, z));

  stabilizer.append(getVertex(x, 0, z));
  stabilizer.append(getVertex(x, 0, z + depth));
  stabilizer.append(getVertex(x + (border < 4?border:4), 0, z));

  stabilizer.append(getVertex(x + (border < 4?border:4), 0, z));
  stabilizer.append(getVertex(x, 0, z + depth));
  stabilizer.append(getVertex(x + (border < 4?border:4), 0, z + depth));

  stabilizer.append(getVertex(x, 0, z));
  stabilizer.append(getVertex(x, height, z));
  stabilizer.append(getVertex(x, 0, z + depth));

  stabilizer.append(getVertex(x + (border < 4?border:4), 0, z + depth));
  stabilizer.append(getVertex(x + (border < 4?border:4), height, z));
  stabilizer.append(getVertex(x + (border < 4?border:4), 0, z));

  stabilizer.append(getVertex(x, 0, z + depth));
  stabilizer.append(getVertex(x, height, z));
  stabilizer.append(getVertex(x + (border < 4?border:4), height, z));

  stabilizer.append(getVertex(x + (border < 4?border:4), height, z));
  stabilizer.append(getVertex(x + (border < 4?border:4), 0, z + depth));
  stabilizer.append(getVertex(x, 0, z + depth));


  stabilizer.append(addCube(x, height - 4, z - 1, 1));
  stabilizer.append(addCube(x + (border < 4?border:4) - 1, height - 4, z - 1, 1));
  */
  // Back
  z = (minThicknessLineEdit->text().toFloat() * -1) - 1;
  stabilizer.append(getVertex(x, height, z));
  stabilizer.append(getVertex(x + (border < 4?border:4), height, z));
  stabilizer.append(getVertex(x + (border < 4?border:4), 0, z));

  stabilizer.append(getVertex(x + (border < 4?border:4), 0, z));
  stabilizer.append(getVertex(x, 0, z));
  stabilizer.append(getVertex(x, height, z));

  stabilizer.append(getVertex(x + (border < 4?border:4), 0, z));
  stabilizer.append(getVertex(x, 0, z - depth));
  stabilizer.append(getVertex(x, 0, z));

  stabilizer.append(getVertex(x + (border < 4?border:4), 0, z - depth));
  stabilizer.append(getVertex(x, 0, z - depth));
  stabilizer.append(getVertex(x + (border < 4?border:4), 0, z));

  stabilizer.append(getVertex(x, 0, z - depth));
  stabilizer.append(getVertex(x, height, z));
  stabilizer.append(getVertex(x, 0, z));

  stabilizer.append(getVertex(x + (border < 4?border:4), 0, z));
  stabilizer.append(getVertex(x + (border < 4?border:4), height, z));
  stabilizer.append(getVertex(x + (border < 4?border:4), 0, z - depth));

  stabilizer.append(getVertex(x + (border < 4?border:4), height, z));
  stabilizer.append(getVertex(x, height, z));
  stabilizer.append(getVertex(x, 0, z - depth));

  stabilizer.append(getVertex(x, 0, z - depth));
  stabilizer.append(getVertex(x + (border < 4?border:4), 0, z - depth));
  stabilizer.append(getVertex(x + (border < 4?border:4), height, z));

  // Back cubes
  stabilizer.append(addCube(x, height - 4, z, 1));
  stabilizer.append(addCube(x + (border < 4?border:4) - 1, height - 4, z, 1));

  return stabilizer;
}

QList<QVector3D> MainWindow::addCube(const float &x, const float &y, const float &z, const float &size)
{
  QList<QVector3D> cube;
  cube.append(getVertex(x, y, z));
  cube.append(getVertex(x, y + size, z));
  cube.append(getVertex(x, y + size, z + size));

  cube.append(getVertex(x, y + size, z + size));
  cube.append(getVertex(x, y, z + size));
  cube.append(getVertex(x, y, z));

  cube.append(getVertex(x, y, z + size));
  cube.append(getVertex(x, y + size, z + size));
  cube.append(getVertex(x + size, y + size, z + size));

  cube.append(getVertex(x + size, y + size, z + size));
  cube.append(getVertex(x + size, y, z + size));
  cube.append(getVertex(x, y, z + size));

  cube.append(getVertex(x, y + size, z));
  cube.append(getVertex(x + size, y + size, z));
  cube.append(getVertex(x, y + size, z + size));

  cube.append(getVertex(x, y + size, z + size));
  cube.append(getVertex(x + size, y + size, z));
  cube.append(getVertex(x + size, y + size, z + size));

  cube.append(getVertex(x, y, z));
  cube.append(getVertex(x + size, y, z));
  cube.append(getVertex(x, y + size, z));

  cube.append(getVertex(x, y + size, z));
  cube.append(getVertex(x + size, y, z));
  cube.append(getVertex(x + size, y + size, z));

  cube.append(getVertex(x, y, z));
  cube.append(getVertex(x, y, z + size));
  cube.append(getVertex(x + size, y, z));

  cube.append(getVertex(x + size, y, z));
  cube.append(getVertex(x, y, z + size));
  cube.append(getVertex(x + size, y, z + size));

  cube.append(getVertex(x + size, y, z));
  cube.append(getVertex(x + size, y, z + size));
  cube.append(getVertex(x + size, y + size, z));

  cube.append(getVertex(x + size, y + size, z));
  cube.append(getVertex(x + size, y, z + size));
  cube.append(getVertex(x + size, y + size, z + size));

  return cube;
}

QList<QVector3D> MainWindow::addFrame(const float &width, const float &height)
{
  float minThickness = minThicknessLineEdit->text().toFloat();
  float depth = totalThicknessLineEdit->text().toFloat() - minThickness;
  float frameSlope = depth / settings->value("render/frameSlopeFactor", "1.5").toFloat();

  QList<QVector3D> frame;
  frame.append(getVertex(width, height, - minThickness));
  frame.append(getVertex(0.000000, height, - minThickness));
  frame.append(getVertex(0.000000, height, depth));

  frame.append(getVertex(width, height, - minThickness));
  frame.append(getVertex(0.000000, height, depth));
  frame.append(getVertex(width, height, depth));

  frame.append(getVertex(width - border - frameSlope, border + frameSlope, 0.100000));
  frame.append(getVertex(width - border - frameSlope, height - border - frameSlope, 0.100000));
  frame.append(getVertex(border + frameSlope, height - border - frameSlope, 0.100000));

  frame.append(getVertex(width - border - frameSlope, border + frameSlope, 0.100000));
  frame.append(getVertex(border + frameSlope, height - border - frameSlope, 0.100000));
  frame.append(getVertex(border + frameSlope, border + frameSlope, 0.100000));

  frame.append(getVertex(0.000000, 0.000000, depth));
  frame.append(getVertex(0.000000, height, depth));
  frame.append(getVertex(0.000000, height, - minThickness));

  frame.append(getVertex(0.000000, 0.000000, depth));
  frame.append(getVertex(0.000000, height, - minThickness));
  frame.append(getVertex(0.000000, 0.000000, - minThickness));

  frame.append(getVertex(0.000000, 0.000000, - minThickness));
  frame.append(getVertex(width, 0.000000, - minThickness));
  frame.append(getVertex(width, 0.000000, depth));

  frame.append(getVertex(0.000000, 0.000000, - minThickness));
  frame.append(getVertex(width, 0.000000, depth));
  frame.append(getVertex(0.000000, 0.000000, depth));

  frame.append(getVertex(width, 0.000000, - minThickness));
  frame.append(getVertex(width, height, - minThickness));
  frame.append(getVertex(width, height, depth));

  frame.append(getVertex(width, 0.000000, - minThickness));
  frame.append(getVertex(width, height, depth));
  frame.append(getVertex(width, 0.000000, depth));

  frame.append(getVertex(0.000000, 0.000000, - minThickness));
  frame.append(getVertex(0.000000, height, - minThickness));
  frame.append(getVertex(width, height, - minThickness));

  frame.append(getVertex(0.000000, 0.000000, - minThickness));
  frame.append(getVertex(width, height, - minThickness));
  frame.append(getVertex(width, 0.000000, - minThickness));

  frame.append(getVertex(border, border, depth));
  frame.append(getVertex(border, height - border, depth));
  frame.append(getVertex(0.000000, height, depth));

  frame.append(getVertex(border, border, depth));
  frame.append(getVertex(0.000000, height, depth));
  frame.append(getVertex(0.000000, 0.000000, depth));

  frame.append(getVertex(width - border, height - border, depth));
  frame.append(getVertex(width - border, border, depth));
  frame.append(getVertex(width, 0.000000, depth));

  frame.append(getVertex(width - border, height - border, depth));
  frame.append(getVertex(width, 0.000000, depth));
  frame.append(getVertex(width, height, depth));

  frame.append(getVertex(border, height - border, depth));
  frame.append(getVertex(width - border, height - border, depth));
  frame.append(getVertex(width, height, depth));

  frame.append(getVertex(border, height - border, depth));
  frame.append(getVertex(width, height, depth));
  frame.append(getVertex(0.000000, height, depth));

  frame.append(getVertex(width - border, border, depth));
  frame.append(getVertex(border, border, depth));
  frame.append(getVertex(0.000000, 0.000000, depth));

  frame.append(getVertex(width - border, border, depth));
  frame.append(getVertex(0.000000, 0.000000, depth));
  frame.append(getVertex(width, 0.000000, depth));

  frame.append(getVertex(border + frameSlope, border + frameSlope, 0.100000));
  frame.append(getVertex(border + frameSlope, height - border - frameSlope, 0.100000));
  frame.append(getVertex(border, height - border, depth));

  frame.append(getVertex(border + frameSlope, border + frameSlope, 0.100000));
  frame.append(getVertex(border, height - border, depth));
  frame.append(getVertex(border, border, depth));

  frame.append(getVertex(width - border - frameSlope, height - border - frameSlope, 0.100000));
  frame.append(getVertex(width - border - frameSlope, border + frameSlope, 0.100000));
  frame.append(getVertex(width - border, border, depth));

  frame.append(getVertex(width - border - frameSlope, height - border - frameSlope, 0.100000));
  frame.append(getVertex(width - border, border, depth));
  frame.append(getVertex(width - border, height - border, depth));

  frame.append(getVertex(border + frameSlope, height - border - frameSlope, 0.100000));
  frame.append(getVertex(width - border - frameSlope, height - border - frameSlope, 0.100000));
  frame.append(getVertex(width - border, height - border, depth));

  frame.append(getVertex(border + frameSlope, height - border - frameSlope, 0.100000));
  frame.append(getVertex(width - border, height - border, depth));
  frame.append(getVertex(border, height - border, depth));

  frame.append(getVertex(width - border - frameSlope, border + frameSlope, 0.100000));
  frame.append(getVertex(border + frameSlope, border + frameSlope, 0.100000));
  frame.append(getVertex(border, border, depth));

  frame.append(getVertex(width - border - frameSlope, border + frameSlope, 0.100000));
  frame.append(getVertex(border, border, depth));
  frame.append(getVertex(width - border, border, depth));

  return frame;
}

int MainWindow::getPixel(const QImage &image, const int &x, const int &y)
{
  return image.pixelColor(x, image.height() - 1 - y).red();
}

QByteArray MainWindow::beginTriangle()
{
  return QByteArray("\tfacet normal 0.0 0.0 0.0\n\t\touter loop\n");
}

QVector3D MainWindow::getVertex(float x, float y, float z, const bool &scale)
{
  float add = 0.0;
  if(scale) {
    x = x * widthFactor;
    y = y * widthFactor;
    //z = z * widthFactor;
    add = border;
  }
  return QVector3D(x + add, y + add, z);
}

QByteArray MainWindow::endTriangle()
{
  return QByteArray("\t\tendloop\n\tendfacet\n");
}

void MainWindow::inputSelect()
{
  QString selectedFile = QFileDialog::getOpenFileName(this, tr("Select input file"), QFileInfo(inputLineEdit->text()).absolutePath(), "*.png");
  if(selectedFile != QByteArray()) {
    inputLineEdit->setText(selectedFile);
  }
}

void MainWindow::outputSelect()
{
  QString selectedFile = QFileDialog::getSaveFileName(this, tr("Enter output file"), QFileInfo(outputLineEdit->text()).absolutePath(), "*.stl");
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
