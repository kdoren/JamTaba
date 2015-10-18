#include "LocalTrackView.h"
#include "ui_BaseTrackView.h"
#include "FxPanel.h"
#include "FxPanelItem.h"
#include "plugins/guis.h"
#include "../MainController.h"
#include "../midi/MidiDriver.h"
#include "../audio/core/AudioDriver.h"
#include "../audio/core/AudioNode.h"
#include "Highligther.h"
#include <QMenu>
#include <QToolButton>

const QString LocalTrackView::MIDI_ICON = ":/images/input_midi.png";
const QString LocalTrackView::MONO_ICON = ":/images/input_mono.png";
const QString LocalTrackView::STEREO_ICON = ":/images/input_stereo.png";
const QString LocalTrackView::NO_INPUT_ICON= ":/images/input_no.png";

LocalTrackView::LocalTrackView(Controller::MainController *mainController, int channelIndex, float initialGain, float initialPan, bool muted)
    :BaseTrackView( mainController, 1), fxPanel(nullptr), inputNode(nullptr)
{
    init(channelIndex, initialGain, initialPan, muted);
}

LocalTrackView::LocalTrackView(Controller::MainController *mainController, int channelIndex)
    :BaseTrackView(mainController, 1), fxPanel(nullptr), inputNode(nullptr)
{
    init(channelIndex, 1, 0, false);//unit gain and pan in center, not muted
}

void LocalTrackView::detachMainController(){
    this->mainController = nullptr;
}

void LocalTrackView::init(int channelIndex, float initialGain, float initialPan, bool muted){
    if(!mainController){
        qCritical() << "LocalTrackView::init() mainController is null!";
        return;
    }

    //add separator before effects panel
    fxPanel = createFxPanel();
    ui->mainLayout->addSpacing(20);
    ui->mainLayout->addWidget( fxPanel );

    //create input panel in the bottom
    this->inputPanel = createInputPanel();
    fxSpacer = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed);
    ui->mainLayout->addSpacerItem(fxSpacer);

    ui->mainLayout->addWidget(inputPanel);

    this->inputTypeIconLabel = createInputTypeIconLabel(this);
    ui->mainLayout->addWidget(inputTypeIconLabel);
    ui->mainLayout->setAlignment(this->inputTypeIconLabel, Qt::AlignCenter);

    if(mainController->isRunningAsVstPlugin()){
        fxPanel->setVisible(false);
        fxSpacer->changeSize(0, 0);
        inputPanel->setVisible(false);
        inputTypeIconLabel->setVisible(false);
    }

    //insert a input node in controller
    this->inputNode = new Audio::LocalInputAudioNode(channelIndex);
    this->trackID = mainController->addInputTrackNode(this->inputNode);

    ui->levelSlider->setValue( 100 * initialGain );
    ui->panSlider->setValue( initialPan * 4 );

    if(muted){
        ui->muteButton->click();
    }

    setUnlightStatus(false);

    faderOnly = false;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QSize LocalTrackView::sizeHint() const{
    if(faderOnly){
        int width = ui->faderPanel->sizeHint().width() + layout()->contentsMargins().left() + layout()->contentsMargins().right();
        return QSize( width, height());
    }
    return BaseTrackView::sizeHint();
}

