#pragma once

#include <QDialog>
#include "ConfigBundleStructure.h"
//#include "ui_ConfigDialog.h"

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    ConfigDialog(ConfigBundle &c, QWidget *parent = Q_NULLPTR);
    ~ConfigDialog();

private:
    //Ui::ConfigDialog ui;

    ConfigBundle &config;

    // data retrieving
    QCheckBox *checkMesh;
    QCheckBox *checkPointCloud;
    QCheckBox *checkSlice;

    QLineEdit *lineMeshAlpha;
    QLineEdit *linePointCloudAlpha;
    QLineEdit *lineSliceAlpha;

    QCheckBox *checkCullFace;
    QCheckBox *checkDrawFace;

    QLineEdit *lineAmbient;
    QLineEdit *lineDiffuse;
    QLineEdit *lineSpecular;
    QLineEdit *lineShininess;

    void initContentFromConfig();
    void setConfigFromContent();
};
