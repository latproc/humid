/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#include <boost/algorithm/string.hpp>
#include <iostream>
#include <list>
#include <nanogui/common.h>
#include <regular_expressions.h>

#include "DebugExtra.h"
#include "curl_helper.h"
#include "editor.h"
#include "editorbutton.h"
#include "editorgui.h"
#include "editorimageview.h"
#include "editorlabel.h"
#include "editorlineplot.h"
#include "editorprogressbar.h"
#include "editorproject.h"
#include "editorsettings.h"
#include "editortextbox.h"
#include "editorwidget.h"
#include "helper.h"
#include "linkableobject.h"
#include "linkmanager.h"
#include "objectwindow.h"
#include "panelscreen.h"
#include "patternswindow.h"
#include "propertyformhelper.h"
#include "propertywindow.h"
#include "resourcemanager.h"
#include "screenswindow.h"
#include "selectablebutton.h"
#include "startupwindow.h"
#include "structureswindow.h"
#include "thememanager.h"
#include "toolbar.h"
#include "userwindowwin.h"
#include "viewswindow.h"

extern std::map<std::string, Structure *> structures;
extern std::list<Structure *> st_structures;
extern long collect_history;

extern const int DEBUG_ALL;
extern int debug;
#define DEBUG_BASIC (1 & debug)
#define DEBUG_WIDGET (4 & debug)
extern int run_only;
extern std::list<Structure *> hm_structures;

void setupTheme(nanogui::Theme *theme);

Structure *EditorGUI::system_settings = 0;

ResourceManager::Factory resource_manager_factory;

class Texture {
  public:
    Texture(GLTexture tex, GLTexture::handleType dat)
        : texture(std::move(tex)), data(std::move(dat)) {}
    GLTexture texture;
    GLTexture::handleType data;
};
std::map<std::string, Texture *> texture_cache;
std::map<GLuint, std::string> loaded_textures;

class EditorKeyboard {
  public:
    static EditorKeyboard *instance() {
        if (instance_ == nullptr)
            instance_ = new EditorKeyboard();
        return instance_;
    }
    const std::string &keyName(int key) {
        assert(key_names.size() > 1);
        auto found = key_names.find(key);
        if (found != key_names.end())
            return (*found).second;
        return unknown_key;
    }

  private:
    EditorKeyboard();
    static EditorKeyboard *instance_;
    std::map<int, std::string> key_names;
    std::string unknown_key;
};
EditorKeyboard *EditorKeyboard::instance_ = nullptr;

EditorGUI::EditorGUI(int width, int height, bool full_screen, bool run_only)
    : ClockworkClient(nanogui::Vector2i(width, height), full_screen || run_only ? "" : "Humid",
                      !full_screen, full_screen),
      theme(0), state(GUIWELCOME), editor(0), w_toolbar(0), w_properties(0), w_theme(0), w_user(0),
      w_patterns(0), w_structures(0), w_connections(0), w_startup(0), w_screens(0), w_views(0),
      needs_update(false), sample_buffer_size(5000), project(0) {
    old_size = mSize;
}

Structure *EditorGUI::getSettings() {
    Structure *es = EditorSettings::find("EditorSettings");
    return (es) ? es : EditorSettings::create();
}

void EditorGUI::addLinkableProperty(const std::string name, LinkableProperty *lp) {
    std::lock_guard<std::recursive_mutex> lock(linkables_mutex);
    linkables[name] = lp;
}

nanogui::Window *EditorGUI::getNamedWindow(const std::string name) {
    if (name == "Structures")
        return (getStructuresWindow()) ? getStructuresWindow()->getWindow() : nullptr;
    if (name == "Properties")
        return (getPropertyWindow()) ? getPropertyWindow()->getWindow() : nullptr;
    if (name == "Objects")
        return (getObjectWindow()) ? getObjectWindow()->getWindow() : nullptr;
    if (name == "Patterns")
        return (getPatternsWindow()) ? getPatternsWindow()->getWindow() : nullptr;
    if (name == "Screens")
        return (getScreensWindow()) ? getScreensWindow()->getWindow() : nullptr;
    return nullptr;
}

