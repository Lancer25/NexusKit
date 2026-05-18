"""Check that public entities in nexus headers have Doxygen /// comments."""

import argparse
import re
import sys
from pathlib import Path

TARGET_HEADERS = [
    "include/nexus/core/status.h",
    "include/nexus/core/result.h",
    "include/nexus/core/version.h",
    "include/nexus/core/export.h",
    "include/nexus/common/string.h",
    "include/nexus/common/time.h",
    "include/nexus/common/binary.h",
    "include/nexus/common/platform.h",
    "include/nexus/common/math.h",
    "include/nexus/common/logging.h",
    "include/nexus/common/status.h",
    "include/nexus/common/json.h",
    "include/nexus/common/xml.h",
    "include/nexus/media/media.h",
    "include/nexus/screen/screen.h",
    "include/nexus/net/export.h",
    "include/nexus/net/http.h",
    "include/nexus/net/tcp.h",
    "include/nexus/net/udp.h",
    "include/nexus/net/websocket_client.h",
    "include/nexus/usb/export.h",
    "include/nexus/usb/usb.h",
    "include/nexus/hid/export.h",
    "include/nexus/hid/hid.h",
    "include/nexus/log/export.h",
    "include/nexus/log/logger.h",
]

DOXY_RE = re.compile(r"^\s*///")

# --- entity patterns (name captured in group 1) ---

ENUM_RE = re.compile(
    r"^\s*(?:enum\s+class|enum)\s+(\w+)")

CLASS_RE = re.compile(
    r"^\s*(?:class|struct)\s+(?:NEXUS_\w+_API\s+)?(\w+)(?:\s*:\s*public\s+\w+|)(?:\s*\{|;)")

CONSTEXPR_RE = re.compile(
    r"^\s*constexpr\s+\S+\s+(\w+)\s*=")

FREE_FUNC_RE = re.compile(
    r"^\s*(?:NEXUS_\w+_API\s+)?"
    r"(?:[-_a-zA-Z0-9:]+(?:<[^>]*>)?(?:\s*[*&])?)\s+"
    r"(\w+)\s*\([^)]*\)\s*(?:const\s*)?\s*(?:&{0,2}\s*)?\s*"
    r"(?:noexcept\s*)?\s*(?:override\s*)?(?:final\s*)?\s*;")

STATIC_FACTORY_RE = re.compile(
    r"^\s*static\s+[\w:]+(?:<[^>]*>)?\s+(\w+)\s*\(.*\)\s*(?:;|\{)")

PUBLIC_METHOD_RE = re.compile(
    r"^\s*(?:(?:virtual|explicit|static|constexpr)\s+)*"
    r"(?:[-_a-zA-Z0-9:]+(?:<[^>]*>)?(?:\s*[*&])?)\s+"
    r"(\w+)\s*\([^)]*\)\s*(?:const\s*)?\s*(?:&{0,2}\s*)?\s*"
    r"(?:noexcept\s*)?\s*(?:override\s*)?(?:final\s*)?\s*(?:;|\{|=\s*default|\{\s*return)")

MACRO_RE = re.compile(r"^\s*#define\s+(NEXUS_\w+)")


def _is_constructor_like(name, enclosing_class):
    """Return True when `name` matches the enclosing class (ctor/dtor)."""
    if not enclosing_class:
        return False
    if name == enclosing_class:
        return True
    if name == f"~{enclosing_class}":
        return True
    return False


def _is_operator(name):
    return name.startswith("operator")


# Lines to skip when scanning backward for /// comments
SKIP_BACK_RE = re.compile(
    r"^\s*(?:"
    r"//|/\*|\*|#|"
    r"namespace|using|"
    r"public:|private:|protected:|"
    r"template\s*<|"
    r"(?:friend|explicit)\s|"
    r"\)|"
    r"$"
    r")")


def has_doxygen(lines, decl_line_idx):
    """Check if there is a /// comment block immediately before lines[decl_line_idx]."""
    for i in range(decl_line_idx - 1, max(decl_line_idx - 20, -1), -1):
        line = lines[i].rstrip()
        stripped = line.strip()
        if DOXY_RE.match(line):
            return True
        if SKIP_BACK_RE.match(stripped) and not stripped.startswith("///"):
            continue
        # Stop at non-skippable content
        break
    return False


