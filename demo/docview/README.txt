DOCVIEW demo (wiring / multi-page drawings)
==========================================

DOCVIEW displays offline-converted PDF pages as PNG images loaded over HTTP.
It is for wiring diagrams and CAD sheets (zoom + pan + page flip). Long text
manuals stay on HTMLVIEW with HTML sources.

Build: normal humid build (no extra deps beyond IMAGE/HTTP already used).

1. Serve converted page PNGs (required for the default source URL).

   Public sample (3 pages):

   cd /Users/mike/src/github/humid/demo/docview/pages
   python3 -m http.server 8767

   Plant wiring pilot (ELEC-0297, 6 pages @150dpi) — after offline convert:

   cd /tmp/humid-docview-pilot/web   # or your plant web/ tree
   python3 -m http.server 8767

   Default demo screen points at the public sample:
   http://127.0.0.1:8767/sample-drawing/

   For plant wiring (e.g. ELEC-0297), serve the converted tree and set
   DOCVIEW source/pages accordingly (private PDFs stay out of this repo).

2. Run the demo:

   cd /Users/mike/src/github/humid/demo
   ../build/humid docview/humid

   Or:  /path/to/humid /path/to/humid/demo/docview/humid

3. Screen Drawings loads DOCVIEW (pages: 3 for the sample pack).

   Controls (keyboard-first; mouse pan/wheel off):
   - Arrows: pan (Shift = larger step)
   - + / - : zoom about centre
   - F or 0: fit; C: centre
   - PgUp / PgDn or [ / ]: previous / next page
   - Home / End: first / last page
   - On-screen Prev / Next / Fit buttons

4. Plant drawings (private PDFs):
   - Convert offline with llm-rules/tools/pdf_to_web.py at ~150 dpi
     (see llm-rules/TOOLS.md and humid doc/document-viewing.md):
       python3 llm-rules/tools/pdf_to_web.py --width 1240 --height 700 --dpi 150 \
         --source /path/to/pdfs --web /path/to/web
   - Serve that tree over HTTP (do not commit private PDFs into humid).
   - Set DOCVIEW source to the document base URL (trailing slash OK) and
     pages to the page count.

Example:

   Doc DOCVIEW(
     source: "http://panel-docs:8767/elec-0297-…/",
     page: 1,
     pages: 6,
     interactive: 1,
     width: 1260, height: 660, pos_x: 10, pos_y: 56);

5. Clockwork:
   - String remote on DOCVIEW updates source (document base URL).
   - For page number, use a numeric link target (LinkableNumber) when wired.
