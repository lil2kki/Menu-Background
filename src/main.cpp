#include <imgui-cocos.hpp>
inline static auto NextDraw = geode::ScheduledFunction();

#include <Geode/ui/GeodeUI.hpp>
using namespace geode::prelude;
class ModSettingsPopup : public Popup {};

#define SETTING(type, key_name) Mod::get()->getSettingValue<type>(key_name)

$on_mod(Loaded) {
    //imgui really
    ImGuiCocos::get().setup();
    ImGuiCocos::get().draw([] { if (NextDraw) NextDraw(); NextDraw = nullptr; });

    //more search paths
    for (auto path : {
        string::pathToString(getMod()->getConfigDir()),
        string::pathToString(getMod()->getSaveDir()),
        string::pathToString(getMod()->getTempDir())
    }) CCFileUtils::get()->addPriorityPath(path.c_str());

    //gif file preload
    auto BACKGROUND_FILE = string::pathToString(SETTING(std::filesystem::path, "BACKGROUND_FILE"));
    BACKGROUND_FILE = CCFileUtils::get()->fullPathForFilename(BACKGROUND_FILE.c_str(), 0).c_str();
    CCSprite::create(BACKGROUND_FILE.c_str());
}

#include <Geode/modify/MenuGameLayer.hpp>
class $modify(MenuGameLayerExt, MenuGameLayer) {
public:
    bool init() {

        auto loadedBgID = GameManager::get()->m_loadedBgID;
        auto loadedGroundID = GameManager::get()->m_loadedGroundID;

        if (auto id = SETTING(int, "FORCE_BACKGROUND_ID")) GameManager::get()->m_loadedBgID = id;
        if (auto id = SETTING(int, "FORCE_GROUND_ID")) GameManager::get()->m_loadedGroundID = id;

        if (!MenuGameLayer::init()) return false;

        GameManager::get()->m_loadedBgID = loadedBgID;
        GameManager::get()->m_loadedGroundID = loadedGroundID;

        auto BACKGROUND_FILE = string::pathToString(SETTING(std::filesystem::path, "BACKGROUND_FILE"));
        BACKGROUND_FILE = CCFileUtils::get()->fullPathForFilename(BACKGROUND_FILE.c_str(), 0).c_str();

        CCNodeRGBA* inital_sprite = CCSprite::create(BACKGROUND_FILE.c_str());

        if (inital_sprite) {

            auto size = this->getContentSize();

            Ref scroll = ScrollLayer::create(size);
            scroll->setID("background-layer"_spr);

            scroll->m_contentLayer->setPositionX(SETTING(float, "BG_POSX"));
            scroll->m_contentLayer->setPositionY(SETTING(float, "BG_POSY"));

            static std::function<void(CCNode*, CCNode*)> setupSprite = [](
                Ref<CCNode> sprite, CCNode* parent
                ) {
                    if (!parent) return log::error("can't resolve content layer");
                    while (Ref a = parent->getChildByID("sprite"_spr)) a->removeMeAndCleanup();
                    auto BACKGROUND_FILE = string::pathToString(SETTING(std::filesystem::path, "BACKGROUND_FILE"));
                    BACKGROUND_FILE = CCFileUtils::get()->fullPathForFilename(BACKGROUND_FILE.c_str(), 0).c_str();
                    log::info("Creating sprite with {}", BACKGROUND_FILE);
                    if (sprite and !sprite->isRunning()) sprite = CCSprite::create(BACKGROUND_FILE.c_str());
                    if (sprite) {
                        sprite->setPosition(parent->getContentSize() * 0.5f);
                        sprite->setID("sprite"_spr);
                        sprite->runAction(CCRepeatForever::create(CCSpawn::create(CallFuncExt::create([sprite] {sprite->setVisible(1); }), nullptr)));
                        parent->addChild(sprite);
                    };
                };

            setupSprite(inital_sprite, scroll->m_contentLayer);

            static Ref<CCNode> listenerTarget;
            static Ref<CCNode> listenerTarget2;
            if (!listenerTarget) listenForSettingChanges<std::filesystem::path>(
                "BACKGROUND_FILE", [](std::filesystem::path value) {
                    setupSprite(listenerTarget, listenerTarget2);
                    //test
                    unsigned long aaw = 0u;
                    auto BACKGROUND_FILE = string::pathToString(value);
                    if (!CCFileUtils::get()->getFileData(BACKGROUND_FILE.c_str(), "rb", &aaw)) {
                        for (auto c : BACKGROUND_FILE) if (!string::contains(
                            R"(0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!¹;%:?*()_+-=.,\/|"'@#$^&{}[])"
                            , c
                        )) {
                            auto e = createQuickPopup(
                                "Unexcepted characters!",
                                "Additional check detected unexcepted characters in file path...\nThis may be one of the problems. Please make sure that the file path does not have russian or any other special characters.\n ",
                                "OK", nullptr, nullptr
                            );
                            auto list = std::string();
                            for (auto c : BACKGROUND_FILE) if (!string::contains(
                                R"(0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!¹;%:?*()_+-=.,\/|"'@#$^&{}[])"
                                , c
                            )) list += c;
                            list += "\n \n \n \n ";
                            auto listLabel = CCLabelTTF::create(list.c_str(), "", 14.f);
                            if (listLabel) e->m_buttonMenu->addChild(listLabel);
                            if (listLabel) listLabel->setColor(ccRED);
                            break;
                        }
                        createQuickPopup(
                            "Unreadable file",
                            "Please select a valid background file. Maybe something wrong with its name or no permissions to read it.",
                            "OK", nullptr, nullptr
                        );
					}
					std::string ext = BACKGROUND_FILE.substr(BACKGROUND_FILE.find_last_of('.'));
					if ((ext == ".jxl" || ext == ".gif" || ext == ".webp" || ext == ".qoi") && !Loader::get()->isModLoaded("prever.imageplus")) {
						MDPopup::create("Unhandleable Format", "You can't use that file without [Image Plus](mod:prevter.imageplus) mod loaded!", "OK.")->show();
					}
                }
            );
            listenerTarget = inital_sprite;
            listenerTarget2 = scroll->m_contentLayer;

            scroll->m_disableHorizontal = 0;
            scroll->m_scrollLimitTop = 9999;
            scroll->m_scrollLimitBottom = 9999;
            scroll->m_cutContent = false;

            scroll->setMouseEnabled(SETTING(bool, "SETUP_MODE_ENABLED"));

            Ref scroll_bg = CCSprite::create("groundSquare_18_001.png");//geode::createLayerBG();
            scroll_bg->setScale(9999.f);
            scroll_bg->setColor(ccBLACK);
            scroll->setID("bg"_spr);
            scroll->addChild(scroll_bg, -1);

            scroll->runAction(CCRepeatForever::create(CCSpawn::create(CallFuncExt::create(
                [scroll, scroll_bg, this] {
                    Ref content = scroll->m_contentLayer;

                    scroll->m_disableMovement = not SETTING(bool, "SETUP_MODE_ENABLED");
                    scroll->setMouseEnabled(SETTING(bool, "SETUP_MODE_ENABLED"));

                    if (auto a = scroll->getChildByIDRecursive("menu_top_container"_spr)) a->setScale(SETTING(float, "SETUP_WINDOW_SCALE"));

                    if (auto a = this->getChildByType<GJGroundLayer>(-1)) a->setVisible(not SETTING(bool, "HIDE_GROUND"));

                    scroll_bg->setVisible(SETTING(bool, "HIDE_GAME"));
                    scroll->setZOrder(SETTING(bool, "HIDE_GAME") ? 999 : SETTING(int, "BG_ZORDER"));

                    if (auto a = SETTING(float, "BG_SCALEX")) content->setScaleX(a); else content->setScaleX(1.f);
                    if (auto a = SETTING(float, "BG_SCALEY")) content->setScaleY(a); else content->setScaleY(1.f);
                    if (!SETTING(float, "BG_SCALEX") and !SETTING(float, "BG_SCALEY")) {
                        auto sprite = content->getChildByID("sprite"_spr); 
                        sprite->setScale(1.f);
                        //"one-of": ["Fullscreen Stretch", "Up to WinHeight", "Up to WinWidth", "Up to WinSize", "NONE"]
                        auto type = SETTING(std::string, "BG_SCALE_TYPE");
                        if (type == "Fullscreen Stretch") {
                            sprite->setScaleX((content->getContentWidth() / sprite->getContentWidth()));
                            sprite->setScaleY((content->getContentHeight() / sprite->getContentHeight()));
                        }
                        if (type == "Up to WinHeight") cocos::limitNodeHeight(sprite, content->getContentHeight(), 9999.f, 0.1f);
                        if (type == "Up to WinWidth") cocos::limitNodeWidth(sprite, content->getContentWidth(), 9999.f, 0.1f);
                        if (type == "Up to WinSize") cocos::limitNodeSize(sprite, content->getContentSize(), 9999.f, 0.1f);
                    }

                    if (not SETTING(bool, "SETUP_MODE_ENABLED")) {
                        content->setPositionX(SETTING(float, "BG_POSX"));
                        content->setPositionY(SETTING(float, "BG_POSY"));
                    }
                    else if (!scroll->m_touchDown and !CCScene::get()->getChildByType<ModSettingsPopup>(0)) {

                        Mod::get()->setSettingValue("BG_POSX", content->getPositionX());
                        Mod::get()->setSettingValue("BG_POSY", content->getPositionY());

                        NextDraw = [=, ref = Ref(this)]() mutable
                            {
                                ImGui::GetIO().FontAllowUserScaling = true;
                                ImGui::GetIO().FontGlobalScale = 2.28f * SETTING(float, "SETUP_WINDOW_SCALE");
                                bool enabled = SETTING(bool, "SETUP_MODE_ENABLED");
                                if (ImGui::Begin("Background Setup", &enabled, ImGuiWindowFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeX)) {
                                    //if close button pressed the enabled bool is set to false
                                    if (!enabled) getMod()->setSettingValue("SETUP_MODE_ENABLED", false);
                                    //btns
                                    if (ImGui::Button(
                                        ref->getPositionX() ? "Show Menu" : "Hide Menu"
                                    )) if (Ref parent = ref->getParent()) {
                                        if (ref->getPositionX()) {
                                            parent->setPositionX(parent->getPositionX() - parent->getContentWidth() * 3);
                                            ref->setPositionX(ref->getPositionX() + ref->getContentWidth() * 3);
                                        }
                                        else {
                                            parent->setPositionX(parent->getContentWidth() * 3);
                                            ref->setPositionX(ref->getContentWidth() * -3);
                                        };
                                    };
                                    ImGui::SameLine();
                                    ImGui::Button("Open Settings") ? openSettingsPopup(getMod()) : void();
                                    ImGui::SameLine();
                                    ImGui::Text("| Window Scale:");
									ImGui::SameLine();
									ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
									float windowscale = SETTING(float, "SETUP_WINDOW_SCALE");
									ImGui::DragFloat("###windowscale", &windowscale, 0.05f, 0.5f) ? Mod::get()->setSettingValue(
										"SETUP_WINDOW_SCALE", windowscale
									) : !"a";
                                    //togglers
                                    auto hideGround = SETTING(bool, "HIDE_GROUND");
                                    ImGui::Checkbox("Hide Ground", &hideGround) ? Mod::get()->setSettingValue(
                                        "HIDE_GROUND", hideGround
                                    ) : !"HIDE_GROUND TOGGLE CHECKBOX HERE :D";
                                    auto hideGame = SETTING(bool, "HIDE_GAME");
                                    ImGui::Checkbox("Hide Game", &hideGame) ? Mod::get()->setSettingValue(
                                        "HIDE_GAME", hideGame
                                    ) : !"HIDE_GAME TOGGLE CHECKBOX HERE :D";
                                    //zOrder
                                    auto zzz = SETTING(int, "BG_ZORDER");
                                    ImGui::InputInt(
                                        "Z Order (60 to hide icons)", &zzz
                                    ) ? Mod::get()->setSettingValue(
                                        "BG_ZORDER", zzz
                                    ) : !"a";
                                    //pos
                                    ImGui::SeparatorText("Position");
                                    ImGui::Button("RESET###pos") ? content->setPosition(CCPointZero) : void();
                                    ImGui::SameLine();
                                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                                    float p[2] = { content->getPositionX(), content->getPositionY() };
                                    ImGui::DragFloat2("###pos", p) ? content->setPosition({p[0],p[1]}) : void();
                                    //scale
                                    ImGui::SeparatorText("Scale");
                                    if (ImGui::Button(
                                        (SETTING(std::string, "BG_SCALE_TYPE") + "###stypesw").c_str(),
                                        { ImGui::GetContentRegionAvail().x, 0.f}
                                    )) {
                                        Mod::get()->setSettingValue("BG_SCALEX", 0.f);
                                        Mod::get()->setSettingValue("BG_SCALEY", 0.f);
                                        auto node = getMod()->getSetting("BG_SCALE_TYPE")->createNode(10.f);
                                        if (auto a = node->getButtonMenu()) 
                                            if (auto b = a->getChildByType<CCMenuItemSpriteExtra*>(-1)) 
                                                b->activate(); //right arrow button
                                        node->commit();
                                    };
                                    ImGui::BeginDisabled();
                                    ImGui::Button("Scale values below is optional and being ignored if set to 0");
									ImGui::EndDisabled();
									if (ImGui::Button("RESET###scale")) {
                                        Mod::get()->setSettingValue("BG_SCALEX", 0.f);
                                        Mod::get()->setSettingValue("BG_SCALEY", 0.f);
                                    };
                                    ImGui::SameLine();
                                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                                    float s[2] = { SETTING(float, "BG_SCALEX"), SETTING(float, "BG_SCALEY") };
                                    if (ImGui::DragFloat2("###scale", s, 0.005f)) {
                                        Mod::get()->setSettingValue("BG_SCALEX", s[0]);
										Mod::get()->setSettingValue("BG_SCALEY", s[1]);
                                    };
                                }
                                ImGui::End();
                            };

                    }
                }
            ), nullptr)));

            this->addChild(scroll);
        }

        return true;
    }
};

#include <Geode/modify/CCSprite.hpp>
class $modify(AddMenuGameLayerExt, CCSprite) {
    static CCSprite* create(const char* pszFileName) {
        auto rtn = CCSprite::create(pszFileName);
        if (SETTING(bool, "OVERLAP_ALLGRADBG")) if (std::string(pszFileName) == "GJ_gradientBG.png") queueInMainThread(
            [rtn = Ref(rtn)] {
                if (auto a = rtn->getParent()) {
                    if (SETTING(bool, "HIDE_GROUND")) findFirstChildRecursive<GJGroundLayer>(
                        a, [](GJGroundLayer* a) {
                            a->setScale(0);
                            return false;
                        }
                    );
                    auto game = MenuGameLayer::create();
                    game->setTouchEnabled(0);
                    a->insertAfter(game, rtn);
                }
            }
        );
        return rtn;
    }
};
