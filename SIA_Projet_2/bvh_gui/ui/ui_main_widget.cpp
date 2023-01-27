#include "ui_main_widget.h"


UIMainWidget::UIMainWidget(std::string filename) {
    // Constructor
    this->panel = new QPushButton("Update", this);
    this->quoi = new QPushButton("Quoient ?", this);
    this->window = new MainWidget(filename);
    connect(panel, &QPushButton::clicked,this, &UIMainWidget::resetAnimation);
    connect(quoi, &QPushButton::clicked, this, &UIMainWidget::feur);
    QHBoxLayout* layout = new QHBoxLayout;
    QVBoxLayout* buttonLayout = new QVBoxLayout;
    buttonLayout->addWidget(quoi);
    buttonLayout->addWidget(panel);

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