def check_header(filepath):
    path = Path(filepath)
    lines = path.read_text(encoding="utf-8").splitlines()

    undocumented = []
    in_class = False
    class_name = None
    in_public = False
    brace_depth = 0
    is_struct = False

    for idx, raw in enumerate(lines):
        line = raw.rstrip()
        stripped = line.strip()

        # Track class/struct scope
        class_match = CLASS_RE.match(stripped)
        if class_match:
            name = class_match.group(1)
            is_struct_type = stripped.lstrip().startswith("struct ")
            if "{" in stripped or (idx + 1 < len(lines) and lines[idx + 1].strip() == "{"):
                if not has_doxygen(lines, idx):
                    undocumented.append(name)
                in_class = True
                in_public = True  # structs default to public
                class_name = name
                brace_depth = 0
                is_struct = is_struct_type
            continue

        if stripped == "{":
            if in_class:
                brace_depth += 1
            continue

        if stripped == "}" or stripped == "};":
            if in_class:
                brace_depth -= 1
                if brace_depth < 0:
                    in_class = False
                    class_name = None
                    in_public = False
                    brace_depth = 0
                    is_struct = False
            continue

        if in_class:
            if re.match(r"^\s*public\s*:", line):
                in_public = True
                # Once we see `public:` in a class, it's not a plain struct
                is_struct = False
                continue
            if re.match(r"^\s*(?:private|protected)\s*:", line):
                in_public = False
                is_struct = False
                continue

        # Struct fields (in_public within a struct)
        if is_struct and in_public and not in_class:
            # Already reset — shouldn't happen. Struct was closed.
            pass

        if is_struct and in_public:
            # Check for field-like declarations: type name; or type name = value;
            field_m = re.match(
                r"^\s*[\w:]+(?:<[^>]*>)?\s+(\w+)\s*(?:=\s*[^;]+)?\s*;", stripped)
            if field_m:
                fname = field_m.group(1)
                if not has_doxygen(lines, idx):
                    undocumented.append(fname)
                continue

        # Check for public entities
        entity_name = None

        # Enums
        m = ENUM_RE.match(stripped)
        if m:
            entity_name = m.group(1)
        # Free functions (namespace scope only)
        elif not in_class:
            m = FREE_FUNC_RE.match(stripped)
            if m:
                entity_name = m.group(1)
            else:
                m = MACRO_RE.match(stripped)
                if m:
                    entity_name = m.group(1)
        # Constexpr
        elif not in_class:
            m = CONSTEXPR_RE.match(stripped)
            if m:
                entity_name = m.group(1)

        # In-class public methods
        if not entity_name and in_class and in_public:
            m = STATIC_FACTORY_RE.match(stripped)
            if m:
                entity_name = m.group(1)
            else:
                m = PUBLIC_METHOD_RE.match(stripped)
                if m:
                    entity_name = m.group(1)

        if not entity_name:
            continue

        # Skip detail, constructors, operators
        if entity_name == "detail":
            continue
        if _is_constructor_like(entity_name, class_name):
            continue
        if _is_operator(entity_name):
            continue

        if not has_doxygen(lines, idx):
            undocumented.append(entity_name)

    return undocumented


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dir", default=".")
    args = parser.parse_args()

    root = Path(args.dir)
    all_undoc = {}

    for hdr in TARGET_HEADERS:
        fp = root / hdr
        if not fp.exists():
            continue
        missing = check_header(str(fp))
        if missing:
            all_undoc[hdr] = missing

    if all_undoc:
        print(f"\n{len(all_undoc)} header(s) with undocumented entities:\n")
        for hdr, names in all_undoc.items():
            print(f"  {hdr}:")
            for n in names:
                print(f"    - {n}")
        sys.exit(1)
    else:
        print("All public entities documented.")
        sys.exit(0)


if __name__ == "__main__":
    main()
