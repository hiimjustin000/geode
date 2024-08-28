#include "SettingNodeV3.hpp"
#include <Geode/loader/SettingNode.hpp>
#include <Geode/utils/ColorProvider.hpp>
#include <ui/mods/GeodeStyle.hpp>

class SettingNodeSizeChangeEventV3::Impl final {
public:
    SettingNodeV3* node;
};

SettingNodeSizeChangeEventV3::SettingNodeSizeChangeEventV3(SettingNodeV3* node)
  : m_impl(std::make_shared<Impl>())
{
    m_impl->node = node;
}
SettingNodeSizeChangeEventV3::~SettingNodeSizeChangeEventV3() = default;

SettingNodeV3* SettingNodeSizeChangeEventV3::getNode() const {
    return m_impl->node;
}

class SettingNodeValueChangeEventV3::Impl final {
public:
    SettingNodeV3* node;
    bool commit = false;
};

SettingNodeValueChangeEventV3::SettingNodeValueChangeEventV3(SettingNodeV3* node, bool commit)
  : m_impl(std::make_shared<Impl>())
{
    m_impl->node = node;
    m_impl->commit = commit;
}
SettingNodeValueChangeEventV3::~SettingNodeValueChangeEventV3() = default;

SettingNodeV3* SettingNodeValueChangeEventV3::getNode() const {
    return m_impl->node;
}
bool SettingNodeValueChangeEventV3::isCommit() const {
    return m_impl->commit;
}

class SettingNodeV3::Impl final {
public:
    std::shared_ptr<SettingV3> setting;
    CCLayerColor* bg;
    CCLabelBMFont* nameLabel;
    CCMenu* nameMenu;
    CCMenu* buttonMenu;
    CCMenuItemSpriteExtra* resetButton;
    CCLabelBMFont* errorLabel;
    ccColor4B bgColor = ccc4(0, 0, 0, 0);
    bool committed = false;
};

bool SettingNodeV3::init(std::shared_ptr<SettingV3> setting, float width) {
    if (!CCNode::init())
        return false;
    
    m_impl = std::make_shared<Impl>();
    m_impl->setting = setting;

    m_impl->bg = CCLayerColor::create({ 0, 0, 0, 0 });
    m_impl->bg->setContentSize({ width, 0 });
    m_impl->bg->ignoreAnchorPointForPosition(false);
    m_impl->bg->setAnchorPoint(ccp(.5f, .5f));
    this->addChildAtPosition(m_impl->bg, Anchor::Center);
    
    m_impl->nameMenu = CCMenu::create();
    m_impl->nameMenu->setContentWidth(width / 2 + 25);

    m_impl->nameLabel = CCLabelBMFont::create(setting->getDisplayName().c_str(), "bigFont.fnt");
    m_impl->nameLabel->setLayoutOptions(AxisLayoutOptions::create()->setScaleLimits(.1f, .4f)->setScalePriority(1));
    m_impl->nameMenu->addChild(m_impl->nameLabel);

    m_impl->errorLabel = CCLabelBMFont::create("", "bigFont.fnt");
    m_impl->errorLabel->setScale(.25f);
    this->addChildAtPosition(m_impl->errorLabel, Anchor::Left, ccp(10, -10), ccp(0, .5f));

    if (setting->getDescription()) {
        auto descSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
        descSpr->setScale(.5f);
        auto descBtn = CCMenuItemSpriteExtra::create(
            descSpr, this, menu_selector(SettingNodeV3::onDescription)
        );
        m_impl->nameMenu->addChild(descBtn);
    }

    auto resetSpr = CCSprite::createWithSpriteFrameName("reset-gold.png"_spr);
    resetSpr->setScale(.5f);
    m_impl->resetButton = CCMenuItemSpriteExtra::create(
        resetSpr, this, menu_selector(SettingNodeV3::onReset)
    );
    m_impl->nameMenu->addChild(m_impl->resetButton);

    m_impl->nameMenu->setLayout(RowLayout::create()->setAxisAlignment(AxisAlignment::Start));
    m_impl->nameMenu->getLayout()->ignoreInvisibleChildren(true);
    this->addChildAtPosition(m_impl->nameMenu, Anchor::Left, ccp(10, 0), ccp(0, .5f));

    m_impl->buttonMenu = CCMenu::create();
    m_impl->buttonMenu->setContentSize({ width / 2 - 55, 30 });
    m_impl->buttonMenu->setLayout(AnchorLayout::create());
    this->addChildAtPosition(m_impl->buttonMenu, Anchor::Right, ccp(-10, 0), ccp(1, .5f));

    this->setAnchorPoint({ .5f, .5f });
    this->setContentSize({ width, 30 });

    return true;
}

