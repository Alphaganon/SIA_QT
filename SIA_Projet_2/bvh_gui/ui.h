#include <QWidget>
#include <QPushButton>
#include <string>
#include <QObject>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <iostream>
#include <QWizard>
#include <QWizardPage>
#include <string>
#include <QLabel>
#include <QVBoxLayout>
#include <iostream>
#include <QDialog>
#include <QGroupBox>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>

#include "mainwidget.h"

class BVHImportWizard : public QWizard {
    Q_OBJECT
    public:
        BVHImportWizard(QWidget* parent = nullptr);
        ~BVHImportWizard(){};
        std::string importedPath;
        QWidget* relatedWidget;


    public slots:
        void launch();
        void accept() override;
};


class IntroPage : public QWizardPage {
    Q_OBJECT
    public:
        BVHImportWizard* parentWizard;
        IntroPage(BVHImportWizard* parent = nullptr);
        QPushButton* browseButton;
        QLineEdit* path;
        QLabel* pathlabel;
        QGroupBox* optionBox;

    public slots:
        void browse();
        void updatePath();
        
};

class ImportPage : public QWizardPage {
    Q_OBJECT
    public:
        ImportPage(QWidget* parent= nullptr);
        QLabel* label;
};

class UIMainWidget : public QWidget {
    Q_OBJECT
    public:
        QPushButton* panel;
        QPushButton* quoi;
        QPushButton* importButton;
        MainWidget* window;
        UIMainWidget(std::string filename);
        ~UIMainWidget(){};
        BVHImportWizard* importer;
        void displayWidget();
        std::string path;

    public slots:
        void resetAnimation();
        void feur();
        void callWizard();
};