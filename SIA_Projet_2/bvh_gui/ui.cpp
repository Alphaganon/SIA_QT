#include "ui.h"

UIMainWidget::UIMainWidget(std::string filename) {
    // Constructor
    this->path = filename;    
}

void UIMainWidget::resetAnimation() {
    this->window->show();
}

void UIMainWidget::displayWidget() {
    this->panel = new QPushButton("Update", this);
    this->quoi = new QPushButton("Quoient ?", this);
    this->importButton = new QPushButton("Import", this);
    this->importer = new BVHImportWizard(this);
    this->window = new MainWidget(this->path);
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
    this->show();
}

void UIMainWidget::feur() {
    std::cout << "feurent" << std::endl;
    this->path = "../walk1.bvh";
    delete this->layout();
    this->displayWidget();
}

void UIMainWidget::callWizard() {
    std::cout << "HURLE QUEUE DIXIT MARTIN" << std::endl;
    this->importer->launch();
}

BVHImportWizard::BVHImportWizard(QWidget* parent) : QWizard(parent) {
    this->setWindowTitle("Zilli l'importateur de bvh là");
    this->addPage(new IntroPage());
    this->addPage(new ImportPage());
    this->importedPath = "walk1.bvh";
}

void BVHImportWizard::launch() {
    this->show();
}

void BVHImportWizard::accept() {
    std::cout << "C'est bueno" << std::endl;
    ((UIMainWidget*) this->parentWidget())->path = this->importedPath;
    QDialog::accept();
    delete ((UIMainWidget*) this->parentWidget())->layout();
    ((UIMainWidget*) this->parentWidget())->displayWidget();
}

IntroPage::IntroPage(BVHImportWizard* parent) {
    // this->parentWizard = parent;
    // optionBox = new QGroupBox("Import BVH file :");
    // QGridLayout* grid_layout = new QGridLayout;
    // browseButton = new QPushButton("Browse...");
    // connect(browseButton, &QPushButton::clicked, this, &IntroPage::browse);
    // path = new QLineEdit();
    // path->setReadOnly(true);
    // connect(path, &QLineEdit::textChanged, this, &IntroPage::updatePath);
    
    // pathlabel = new QLabel("Path to file : ");
    // grid_layout->addWidget(pathlabel,0,0,1,1);
    // grid_layout->addWidget(path,0,1,1,2);
    // grid_layout->addWidget(browseButton, 0,3,1,1);
    // optionBox->setLayout(grid_layout);
    // QVBoxLayout* layout = new QVBoxLayout;
    // layout->addWidget(optionBox);
    // this->setLayout(layout);
    this->browseButton = new QPushButton("browse",this);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(browseButton);
    this->setLayout(layout);
}

void IntroPage::browse() {
    std::cout << "ça browse de fou là" << std::endl;
}

void IntroPage::updatePath() {
    std::cout << "update la" << std::endl;
}

ImportPage::ImportPage(QWidget* parent) {
    setTitle("BBBBBBBBBb");
    label = new QLabel("Au revoir, dirait Giscard");
    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(label);
    this->setLayout(layout);
}