void SettingNodeV3::updateState() {
    m_impl->errorLabel->setVisible(false);

    m_impl->nameLabel->setColor(this->hasUncommittedChanges() ? ccc3(17, 221, 0) : ccWHITE);
    m_impl->resetButton->setVisible(this->hasNonDefaultValue());

    m_impl->bg->setColor(to3B(m_impl->bgColor));
    m_impl->bg->setOpacity(m_impl->bgColor.a);

    if (!m_impl->setting->shouldEnable()) {
        if (auto desc = m_impl->setting->getEnableIfDescription()) {
            m_impl->nameLabel->setColor(ccGRAY);
            m_impl->errorLabel->setVisible(true);
            m_impl->errorLabel->setColor("mod-list-errors-found"_cc3b);
            m_impl->errorLabel->setString(desc->c_str());
        }
    }
    if (m_impl->setting->requiresRestart() && (this->hasUncommittedChanges() || m_impl->committed)) {
        m_impl->errorLabel->setVisible(true);
        m_impl->errorLabel->setColor("mod-list-restart-required-label"_cc3b);
        m_impl->errorLabel->setString("Restart Required");
        m_impl->bg->setColor("mod-list-restart-required-label-bg"_cc3b);
        m_impl->bg->setOpacity(75);
    }

    m_impl->nameMenu->updateLayout();
}

void SettingNodeV3::onDescription(CCObject*) {
    auto title = m_impl->setting->getDisplayName();
    FLAlertLayer::create(
        nullptr,
        title.c_str(),
        m_impl->setting->getDescription().value_or("No description provided"),
        "OK", nullptr,
        clamp(title.size() * 16, 300, 400)
    )->show();
}
void SettingNodeV3::onReset(CCObject*) {
    createQuickPopup(
        "Reset",
        fmt::format(
            "Are you sure you want to <cr>reset</c> <cl>{}</c> to <cy>default</c>?",
            this->getSetting()->getDisplayName()
        ),
        "Cancel", "Reset",
        [this](auto, bool btn2) {
            if (btn2) {
                this->resetToDefault();
            }
        }
    );
}

void SettingNodeV3::setBGColor(ccColor4B const& color) {
    m_impl->bgColor = color;
    this->updateState();
}

void SettingNodeV3::markChanged() {
    this->updateState();
    SettingNodeValueChangeEventV3(this, false).post();
}
void SettingNodeV3::commit() {
    this->onCommit();
    m_impl->committed = true;
    this->updateState();
    SettingNodeValueChangeEventV3(this, true).post();
}
void SettingNodeV3::resetToDefault() {
    m_impl->setting->reset();
    this->onResetToDefault();
    this->updateState();
    SettingNodeValueChangeEventV3(this, false).post();
}

void SettingNodeV3::setContentSize(CCSize const& size) {
    CCNode::setContentSize(size);
    m_impl->bg->setContentSize(size);
    this->updateLayout();
    SettingNodeSizeChangeEventV3(this).post();
}

CCLabelBMFont* SettingNodeV3::getNameLabel() const {
    return m_impl->nameLabel;
}
CCMenu* SettingNodeV3::getNameMenu() const {
    return m_impl->nameMenu;
}
CCMenu* SettingNodeV3::getButtonMenu() const {
    return m_impl->buttonMenu;
}