void LocalTrackView::setFaderOnlyMode(bool faderOnly){
    if(this->faderOnly != faderOnly){
        this->faderOnly = faderOnly;
        //ui->faderPanel->setVisible(!this->faderOnly);
        ui->topPanel->setVisible(!this->faderOnly);
        ui->levelSlider->setVisible(!this->faderOnly);
        ui->panSpacer->changeSize(20, 20, QSizePolicy::Minimum, this->faderOnly ? QSizePolicy::Ignored : QSizePolicy::Fixed);
        ui->panSpacer->invalidate();

        QMargins margins = layout()->contentsMargins();
        margins.setLeft(faderOnly ? 2 : 6);
        margins.setRight(faderOnly ? 2 : 6);
        layout()->setContentsMargins(margins);

        if(fxPanel){
            fxPanel->setVisible(!faderOnly);
            inputPanel->setVisible(!faderOnly);

            fxSpacer->changeSize(20, faderOnly ? 0 : 20, QSizePolicy::Minimum, faderOnly ? QSizePolicy::Ignored : QSizePolicy::Fixed);
        }
        ui->soloButton->setVisible(!faderOnly);
        ui->muteButton->setVisible(!faderOnly);
        ui->peaksDbLabel->setVisible(!faderOnly);
        //vertical spacer in bottom of VUs
        ui->verticalSpacer->changeSize(20, 20, QSizePolicy::Minimum, this->faderOnly ? QSizePolicy::Ignored : QSizePolicy::Fixed);
        ui->horizontalSpacer->changeSize( faderOnly ? 0 : 20,20, QSizePolicy::Minimum, QSizePolicy::Fixed);

        if(inputTypeIconLabel){
            inputTypeIconLabel->setVisible(!faderOnly);
        }

        this->drawDbValue = !faderOnly;

        updateGeometry();

        setProperty("faderOnly", QVariant(faderOnly));
        style()->unpolish(this);
        style()->polish(this);
    }
}

