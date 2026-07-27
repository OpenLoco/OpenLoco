import re
import sys
from dataclasses import dataclass, field
from functools import cmp_to_key
from pathlib import Path
from typing import Dict, List, Optional

# Order expected for each version block
CHANGE_TYPES: List[str] = [
    "Headline feature",
    "Feature",
    "Improved",
    "Change",
    "Removed",
    "Fix",
    "Technical",
]

# Map deprecated change types to modern equivalents
CHANGE_TYPE_SUBSTITUTIONS: Dict[str, str] = {
    "Improve": "Improved",
}

HEADER_SEPARATOR: str = (
    "------------------------------------------------------------------------"
)


@dataclass
class Line:
    text: str
    index: int


@dataclass
class Entry:
    change_type: str
    refs: List[str]
    description: str


@dataclass
class VersionInfo:
    version: str
    date: str
    headline: str


@dataclass
class VersionBlock:
    version: str
    date: str
    headline: str
    entries: List[Entry] = field(default_factory=list)


@dataclass
class Context:
    lines: List[Line]
    line_index: int = 0
    versions: List[VersionBlock] = field(default_factory=list)


def report_error(ctx: Context, message: str) -> None:
    print(f"Error on line {ctx.line_index}: {message}", file=sys.stderr)
    sys.exit(-1)


def report_line_error(line: Line, message: str) -> None:
    print(f"Error on line {line.index + 1}: {message}", file=sys.stderr)
    sys.exit(-1)


def consume_line(ctx: Context) -> Line:
    if ctx.line_index >= len(ctx.lines):
        report_error(ctx, "Unexpected end of file")
    line = ctx.lines[ctx.line_index]
    ctx.line_index += 1
    return line


def peek_line(ctx: Context) -> Optional[Line]:
    if ctx.line_index >= len(ctx.lines):
        return None
    return ctx.lines[ctx.line_index]


def read_version_header(ctx: Context) -> VersionInfo:
    version_line = consume_line(ctx)
    pattern = re.compile(
        r"^((\d+\.\d+(\.\d+)?(\+|-\w+)?) \((\d{4}-\d{2}-\d{2}|\?\?\?)\))$"
    )
    version_match = pattern.match(version_line.text)

    if not version_match:
        report_line_error(version_line, "Expected a version number")

    version_separator = consume_line(ctx)
    if version_separator.text != HEADER_SEPARATOR:
        report_line_error(version_separator, "Expected version seperator")

    headline_line = peek_line(ctx)
    if (
        headline_line is not None
        and headline_line.text != ""
        and not headline_line.text.startswith("-")
    ):
        headline_line = consume_line(ctx)

        empty_line = peek_line(ctx)
        if empty_line is None or empty_line.text != "":
            report_error(ctx, "Expected empty line after headline")

        consume_line(ctx)
        headline = headline_line.text
    else:
        headline = ""

    return VersionInfo(
        version=version_match.group(2),
        date=version_match.group(5),
        headline=headline,
    )


def is_valid_reference(ref: str) -> bool:
    return bool(re.match(r"^\w*#\d+( \(partial\))?$", ref))


def read_version_entry(
    ctx: Context, raw_entry: Line, version_info: VersionInfo
) -> Entry:
    text = raw_entry.text
    if not text.startswith("- "):
        report_line_error(raw_entry, "Expected entry prefix '- '")

    text = text[2:]

    change_type_idx = text.find(":")
    if change_type_idx == -1:
        report_line_error(raw_entry, "Expected change type")

    change_type = text[:change_type_idx].strip()
    text = text[change_type_idx + 1 :].strip()

    if change_type not in CHANGE_TYPES:
        if change_type in CHANGE_TYPE_SUBSTITUTIONS:
            change_type = CHANGE_TYPE_SUBSTITUTIONS[change_type]
        else:
            types_str = ", ".join(CHANGE_TYPES)
            report_line_error(
                raw_entry,
                f"Invalid change type '{change_type}', types can be {types_str}",
            )

    refs: List[str] = []
    if text.startswith("["):
        refs_end_idx = text.find("]")
        if refs_end_idx == -1:
            report_line_error(
                raw_entry, "Open reference bracket '[' without closing bracket ']'"
            )
        refs = [ref.strip() for ref in text[1:refs_end_idx].split(",")]
        text = text[refs_end_idx + 1 :].strip()
    else:
        if version_info.date == "???":
            report_line_error(raw_entry, "Expected reference [#REF]")

    if len(text) == 0:
        report_line_error(raw_entry, "Expected description text")

    if text[0] in ("-", ":"):
        report_line_error(
            raw_entry,
            f"Description must start with text, '{text[0]}' is not allowed",
        )

    for ref in refs:
        if not is_valid_reference(ref):
            report_line_error(
                raw_entry,
                f"Invalid reference '{ref}', must be '#123' or 'project#123'",
            )

    return Entry(change_type=change_type, refs=refs, description=text.strip())


