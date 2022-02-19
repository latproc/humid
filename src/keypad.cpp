#include "keypad.h"
#include "helper.h"

bool Keypad::show(const std::string &dialog_name) {
    auto s = findScreen(dialog_name);
    if (s) {
        setStructure(s);
        setVisible(true);
        // TODO add the text field, linked to the target
        return true;
    }
    return false;
}
