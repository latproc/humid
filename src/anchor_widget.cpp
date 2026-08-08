//
//  Anchor widget-side methods (EditorWidget / GUI). Not linked into hmifile_check.
//

#include "anchor.h"
#include "editorwidget.h"

std::ostream &WidgetPropertyAnchor::operator()(std::ostream &out) const {
  return out;
}
std::ostream &operator<<(std::ostream &out, const WidgetPropertyAnchor &wpa) {
  return wpa.operator()(out);
}

Value WidgetPropertyAnchor::get() {
  return ew->getPropertyValue(property_name);
}

void WidgetPropertyAnchor::set(Value v) {
  ew->setPropertyValue(property_name, v);
}
