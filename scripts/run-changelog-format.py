#!/usr/bin/env python3

"""A wrapper script around clang-format, suitable for linting multiple files
and to use for continuous integration.

This is an alternative API for the clang-format command line.
It runs over multiple files and directories in parallel.
A diff output is produced and a sensible exit code is returned.

"""

from __future__ import print_function, unicode_literals

import argparse
import codecs
import difflib
import fnmatch
import io
import multiprocessing
import os
import re
import signal
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
import subprocess
import sys
import traceback

from functools import partial

DEFAULT_EXTENSIONS = 'c,h,C,H,cpp,hpp,cc,hh,c++,h++,cxx,hxx'


class ExitStatus:
    SUCCESS = 0
    DIFF = 1
    TROUBLE = 2


def list_files(files, recursive=False, extensions=None, exclude=None):
    if extensions is None:
        extensions = []
    if exclude is None:
        exclude = []

    out = []
    for file in files:
        if recursive and os.path.isdir(file):
            for dirpath, dnames, fnames in os.walk(file):
                fpaths = [os.path.join(dirpath, fname) for fname in fnames]
                for pattern in exclude:
                    # os.walk() supports trimming down the dnames list
                    # by modifying it in-place,
                    # to avoid unnecessary directory listings.
                    dnames[:] = [
                        x for x in dnames
                        if
                        not fnmatch.fnmatch(os.path.join(dirpath, x), pattern)
                    ]
                    fpaths = [
                        x for x in fpaths if not fnmatch.fnmatch(x, pattern)
                    ]
                for f in fpaths:
                    ext = os.path.splitext(f)[1][1:]
                    if ext in extensions:
                        out.append(f)
        else:
            out.append(file)
    return out


def make_diff(file, original, reformatted):
    return list(
        difflib.unified_diff(
            original,
            reformatted,
            fromfile='{}\t(original)'.format(file),
            tofile='{}\t(reformatted)'.format(file),
            n=3))


class DiffError(Exception):
    def __init__(self, message, errs=None):
        super(DiffError, self).__init__(message)
        self.errs = errs or []


class UnexpectedError(Exception):
    def __init__(self, message, exc=None):
        super(UnexpectedError, self).__init__(message)
        self.formatted_traceback = traceback.format_exc()
        self.exc = exc

def run_clang_format_version(args):
    invocation = [args.clang_format_executable, "--version"]

    encoding_py3 = {}
    if sys.version_info[0] >= 3:
        encoding_py3['encoding'] = 'utf-8'

    try:
	    proc = subprocess.Popen(
            invocation,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            **encoding_py3)
    except OSError as exc:
        raise DiffError(str(exc))
    proc_stdout = proc.stdout
    proc_stderr = proc.stderr
    if sys.version_info[0] < 3:
        # make the pipes compatible with Python 3,
        # reading lines should output unicode
        encoding = 'utf-8'
        proc_stdout = codecs.getreader(encoding)(proc_stdout)
        proc_stderr = codecs.getreader(encoding)(proc_stderr)
    # hopefully the stderr pipe won't get full and block the process
    outs = list(proc_stdout.readlines())
    errs = list(proc_stderr.readlines())
    proc.wait()
    if proc.returncode:
        raise DiffError("clang-format exited with status {}: '{}'".format(
            proc.returncode, file), errs)
    print(outs[0])
    return

def run_clang_format_diff_wrapper(args, file):
    try:
        ret = run_clang_format_diff(args, file)
        return ret
    except DiffError:
        raise
    except Exception as e:
        raise UnexpectedError('{}: {}: {}'.format(file, e.__class__.__name__,
                                                  e), e)


def run_clang_format_diff(args, file):
    try:
        with io.open(file, 'r', encoding='utf-8') as f:
            original = f.readlines()
    except IOError as exc:
        raise DiffError(str(exc))
    invocation = [args.clang_format_executable, file]

    # Use of utf-8 to decode the process output.
    #
    # Hopefully, this is the correct thing to do.
    #
    # It's done due to the following assumptions (which may be incorrect):
    # - clang-format will returns the bytes read from the files as-is,
    #   without conversion, and it is already assumed that the files use utf-8.
    # - if the diagnostics were internationalized, they would use utf-8:
    #   > Adding Translations to Clang
    #   >
    #   > Not possible yet!
    #   > Diagnostic strings should be written in UTF-8,
    #   > the client can translate to the relevant code page if needed.
    #   > Each translation completely replaces the format string
    #   > for the diagnostic.
    #   > -- http://clang.llvm.org/docs/InternalsManual.html#internals-diag-translation
    #
    # It's not pretty, due to Python 2 & 3 compatibility.
    encoding_py3 = {}
    if sys.version_info[0] >= 3:
        encoding_py3['encoding'] = 'utf-8'

    try:
        proc = subprocess.Popen(
            invocation,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            **encoding_py3)
    except OSError as exc:
        raise DiffError(str(exc))
    proc_stdout = proc.stdout
    proc_stderr = proc.stderr
    if sys.version_info[0] < 3:
        # make the pipes compatible with Python 3,
        # reading lines should output unicode
        encoding = 'utf-8'
        proc_stdout = codecs.getreader(encoding)(proc_stdout)
        proc_stderr = codecs.getreader(encoding)(proc_stderr)
    # hopefully the stderr pipe won't get full and block the process
    outs = list(proc_stdout.readlines())
    errs = list(proc_stderr.readlines())
    proc.wait()
    if proc.returncode:
        raise DiffError("clang-format exited with status {}: '{}'".format(
            proc.returncode, file), errs)
    return make_diff(file, original, outs), errs


