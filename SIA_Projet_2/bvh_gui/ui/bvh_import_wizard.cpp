#include "bvh_import_wizard.h"


BVHImportWizard::BVHImportWizard(QWidget* parent) : QWizard(parent) {
    this->setWindowTitle("Zilli l'importateur de bvh là");
    this->addPage(new IntroPage());
    this->addPage(new ImportPage());
    
}

void BVHImportWizard::launch() {
    this->show();
}

void BVHImportWizard::accept() {
    std::cout << "C'est bueno" << std::endl;
    QDialog::accept();
}

IntroPage::IntroPage(BVHImportWizard* parent) {
    this->parentWizard = parent;
    optionBox = new QGroupBox("Import BVH file :");
    QGridLayout* grid_layout = new QGridLayout;
    browseButton = new QPushButton("Browse...");
    connect(browseButton, &QPushButton::clicked, this, &IntroPage::browse);
    path = new QLineEdit();
    path->setReadOnly(true);
    connect(path, &QLineEdit::textChanged, this, &IntroPage::updatePath);
    
    pathlabel = new QLabel("Path to file : ");
    grid_layout->addWidget(pathlabel,0,0,1,1);
    grid_layout->addWidget(path,0,1,1,2);
    grid_layout->addWidget(browseButton, 0,3,1,1);
    optionBox->setLayout(grid_layout);
    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(optionBox);
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