std::shared_ptr<SettingV3> SettingNodeV3::getSetting() const {
    return m_impl->setting;
}

// TitleSettingNodeV3

bool TitleSettingNodeV3::init(std::shared_ptr<TitleSettingV3> setting, float width) {
    if (!SettingNodeV3::init(setting, width))
        return false;
    
    this->getNameLabel()->setFntFile("goldFont.fnt");
    this->getNameMenu()->updateLayout();
    this->setContentHeight(20);
    this->updateState();
    
    return true;
}

void TitleSettingNodeV3::onCommit() {}

bool TitleSettingNodeV3::hasUncommittedChanges() const {
    return false;
}
bool TitleSettingNodeV3::hasNonDefaultValue() const {
    return false;
}
void TitleSettingNodeV3::onResetToDefault() {}

std::shared_ptr<TitleSettingV3> TitleSettingNodeV3::getSetting() const {
    return std::static_pointer_cast<TitleSettingV3>(SettingNodeV3::getSetting());
}

TitleSettingNodeV3* TitleSettingNodeV3::create(std::shared_ptr<TitleSettingV3> setting, float width) {
    auto ret = new TitleSettingNodeV3();
    if (ret && ret->init(setting, width)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

// BoolSettingNodeV3

bool BoolSettingNodeV3::init(std::shared_ptr<BoolSettingV3> setting, float width) {
    if (!SettingNodeV3::init(setting, width))
        return false;

    this->getButtonMenu()->setContentWidth(20);
    this->getNameMenu()->setContentWidth(width - 50);
    this->getNameMenu()->updateLayout();
    
    m_toggle = CCMenuItemToggler::createWithStandardSprites(
        this, menu_selector(BoolSettingNodeV3::onToggle), .55f
    );
    m_toggle->m_onButton->setContentSize({ 25, 25 });
    m_toggle->m_onButton->getNormalImage()->setPosition(ccp(25, 25) / 2);
    m_toggle->m_offButton->setContentSize({ 25, 25 });
    m_toggle->m_offButton->getNormalImage()->setPosition(ccp(25, 25) / 2);
    m_toggle->m_notClickable = true;
    m_toggle->toggle(setting->getValue());
    this->getButtonMenu()->addChildAtPosition(m_toggle, Anchor::Right, ccp(-10, 0));

    this->updateState();

    return true;
}

void BoolSettingNodeV3::updateState() {
    SettingNodeV3::updateState();
    auto enable = this->getSetting()->shouldEnable();
    m_toggle->setCascadeColorEnabled(true);
    m_toggle->setCascadeOpacityEnabled(true);
    m_toggle->setEnabled(enable);
    m_toggle->setColor(enable ? ccWHITE : ccGRAY);
    m_toggle->setOpacity(enable ? 255 : 155);
}

void BoolSettingNodeV3::onCommit() {
    this->getSetting()->setValue(m_toggle->isToggled());
}
void BoolSettingNodeV3::onToggle(CCObject*) {
    m_toggle->toggle(!m_toggle->isToggled());
    this->markChanged();
}

bool BoolSettingNodeV3::hasUncommittedChanges() const {
    return m_toggle->isToggled() != this->getSetting()->getValue();
}
bool BoolSettingNodeV3::hasNonDefaultValue() const {
    return m_toggle->isToggled() != this->getSetting()->getDefaultValue();
}
void BoolSettingNodeV3::onResetToDefault() {
    m_toggle->toggle(this->getSetting()->getDefaultValue());
}

std::shared_ptr<BoolSettingV3> BoolSettingNodeV3::getSetting() const {
    return std::static_pointer_cast<BoolSettingV3>(SettingNodeV3::getSetting());
}

BoolSettingNodeV3* BoolSettingNodeV3::create(std::shared_ptr<BoolSettingV3> setting, float width) {
    auto ret = new BoolSettingNodeV3();
    if (ret && ret->init(setting, width)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

// StringSettingNodeV3

bool StringSettingNodeV3::init(std::shared_ptr<StringSettingV3> setting, float width) {
    if (!SettingNodeV3::init(setting, width))
        return false;

    m_input = TextInput::create(setting->getEnumOptions() ? width / 2 - 50 : width / 2, "Text");
    m_input->setCallback([this](auto const&) {
        this->markChanged();
    });
    m_input->setScale(.7f);
    m_input->setString(this->getSetting()->getValue());
    this->getButtonMenu()->addChildAtPosition(m_input, Anchor::Center);
    
    if (setting->getEnumOptions()) {
        m_input->getBGSprite()->setVisible(false);
        m_input->setEnabled(false);
        m_input->getInputNode()->m_placeholderLabel->setOpacity(255);
        m_input->getInputNode()->m_placeholderLabel->setColor(ccWHITE);
        
        m_arrowLeftSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
        m_arrowLeftSpr->setFlipX(true);
        m_arrowLeftSpr->setScale(.4f);
        auto arrowLeftBtn = CCMenuItemSpriteExtra::create(
            m_arrowLeftSpr, this, menu_selector(StringSettingNodeV3::onArrow)
        );
        arrowLeftBtn->setTag(-1);
        this->getButtonMenu()->addChildAtPosition(arrowLeftBtn, Anchor::Left, ccp(5, 0));

        m_arrowRightSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
        m_arrowRightSpr->setScale(.4f);
        auto arrowRightBtn = CCMenuItemSpriteExtra::create(
            m_arrowRightSpr, this, menu_selector(StringSettingNodeV3::onArrow)
        );
        arrowRightBtn->setTag(1);
        this->getButtonMenu()->addChildAtPosition(arrowRightBtn, Anchor::Right, ccp(-5, 0));
    }

    this->updateState();

    return true;
}

void StringSettingNodeV3::updateState() {
    SettingNodeV3::updateState();
    auto enable = this->getSetting()->shouldEnable();
    if (!this->getSetting()->getEnumOptions()) {
        m_input->setEnabled(enable);
    }
    else {
        m_arrowRightSpr->setOpacity(enable ? 255 : 155);
        m_arrowRightSpr->setColor(enable ? ccWHITE : ccGRAY);
        m_arrowLeftSpr->setOpacity(enable ? 255 : 155);
        m_arrowLeftSpr->setColor(enable ? ccWHITE : ccGRAY);
    }
}

void StringSettingNodeV3::onArrow(CCObject* sender) {
    auto options = *this->getSetting()->getEnumOptions();
    auto index = ranges::indexOf(options, m_input->getString()).value_or(0);
    if (sender->getTag() > 0) {
        index = index < options.size() - 1 ? index + 1 : 0;
    }
    else {
        index = index > 0 ? index - 1 : options.size() - 1;
    }
    m_input->setString(options.at(index));
    this->updateState();
}

void StringSettingNodeV3::onCommit() {
    this->getSetting()->setValue(m_input->getString());
}
bool StringSettingNodeV3::hasUncommittedChanges() const {
    return m_input->getString() != this->getSetting()->getValue();
}
bool StringSettingNodeV3::hasNonDefaultValue() const {
    return m_input->getString() != this->getSetting()->getDefaultValue();
}
void StringSettingNodeV3::onResetToDefault() {
    m_input->setString(this->getSetting()->getDefaultValue());
}

std::shared_ptr<StringSettingV3> StringSettingNodeV3::getSetting() const {
    return std::static_pointer_cast<StringSettingV3>(SettingNodeV3::getSetting());
}

StringSettingNodeV3* StringSettingNodeV3::create(std::shared_ptr<StringSettingV3> setting, float width) {
    auto ret = new StringSettingNodeV3();
    if (ret && ret->init(setting, width)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

// FileSettingNodeV3

bool FileSettingNodeV3::init(std::shared_ptr<FileSettingV3> setting, float width) {
    if (!SettingNodeV3::init(setting, width))
        return false;
    
    m_path = setting->getValue();

    auto labelBG = extension::CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    labelBG->setScale(.25f);
    labelBG->setColor({ 0, 0, 0 });
    labelBG->setOpacity(90);
    labelBG->setContentSize({ 420, 80 });
    this->getButtonMenu()->addChildAtPosition(labelBG, Anchor::Center, ccp(-10, 0));

    m_fileIcon = CCSprite::create();
    this->getButtonMenu()->addChildAtPosition(m_fileIcon, Anchor::Left, ccp(5, 0));

    m_nameLabel = CCLabelBMFont::create("", "bigFont.fnt");
    this->getButtonMenu()->addChildAtPosition(m_nameLabel, Anchor::Left, ccp(13, 0), ccp(0, .5f));

    m_selectBtnSpr = CCSprite::createWithSpriteFrameName("GJ_plus2Btn_001.png");
    m_selectBtnSpr->setScale(.7f);
    m_selectBtn = CCMenuItemSpriteExtra::create(
        m_selectBtnSpr, this, menu_selector(FileSettingNodeV3::onPickFile)
    );
    this->getButtonMenu()->addChildAtPosition(m_selectBtn, Anchor::Right, ccp(-5, 0));

    this->updateState();

    return true;
}

void FileSettingNodeV3::updateState() {
    SettingNodeV3::updateState();
    m_fileIcon->setDisplayFrame(CCSpriteFrameCache::get()->spriteFrameByName(
        this->getSetting()->isFolder() ? "folderIcon_001.png" : "file.png"_spr
    ));
    limitNodeSize(m_fileIcon, ccp(10, 10), 1.f, .1f);
    if (m_path.empty()) {
        m_nameLabel->setString(this->getSetting()->isFolder() ? "No Folder Selected" : "No File Selected");
        m_nameLabel->setColor(ccGRAY);
        m_nameLabel->setOpacity(155);
    }
    else {
        m_nameLabel->setString(m_path.filename().string().c_str());
        m_nameLabel->setColor(ccWHITE);
        m_nameLabel->setOpacity(255);
    }
    m_nameLabel->limitLabelWidth(75, .35f, .1f);

    auto enable = this->getSetting()->shouldEnable();
    m_selectBtnSpr->setOpacity(enable ? 255 : 155);
    m_selectBtnSpr->setColor(enable ? ccWHITE : ccGRAY);
    m_selectBtn->setEnabled(enable);
}

void FileSettingNodeV3::onPickFile(CCObject*) {
    m_pickListener.bind([this](auto* event) {
        auto value = event->getValue();
        if (!value) {
            return;
        }
        if (value->isOk()) {
            m_path = value->unwrap().string();
            this->markChanged();
        }
        else {
            FLAlertLayer::create(
                "Failed",
                fmt::format("Failed to pick file: {}", value->unwrapErr()),
                "Ok"
            )->show();
        }
    });
    std::error_code ec;
    m_pickListener.setFilter(file::pick(
        this->getSetting()->isFolder() ? 
            file::PickMode::OpenFolder : 
            (this->getSetting()->useSaveDialog() ? file::PickMode::SaveFile : file::PickMode::OpenFile), 
        {
            // Prefer opening the current path directly if possible
            m_path.empty() || !std::filesystem::exists(m_path.parent_path(), ec) ? dirs::getGameDir() : m_path,
            this->getSetting()->getFilters().value_or(std::vector<file::FilePickOptions::Filter>())
        }
    ));
}

void FileSettingNodeV3::onCommit() {
    this->getSetting()->setValue(m_path);
}
bool FileSettingNodeV3::hasUncommittedChanges() const {
    return m_path != this->getSetting()->getValue();
}
bool FileSettingNodeV3::hasNonDefaultValue() const {
    return m_path != this->getSetting()->getDefaultValue();
}
void FileSettingNodeV3::onResetToDefault() {
    m_path = this->getSetting()->getDefaultValue();
}

std::shared_ptr<FileSettingV3> FileSettingNodeV3::getSetting() const {
    return std::static_pointer_cast<FileSettingV3>(SettingNodeV3::getSetting());
}

FileSettingNodeV3* FileSettingNodeV3::create(std::shared_ptr<FileSettingV3> setting, float width) {
    auto ret = new FileSettingNodeV3();
    if (ret && ret->init(setting, width)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

// Color3BSettingNodeV3

bool Color3BSettingNodeV3::init(std::shared_ptr<Color3BSettingV3> setting, float width) {
    if (!SettingNodeV3::init(setting, width))
        return false;
    
    m_value = setting->getValue();

    m_colorSprite = ColorChannelSprite::create();
    m_colorSprite->setScale(.65f);

    m_colorBtn = CCMenuItemSpriteExtra::create(
        m_colorSprite, this, menu_selector(Color3BSettingNodeV3::onSelectColor)
    );
    this->getButtonMenu()->addChildAtPosition(m_colorBtn, Anchor::Right, ccp(-10, 0));

    this->updateState();

    return true;
}

void Color3BSettingNodeV3::updateState() {
    SettingNodeV3::updateState();
    m_colorSprite->setColor(m_value);
    
    auto enable = this->getSetting()->shouldEnable();
    m_colorSprite->setOpacity(enable ? 255 : 155);
    m_colorBtn->setEnabled(enable);
}

void Color3BSettingNodeV3::onSelectColor(CCObject*) {
    auto popup = ColorPickPopup::create(m_value);
    popup->setDelegate(this);
    popup->show();
}
void Color3BSettingNodeV3::updateColor(ccColor4B const& color) {
    m_value = to3B(color);
    this->markChanged();
}

void Color3BSettingNodeV3::onCommit() {
    this->getSetting()->setValue(m_value);
}
bool Color3BSettingNodeV3::hasUncommittedChanges() const {
    return m_value != this->getSetting()->getValue();
}
bool Color3BSettingNodeV3::hasNonDefaultValue() const {
    return m_value != this->getSetting()->getDefaultValue();
}
void Color3BSettingNodeV3::onResetToDefault() {
    m_value = this->getSetting()->getDefaultValue();
}

std::shared_ptr<Color3BSettingV3> Color3BSettingNodeV3::getSetting() const {
    return std::static_pointer_cast<Color3BSettingV3>(SettingNodeV3::getSetting());
}

Color3BSettingNodeV3* Color3BSettingNodeV3::create(std::shared_ptr<Color3BSettingV3> setting, float width) {
    auto ret = new Color3BSettingNodeV3();
    if (ret && ret->init(setting, width)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

// Color4BSettingNodeV3

bool Color4BSettingNodeV3::init(std::shared_ptr<Color4BSettingV3> setting, float width) {
    if (!SettingNodeV3::init(setting, width))
        return false;
    
    m_value = setting->getValue();

    m_colorSprite = ColorChannelSprite::create();
    m_colorSprite->setScale(.65f);

    m_colorBtn = CCMenuItemSpriteExtra::create(
        m_colorSprite, this, menu_selector(Color4BSettingNodeV3::onSelectColor)
    );
    this->getButtonMenu()->addChildAtPosition(m_colorBtn, Anchor::Right, ccp(-10, 0));

    this->updateState();

    return true;
}

void Color4BSettingNodeV3::updateState() {
    SettingNodeV3::updateState();
    m_colorSprite->setColor(to3B(m_value));
    m_colorSprite->updateOpacity(m_value.a / 255.f);
    
    auto enable = this->getSetting()->shouldEnable();
    m_colorSprite->setOpacity(enable ? 255 : 155);
    m_colorBtn->setEnabled(enable);
}

void Color4BSettingNodeV3::onSelectColor(CCObject*) {
    auto popup = ColorPickPopup::create(m_value);
    popup->setDelegate(this);
    popup->show();
}
void Color4BSettingNodeV3::updateColor(ccColor4B const& color) {
    m_value = color;
    this->markChanged();
}

void Color4BSettingNodeV3::onCommit() {
    this->getSetting()->setValue(m_value);
}
bool Color4BSettingNodeV3::hasUncommittedChanges() const {
    return m_value != this->getSetting()->getValue();
}
bool Color4BSettingNodeV3::hasNonDefaultValue() const {
    return m_value != this->getSetting()->getDefaultValue();
}
void Color4BSettingNodeV3::onResetToDefault() {
   m_value = this->getSetting()->getDefaultValue();
}

std::shared_ptr<Color4BSettingV3> Color4BSettingNodeV3::getSetting() const {
    return std::static_pointer_cast<Color4BSettingV3>(SettingNodeV3::getSetting());
}

Color4BSettingNodeV3* Color4BSettingNodeV3::create(std::shared_ptr<Color4BSettingV3> setting, float width) {
    auto ret = new Color4BSettingNodeV3();
    if (ret && ret->init(setting, width)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

// UnresolvedCustomSettingNodeV3

bool UnresolvedCustomSettingNodeV3::init(std::shared_ptr<LegacyCustomSettingV3> setting, float width) {
    if (!SettingNodeV3::init(setting, width))
        return false;
    
    this->setContentHeight(30);
    
    auto label = CCLabelBMFont::create(
        fmt::format("Missing setting '{}'", setting->getKey()).c_str(),
        "bigFont.fnt"
    );
    label->limitLabelWidth(width - m_obContentSize.height, .5f, .1f);
    this->addChildAtPosition(label, Anchor::Left, ccp(m_obContentSize.height / 2, 0));
    
    return true;
}

void UnresolvedCustomSettingNodeV3::onCommit() {}

bool UnresolvedCustomSettingNodeV3::hasUncommittedChanges() const {
    return false;
}
bool UnresolvedCustomSettingNodeV3::hasNonDefaultValue() const {
    return false;
}
void UnresolvedCustomSettingNodeV3::onResetToDefault() {}

std::shared_ptr<LegacyCustomSettingV3> UnresolvedCustomSettingNodeV3::getSetting() const {
    return std::static_pointer_cast<LegacyCustomSettingV3>(SettingNodeV3::getSetting());
}

UnresolvedCustomSettingNodeV3* UnresolvedCustomSettingNodeV3::create(std::shared_ptr<LegacyCustomSettingV3> setting, float width) {
    auto ret = new UnresolvedCustomSettingNodeV3();
    if (ret && ret->init(setting, width)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

// LegacyCustomSettingToV3Node

bool LegacyCustomSettingToV3Node::init(std::shared_ptr<LegacyCustomSettingV3> original, float width) {
    if (!SettingNodeV3::init(original, width))
        return false;
    
    this->getNameMenu()->setVisible(false);
    this->getButtonMenu()->setVisible(false);

    m_original = original->getValue()->createNode(width);
    m_original->setDelegate(this);
    this->setContentSize({ width, m_original->getContentHeight() });
    this->addChildAtPosition(m_original, Anchor::BottomLeft, ccp(0, 0), ccp(0, 0));
    
    return true;
}

void LegacyCustomSettingToV3Node::settingValueChanged(SettingNode*) {
    SettingNodeValueChangeEventV3(this, false).post();
}
void LegacyCustomSettingToV3Node::settingValueCommitted(SettingNode*) {
    SettingNodeValueChangeEventV3(this, true).post();
}

void LegacyCustomSettingToV3Node::onCommit() {
    m_original->commit();
}

bool LegacyCustomSettingToV3Node::hasUncommittedChanges() const {
    return m_original->hasUncommittedChanges();
}
bool LegacyCustomSettingToV3Node::hasNonDefaultValue() const {
    return m_original->hasNonDefaultValue();
}
void LegacyCustomSettingToV3Node::onResetToDefault() {
    m_original->resetToDefault();
}

LegacyCustomSettingToV3Node* LegacyCustomSettingToV3Node::create(std::shared_ptr<LegacyCustomSettingV3> original, float width) {
    auto ret = new LegacyCustomSettingToV3Node();
    if (ret && ret->init(original, width)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}
