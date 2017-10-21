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
    QCheckBox *checkMeshInner;
    QCheckBox *checkMeshOuter;
    QCheckBox *checkPointCloud;
    QCheckBox *checkSlice;

    QLineEdit *lineMeshInnerAlpha;
    QLineEdit *lineMeshOuterAlpha;
    QLineEdit *linePointCloudAlpha;
    QLineEdit *lineSliceAlpha;

    QCheckBox *checkCullFace;
    QCheckBox *checkDrawFace;
    QCheckBox *checkFaceNormal;

    QLineEdit *lineAmbient;
    QLineEdit *lineDiffuse;
    QLineEdit *lineSpecular;
    QLineEdit *lineShininess;

    QCheckBox *checkEnableSlicing;
    QCheckBox *checkEnableSlicingMesh;
    QCheckBox *checkEnableSlicingPC;
    QPushButton *btnSlicingDirection;
    QPushButton *btnSlicingRemovePolicy;
    QMenu     *menuDirection;
    QMenu     *menuPolicy;
    int        index_direction;
    int        index_policy;


    void initContentFromConfig();
    void setConfigFromContent();
};
