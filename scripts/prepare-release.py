#!/usr/bin/env python3
import argparse
import os
import re
import sys
import subprocess
from datetime import datetime

# -----------------------------------------------------------------
# 24-bit True Color Palette (Optimized for Dark Terminals)
# -----------------------------------------------------------------
CLR_RESET   = "\033[0m"
CLR_BOLD    = "\033[1m"

# Semantic True Colors (RGB)
CLR_RED     = "\033[38;2;255;100;100m"  # Soft Coral Red (Errors / Failures / Deletions)
CLR_GREEN   = "\033[38;2;120;220;140m"  # Pastel Sage Green (Success / Executed / Additions)
CLR_YELLOW  = "\033[38;2;255;210;100m"  # Warm Amber (Warnings / Dry-run / Info)
CLR_BLUE    = "\033[38;2;100;180;255m"  # Light Sky Blue (Sub-sections / Labels)
CLR_STEEL   = "\033[38;2;74;128;168m"   # Darker Steel/Slate Blue (Step Banners & Separators)
CLR_CYAN    = "\033[38;2;110;225;245m"  # Soft Electric Cyan (Highlights)
CLR_MAGENTA = "\033[38;2;180;140;255m"  # Soft Lavender/Purple (Hunk Headers @@)

def format_diff_line(line):
    """Applies standard git diff syntax highlighting to unified diff lines."""
    if line.startswith("@@"):
        return f"{CLR_MAGENTA}{line}{CLR_RESET}"
    elif line.startswith("+") and not line.startswith("+++"):
        return f"{CLR_GREEN}{line}{CLR_RESET}"
    elif line.startswith("-") and not line.startswith("---"):
        return f"{CLR_RED}{line}{CLR_RESET}"
    elif line.startswith("+++") or line.startswith("---"):
        return f"{CLR_BOLD}{CLR_BLUE}{line}{CLR_RESET}"
    return line

def show_diff(filename, old_content, new_content):
    import difflib
    print(f"\n    {CLR_BLUE}>>> PROPOSED DIFF: {filename}{CLR_RESET}")
    diff = list(difflib.unified_diff(
        old_content.splitlines(keepends=True),
        new_content.splitlines(keepends=True),
        fromfile=f"a/{filename}",
        tofile=f"b/{filename}"
    ))
    if diff:
        colored_diff = "".join(format_diff_line(line) for line in diff)
        print(colored_diff, end="")
    else:
        print(f"    {CLR_YELLOW}(No changes detected){CLR_RESET}")

def run_git_cmd(cmd, dry_run=False):
    cmd_str = " ".join(cmd)
    if dry_run:
        print(f"    {CLR_YELLOW}[DRY-RUN EXEC]{CLR_RESET} {cmd_str}")
    else:
        print(f"    {CLR_GREEN}[EXEC]{CLR_RESET} {cmd_str}")
        subprocess.run(cmd, check=True)

def print_step(step_num, title, dry_run=False):
    prefix = f"{CLR_YELLOW}[DRY-RUN]{CLR_RESET}" if dry_run else f"{CLR_GREEN}[EXEC]{CLR_RESET}"
    bar = "=" * 72
    print(f"\n{CLR_STEEL}{bar}{CLR_RESET}")
    print(f" {prefix} {CLR_BOLD}{CLR_STEEL}[Step {step_num}/5]{CLR_RESET} {CLR_BOLD}{title}{CLR_RESET}")
    print(f"{CLR_STEEL}{bar}{CLR_RESET}")

