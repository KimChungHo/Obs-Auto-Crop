"""Restrict a synthesized macOS installer distribution to the user's home."""

import sys
import xml.etree.ElementTree as ET


distribution_path = sys.argv[1]
tree = ET.parse(distribution_path)
root = tree.getroot()
if root.tag != "installer-gui-script":
    raise ValueError("Unexpected productbuild distribution format")

domains = root.find("domains")
if domains is None:
    domains = ET.Element("domains")
    options = root.find("options")
    insert_at = list(root).index(options) + 1 if options is not None else 0
    root.insert(insert_at, domains)

domains.attrib.update(
    enable_anywhere="false",
    enable_currentUserHome="true",
    enable_localSystem="false",
)
tree.write(distribution_path, encoding="utf-8", xml_declaration=True)
