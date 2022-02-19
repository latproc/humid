#include <nanogui/widget.h>

#include "listpanel.h"
#include "selectable.h"

ListPanel::ListPanel(nanogui::Widget *parent) : nanogui::Widget(parent) {}

void ListPanel::update() {}
Selectable *ListPanel::getSelectedItem() {
    if (!hasSelections())
        return 0;
    return *selections.begin();
}
void ListPanel::selectFirst() {}