def read_version_entries(
    ctx: Context, version_info: VersionInfo
) -> List[Entry]:
    entries: List[Entry] = []
    while ctx.line_index < len(ctx.lines):
        raw_entry = consume_line(ctx)
        if raw_entry.text == "":
            break
        entry = read_version_entry(ctx, raw_entry, version_info)
        entries.append(entry)
    return entries


def read_version_block(ctx: Context) -> VersionBlock:
    version_info = read_version_header(ctx)
    entries = read_version_entries(ctx, version_info)
    return VersionBlock(
        version=version_info.version,
        date=version_info.date,
        headline=version_info.headline,
        entries=entries,
    )


def parse_changelog(changelog: str) -> List[VersionBlock]:
    raw_lines = changelog.split("\n")
    lines: List[Line] = []

    for idx, text in enumerate(raw_lines):
        if text == "" and (len(lines) == 0 or lines[-1].text == ""):
            continue
        lines.append(Line(text=text, index=idx))

    ctx = Context(lines=lines)

    while ctx.line_index < len(ctx.lines):
        version_block = read_version_block(ctx)
        ctx.versions.append(version_block)

    return ctx.versions


def extract_num(ref: str) -> int:
    digits = re.sub(r"\D", "", ref)
    return int(digits) if digits else 0


def reference_sort(a: str, b: str) -> int:
    num_a = extract_num(a)
    num_b = extract_num(b)

    is_direct_a = a.startswith("#")
    is_direct_b = b.startswith("#")

    if is_direct_a and is_direct_b:
        return num_a - num_b
    elif not is_direct_a and not is_direct_b:
        name_a = a.split("#")[0].lower()
        name_b = b.split("#")[0].lower()
        if name_a < name_b:
            return -1
        if name_a > name_b:
            return 1
        return num_a - num_b
    else:
        return -1 if is_direct_a else 1


def reorder_groups(a: Entry, b: Entry) -> int:
    idx_a = CHANGE_TYPES.index(a.change_type)
    idx_b = CHANGE_TYPES.index(b.change_type)
    change_type_diff = idx_a - idx_b

    if change_type_diff != 0:
        return change_type_diff

    has_main_ref_a = len(a.refs) > 0 and a.refs[0].startswith("#")
    has_main_ref_b = len(b.refs) > 0 and b.refs[0].startswith("#")

    if has_main_ref_a and has_main_ref_b:
        num_a = extract_num(a.refs[0])
        num_b = extract_num(b.refs[0])
        return num_a - num_b
    elif has_main_ref_a and not has_main_ref_b:
        return -1
    elif not has_main_ref_a and has_main_ref_b:
        return 1
    else:
        name_a = (a.refs[0].split("#")[0] if len(a.refs) > 0 else "").lower()
        name_b = (b.refs[0].split("#")[0] if len(b.refs) > 0 else "").lower()
        if name_a < name_b:
            return -1
        if name_a > name_b:
            return 1
        return 0


def cleanup_version_block(version: VersionBlock) -> None:
    for entry in version.entries:
        entry.refs.sort(key=cmp_to_key(reference_sort))

    version.entries.sort(key=cmp_to_key(reorder_groups))

    for entry in version.entries:
        desc = entry.description
        original_bug = False

        if desc.endswith("(original bug)"):
            original_bug = True
            bug_idx = desc.rfind("(original bug)")
            entry.description = desc[:bug_idx].strip()

        if not entry.description.endswith("."):
            entry.description += "."

        if original_bug:
            entry.description += " (original bug)"


def cleanup_versions(versions: List[VersionBlock]) -> None:
    for version in versions:
        cleanup_version_block(version)


def print_changelog(versions: List[VersionBlock]) -> None:
    for version in versions:
        print(f"{version.version} ({version.date})")
        print(HEADER_SEPARATOR)
        if version.headline != "":
            print(version.headline)
            print()
        for entry in version.entries:
            line = f"- {entry.change_type}: "
            if entry.refs:
                line += f"[{', '.join(entry.refs)}] "
            line += entry.description
            print(line)
        print()


def check_line_endings(changelog: str) -> None:
    if "\r\n" in changelog:
        print(
            "Changelog contains Windows line endings, please convert to Unix line endings.",
            file=sys.stderr,
        )
        sys.exit(-1)


def main() -> None:
    if len(sys.argv) < 2:
        print("Usage: python format-changelog.py <path-to-changelog>", file=sys.stderr)
        sys.exit(-1)

    file_path = Path(sys.argv[1])

    try:
        data = file_path.read_text(encoding="utf-8")
    except Exception as err:
        print(f"Error reading the file: {err}", file=sys.stderr)
        sys.exit(-1)

    check_line_endings(data)
    versions = parse_changelog(data)
    cleanup_versions(versions)
    print_changelog(versions)


if __name__ == "__main__":
    main()