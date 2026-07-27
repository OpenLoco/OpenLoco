from __future__ import annotations

import argparse
import difflib
import re
import sys
from dataclasses import dataclass, field
from functools import cmp_to_key
from pathlib import Path

# This is also the order expected for each version.
CHANGE_TYPES: list[str] = [
    "Headline feature",
    "Feature",
    "Improved",
    "Change",
    "Removed",
    "Fix",
    "Technical",
]

# A list of change types that were previously used but aren't accepted anymore
CHANGE_TYPE_SUBSTITUTIONS: dict[str, str] = {
    "Improve": "Improved",
}

HEADER_SEPARATOR: str = "-" * 72


@dataclass
class Line:
    text: str
    index: int


@dataclass
class Entry:
    change_type: str
    refs: list[str]
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
    entries: list[Entry] = field(default_factory=list)


@dataclass
class Context:
    lines: list[Line]
    line_index: int = 0
    versions: list[VersionBlock] = field(default_factory=list)


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


def peek_line(ctx: Context) -> Line | None:
    if ctx.line_index >= len(ctx.lines):
        return None
    return ctx.lines[ctx.line_index]


def read_version_header(ctx: Context) -> VersionInfo:
    version_line = consume_line(ctx)
    version_match = re.match(
        r"^((\d+\.\d+(\.\d+)?(\+|-\w+)?) \((\d{4}-\d{2}-\d{2}|\?\?\?)\))$",
        version_line.text,
    )
    if version_match is None:
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
        consume_line(ctx)

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


def read_version_entry(ctx: Context, raw_entry: Line, version_info: VersionInfo) -> Entry:
    text = raw_entry.text
    if not text.startswith("- "):
        report_line_error(raw_entry, "Expected entry prefix '- '")
    text = text[2:]

    change_type_idx = text.find(":")
    if change_type_idx == -1:
        report_line_error(raw_entry, "Expected change type")

    change_type = text[:change_type_idx].strip()
    text = text[change_type_idx + 1:].strip()

    if change_type not in CHANGE_TYPES:
        if change_type in CHANGE_TYPE_SUBSTITUTIONS:
            change_type = CHANGE_TYPE_SUBSTITUTIONS[change_type]
        else:
            types_str = ", ".join(CHANGE_TYPES)
            report_line_error(
                raw_entry,
                f"Invalid change type '{change_type}', types can be {types_str}",
            )

    refs: list[str] = []
    if text.startswith("["):
        refs_end_idx = text.find("]")
        if refs_end_idx == -1:
            report_line_error(
                raw_entry, "Open reference bracket '[' without closing bracket ']'"
            )
        refs = [ref.strip() for ref in text[1:refs_end_idx].split(",")]
        text = text[refs_end_idx + 1:].strip()
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

    return Entry(
        change_type=change_type,
        refs=refs,
        description=text.strip(),
    )


def read_version_entries(ctx: Context, version_info: VersionInfo) -> list[Entry]:
    entries: list[Entry] = []
    while ctx.line_index < len(ctx.lines):
        raw_entry = consume_line(ctx)
        if raw_entry.text == "":
            break
        entries.append(read_version_entry(ctx, raw_entry, version_info))
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


def parse_changelog(changelog: str) -> list[VersionBlock]:
    raw_lines = changelog.split("\n")
    lines = [Line(text=text, index=idx) for idx, text in enumerate(raw_lines)]

    i = 0
    while i < len(lines):
        if lines[i].text == "" and (i == 0 or lines[i - 1].text == ""):
            lines.pop(i)
        else:
            i += 1

    ctx = Context(lines=lines)
    while ctx.line_index < len(ctx.lines):
        ctx.versions.append(read_version_block(ctx))

    return ctx.versions


