#include "stdafx.h"
#include "ConfigDialog.h"

ConfigDialog::ConfigDialog(ConfigBundle &c, QWidget *parent)
    : QDialog(parent), config(c)
{
    //ui.setupUi(this);
    QGridLayout *gridLayout = new QGridLayout;

    // Alpha
    auto groupAlpha = new QGroupBox(tr("&Alpha"));
    groupAlpha->setStyleSheet("QGroupBox::title { color: gray; } ");
    {
        auto groupAlphaLayout = new QGridLayout;
        checkMesh = new QCheckBox;
        checkPointCloud = new QCheckBox;
        checkSlice = new QCheckBox;
        groupAlphaLayout->addWidget(checkMesh, 0, 0);
        groupAlphaLayout->addWidget(checkPointCloud, 1, 0);
        groupAlphaLayout->addWidget(checkSlice, 2, 0);
        groupAlphaLayout->addWidget(new QLabel(tr("Surface:")), 0, 1);
        groupAlphaLayout->addWidget(new QLabel(tr("Point Cloud:")), 1, 1);
        groupAlphaLayout->addWidget(new QLabel(tr("Slicing Plane:")), 2, 1);
        lineMeshAlpha = new QLineEdit;
        linePointCloudAlpha = new QLineEdit;
        lineSliceAlpha = new QLineEdit;
        groupAlphaLayout->addWidget(lineMeshAlpha, 0, 2);
        groupAlphaLayout->addWidget(linePointCloudAlpha, 1, 2);
        groupAlphaLayout->addWidget(lineSliceAlpha, 2, 2);
        groupAlpha->setLayout(groupAlphaLayout);
    }
    gridLayout->addWidget(groupAlpha, 0, 0);

    /*connect(checkMesh, &QCheckBox::stateChanged, this, [this]() {
        lineMeshAlpha->setReadOnly(!checkMesh->isChecked());
    });*/ // no need

    // Render
    auto groupRender = new QGroupBox(tr("&Render"));
    groupRender->setStyleSheet("QGroupBox::title { color: gray; } ");
    {
        auto groupRenderLayout = new QGridLayout;
        //groupRenderLayout->addWidget(new QLabel(tr("Cull Face:")), 0, 0);
        //groupRenderLayout->addWidget(new QLabel(tr("Draw Face:")), 1, 0);
        checkCullFace = new QCheckBox("Cull Face");
        checkDrawFace = new QCheckBox("Draw Face");
        groupRenderLayout->addWidget(checkCullFace, 0, 0);
        groupRenderLayout->addWidget(checkDrawFace, 1, 0);
        groupRender->setLayout(groupRenderLayout);
    }
    gridLayout->addWidget(groupRender, 0, 1);

    // Material
    auto groupMaterial = new QGroupBox(tr("&Material"));
    groupMaterial->setStyleSheet("QGroupBox::title { color: gray; } ");
    {
        auto groupMaterialLayout = new QGridLayout;
        groupMaterialLayout->addWidget(new QLabel(tr("ambient:")), 0, 0);
        groupMaterialLayout->addWidget(new QLabel(tr("diffuse:")), 1, 0);
        groupMaterialLayout->addWidget(new QLabel(tr("specular:")), 2, 0);
        groupMaterialLayout->addWidget(new QLabel(tr("shininess:")), 3, 0);
        lineAmbient = new QLineEdit;
        lineDiffuse = new QLineEdit;
        lineSpecular = new QLineEdit;
        lineShininess = new QLineEdit;
        groupMaterialLayout->addWidget(lineAmbient, 0, 1);
        groupMaterialLayout->addWidget(lineDiffuse, 1, 1);
        groupMaterialLayout->addWidget(lineSpecular, 2, 1);
        groupMaterialLayout->addWidget(lineShininess, 3, 1);
        groupMaterial->setLayout(groupMaterialLayout);
    }
    gridLayout->addWidget(groupMaterial, 1, 0);

    QPushButton *closeButton = new QPushButton(tr("&Cancel"));
    connect(closeButton, &QAbstractButton::clicked, this, &QWidget::close);
    QPushButton *okButton = new QPushButton(tr("&OK"));
    connect(okButton, &QAbstractButton::clicked, this, [this]() {
        setConfigFromContent();
        close();
    });

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(2);
    buttonsLayout->addWidget(okButton, 1);
    buttonsLayout->addWidget(closeButton, 1);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(gridLayout);
    mainLayout->addStretch(1);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);


    initContentFromConfig();
    setWindowTitle(tr("Configure"));
}

ConfigDialog::~ConfigDialog()
{
}

void ConfigDialog::initContentFromConfig()
{
    checkMesh->setChecked(config.render_config.mesh_visible);
    checkPointCloud->setChecked(config.render_config.point_cloud_visible);
    checkSlice->setChecked(config.render_config.slice_visible);

    lineMeshAlpha->setText(tr("%0").arg(config.render_config.mesh_alpha));
    linePointCloudAlpha->setText(tr("%0").arg(config.render_config.point_cloud_alpha));
    lineSliceAlpha->setText(tr("%0").arg(config.render_config.slice_alpha));

    /* no need
    lineMeshAlpha->setReadOnly(!config.render_config.mesh_visible);
    linePointCloudAlpha->setReadOnly(!config.render_config.point_cloud_visible);
    lineSliceAlpha->setReadOnly(!config.render_config.slice_visible);*/

    checkCullFace->setChecked(config.render_config.cull_face);
    checkDrawFace->setChecked(config.render_config.draw_face);

    lineAmbient->setText(tr("%0").arg(config.render_config.material.ambient));
    lineDiffuse->setText(tr("%0").arg(config.render_config.material.diffuse));
    lineSpecular->setText(tr("%0").arg(config.render_config.material.specular));
    lineShininess->setText(tr("%0").arg(config.render_config.material.shininess));
}

void ConfigDialog::setConfigFromContent()
{
    config.render_config.mesh_visible = checkMesh->isChecked();
    config.render_config.point_cloud_visible = checkPointCloud->isChecked();
    config.render_config.slice_visible = checkSlice->isChecked();

    config.render_config.mesh_alpha = lineMeshAlpha->text().toFloat();
    config.render_config.point_cloud_alpha = linePointCloudAlpha->text().toFloat();
    config.render_config.slice_alpha = lineSliceAlpha->text().toFloat();

    config.render_config.cull_face = checkCullFace->isChecked();
    config.render_config.draw_face = checkDrawFace->isChecked();

    config.render_config.material.ambient = lineAmbient->text().toFloat();
    config.render_config.material.diffuse = lineDiffuse->text().toFloat();
    config.render_config.material.specular = lineSpecular->text().toFloat();
    config.render_config.material.shininess = lineShininess->text().toFloat();
}
