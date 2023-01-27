#include "ui_main_widget.h"


UIMainWidget::UIMainWidget(std::string filename) {
    // Constructor
    this->panel = new QPushButton("Update", this);
    this->quoi = new QPushButton("Quoient ?", this);
    this->importButton = new QPushButton("Import", this);
    this->importer = new BVHImportWizard(this);
    this->window = new MainWidget(filename);
    connect(panel, &QPushButton::clicked,this, &UIMainWidget::resetAnimation);
    connect(quoi, &QPushButton::clicked, this, &UIMainWidget::feur);
    connect(importButton, &QPushButton::clicked, this, &UIMainWidget::callWizard);
    QHBoxLayout* layout = new QHBoxLayout;
    QVBoxLayout* buttonLayout = new QVBoxLayout;
    buttonLayout->addWidget(quoi);
    buttonLayout->addWidget(panel);
    buttonLayout->addWidget(importButton);
    layout->addLayout(buttonLayout);
    layout->addWidget(this->window);
    this->setLayout(layout);
    this->setGeometry(100,100,1000,600);
}

void UIMainWidget::resetAnimation() {
    this->window->show();
}

void UIMainWidget::feur() {
    std::cout << "feurent" << std::endl;
}

void UIMainWidget::callWizard() {
    std::cout << "HURLE QUEUE DIXIT MARTIN" << std::endl;
    this->importer->launch();
}