def main():
    # Enable ANSI escape sequence support on Windows natively
    if os.name == 'nt':
        import ctypes
        kernel32 = ctypes.windll.kernel32
        kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)

    parser = argparse.ArgumentParser(description="Prepare and tag a new release.")
    parser.add_argument("version", help="Release version string without 'v' prefix (e.g. 26.07)")
    parser.add_argument(
        "-r", "--remote",
        required=True,
        help="Git remote name to push to (e.g. origin, upstream)."
    )
    parser.add_argument(
        "-n", "--dry-run",
        action="store_true",
        help="Perform a dry run without modifying files, committing, or pushing."
    )
    args = parser.parse_args()

    remote = args.remote
    raw_version = args.version.lstrip('v')   # e.g. "26.07"
    tag_name = f"v{raw_version}"              # e.g. "v26.07"
    bug_report_version = f"{raw_version}"   # e.g. "26.07.1"
    today_str = datetime.now().strftime("%Y-%m-%d")
    dry_run = args.dry_run

    issue_template_path = ".github/ISSUE_TEMPLATE/bug_report.md"
    changelog_path = "CHANGELOG.md"
    cmakelists_path = "CMakeLists.txt"

    print(f"\n🚀 {CLR_BOLD}{CLR_CYAN}STARTING RELEASE PROCESS FOR {tag_name} ({today_str}) -> Remote: {remote}{CLR_RESET}")
    if dry_run:
        print(f"⚠️  {CLR_YELLOW}RUNNING IN DRY-RUN MODE — No files will be modified, no git commands executed.{CLR_RESET}")

    # -----------------------------------------------------------------
    # Step 1: Update CHANGELOG.md & Extract Tag Description
    # -----------------------------------------------------------------
    print_step(1, "Parsing CHANGELOG.md & Updating Release Header", dry_run)
    if not os.path.exists(changelog_path):
        print(f"    ❌ {CLR_RED}Error: {changelog_path} not found.{CLR_RESET}")
        sys.exit(1)

    with open(changelog_path, "r", encoding="utf-8") as f:
        changelog_content = f.read()

    changelog_match = re.search(
        r"^[^\n]+\n(.*?)(?=\n[0-9]+\.[0-9]+|\Z)",
        changelog_content,
        re.MULTILINE | re.DOTALL
    )

    if not changelog_match:
        print(f"    ❌ {CLR_RED}Error: Could not find top section in {changelog_path}{CLR_RESET}")
        sys.exit(1)

    release_notes = changelog_match.group(1).strip()
    release_notes = "\n".join(release_notes.splitlines()[1:])
    
    new_header = f"{raw_version} ({today_str})"
    updated_changelog = re.sub(
        r"^[^\n]+",
        new_header,
        changelog_content,
        count=1,
        flags=re.MULTILINE
    )

    print(f"    ℹ️  {CLR_YELLOW}Extracted {len(release_notes.splitlines())} lines of release notes.{CLR_RESET}")

    if dry_run:
        show_diff(changelog_path, changelog_content, updated_changelog)
        print(f"\n    {CLR_BLUE}>>> EXTRACTED RELEASE NOTES:{CLR_RESET}")
        for line in release_notes.splitlines():
            print(f"        | {line}")
    else:
        with open(changelog_path, "w", encoding="utf-8") as f:
            f.write(updated_changelog)
        print(f"    ✅ {CLR_GREEN}Updated {changelog_path}{CLR_RESET}")

    # -----------------------------------------------------------------
    # Step 2: Update Issue Template & CMakeLists.txt
    # -----------------------------------------------------------------
    print_step(2, "Updating Project Version Files (Issue Template & CMakeLists.txt)", dry_run)

    # Targeted update for bug_report.md matching: - Version: [e.g. 26.07]
    if os.path.exists(issue_template_path):
        with open(issue_template_path, "r", encoding="utf-8") as f:
            template_content = f.read()

        updated_template = re.sub(
            r"(^\s*-\s*Version:\s*\[e\.g\.\s*)[^\]]+(\])",
            rf"\g<1>{bug_report_version}\g<2>",
            template_content,
            flags=re.MULTILINE
        )

        if dry_run:
            show_diff(issue_template_path, template_content, updated_template)
        else:
            with open(issue_template_path, "w", encoding="utf-8") as f:
                f.write(updated_template)
            print(f"    ✅ {CLR_GREEN}Updated {issue_template_path}{CLR_RESET}")

    # CMakeLists.txt
    if os.path.exists(cmakelists_path):
        with open(cmakelists_path, "r", encoding="utf-8") as f:
            cmake_content = f.read()

        updated_cmake = re.sub(
            r"(project\s*\([^)]*VERSION\s+)\d+\.\d+(\.\d+)?",
            rf"\g<1>{raw_version}",
            cmake_content,
            flags=re.IGNORECASE
        )

        if dry_run:
            show_diff(cmakelists_path, cmake_content, updated_cmake)
        else:
            with open(cmakelists_path, "w", encoding="utf-8") as f:
                f.write(updated_cmake)
            print(f"    ✅ {CLR_GREEN}Updated {cmakelists_path}{CLR_RESET}")

    # -----------------------------------------------------------------
    # Step 3: Commit and Tag Release
    # -----------------------------------------------------------------
    print_step(3, f"Creating Commit and Tag '{tag_name}'", dry_run)
    run_git_cmd(["git", "add", issue_template_path, changelog_path, cmakelists_path], dry_run)
    run_git_cmd(["git", "commit", "-m", f"Release {tag_name}"], dry_run)
    run_git_cmd(["git", "tag", "-a", tag_name, "-m", f'"{release_notes}"'], dry_run)

    # -----------------------------------------------------------------
    # Step 4: Push Commit & Tag to Remote
    # -----------------------------------------------------------------
    print_step(4, f"Pushing Release Commit & Tag to '{remote}'", dry_run)
    run_git_cmd(["git", "push", remote, "--follow-tags"], dry_run)

    # -----------------------------------------------------------------
    # Step 5: Prepare Next Development Cycle in CHANGELOG.md
    # -----------------------------------------------------------------
    print_step(5, f"Setting up CHANGELOG.md for Next Cycle ({raw_version}+ (???))", dry_run)
    next_cycle_header = f"{raw_version}+ (???)\n\n"
    base_changelog = updated_changelog if dry_run else open(changelog_path, "r", encoding="utf-8").read()
    new_dev_changelog = next_cycle_header + base_changelog

    if dry_run:
        show_diff(f"{changelog_path} (next dev cycle)", base_changelog, new_dev_changelog)
    else:
        with open(changelog_path, "w", encoding="utf-8") as f:
            f.write(new_dev_changelog)
        print(f"    ✅ {CLR_GREEN}Prepended next cycle header to {changelog_path}{CLR_RESET}")

    run_git_cmd(["git", "add", changelog_path], dry_run)
    run_git_cmd(["git", "commit", "-m", "Prepare for next development cycle [skip ci]"], dry_run)
    run_git_cmd(["git", "push", remote], dry_run)

    # -----------------------------------------------------------------
    # Summary
    # -----------------------------------------------------------------
    bar = "=" * 72
    print(f"\n{CLR_STEEL}{bar}{CLR_RESET}")
    if dry_run:
        print(f"🎉 {CLR_BOLD}{CLR_YELLOW}DRY-RUN COMPLETE: No files were changed and no commits were pushed.{CLR_RESET}")
    else:
        print(f"🎉 {CLR_BOLD}{CLR_GREEN}SUCCESS: Release {tag_name} tagged, pushed to '{remote}', and next cycle prepared!{CLR_RESET}")
    print(f"{CLR_STEEL}{bar}{CLR_RESET}\n")

if __name__ == "__main__":
    main()