void LocalTrackView::toggleFaderOnlyMode(){
    setFaderOnlyMode(!faderOnly);
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void LocalTrackView::setUnlightStatus(bool unlighted){
    BaseTrackView::setUnlightStatus(unlighted);
    if(fxPanel){
        style()->unpolish(fxPanel);
        style()->polish(fxPanel);

        QList<FxPanelItem*> items = fxPanel->getItems();
        foreach (FxPanelItem* item, items) {
            style()->unpolish(item);
            style()->polish(item);
        }
    }
    if(inputPanel){
        style()->unpolish(inputPanel);
        style()->polish(inputPanel);
    }
    update();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Audio::LocalInputAudioNode* LocalTrackView::getInputNode() const{
    return inputNode;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void LocalTrackView::setToNoInput(){
    if(inputNode){
        inputNode->setToNoInput();
        refreshInputSelectionName();
    }

}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
QWidget* LocalTrackView::createInputPanel(){
    QWidget* inputPanel = new QWidget(this);
    inputPanel->setObjectName("inputPanel");
    inputPanel->setLayout(new QHBoxLayout(inputPanel));
    inputPanel->layout()->setContentsMargins(0, 0, 0, 0);
    inputPanel->layout()->setSpacing(0);
    inputPanel->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));


    this->inputSelectionButton = createInputSelectionButton(inputPanel);
    //inputPanel->layout()->addWidget(inputTypeIconLabel);
    inputPanel->layout()->addWidget(inputSelectionButton);//button in right
    return inputPanel;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
QLabel* LocalTrackView::createInputTypeIconLabel(QWidget *parent){
    QLabel* label = new QLabel(parent);
    label->setObjectName("inputSelectionIcon");
    label->setTextFormat(Qt::RichText);
    label->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
    return label;
}

QPushButton* LocalTrackView::createInputSelectionButton(QWidget* parent){
    QPushButton* fakeComboButton = new QPushButton(parent);
    fakeComboButton->setObjectName("inputSelectionButton");
    fakeComboButton->setText("inputs ...");
    fakeComboButton->setToolTip("Choose input channels ...");
    fakeComboButton->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

    QObject::connect( fakeComboButton, SIGNAL(clicked()), this, SLOT(on_inputSelectionButtonClicked()));

    return fakeComboButton;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void LocalTrackView::on_inputSelectionButtonClicked(){
    QMenu menu(this);
    menu.addMenu(createMonoInputsMenu(&menu));
    menu.addMenu(createStereoInputsMenu(&menu));
    menu.addMenu(createMidiInputsMenu(&menu));
    QAction* noInputAction = menu.addAction(QIcon(NO_INPUT_ICON), "no input");
    QObject::connect( noInputAction, SIGNAL(triggered()), this, SLOT(on_noInputMenuSelected()));

    menu.move(mapToGlobal(inputSelectionButton->parentWidget()->pos()));
    menu.exec();
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void LocalTrackView::on_noInputMenuSelected(){
    mainController->setInputTrackToNoInput(getTrackID());
    //setUnlightStatus(true);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QMenu* LocalTrackView::createMonoInputsMenu(QMenu* parent){
    QMenu* monoInputsMenu = new QMenu("Mono", parent);
    monoInputsMenu->setIcon(QIcon(MONO_ICON));
    Audio::AudioDriver* audioDriver = mainController->getAudioDriver();
    Audio::ChannelRange globalInputsRange = audioDriver->getSelectedInputs();
    int globalInputs = globalInputsRange.getChannels();
    int firstGlobalInputIndex = globalInputsRange.getFirstChannel();
    QString deviceName(audioDriver->getInputDeviceName(audioDriver->getInputDeviceIndex()));
    //Audio::ChannelRange thisTrackInputRange = inputNode->getAudioInputRange();
    //bool canEnableMenu = false;
    for (int i = 0; i < globalInputs; ++i) {
        int index = firstGlobalInputIndex + i;
        QString channelName = QString(audioDriver->getInputChannelName(index)).trimmed();
        if(channelName.isNull() || channelName.isEmpty()){
            channelName  = QString::number(i+1)  + " "+ audioDriver->getInputDeviceName();
        }
        QString inputName = channelName + "  (" + deviceName + ")";
        QAction* action = monoInputsMenu->addAction(inputName);
        action->setData( index );//using the channel index as action data
        action->setIcon(monoInputsMenu->icon());
//        action->setEnabled(mainController->audioMonoInputIsFreeToSelect(index) || index == thisTrackInputRange.getFirstChannel() || index == thisTrackInputRange.getLastChannel());
//        if(action->isEnabled()){//at least one action is enabled
//            canEnableMenu = true;
//        }
    }
//    monoInputsMenu->setEnabled(canEnableMenu);
//    if(!canEnableMenu){
//        monoInputsMenu->setTitle("Mono (all mono inputs are already in use)");
//    }
    QObject::connect( monoInputsMenu, SIGNAL(triggered(QAction*)), this, SLOT(on_monoInputMenuSelected(QAction*)));
    return monoInputsMenu;
}

void LocalTrackView::on_monoInputMenuSelected(QAction *action){
    int selectedInputIndexInAudioDevice = action->data().toInt();
    mainController->setInputTrackToMono(getTrackID(), selectedInputIndexInAudioDevice);
    //setUnlightStatus(false);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
QMenu* LocalTrackView::createStereoInputsMenu(QMenu* parent){
    QMenu* stereoInputsMenu = new QMenu("Stereo", parent);
    stereoInputsMenu->setIcon(QIcon(STEREO_ICON));
    Audio::AudioDriver* audioDriver = mainController->getAudioDriver();
    Audio::ChannelRange globalInputRange = audioDriver->getSelectedInputs();
    //Audio::ChannelRange thisTrackInputRange = mainController->getInputTrack(getTrackID())->getAudioInputRange();
    int globalInputs = globalInputRange.getChannels();
    QString deviceName(audioDriver->getInputDeviceName(audioDriver->getInputDeviceIndex()));
    //bool canEnableMenu = false;
    for (int i = 0; i < globalInputs; i += 2) {
        if(i + 1 < globalInputs){//we can make a channel pair using (i) and (i+1)?
            int index = globalInputRange.getFirstChannel() + i;
            QString firstName = getInputChannelNameOnly(index);
            QString indexes = QString::number(index+1) + " + " + QString::number(index+2);
            QString inputName = firstName + " [" + indexes +  "]  (" + deviceName + ")";
            QAction* action = stereoInputsMenu->addAction(inputName);
            action->setData(index);//use the first input pair index as action data
            action->setIcon(stereoInputsMenu->icon());
//            bool inputPairIsFree = mainController->audioStereoInputIsFreeToSelect(index);
//            bool inputUsageIsOk = inputIsUsedByMe(index) || (inputIsUsedByMe(index+1) && thisTrackInputRange.isMono());
//            action->setEnabled( inputPairIsFree || inputUsageIsOk );
//            if(action->isEnabled()){
//                canEnableMenu = true;
//            }
        }
    }
    stereoInputsMenu->setEnabled(globalInputs/2 >= 1);//at least one pair
    if(!stereoInputsMenu->isEnabled()){
        stereoInputsMenu->setTitle( stereoInputsMenu->title() + "  (not enough available inputs to make stereo)" );
    }
    QObject::connect(stereoInputsMenu, SIGNAL(triggered(QAction*)), this, SLOT(on_stereoInputMenuSelected(QAction*)));
    return stereoInputsMenu;
}

//bool LocalTrackView::inputIsUsedByMe(int inputIndexInAudioDevice) const{
//    Audio::ChannelRange range = inputNode->getAudioInputRange();
//    return range.getFirstChannel() == inputIndexInAudioDevice || range.getLastChannel() == inputIndexInAudioDevice;
//}

void LocalTrackView::on_stereoInputMenuSelected(QAction *action){
    int firstInputIndexInAudioDevice = action->data().toInt();
    mainController->setInputTrackToStereo(getTrackID(), firstInputIndexInAudioDevice);
    //setUnlightStatus(false);
}

QString LocalTrackView::getInputChannelNameOnly(int inputIndex){
    QString fullName(mainController->getAudioDriver()->getInputChannelName(inputIndex));
    if(fullName.isEmpty()){//mac return empy channel names if user don't rename the channels
        fullName = mainController->getAudioDriver()->getInputDeviceName();
    }
    int spaceIndex = fullName.lastIndexOf(" ");
    if(spaceIndex > 0){
        return fullName.left( spaceIndex );
    }
    return fullName;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void LocalTrackView::refreshInputSelectionName(){
    Audio::LocalInputAudioNode* inputTrack = mainController->getInputTrack(getTrackID());
    QString channelName;
    QString iconFile;
    if(inputTrack->isAudio()){//using audio as input method
        Audio::ChannelRange inputRange = inputTrack->getAudioInputRange();
        if(inputTrack->isStereo()){
            int firstInputIndex = inputRange.getFirstChannel();
            QString indexes = "(" + QString::number(firstInputIndex+1) + "+" + QString::number(firstInputIndex+2) + ") ";
            channelName = indexes + getInputChannelNameOnly(firstInputIndex);
            iconFile = STEREO_ICON;
        }
        else if(inputTrack->isMono()){
            int index = inputRange.getFirstChannel();
            QString name = QString(mainController->getAudioDriver()->getInputChannelName(index));
            channelName = QString(QString::number(index+1) + " - ");
            if(!name.isNull() && !name.isEmpty()){
                channelName += name;
            }
            else{
                channelName += QString(mainController->getAudioDriver()->getInputDeviceName());
            }

            iconFile = MONO_ICON;
        }
        else{//range is empty = no audio input
            channelName = "No input";
            iconFile = NO_INPUT_ICON;
        }
    }
    else{//using midi as input method
        if(inputTrack->isMidi()){
            Midi::MidiDriver* midiDriver = mainController->getMidiDriver();
            int selectedDeviceIndex = inputTrack->getMidiDeviceIndex();
            if( selectedDeviceIndex < midiDriver->getMaxInputDevices() && midiDriver->deviceIsGloballyEnabled(selectedDeviceIndex)){
                channelName = midiDriver->getInputDeviceName(selectedDeviceIndex);
                iconFile = MIDI_ICON;
            }
        }
        else{
            channelName = "No input";
            iconFile = NO_INPUT_ICON;
        }

    }

    //set the input name
    QString elidedName = inputSelectionButton->fontMetrics().elidedText(channelName, Qt::ElideRight, inputSelectionButton->width() - inputSelectionButton->iconSize().width());
    inputSelectionButton->setText(elidedName);

    //set the icon
    if(inputTypeIconLabel){
        this->inputTypeIconLabel->setStyleSheet("background-image: url(" + iconFile + ");");
    }

    updateGeometry();

    update();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
QMenu* LocalTrackView::createMidiInputsMenu(QMenu* parent){
    QMenu* midiInputsMenu = new QMenu("MIDI", parent);
    midiInputsMenu->setIcon(QIcon(MIDI_ICON));
    int totalMidiDevices = mainController->getMidiDriver()->getMaxInputDevices();
    int globallyEnabledMidiDevices = 0;
    for (int d = 0; d < totalMidiDevices; ++d) {
        if(mainController->getMidiDriver()->deviceIsGloballyEnabled(d)){
            globallyEnabledMidiDevices++;
            QMenu* midiChannelsMenu = new QMenu(midiInputsMenu);
            QActionGroup* actionGroup = new QActionGroup(midiChannelsMenu);

            QAction* allChannelsAction = midiChannelsMenu->addAction("All channels");
            allChannelsAction->setData(QString(QString::number(d) + ":" + QString::number(-1)));//use -1 to all channels
            allChannelsAction->setActionGroup(actionGroup);
            allChannelsAction->setCheckable(true);
            allChannelsAction->setChecked(getInputNode()->isMidi() && getInputNode()->getMidiDeviceIndex() == d && getInputNode()->isReceivingAllMidiChannels());

            midiChannelsMenu->addSeparator();
            for (int c = 0; c < 16; ++c) {
                QAction* a = midiChannelsMenu->addAction("Channel " + QString::number(c+1));
                a->setData(QString(QString::number(d) + ":" + QString::number(c)));//use device:channel_index as data
                a->setActionGroup(actionGroup);
                a->setCheckable(true);
                a->setChecked(getInputNode()->isMidi() && getInputNode()->getMidiChannelIndex() == c && getInputNode()->getMidiDeviceIndex() == d);
            }
            char deviceName[32];
            strcpy(deviceName, mainController->getMidiDriver()->getInputDeviceName(d));

            QAction* action = midiInputsMenu->addAction(QString(deviceName));
            action->setMenu(midiChannelsMenu);
            action->setData(d);//using midi device index as action data
            action->setIcon(midiInputsMenu->icon());
            QObject::connect(midiChannelsMenu, SIGNAL(triggered(QAction*)), this, SLOT(on_MidiInputDeviceSelected(QAction*)));
        }
    }
    midiInputsMenu->setEnabled(globallyEnabledMidiDevices > 0);
    if(!midiInputsMenu->isEnabled()){
        midiInputsMenu->setTitle( midiInputsMenu->title() + "  (no MIDI devices detected or enabled in 'Preferences' menu')" );
    }
    //QObject::connect(midiInputsMenu, SIGNAL(triggered(QAction*)), this, SLOT(on_MidiInputDeviceSelected(QAction*)));
    return midiInputsMenu;
}

void LocalTrackView::on_MidiInputDeviceSelected(QAction *action){
    QString indexes = action->data().toString();
    if(!indexes.contains(":")){
        return;
    }
    QStringList indexesParts = indexes.split(":");
    if(indexesParts.size() != 2){
        return;
    }
    QString midiChannelString = indexesParts.at(1);
    QString midiDeviceString = indexesParts.at(0);
    int midiChannel = midiChannelString.toInt();
    int midiDeviceIndex = midiDeviceString.toInt();

    mainController->setInputTrackToMIDI(getTrackID(), midiDeviceIndex, midiChannel);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void LocalTrackView::addPlugin(Audio::Plugin* plugin, bool bypassed){
    if(fxPanel){
        //plugin->setEditor(new Audio::PluginWindow(plugin));
        plugin->setBypass(bypassed);
        this->fxPanel->addPlugin(plugin);
        update();
    }
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
FxPanel *LocalTrackView::createFxPanel(){
    FxPanel* panel = new FxPanel(this, mainController);

    //panel->connect(panel, SIGNAL(editingPlugin(Audio::Plugin*)), this, SIGNAL(editingPlugin(Audio::Plugin*)));
    //panel->connect(panel, SIGNAL(pluginRemoved(Audio::Plugin*)), this, SIGNAL(removingPlugin(Audio::Plugin*)));
    return panel;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
LocalTrackView::~LocalTrackView()
{
    if(mainController){
        mainController->removeInputTrackNode(getTrackID());
        //mainController->removeTrack(getTrackID());
    }
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
QList<const Audio::Plugin *> LocalTrackView::getInsertedPlugins() const{
    QList<const Audio::Plugin*> plugins;
    foreach (FxPanelItem* item, fxPanel->getItems()) {
        if(item->containPlugin() ){
            plugins.append(item->getAudioPlugin());
        }
    }
    return plugins;
}