bool EditorGUI::keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (EDITOR->isEditMode()) {
        return nanogui::Screen::keyboardEvent(key, scancode, action, modifiers);
    }
    else {
        if (key == GLFW_KEY_ESCAPE) {
            w_user->getWindow()->requestFocus();
            return true;
        }
        if (action == GLFW_PRESS) {
            bool function_key = false;
            std::string key_name("");
            if ((key >= GLFW_KEY_F1 && key <= GLFW_KEY_F25)) {
                char buf[10];
                snprintf(buf, 10, "KEY_F%d", key - GLFW_KEY_F1 + 1);
                key_name = buf;
                function_key = true;
            }
            if (key == GLFW_KEY_KP_ENTER &&
                nanogui::Screen::keyboardEvent(GLFW_KEY_ENTER, scancode, action, modifiers))
                return true;
            else if (!function_key &&
                     nanogui::Screen::keyboardEvent(key, scancode, action, modifiers))
                return true;

            key_name = EditorKeyboard::instance()->keyName(key);
            if (key_name.empty()) {
                return nanogui::Screen::keyboardEvent(key, scancode, action, modifiers);
            }

            if (w_user && w_user->structure()) {
                if (debug) {
                    std::cout << "detected key: " << key_name << "\n";
                }
                std::string conn;
                StructureClass *sc = w_user->structure()->getStructureDefinition();
                if (sc) {
                    std::map<std::string, Value>::iterator found = sc->getOptions().find(key_name);
                    if (found != sc->getOptions().end())
                        conn = (*found).second.asString();
                }
                if (conn.empty() && EditorGUI::systemSettings()) {
                    sc = EditorGUI::systemSettings()->getStructureDefinition();
                    if (!sc) {
                        std::cout << "ERROR: no class defined for system settings\n";
                        return false;
                    }
                    std::map<std::string, Value>::iterator found = sc->getOptions().find(key_name);
                    if (found != sc->getOptions().end())
                        conn = (*found).second.asString();
                }
                if (conn.length()) {
                    std::cout << "found key mapping: " << conn << "\n";
                    size_t dpos = conn.find(':');
                    if (dpos != std::string::npos) {
                        std::string remote = conn.substr(dpos + 1);
                        conn = conn.erase(dpos);

                        LinkableProperty *lp = findLinkableProperty(remote);
                        if (lp) {
                            std::string msgon = getIODSyncCommand(conn, 0, lp->address(), 1);
                            queueMessage(conn, msgon,
                                         [](std::string s) { std::cout << ": " << s << "\n"; });
                            std::string msgoff = getIODSyncCommand(conn, 0, lp->address(), 0);
                            queueMessage(conn, msgoff,
                                         [](std::string s) { std::cout << ": " << s << "\n"; });
                            return true;
                        }
                    }
                    else {
                        //search the current screen for a button with this name and click it
                        for (auto w : w_user->getWindow()->children()) {
                            //EditorWidget *ew = dynamic_cast<EditorWidget*>(w);
                            EditorTextBox *et = dynamic_cast<EditorTextBox *>(w);
                            EditorButton *eb = dynamic_cast<EditorButton *>(w);
                            if (et && et->getName() == conn) {
                                w->requestFocus();
                                et->selectAll();
                                return true;
                            }
                            else if (eb && eb->getName() == conn) {
                                if (eb->callback())
                                    eb->callback()();
                                if (eb->changeCallback()) {
                                    if (eb->flags() & nanogui::Button::ToggleButton)
                                        eb->changeCallback()(!eb->pushed());
                                    else
                                        eb->changeCallback()(eb->pushed());
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return nanogui::Screen::keyboardEvent(key, scancode, action, modifiers);
}

bool isURL(const std::string name) {
    //if (matches(name.c_str(), "((http[s]?|ftp):/)?/?([^:/\\s]+)((/\\w+)*/)([\\w-\\.]+[^#?\\s]+)(.*)?(#[\\w\\-]+)?")) return true;
    return matches(name.c_str(), "^http://.*");
}

void EditorGUI::freeImage(GLuint image_id) {
    auto found = loaded_textures.find(image_id);
    if (found != loaded_textures.end()) {
        std::string tex_name = (*found).second;
        auto found_tex = texture_cache.find(tex_name);
        if (found_tex != texture_cache.end()) {
            Texture *tex = (*found_tex).second;
            delete tex;
            //std::cout << "remaining textures: " << texture_cache.size() << " (" << loaded_textures.size() << ")\n";
        }
    }
}

GLuint EditorGUI::getImageId(const char *source, bool reload) {
    GLuint blank_id = 0;
    auto project_settings = findStructure("ProjectSettings");
    std::string asset_path = project_settings->getStringProperty("asset_path", ".");
    std::string blank_name = "images/blank";
    std::string name(source);
    /*
	for(auto it = mImagesData.begin(); it != mImagesData.end(); ++it) {
		const GLTexture &tex = (*it).first;
		if (tex.textureName() == name) {
			cout << "found image " << tex.textureName() << "\n";
			return tex.texture();
		}
		else if (blank_name == tex.textureName())
			blank_id = tex.texture();
	}
	*/
    if (isURL(name)) {
        std::string cache_name = "cache";
        if (!boost::filesystem::exists(cache_name))
            boost::filesystem::create_directory(cache_name);
        cache_name += "/" + shortName(name) + "." + extn(name);
        // example: http://www.valeparksoftwaredevelopment.com/cmsimages/logo-full-1.png
        if ((reload || !boost::filesystem::exists(cache_name)) && !get_file(name, cache_name)) {
            std::cerr << "Error fetching image file\n";
            return blank_id;
        }
        name = cache_name;
    }
    std::string tex_name = shortName(name);
    auto found = texture_cache.find(tex_name);
    if (!reload && found != texture_cache.end())
        return (*found).second->texture.texture();
    else if (reload || found == texture_cache.end()) {
        if (found != texture_cache.end()) {
            Texture *old = (*found).second;
            if (old->texture.texture()) {
                int texture_id = old->texture.texture();
                assert(texture_id);
                int refs = ResourceManager::release(texture_id);
                if (refs == 0) {
                    texture_cache.erase(found);
                }
                else {
                    // there are image views that still reference this image but we have been asked to
                    // reload the cached value. We hand this over to a texture resource manager
                    // and let it remove the object once the last release occurs
                    TextureResourceManagerFactory factory;
                    ResourceManager::handover(texture_id, factory);
                    old->texture.detach(); // do not call glDeleteTextures() when old is deleted
                }
                delete old;
            }
        }

        // not already loaded, attempt to load from file
        GLTexture tex(tex_name);
        try {
            auto tex_data = tex.load(name);
            GLuint res = tex.texture();
            if (res) {
                texture_cache[tex_name] = new Texture(std::move(tex), std::move(tex_data));
                loaded_textures[res] = tex_name;
                ResourceManager::manage(res);
            }
            return res;
        }
        catch (std::invalid_argument &err) {
            std::cerr << err.what() << "\n";
        }
    }
    return blank_id;
}

void cleanupTextureCache() {
    uint64_t now = microsecs();
    if (texture_cache.size() < 2)
        return;
    std::map<uint64_t, Texture *> to_remove;
    auto iter = texture_cache.begin();
    while (iter != texture_cache.end()) {
        const std::pair<std::string, Texture *> &item = *iter;
        Texture *texture = item.second;
        GLuint tex = item.second->texture.texture();
        ResourceManager *manager = ResourceManager::find(tex);
        if (manager && manager->uses() == 1 && manager->lastReleaseTime() &&
            now - manager->lastReleaseTime() > 10000000) {
            //ResourceManager::release(tex);
            //delete texture;
            //iter = texture_cache.erase(iter);
            to_remove.insert(std::make_pair(manager->lastReleaseTime(), texture));
            ++iter;
        }
        else
            ++iter;
    }
    auto remove = to_remove.begin();
    int n = texture_cache.size();
    while (remove != to_remove.end()) {
        const std::pair<uint64_t, Texture *> &item = *remove;
        if (--n > 8 || (now - (*remove).first) >
                           60000000) { //minimum cache size and images older than 10m are flushed
            Texture *texture = (*remove).second;
            GLuint tex = (*remove).second->texture.texture();
            texture_cache.erase(texture_cache.find(texture->texture.textureName()));
            ResourceManager::release(tex);
            delete texture;
            remove = to_remove.erase(remove);
        }
        else
            ++remove;
        //std::cout << "texture cache flushed. remaining: " << texture_cache.size() << "\n";
    }
}
/*
void cleanupTextureCache(GLuint tex) {
	auto iter = texture_cache.begin();
	while (iter != texture_cache.end()) {
		const std::pair<std::string, Texture*> &item = *iter;
		if (item.second->texture.texture() == tex) {
			Texture *texture = item.second;
			delete texture;
			iter = texture_cache.erase(iter);
			return;
		}
		else ++iter;
	}
}

void cleanupTextureCache() {
	auto iter = deferred_texture_cleanup.begin();
	while (iter != deferred_texture_cleanup.end()) {
		std::pair<GLuint, uint64_t> item = *iter;
		if (item.second - now > 60000) {
			GLuint id = item.first;
			if (ResourceManager::release(id) == 0)
				cleanupTextureCache(id);

			iter = deferred_texture_cleanup.erase(iter);
		}
		else ++iter;
	}
}
*/

EditorKeyboard::EditorKeyboard() {
    key_names.insert(std::make_pair(GLFW_KEY_SPACE, "KEY_SPACE"));
    key_names.insert(std::make_pair(GLFW_KEY_APOSTROPHE, "KEY_APOSTROPHE"));
    key_names.insert(std::make_pair(GLFW_KEY_COMMA, "KEY_COMMA"));
    key_names.insert(std::make_pair(GLFW_KEY_MINUS, "KEY_MINUS"));
    key_names.insert(std::make_pair(GLFW_KEY_PERIOD, "KEY_PERIOD"));
    key_names.insert(std::make_pair(GLFW_KEY_SLASH, "KEY_SLASH"));
    key_names.insert(std::make_pair(GLFW_KEY_0, "KEY_0"));
    key_names.insert(std::make_pair(GLFW_KEY_1, "KEY_1"));
    key_names.insert(std::make_pair(GLFW_KEY_2, "KEY_2"));
    key_names.insert(std::make_pair(GLFW_KEY_3, "KEY_3"));
    key_names.insert(std::make_pair(GLFW_KEY_4, "KEY_4"));
    key_names.insert(std::make_pair(GLFW_KEY_5, "KEY_5"));
    key_names.insert(std::make_pair(GLFW_KEY_6, "KEY_6"));
    key_names.insert(std::make_pair(GLFW_KEY_7, "KEY_7"));
    key_names.insert(std::make_pair(GLFW_KEY_8, "KEY_8"));
    key_names.insert(std::make_pair(GLFW_KEY_9, "KEY_9"));
    key_names.insert(std::make_pair(GLFW_KEY_SEMICOLON, "KEY_SEMICOLON"));
    key_names.insert(std::make_pair(GLFW_KEY_EQUAL, "KEY_EQUAL"));
    key_names.insert(std::make_pair(GLFW_KEY_A, "KEY_A"));
    key_names.insert(std::make_pair(GLFW_KEY_B, "KEY_B"));
    key_names.insert(std::make_pair(GLFW_KEY_C, "KEY_C"));
    key_names.insert(std::make_pair(GLFW_KEY_D, "KEY_D"));
    key_names.insert(std::make_pair(GLFW_KEY_E, "KEY_E"));
    key_names.insert(std::make_pair(GLFW_KEY_F, "KEY_F"));
    key_names.insert(std::make_pair(GLFW_KEY_G, "KEY_G"));
    key_names.insert(std::make_pair(GLFW_KEY_H, "KEY_H"));
    key_names.insert(std::make_pair(GLFW_KEY_I, "KEY_I"));
    key_names.insert(std::make_pair(GLFW_KEY_J, "KEY_J"));
    key_names.insert(std::make_pair(GLFW_KEY_K, "KEY_K"));
    key_names.insert(std::make_pair(GLFW_KEY_L, "KEY_L"));
    key_names.insert(std::make_pair(GLFW_KEY_M, "KEY_M"));
    key_names.insert(std::make_pair(GLFW_KEY_N, "KEY_N"));
    key_names.insert(std::make_pair(GLFW_KEY_O, "KEY_O"));
    key_names.insert(std::make_pair(GLFW_KEY_P, "KEY_P"));
    key_names.insert(std::make_pair(GLFW_KEY_Q, "KEY_Q"));
    key_names.insert(std::make_pair(GLFW_KEY_R, "KEY_R"));
    key_names.insert(std::make_pair(GLFW_KEY_S, "KEY_S"));
    key_names.insert(std::make_pair(GLFW_KEY_T, "KEY_T"));
    key_names.insert(std::make_pair(GLFW_KEY_U, "KEY_U"));
    key_names.insert(std::make_pair(GLFW_KEY_V, "KEY_V"));
    key_names.insert(std::make_pair(GLFW_KEY_W, "KEY_W"));
    key_names.insert(std::make_pair(GLFW_KEY_X, "KEY_X"));
    key_names.insert(std::make_pair(GLFW_KEY_Y, "KEY_Y"));
    key_names.insert(std::make_pair(GLFW_KEY_Z, "KEY_Z"));
    key_names.insert(std::make_pair(GLFW_KEY_LEFT_BRACKET, "KEY_LEFT_BRACKET"));
    key_names.insert(std::make_pair(GLFW_KEY_BACKSLASH, "KEY_BACKSLASH"));
    key_names.insert(std::make_pair(GLFW_KEY_RIGHT_BRACKET, "KEY_RIGHT_BRACKET"));
    key_names.insert(std::make_pair(GLFW_KEY_GRAVE_ACCENT, "KEY_GRAVE_ACCENT"));
    key_names.insert(std::make_pair(GLFW_KEY_WORLD_1, "KEY_WORLD_1"));
    key_names.insert(std::make_pair(GLFW_KEY_WORLD_2, "KEY_WORLD_2"));
    key_names.insert(std::make_pair(GLFW_KEY_ESCAPE, "KEY_ESCAPE"));
    key_names.insert(std::make_pair(GLFW_KEY_ENTER, "KEY_ENTER"));
    key_names.insert(std::make_pair(GLFW_KEY_TAB, "KEY_TAB"));
    key_names.insert(std::make_pair(GLFW_KEY_BACKSPACE, "KEY_BACKSPACE"));
    key_names.insert(std::make_pair(GLFW_KEY_INSERT, "KEY_INSERT"));
    key_names.insert(std::make_pair(GLFW_KEY_DELETE, "KEY_DELETE"));
    key_names.insert(std::make_pair(GLFW_KEY_RIGHT, "KEY_RIGHT"));
    key_names.insert(std::make_pair(GLFW_KEY_LEFT, "KEY_LEFT"));
    key_names.insert(std::make_pair(GLFW_KEY_DOWN, "KEY_DOWN"));
    key_names.insert(std::make_pair(GLFW_KEY_UP, "KEY_UP"));
    key_names.insert(std::make_pair(GLFW_KEY_PAGE_UP, "KEY_PAGE_UP"));
    key_names.insert(std::make_pair(GLFW_KEY_PAGE_DOWN, "KEY_PAGE_DOWN"));
    key_names.insert(std::make_pair(GLFW_KEY_HOME, "KEY_HOME"));
    key_names.insert(std::make_pair(GLFW_KEY_END, "KEY_END"));
    key_names.insert(std::make_pair(GLFW_KEY_CAPS_LOCK, "KEY_CAPS_LOCK"));
    key_names.insert(std::make_pair(GLFW_KEY_SCROLL_LOCK, "KEY_SCROLL_LOCK"));
    key_names.insert(std::make_pair(GLFW_KEY_NUM_LOCK, "KEY_NUM_LOCK"));
    key_names.insert(std::make_pair(GLFW_KEY_PRINT_SCREEN, "KEY_PRINT_SCREEN"));
    key_names.insert(std::make_pair(GLFW_KEY_PAUSE, "KEY_PAUSE"));
    key_names.insert(std::make_pair(GLFW_KEY_F1, "KEY_F1"));
    key_names.insert(std::make_pair(GLFW_KEY_F2, "KEY_F2"));
    key_names.insert(std::make_pair(GLFW_KEY_F3, "KEY_F3"));
    key_names.insert(std::make_pair(GLFW_KEY_F4, "KEY_F4"));
    key_names.insert(std::make_pair(GLFW_KEY_F5, "KEY_F5"));
    key_names.insert(std::make_pair(GLFW_KEY_F6, "KEY_F6"));
    key_names.insert(std::make_pair(GLFW_KEY_F7, "KEY_F7"));
    key_names.insert(std::make_pair(GLFW_KEY_F8, "KEY_F8"));
    key_names.insert(std::make_pair(GLFW_KEY_F9, "KEY_F9"));
    key_names.insert(std::make_pair(GLFW_KEY_F10, "KEY_F10"));
    key_names.insert(std::make_pair(GLFW_KEY_F11, "KEY_F11"));
    key_names.insert(std::make_pair(GLFW_KEY_F12, "KEY_F12"));
    key_names.insert(std::make_pair(GLFW_KEY_F13, "KEY_F13"));
    key_names.insert(std::make_pair(GLFW_KEY_F14, "KEY_F14"));
    key_names.insert(std::make_pair(GLFW_KEY_F15, "KEY_F15"));
    key_names.insert(std::make_pair(GLFW_KEY_F16, "KEY_F16"));
    key_names.insert(std::make_pair(GLFW_KEY_F17, "KEY_F17"));
    key_names.insert(std::make_pair(GLFW_KEY_F18, "KEY_F18"));
    key_names.insert(std::make_pair(GLFW_KEY_F19, "KEY_F19"));
    key_names.insert(std::make_pair(GLFW_KEY_F20, "KEY_F20"));
    key_names.insert(std::make_pair(GLFW_KEY_F21, "KEY_F21"));
    key_names.insert(std::make_pair(GLFW_KEY_F22, "KEY_F22"));
    key_names.insert(std::make_pair(GLFW_KEY_F23, "KEY_F23"));
    key_names.insert(std::make_pair(GLFW_KEY_F24, "KEY_F24"));
    key_names.insert(std::make_pair(GLFW_KEY_F25, "KEY_F25"));
    key_names.insert(std::make_pair(GLFW_KEY_KP_0, "KEYPAD_0"));
    key_names.insert(std::make_pair(GLFW_KEY_KP_1, "KEYPAD_1"));
    key_names.insert(std::make_pair(GLFW_KEY_KP_2, "KEYPAD_2"));
    key_names.insert(std::make_pair(GLFW_KEY_KP_3, "KEYPAD_3"));
    key_names.insert(std::make_pair(GLFW_KEY_KP_4, "KEYPAD_4"));
    key_names.insert(std::make_pair(GLFW_KEY_KP_5, "KEYPAD_5"));
    key_names.insert(std::make_pair(GLFW_KEY_KP_6, "KEYPAD_6"));
    key_names.insert(std::make_pair(GLFW_KEY_KP_7, "KEYPAD_7"));
    key_names.insert(std::make_pair(GLFW_KEY_KP_8, "KEYPAD_8"));
    key_names.insert(std::make_pair(GLFW_KEY_KP_9, "KEYPAD_9"));
    key_names.insert(std::make_pair(GLFW_KEY_KP_DECIMAL, "KEYPAD_DECIMAL"));
    key_names.insert(std::make_pair(GLFW_KEY_KP_DIVIDE, "KEYPAD_DIVIDE"));
    key_names.insert(std::make_pair(GLFW_KEY_KP_MULTIPLY, "KEYPAD_MULTIPLY"));
    key_names.insert(std::make_pair(GLFW_KEY_KP_SUBTRACT, "KEYPAD_SUBTRACT"));
    key_names.insert(std::make_pair(GLFW_KEY_KP_ADD, "KEYPAD_ADD"));
    key_names.insert(std::make_pair(GLFW_KEY_KP_ENTER, "KEYPAD_ENTER"));
    key_names.insert(std::make_pair(GLFW_KEY_KP_EQUAL, "KEYPAD_EQUAL"));
    key_names.insert(std::make_pair(GLFW_KEY_LEFT_SHIFT, "KEY_LEFT_SHIFT"));
    key_names.insert(std::make_pair(GLFW_KEY_LEFT_CONTROL, "KEY_LEFT_CONTROL"));
    key_names.insert(std::make_pair(GLFW_KEY_LEFT_ALT, "KEY_LEFT_ALT"));
    key_names.insert(std::make_pair(GLFW_KEY_LEFT_SUPER, "KEY_LEFT_SUPER"));
    key_names.insert(std::make_pair(GLFW_KEY_RIGHT_SHIFT, "KEY_RIGHT_SHIFT"));
    key_names.insert(std::make_pair(GLFW_KEY_RIGHT_CONTROL, "KEY_RIGHT_CONTROL"));
    key_names.insert(std::make_pair(GLFW_KEY_RIGHT_ALT, "KEY_RIGHT_ALT"));
    key_names.insert(std::make_pair(GLFW_KEY_RIGHT_SUPER, "KEY_RIGHT_SUPER"));
    key_names.insert(std::make_pair(GLFW_KEY_MENU, "KEY_MENU"));
}

void EditorGUI::setTheme(nanogui::Theme *new_theme) {
    ClockworkClient::setTheme(new_theme);
    theme = new_theme;
}

void EditorGUI::createWindows() {
    using namespace nanogui;
    editor = new Editor(this);

    UserWindowWin *uww = new UserWindowWin(this, "Untitled");
    user_screens.push_back(uww);
    // The user window uses the main theme
    auto uwTheme = ThemeManager::instance().findTheme("MainTheme");
    if (!uwTheme) {
        uwTheme = ThemeManager::instance().createTheme();
        ThemeManager::instance().addTheme("MainTheme", uwTheme);
    }
    extern int run_only;
    extern int full_screen_mode;
    if (size().y() == uww->size().y() || run_only || full_screen_mode) {
        uwTheme->mWindowHeaderHeight = 0;
    }
    w_user = new UserWindow(this, uwTheme, uww);
    w_theme = new ThemeWindow(this, theme, uwTheme);
    w_properties = new PropertyWindow(this, theme);
    w_toolbar = new Toolbar(this, theme);
    w_startup = new StartupWindow(this, theme);
    w_objects = new ObjectWindow(this, theme);

    w_structures = StructuresWindow::create(this, theme);

    w_patterns = new PatternsWindow(this, theme);
    w_screens = new ScreensWindow(this, theme);
    w_views = new ViewsWindow(this, theme);
    /*
	ConfirmDialog *cd = new ConfirmDialog(this, "Humid V0.23");
	cd->setCallback([cd,this]{
		cd->setVisible(false);
		this->setState(EditorGUI::GUISELECTPROJECT);
	});
	window = cd->getWindow();
	window->setTheme(theme);
	*/

    //EditorSettings::applySettings("MainWindow", this);
    EditorSettings::applySettings("ThemeSettings", w_theme->getWindow());
    EditorSettings::applySettings("Properties", w_properties->getWindow());
    EditorSettings::applySettings("Structures", w_structures->getWindow());
    EditorSettings::applySettings("Patterns", w_patterns->getWindow());
    EditorSettings::applySettings("Objects", w_objects->getWindow());
    EditorSettings::applySettings("ScreensWindow", w_screens->getWindow());

    // delayed adding windows to the view manager window until the visibility settings are loaded.
    w_views->addWindows();

    //EditorSettings::add("MainWindow", this);
    EditorSettings::add("ThemeSettings", w_theme->getWindow());
    EditorSettings::add("Properties", w_properties->getWindow());
    EditorSettings::add("Structures", w_structures->getWindow());
    EditorSettings::add("Patterns", w_patterns->getWindow());
    EditorSettings::add("Objects", w_objects->getWindow());
    EditorSettings::add("ScreensWindow", w_screens->getWindow());

    uww->setMoveListener(
        [](nanogui::Window *value) { updateSettingsStructure("MainWindow", value); });
    w_objects->getSkeletonWindow()->setMoveListener(
        [](nanogui::Window *value) { updateSettingsStructure("Objects", value); });
    w_patterns->getSkeletonWindow()->setMoveListener(
        [](nanogui::Window *value) { updateSettingsStructure("Patterns", value); });
    w_structures->getSkeletonWindow()->setMoveListener(
        [](nanogui::Window *value) { updateSettingsStructure("Structures", value); });
    w_screens->getSkeletonWindow()->setMoveListener(
        [](nanogui::Window *value) { updateSettingsStructure("ScreensWindow", value); });
    w_screens->update();
    performLayout(mNVGContext);
    setState(EditorGUI::GUISELECTPROJECT);
}

void EditorGUI::setState(EditorGUI::GuiState s) {
    bool editmode = false;
    bool done = false;
    while (!done) {
        switch (s) {
        case GUIWELCOME:
            done = true;
            break;
        case GUISELECTPROJECT:
            if (hm_structures.size() > 1) { // files were specified on the commandline
                getStartupWindow()->setVisible(false);
                getScreensWindow()->update();

                Structure *settings = EditorSettings::find("EditorSettings");
                assert(settings);
                const Value &project_base_v(settings->getProperties().find("project_base"));
                if (project_base_v == SymbolTable::Null) {
                    s = GUICREATEPROJECT;
                    done = false;
                    break;
                }
                s = GUIWORKING;
                done = false;
            }
            else {
                window = getStartupWindow()->getWindow();
                window->setVisible(true);
                done = true;
            }
            break;
        case GUICREATEPROJECT: {
            getStartupWindow()->setVisible(false);
            Structure *settings(getSettings());
            const Value &path(settings->getProperties().find("project_base"));
            if (path != SymbolTable::Null)
                project = new EditorProject(path.asString().c_str());
            if (!project)
                project = new EditorProject("UntitledProject");
            getUserWindow()->setStructure(createScreenStructure());
            getUserWindow()->setVisible(true);
            getToolbar()->setVisible(!run_only);
            done = true;
        } break;
        case GUIEDITMODE:
            editmode = true;
            getUserWindow()->startEditMode();
            // fall through
        case GUIWORKING:
            getUserWindow()->endEditMode();
            getUserWindow()->setVisible(true);
            //getScreensWindow()->selectFirst();
            getToolbar()->setVisible(!run_only);
            if (getPropertyWindow()) {
                nanogui::Window *w = getPropertyWindow()->getWindow();
                w->setVisible(editmode && views.get("Properties").visible);
            }
            if (getPatternsWindow()) {
                nanogui::Window *w = getPatternsWindow()->getWindow();
                w->setVisible(editmode && views.get("Patterns").visible);
            }
            if (getStructuresWindow()) {
                nanogui::Window *w = getStructuresWindow()->getWindow();
                w->setVisible(editmode && views.get("Structures").visible);
            }
            if (getObjectWindow()) {
                nanogui::Window *w = getObjectWindow()->getWindow();
                w->setVisible(editmode && views.get("Objects").visible);
            }
            if (getScreensWindow()) {
                nanogui::Window *w = getScreensWindow()->getWindow();
                w->setVisible(editmode && views.get("ScreensWindow").visible);
            }
            Value remote_screen(EditorGUI::systemSettings()->getProperties().find("remote_screen"));
            if (remote_screen != SymbolTable::Null) {
                LinkableProperty *lp = findLinkableProperty(remote_screen.asString());
                if (lp) {
                    lp->apply();
                    std::cout << "\nWorking mode: setting screen to " << lp->value() << "\n\n";
                }
            }
            Value remote_dialog(EditorGUI::systemSettings()->getProperties().find("remote_dialog"));
            if (remote_dialog != SymbolTable::Null) {
                LinkableProperty *lp = findLinkableProperty(remote_dialog.asString());
                if (lp) {
                    lp->apply();
                    std::cout << "\nWorking mode: setting dialog to " << lp->value() << "\n\n";
                }
            }

            done = true;
            break;
        }
    }
    state = s;
}

nanogui::Vector2i fixPlacement(nanogui::Widget *w, nanogui::Widget *container,
                               nanogui::Vector2i &pos) {
    if (pos.x() < 0)
        pos = Vector2i(0, pos.y());
    if (pos.x() + w->width() > container->width())
        pos = Vector2i(container->width() - w->width(), pos.y());
    if (pos.y() < w->theme()->mWindowHeaderHeight)
        pos = Vector2i(pos.x(), w->theme()->mWindowHeaderHeight + 1);
    if (pos.y() + w->height() > container->height())
        pos = Vector2i(pos.x(), container->height() - w->height());
    return pos;
}

void EditorGUI::createStructures(const nanogui::Vector2i &p, std::set<Selectable *> selections) {
    using namespace nanogui;

    Widget *window = w_user->getWindow();
    DragHandle *drag_handle = editor->getDragHandle();

    drag_handle->incRef();
    removeChild(drag_handle);
    PropertyMonitor *pm = drag_handle->propertyMonitor();
    drag_handle->setPropertyMonitor(0);

    assert(w_user->structure());
    StructureClass *screen_sc = w_user->structure()->getStructureDefinition();
    assert(screen_sc);

    unsigned int offset = 0;
    for (Selectable *sel : selections) {
        SelectableButton *item = dynamic_cast<SelectableButton *>(sel);
        if (!item)
            continue;
        std::cout << "creating instance of " << item->getClass() << "\n";
        nanogui::Widget *w = item->create(window);
        if (w) {
            EditorWidget *ew = dynamic_cast<EditorWidget *>(w);
            Parameter param(ew->getName());
            ew->updateStructure();
            param.machine = ew->getDefinition();
            screen_sc->addLocal(param);
            ew->getDefinition()->setOwner(w_user->structure());

            //if (ew) ew->setName( NamedObject::nextName(ew) );
            Vector2i pos(p - window->position() - w->size() / 2 + nanogui::Vector2i(0, offset));
            w->setPosition(fixPlacement(w, window, pos));
            offset += w->height() + 8;
        }
    }
    window->performLayout(nvgContext());
    for (auto child : window->children()) {
        EditorWidget *ew = dynamic_cast<EditorWidget *>(child);
        if (ew)
            ew->updateStructure();
    }

    window->addChild(drag_handle);
    drag_handle->setPropertyMonitor(pm);
    drag_handle->decRef();
}

DialogWindow *EditorGUI::getUserDialog() { return w_dialog; }

void EditorGUI::setUserDialog(const std::string &name) {
    if (name == dialog_name) {
        return;
    }
    // TODO: check for visibility of the old dialog and deal with it.
    dialog_name = name;
}

void EditorGUI::showDialog(bool show) {
    Editor *editor = EDITOR;
    std::cout << "request to " << (show ? "show" : "hide") << " dialog " << dialog_name << "\n";
    if (show) {
        if (!w_dialog) {
            w_dialog = new DialogWindow(editor->gui(), mTheme);
            w_dialog->setVisible(false);
            is_dialog_visible = false;
        }
        auto s = findScreen(dialog_name);
        if (s) {
            w_dialog->setStructure(s);
            w_dialog->setVisible(true);
            is_dialog_visible = true;
        }
    }
    else if (w_dialog) {
        w_dialog->setVisible(false);
        is_dialog_visible = false;
        w_dialog->dispose();
        w_dialog = nullptr;
    }
}

bool EditorGUI::dialogIsVisible() {
    return is_dialog_visible;
}

bool EditorGUI::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) {

    using namespace nanogui;

    nanogui::Window *window = w_user->getWindow();
    if (!window || !window->visible())
        return Screen::mouseButtonEvent(p, button, down, modifiers);

    Widget *clicked = findWidget(p);

    Widget *ww = dynamic_cast<Widget *>(window);

    bool is_user = EDITOR->gui()->getUserWindow()->getWindow()->focused();

    if (button != GLFW_MOUSE_BUTTON_1 || !is_user ||
        !window->contains(p /*- window->position()*/)) {
        if (!clicked)
            return Screen::mouseButtonEvent(p, button, down, modifiers);
        Widget *parent = clicked->parent();
        while (parent && parent->parent()) {
            clicked = parent;
            parent = clicked->parent();
        }
        if (!clicked->focused() && parent != window)
            clicked->requestFocus();
        return Screen::mouseButtonEvent(p, button, down, modifiers);
    }

    bool is_child = false;
    nanogui::Vector2i pos(p - window->position());
    for (auto elem : window->children()) {
        if (elem->contains(pos)) {
            is_child = true;
            break;
        }
    }

    nanogui::DragHandle *drag_handle = editor->getDragHandle();

    if (drag_handle && EDITOR->isEditMode()) {
        if (clicked == ww && !is_child) {
            if (getStructuresWindow()->hasSelections()) {
                if (down) {
                    createStructures(p, getStructuresWindow()->getSelected());
                    getStructuresWindow()->clearSelections();
                }
                return false;
            }
            else if (getObjectWindow()->hasSelections()) {
                if (down) {
                    createStructures(p, getObjectWindow()->getSelected());
                    getObjectWindow()->clearSelections();
                }
                return false;
            }
            else {
                if (down) {
                    window->requestFocus();
                    getUserWindow()->clearSelections();
                    if (getPropertyWindow())
                        getPropertyWindow()->update();
                }
                return Screen::mouseButtonEvent(p, button, down, modifiers);
            }
        }
        else {
            if (down) {
                if (drag_handle)
                    drag_handle->setVisible(false);
                requestFocus();
                // deselect items on the window
                if (w_user->hasSelections() && (modifiers & GLFW_MOD_CONTROL)) {
                    auto selected = w_user->getSelected();
                    auto tmp = selected;
                    for (auto sel : tmp) {
                        sel->deselect();
                        nanogui::Widget *w = dynamic_cast<nanogui::Widget *>(sel);
                    }
                    if (getPropertyWindow())
                        getPropertyWindow()->update();
                }
                return Screen::mouseButtonEvent(p, button, down, modifiers);
            }
        }
        return true;
    }
    else {
        //not edit mode
        //window->requestFocus();
        return Screen::mouseButtonEvent(p, button, down, modifiers);
    }
}

bool EditorGUI::resizeEvent(const Vector2i &new_size) {
    if (old_size == new_size) {
        return false;
    }
    nanogui::Window *windows[] = {
        this->getStructuresWindow()->getWindow(), this->getPropertyWindow()->getWindow(),
        this->getPatternsWindow()->getWindow(),   this->getThemeWindow()->getWindow(),
        this->getViewsWindow()->getWindow(),      this->getObjectWindow()->getWindow(),
        this->getScreensWindow()->getWindow()};
    std::list<std::pair<nanogui::Window *, nanogui::Vector2i>> positions;
    for (unsigned int i = 0; i < 5; ++i) {
        nanogui::Window *w = windows[i];
        positions.push_back(
            std::make_pair(w, nanogui::Vector2i(w->position().x(), w->position().y())));
    }
    float x_scale = (float)new_size.x() / (float)old_size.x();
    float y_scale = (float)new_size.y() / (float)old_size.y();
    bool res = nanogui::Screen::resizeEvent(new_size);
    for (auto it = positions.begin(); it != positions.end(); ++it) {
        std::pair<nanogui::Window *, nanogui::Vector2i> item = *it;
        nanogui::Window *w = item.first;
        nanogui::Vector2i pos = item.second;

        // items closer to the rhs stay the same distance from the rhs after scaling
        // similarly for items close to the lhs
        //pos.x() = (int) ((float)item.second.x() * x_scale);
        //pos.y() = (int) ((float)item.second.y() * y_scale);

        int lhs = pos.x(), rhs = old_size.x() - pos.x() - w->width();
        if (rhs < lhs)
            pos.x() = new_size.x() - rhs - w->width();

        // similarly for vertical offset
        //int top = pos.y(), bot = old_size.y() - pos.y() - w->height();
        //if(bot < lhs) pos.x() = new_size.x() - rhs - w->width();
        pos.y() = (int)((float)item.second.y() * y_scale);

        if (pos.x() < 0)
            pos.x() = 0;
        if (pos.y() < 0)
            pos.y() = 0;
        if (pos.x() + w->width() > new_size.x())
            pos.x() = new_size.x() - w->width();
        if (pos.y() + w->height() > new_size.y())
            pos.y() = new_size.y() - w->height();

        item.first->setPosition(pos);
    }
    old_size = mSize;
    if (w_user) {
        //w_user->getWindow()->setFixedSize(new_size);
        //w_user->getWindow()->setPosition(nanogui::Vector2i(0,0));
        performLayout();
    }
    return true;
}

LinkableProperty *EditorGUI::findLinkableProperty(const std::string name) {
    std::lock_guard<std::recursive_mutex> lock(linkables_mutex);
    std::map<std::string, LinkableProperty *>::iterator found = linkables.find(name);
    if (found == linkables.end()) {
        return LinkManager::instance().links(name);
    }
    // std::cout << "EditorGUI found linkable " << name << " with " << (found->second ? found->second->num_links() : 0) << " links\n";
    return (*found).second;
}

void EditorGUI::handleClockworkMessage(ClockworkClient::Connection *conn, unsigned long now,
                                       const std::string &op, std::list<Value> *message) {
    if (op == "UPDATE") {
        if (!this->getUserWindow())
            return;

        int pos = 0;
        std::string name;
        long val = 0;
        double dval = 0.0;
        CircularBuffer *buf = 0;
        LinkableProperty *lp = 0;
        for (auto &v : *message) {
            if (pos == 2) {
                name = v.asString();
                lp = findLinkableProperty(name);
                buf = w_user->getValues(name);
                if (collect_history) {
                    if (!buf && lp && lp->dataType() != CircularBuffer::STR) {
                        buf = w_user->addDataBuffer(name, lp->dataType(), collect_history);
                        if (!buf)
                            std::cout << "no buffer for " << name << "\n";
                    }
                }
            }
            else if (pos == 4) {
                if (lp) {
                    lp->setValue(v);
                }
                if (buf) {
                    CircularBuffer::DataType dt = buf->getDataType();
                    if (v.asInteger(val)) {
                        if (dt == CircularBuffer::INT16) {
                            buf->addSample(now, (int16_t)(val & 0xffff));
                        }
                        else if (dt == CircularBuffer::INT32) {
                            buf->addSample(now, (int32_t)(val & 0xffffffff));
                        }
                        else
                            buf->addSample(now, val);
                    }
                    else if (v.asFloat(dval)) {
                        buf->addSample(now, dval);
                    }
                }
            }
            ++pos;
        }
    }
    else {
        std::cout << "unhandled: " << op << "\n";
    }
}

void EditorGUI::processModbusInitialisation(const std::string group_name, cJSON *obj) {
    int num_params = cJSON_GetArraySize(obj);
    if (num_params) {
        for (int i = 0; i < num_params; ++i) {
            cJSON *item = cJSON_GetArrayItem(obj, i);
            if (item->type == cJSON_Array) {
                Value group(MessageEncoding::valueFromJSONObject(cJSON_GetArrayItem(item, 0), 0));
                Value addr(MessageEncoding::valueFromJSONObject(cJSON_GetArrayItem(item, 1), 0));
                Value kind(MessageEncoding::valueFromJSONObject(cJSON_GetArrayItem(item, 2), 0));
                Value name(MessageEncoding::valueFromJSONObject(cJSON_GetArrayItem(item, 3), 0));
                Value len(MessageEncoding::valueFromJSONObject(cJSON_GetArrayItem(item, 4), 0));
                Value value(MessageEncoding::valueFromJSONObject(cJSON_GetArrayItem(item, 5), 0));
                if (DEBUG_BASIC)
                    std::cout << name << ": " << group << " " << addr << " " << len << " " << value
                              << "\n";
                if (value.kind == Value::t_string) {
                    std::string valstr = value.asString();
                    //insert((int)group.iValue, (int)addr.iValue-1, valstr.c_str(), valstr.length()+1); // note copying null
                }

                if (name.kind == Value::t_string || name.kind == Value::t_symbol) {

                    size_t n = name.sValue.length();
                    const char *p = name.sValue.c_str();
                    if (n > 2 && p[0] == '"' && p[n - 1] == '"') {
                        char buf[n];
                        memcpy(buf, p + 1, n - 2);
                        buf[n - 2] = 0;
                        name = Value(buf, Value::t_string);
                    }
                }

                std::string prop_name(name.asString());
                {
                    size_t p = prop_name.find(".cmd_");

                    if (p != std::string::npos)
                        prop_name = prop_name.erase(p + 1, 4);
                }

                //else
                //	insert((int)group.iValue, (int)addr.iValue-1, (int)value.iValue, len.iValue);
                LinkableProperty *lp = findLinkableProperty(prop_name);
                if (!lp) {
                    std::lock_guard<std::recursive_mutex> lock(linkables_mutex);
                    char buf[10];
                    snprintf(buf, 10, "'%d%4d", (int)group.iValue, (int)addr.iValue);
                    std::string addr_str(buf);

                    lp = new LinkableProperty(group_name, group.iValue, prop_name, addr_str, "",
                                              len.iValue);
                    linkables[prop_name] = lp;
                }
                if (lp) {
                    if (group.iValue != lp->address_group())
                        std::cout << prop_name << " change of group from " << lp->address_group()
                                  << " to " << group << "\n";
                    if (addr.iValue != lp->address())
                        std::cout << prop_name << " change of address from " << lp->address()
                                  << " to " << addr << "\n";
                    lp->setAddressStr(group.iValue, addr.iValue);
                    lp->setValue(value);

                    //w_user->fixLinks(lp);

                    if (collect_history) {
                        CircularBuffer *buf = getUserWindow()->getValues(prop_name);
                        if (buf) {
                            long v;
                            double fv;
                            buf->clear();
                            if (value.asInteger(v))
                                buf->addSample(buf->getZeroTime(), v);
                            else if (value.asFloat(fv))
                                buf->addSample(buf->getZeroTime(), fv);
                        }
                    }
                }
            }
            else {
                char *node = cJSON_Print(item);
                std::cerr << "item " << i << " is not of the expected format: " << node << "\n";
                free(node);
            }
        }
    }
    if (DEBUG_WIDGET)
        std::cout << "Total linkable properties is now: " << linkables.size() << "\n";
}

void EditorGUI::update(ClockworkClient::Connection *connection) {
    if (connection->getStartupState() != sDONE && connection->getStartupState() != sRELOAD) {
        // if the tag file is loaded, get initial values
        if (/*linkables.size() && */ connection->getStartupState() == sINIT) {
            if (connection) {
                std::cout << "Sending data initialisation request\n";
                connection->setState(sSENT);
                queueMessage(
                    connection->getName(), "MODBUS REFRESH", [this, connection](std::string s) {
                        if (s != "failed") {
                            cJSON *obj = cJSON_Parse(s.c_str());
                            if (!obj) {
                                connection->setState(sINIT);
                                return;
                            }
                            if (obj->type == cJSON_Array) {
                                processModbusInitialisation(connection->getName(), obj);
                                w_objects->rebuildWindow();
                                if (w_user && getState() == GUIWORKING) {
                                    w_user->setStructure(w_user->structure());
                                    const Value remote_screen(
                                        EditorGUI::systemSettings()->getProperties().find(
                                            "remote_screen"));
                                    if (remote_screen != SymbolTable::Null) {
                                        LinkableProperty *lp =
                                            findLinkableProperty(remote_screen.asString());
                                        if (lp) {
                                            lp->link(getUserWindow());
                                        }
                                    }
                                    const Value remote_dialog(
                                        EditorGUI::systemSettings()->getProperties().find(
                                            "remote_dialog"));
                                    if (remote_dialog != SymbolTable::Null) {
                                        LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(
                                            remote_dialog.asString());
                                        if (lp) {
                                            class DialogNameTarget : public LinkTarget {
                                                EditorGUI *m_gui;

                                              public:
                                                DialogNameTarget(EditorGUI *gui) : m_gui(gui) {}
                                                void update(const Value &value) override {
                                                    m_gui->setUserDialog(value.asString());
                                                }
                                            };
                                            lp->clear();
                                            lp->link(new LinkableObject(
                                                new DialogNameTarget(EDITOR->gui())));
                                        }
                                    }

                                    Value remote_dialog_visible(
                                        EditorGUI::systemSettings()->getProperties().find(
                                            "remote_dialog_visibility"));
                                    if (remote_dialog_visible != SymbolTable::Null) {
                                        LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(
                                            remote_dialog_visible.asString());
                                        if (lp) {
                                            class DialogVisibilityTarget : public LinkTarget {
                                                EditorGUI *m_gui;

                                              public:
                                                DialogVisibilityTarget(EditorGUI *gui)
                                                    : m_gui(gui) {}
                                                void update(const Value &value) override {
                                                    long visible;
                                                    if (value.asInteger(visible)) {
                                                        m_gui->showDialog(visible);
                                                    }
                                                    else {
                                                        std::cout
                                                            << "could not parse " << value
                                                            << " as an integer to show/hide dialog\n";
                                                    }
                                                }
                                            };
                                            lp->clear();
                                            lp->link(new LinkableObject(
                                                new DialogVisibilityTarget(EDITOR->gui())));
                                        }
                                    }
                                }
                            }
                            cJSON_Delete(obj);
                            connection->setState(sRELOAD);
                        }
                        else
                            connection->setState(sINIT);
                    });
            }
        }
    }
    if (connection->getStartupState() == sDONE) {
        if (connection->needsRefresh()) {
            connection->setNeedsRefresh(false);
            connection->setState(sINIT);
        }
    }

    if (connection->getStartupState() == sDONE || connection->getStartupState() == sRELOAD) {
        if (w_user && getState() == GUIWORKING) {
            //bool changed = false;
            const Value &active(EditorGUI::systemSettings()->getProperties().find("active_screen"));
            if (active != SymbolTable::Null) {
                if (connection->getStartupState() == sRELOAD ||
                    (w_user->structure() && w_user->structure()->getName() != active.asString())) {
                    Structure *s = findScreen(active.asString());
                    if (connection->getStartupState() == sRELOAD ||
                        (s && w_user->structure() != s)) {
                        w_user->clearSelections();
                        w_user->getWindow()->requestFocus();
                        w_user->setStructure(s);
                        std::cout << "Loaded active screen " << active << "\n";
                        if (connection->getStartupState() == sRELOAD)
                            connection->setState(sDONE);
                    }
                    else if (connection->getStartupState() != sRELOAD)
                        std::cout << "Active screen " << active << " cannot be found\n";
                }
            }
        }
    }

    if (w_user)
        w_user->update();
    if (needs_update) {
        w_properties->getWindow()->performLayout(nvgContext());
        needs_update = false;
    }
    EditorSettings::flush();
    /*
	auto iter = texture_cache.begin();
	while (iter != texture_cache.end()) {
		const std::pair<std::string, GLTexture *> &item = *iter;
		if (item.second && item.second->texture()) {
			if
		}
	}
*/
}