def bold_red(s):
    return '\x1b[1m\x1b[31m' + s + '\x1b[0m'


def colorize(diff_lines):
    def bold(s):
        return '\x1b[1m' + s + '\x1b[0m'

    def cyan(s):
        return '\x1b[36m' + s + '\x1b[0m'

    def green(s):
        return '\x1b[32m' + s + '\x1b[0m'

    def red(s):
        return '\x1b[31m' + s + '\x1b[0m'

    for line in diff_lines:
        if line[:4] in ['--- ', '+++ ']:
            yield bold(line)
        elif line.startswith('@@ '):
            yield cyan(line)
        elif line.startswith('+'):
            yield green(line)
        elif line.startswith('-'):
            yield red(line)
        else:
            yield line


def print_diff(diff_lines, use_color):
    if use_color:
        diff_lines = colorize(diff_lines)
    if sys.version_info[0] < 3:
        sys.stdout.writelines((l.encode('utf-8') for l in diff_lines))
    else:
        sys.stdout.writelines(diff_lines)


def print_trouble(prog, message, use_colors):
    error_text = 'error:'
    if use_colors:
        error_text = bold_red(error_text)
    print("{}: {} {}".format(prog, error_text, message), file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        '--clang-format-executable',
        metavar='EXECUTABLE',
        help='path to the clang-format executable',
        default='clang-format')
    parser.add_argument(
        '--extensions',
        help='comma separated list of file extensions (default: {})'.format(
            DEFAULT_EXTENSIONS),
        default=DEFAULT_EXTENSIONS)
    parser.add_argument(
        '-r',
        '--recursive',
        action='store_true',
        help='run recursively over directories')
    parser.add_argument('files', metavar='file', nargs='+')
    parser.add_argument(
        '-q',
        '--quiet',
        action='store_true')
    parser.add_argument(
        '-j',
        metavar='N',
        type=int,
        default=0,
        help='run N clang-format jobs in parallel'
        ' (default number of cpus + 1)')
    parser.add_argument(
        '--color',
        default='auto',
        choices=['auto', 'always', 'never'],
        help='show colored diff (default: auto)')
    parser.add_argument(
        '-e',
        '--exclude',
        metavar='PATTERN',
        action='append',
        default=[],
        help='exclude paths matching the given glob-like pattern(s)'
        ' from recursive search')

    args = parser.parse_args()

    run_clang_format_version(args)

    # use default signal handling, like diff return SIGINT value on ^C
    # https://bugs.python.org/issue14229#msg156446
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    try:
        signal.SIGPIPE
    except AttributeError:
        # compatibility, SIGPIPE does not exist on Windows
        pass
    else:
        signal.signal(signal.SIGPIPE, signal.SIG_DFL)

    colored_stdout = False
    colored_stderr = False
    if args.color == 'always':
        colored_stdout = True
        colored_stderr = True
    elif args.color == 'auto':
        colored_stdout = sys.stdout.isatty()
        colored_stderr = sys.stderr.isatty()

    retcode = ExitStatus.SUCCESS
    files = list_files(
        args.files,
        recursive=args.recursive,
        exclude=args.exclude,
        extensions=args.extensions.split(','))

    if not files:
        return

    njobs = args.j
    if njobs == 0:
        njobs = multiprocessing.cpu_count() + 1
    njobs = min(len(files), njobs)

    if njobs == 1:
        # execute directly instead of in a pool,
        # less overhead, simpler stacktraces
        it = (run_clang_format_diff_wrapper(args, file) for file in files)
        pool = None
    else:
        pool = multiprocessing.Pool(njobs)
        it = pool.imap_unordered(
            partial(run_clang_format_diff_wrapper, args), files)
    while True:
        try:
            outs, errs = next(it)
        except StopIteration:
            break
        except DiffError as e:
            print_trouble(parser.prog, str(e), use_colors=colored_stderr)
            retcode = ExitStatus.TROUBLE
            sys.stderr.writelines(e.errs)
        except UnexpectedError as e:
            print_trouble(parser.prog, str(e), use_colors=colored_stderr)
            sys.stderr.write(e.formatted_traceback)
            retcode = ExitStatus.TROUBLE
            # stop at the first unexpected error,
            # something could be very wrong,
            # don't process all files unnecessarily
            if pool:
                pool.terminate()
            break
        else:
            sys.stderr.writelines(errs)
            if outs == []:
                continue
            if not args.quiet:
                print_diff(outs, use_color=colored_stdout)
            if retcode == ExitStatus.SUCCESS:
                retcode = ExitStatus.DIFF
    return retcode


if __name__ == '__main__':
    sys.exit(main())
