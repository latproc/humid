HTMLVIEW demo
=============

Requires humid built with HUMID_WITH_HTMLVIEW=ON (default when Cairo/Pango present).

1. Serve the operators manual (required for the default URL):

   cd /Users/mike/src/CW_Simulation/4C04/Docs/operators-manual
   python3 -m http.server 8765

2. From the humid demo directory:

   cd /Users/mike/src/github/humid/demo
   ../build/humid htmlview/humid

   Or from anywhere:

   /path/to/humid /path/to/humid/demo/htmlview/humid

3. Screen MANUAL loads with HTMLVIEW at
   http://127.0.0.1:8765/4C04_Operators_Manual.html

   Navigation:
   - Mouse wheel or drag to scroll
   - Arrow Up/Down, Page Up/Down
   - Home = top of document, End = bottom
   - Click TOC / in-document # links to jump
   - Orange URL fragment on load is applied after render
   - Blue "Top" button (bottom-right) when scrolled down

4. To drive the URL from Clockwork later, set the HTMLVIEW remote
   (e.g. remote: "V_ManualURL") and wire Return like IOCORE
   (remote: "M_CoreControl2Panel.Reset").
