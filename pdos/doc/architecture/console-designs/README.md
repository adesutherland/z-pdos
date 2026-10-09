# Console design alternatives

The user selected **Workbench** on 9 October 2026. Ledger and Focus remain
retained alternatives. The [design measurements](DESIGN-NOTES.md) describe
the three colour layouts, compatibility views and common teletype projection.

The six `*-80.txt` and `*-132.txt` files preserve exact character-grid designs;
the three `*-teletype.txt` files preserve their semantic-stream proposals.
They are illustrative design specifications, not guest screenshots or runtime
receipts. Original colour/monochrome SVG/PNG boards and cell-role maps are
preserved together as generated design artifacts.

These original alternatives predate the final key refinement. Their PF11/PF12
legends remain historical design text; the implemented Workbench uses PF1–PF10
and PF10 toggles output/shell focus. Row padding is intentional and retained by
the scoped Git whitespace attributes.

Implementation must preserve the native 252/256-byte read contracts and the
198-byte joined PCOMM command limit. A smaller compact command dock cannot
silently shorten either contract or spill editable cells into status/key
fields. The selected Workbench layout is adapted and checked against those
requirements before guest acceptance.

The compatibility adaptation for Workbench uses the model-2 program body at
rows 4–12, position row 13, shell heading/history rows 14–16 and command
heading row 17. Input has an attribute at row 18 column 4, 256 data cells
from row 18 column 5 through row 21 column 20, and a protected closing
attribute at row 21 column 21. Status and key labels retain rows 22–24.
Model 5 retains its two-row 256-cell dock. Continuation rows contain no
intervening field attributes. The compact 156-cell draft remains a design
alternative, with capture required before its field is moved or expanded.