def reference_sort(a: str, b: str) -> int:
    num_a = int(re.sub(r"\D", "", a) or 0)
    num_b = int(re.sub(r"\D", "", b) or 0)

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
    change_type_diff = CHANGE_TYPES.index(a.change_type) - CHANGE_TYPES.index(b.change_type)
    if change_type_diff != 0:
        return change_type_diff

    has_main_ref_a = len(a.refs) > 0 and a.refs[0].startswith("#")
    has_main_ref_b = len(b.refs) > 0 and b.refs[0].startswith("#")

    if has_main_ref_a and has_main_ref_b:
        num_a = int(re.sub(r"\D", "", a.refs[0]) or 0)
        num_b = int(re.sub(r"\D", "", b.refs[0]) or 0)
        return num_a - num_b
    elif has_main_ref_a and not has_main_ref_b:
        return -1
    elif not has_main_ref_a and has_main_ref_b:
        return 1
    else:
        name_a = a.refs[0].split("#")[0].lower() if a.refs else ""
        name_b = b.refs[0].split("#")[0].lower() if b.refs else ""
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
            bug_idx = desc.find("(original bug")
            entry.description = desc[:bug_idx].strip()

        if not entry.description.endswith("."):
            entry.description += "."

        if original_bug:
            entry.description += " (original bug)"


def cleanup_versions(versions: list[VersionBlock]) -> None:
    for version in versions:
        cleanup_version_block(version)


def format_changelog(versions: list[VersionBlock]) -> str:
    out: list[str] = []
    for version in versions:
        out.append(f"{version.version} ({version.date})")
        out.append(HEADER_SEPARATOR)
        if version.headline != "":
            out.append(version.headline)
            out.append("")
        for entry in version.entries:
            line = f"- {entry.change_type}: "
            if entry.refs:
                line += f"[{', '.join(entry.refs)}] "
            line += entry.description
            out.append(line)
        out.append("")
    return "\n".join(out)


def check_line_endings(changelog: str) -> None:
    if "\r\n" in changelog:
        print(
            "Changelog contains Windows line endings, please convert to Unix line endings.",
            file=sys.stderr,
        )
        sys.exit(-1)


def colorize_diff(diff_lines: list[str]) -> str:
    """Add ANSI colors to diff output if running in an interactive terminal."""
    if not sys.stdout.isatty():
        return "".join(diff_lines)

    RED = "\033[31m"
    GREEN = "\033[32m"
    CYAN = "\033[36m"
    RESET = "\033[0m"

    colored = []
    for line in diff_lines:
        if line.startswith("---") or line.startswith("+++"):
            colored.append(f"{CYAN}{line}{RESET}")
        elif line.startswith("-"):
            colored.append(f"{RED}{line}{RESET}")
        elif line.startswith("+"):
            colored.append(f"{GREEN}{line}{RESET}")
        elif line.startswith("@@"):
            colored.append(f"{CYAN}{line}{RESET}")
        else:
            colored.append(line)
    return "".join(colored)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Check and format CHANGELOG.md files."
    )
    parser.add_argument(
        "file",
        type=Path,
        nargs="?",
        default=Path("CHANGELOG.md"),
        help="Path to changelog file (default: CHANGELOG.md).",
    )
    parser.add_argument(
        "-a",
        "--apply",
        action="store_true",
        help="Apply the formatting changes to CHANGELOG.md.",
    )
    args = parser.parse_args()

    file_path = args.file
    try:
        data = file_path.read_text(encoding="utf-8")
    except OSError as err:
        print(f"Error reading the file: {err}", file=sys.stderr)
        sys.exit(-1)

    check_line_endings(data)
    versions = parse_changelog(data)
    cleanup_versions(versions)
    formatted_data = format_changelog(versions)

    # Mode 1: Overwrite in-place
    if args.apply:
        file_path.write_text(formatted_data, encoding="utf-8")
        print(f"Successfully formatted '{file_path}'.")
        sys.exit(0)

    # Mode 2: Compare and show diff
    if data != formatted_data:
        print("Script ran successfully. Running diff...\n")
        
        diff = list(
            difflib.unified_diff(
                data.splitlines(keepends=True),
                formatted_data.splitlines(keepends=True),
                fromfile=f"a/{file_path}",
                tofile=f"b/{file_path}",
            )
        )
        print(colorize_diff(diff), end="")
        print("\nDifferences found. You can apply the changes locally with:")
        print(f"  python3 {sys.argv[0]} {file_path} --apply")
        sys.exit(1)

    print("No differences found.")
    sys.exit(0)


if __name__ == "__main__":
    main()