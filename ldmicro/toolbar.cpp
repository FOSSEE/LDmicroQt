#include "toolbar.h"
#include <linuxUI.h>
#include "ldmicro.h"

QIcon* ContactIco;
QIcon* NegContactIco;
QIcon* CoilIco;
QIcon* NegCoilIco;
QIcon* SetCoilIco;
QIcon* ResetCoilIco;
QIcon* ResetTimerIco;
QIcon* TonIco;
QIcon* TofIco;
// QIcon* RtoIco;
QIcon* AddIco;
QIcon* SubIco;
QIcon* MulIco;
QIcon* DivIco;
QIcon* CtuIco;
QIcon* CtdIco;
QAction* ContactBtn;
QAction* NegContactBtn;
QAction* CoilBtn;
QAction* NegCoilBtn;
QAction* SetCoilBtn;
QAction* ResetCoilBtn;
QAction* ResetTimerBtn;
QAction* TonBtn;
QAction* TofBtn;
QAction* RtoBtn;
QAction* AddBtn;
QAction* SubBtn;
QAction* MulBtn;
QAction* DivBtn;
QAction* CtuBtn;
QAction* CtdBtn;
QToolBar* EasyAccessTool;

void CreateToolBar()
{
	ContactIco = new QIcon(CONT_ICON);
	NegContactIco = new QIcon(NEG_CONT_ICON);
	CoilIco = new QIcon(COIL_ICON);
	NegCoilIco = new QIcon(NEG_COIL_ICON);
	SetCoilIco = new QIcon(SET_COIL_ICON);
	ResetCoilIco = new QIcon(RESET_COIL_ICON);
	TonIco = new QIcon(TON_ICON);
	TofIco = new QIcon(TOF_ICON);
	// RtoIco = new QIcon(RTO_ICON);
	AddIco = new QIcon(ADD_ICON);
	SubIco = new QIcon(SUB_ICON);
	MulIco = new QIcon(MUL_ICON);
	DivIco = new QIcon(DIV_ICON);
	CtuIco = new QIcon(CTU_ICON);
	CtdIco = new QIcon(CTD_ICON);

	EasyAccessTool = new QToolBar();
	EasyAccessTool->addSeparator();
	ContactBtn = EasyAccessTool->addAction(*ContactIco, "Normal Contact");
	NegContactBtn = EasyAccessTool->addAction(*NegContactIco, "Negated Contact");
	EasyAccessTool->addSeparator();
	CoilBtn = EasyAccessTool->addAction(*CoilIco, "Normal Coil");
	NegCoilBtn = EasyAccessTool->addAction(*NegCoilIco, "Negated Coil");
	SetCoilBtn = EasyAccessTool->addAction(*SetCoilIco, "Set Only Coil");
	ResetCoilBtn = EasyAccessTool->addAction(*ResetCoilIco, "Reset Only Coil");
	EasyAccessTool->addSeparator();
	TonBtn = EasyAccessTool->addAction(*TonIco, "On Timer");
	TofBtn = EasyAccessTool->addAction(*TofIco, "Off Timer");
	EasyAccessTool->addSeparator();
	CtuBtn = EasyAccessTool->addAction(*CtuIco, "Up Counter");
	CtdBtn = EasyAccessTool->addAction(*CtdIco, "Down Counter");
	EasyAccessTool->addSeparator();
	AddBtn = EasyAccessTool->addAction(*AddIco, "Add");
	SubBtn = EasyAccessTool->addAction(*SubIco, "Substract");
	MulBtn = EasyAccessTool->addAction(*MulIco, "Multiply");
	DivBtn = EasyAccessTool->addAction(*DivIco, "Divide");
	EasyAccessTool->addSeparator();
	EasyAccessTool->adjustSize();
	ToolBarHandler();
}

void ToolBarHandler()
{
	QSignalMapper* CommandMapper = new QSignalMapper (&MenuHandle);
	CommandMapper->setMapping(ContactBtn, MNU_INSERT_CONTACTS);
	CommandMapper->setMapping(NegContactBtn, MNU_INSERT_CONTACTS_N);
	CommandMapper->setMapping(CoilBtn, MNU_INSERT_COIL);
	CommandMapper->setMapping(NegCoilBtn, MNU_INSERT_COIL_N);
	CommandMapper->setMapping(SetCoilBtn, MNU_INSERT_COIL_S);
	CommandMapper->setMapping(ResetCoilBtn, MNU_INSERT_COIL_R);
	CommandMapper->setMapping(TonBtn, MNU_INSERT_TON);
	CommandMapper->setMapping(TofBtn, MNU_INSERT_TOF);
	CommandMapper->setMapping(CtuBtn, MNU_INSERT_CTU);
	CommandMapper->setMapping(CtdBtn, MNU_INSERT_CTD);
	CommandMapper->setMapping(AddBtn, MNU_INSERT_ADD);
	CommandMapper->setMapping(SubBtn, MNU_INSERT_SUB);
	CommandMapper->setMapping(MulBtn, MNU_INSERT_MUL);
	CommandMapper->setMapping(DivBtn, MNU_INSERT_DIV);
	QObject::connect(ContactBtn, SIGNAL(triggered()), CommandMapper, SLOT(map()));
	QObject::connect(NegContactBtn, SIGNAL(triggered()), CommandMapper, SLOT(map()));
	QObject::connect(CoilBtn, SIGNAL(triggered()), CommandMapper, SLOT(map()));
	QObject::connect(NegCoilBtn, SIGNAL(triggered()), CommandMapper, SLOT(map()));
	QObject::connect(SetCoilBtn, SIGNAL(triggered()), CommandMapper, SLOT(map()));
	QObject::connect(ResetCoilBtn, SIGNAL(triggered()), CommandMapper, SLOT(map()));
	QObject::connect(TonBtn, SIGNAL(triggered()), CommandMapper, SLOT(map()));
	QObject::connect(TofBtn, SIGNAL(triggered()), CommandMapper, SLOT(map()));
	QObject::connect(CtuBtn, SIGNAL(triggered()), CommandMapper, SLOT(map()));
	QObject::connect(CtdBtn, SIGNAL(triggered()), CommandMapper, SLOT(map()));
	QObject::connect(AddBtn, SIGNAL(triggered()), CommandMapper, SLOT(map()));
	QObject::connect(SubBtn, SIGNAL(triggered()), CommandMapper, SLOT(map()));
	QObject::connect(MulBtn, SIGNAL(triggered()), CommandMapper, SLOT(map()));
	QObject::connect(DivBtn, SIGNAL(triggered()), CommandMapper, SLOT(map()));
	QObject::connect (CommandMapper, SIGNAL(mapped(int)),
        &MenuHandle, SLOT(LD_WM_Command_call(int))) ;
}