import zipfile
import xml.etree.ElementTree as ET

DST = r"d:\Dev\TinyControlBoard\docs\TinyControlBoard-Documentation-Professional.docx"
ns = "{http://schemas.openxmlformats.org/wordprocessingml/2006/main}"

with zipfile.ZipFile(DST) as z:
    with z.open("word/document.xml") as f:
        tree = ET.parse(f)

root = tree.getroot()
body = root.find(".//" + ns + "body")
paragraphs = []
for para in body.findall(".//" + ns + "p"):
    style_elem = para.find(".//" + ns + "pStyle")
    style = style_elem.get(ns + "val") if style_elem is not None else ""
    texts = [r.text or "" for r in para.findall(".//" + ns + "t")]
    text = "".join(texts)
    paragraphs.append((style, text))

print(f"Total paragraphs: {len(paragraphs)}")
print()

# Heading checks
checks = [
    ("Cover title (Title style)", "TinyControlBoard Documentation", "Title"),
    ("Cover subtitle (Subtitle style)", "Consolidated engineering reference", "Subtitle"),
    ("Appendix heading (Heading1)", "Appendix A", "Heading1"),
    ("Power button (Heading4)", "Power button (index 0):", "Heading4"),
    ("Toggle buttons (Heading4)", "Toggle buttons (Cover", "Heading4"),
    ("Rotary notes (Heading4)", "Rotary (indices 13/14):", "Heading4"),
    ("SPI LED notes (Heading4)", "SPI LED notes:", "Heading4"),
    ("Actions UART (Heading4)", "Actions that send UART", "Heading4"),
    ("Actions local (Heading4)", "Actions that stay local", "Heading4"),
    ("Repository Layout (Heading3)", "Repository Layout", "Heading3"),
    ("Add Meter Configurations (Heading3)", "Add Meter Configurations", "Heading3"),
]

all_ok = True
for label, search, expected in checks:
    match = next(((s, t) for s, t in paragraphs if search in t), None)
    if match:
        ok = expected in match[0]
        status = "PASS" if ok else "FAIL"
        if not ok:
            all_ok = False
        print(f"  {status}  {label}: style=[{match[0]}]")
    else:
        print(f"  MISS  {label}: not found")
        all_ok = False

print()
print("Checking for remaining typos / bad text...")
BAD = ["originaly", "UAart5Listener", "fcopy", "PROJECT DOCUMENTATION PACK", "Generated 2026"]
found_bad = False
for s, t in paragraphs:
    for b in BAD:
        if b in t:
            print(f"  BAD: [{s}] {t[:120]}")
            all_ok = False
            found_bad = True
if not found_bad:
    print("  None found - typos clean")

print()
print("RESULT:", "ALL CHECKS PASSED" if all_ok else "SOME ISSUES REMAIN")
