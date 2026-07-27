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

CLR_RED     = "\033[38;2;255;100;100m"  # Soft Coral Red (Errors / Deletions)
CLR_GREEN   = "\033[38;2;120;220;140m"  # Pastel Sage Green (Success / Executed)
CLR_YELLOW  = "\033[38;2;255;210;100m"  # Warm Amber (Warnings / Dry-run / Prompts)
CLR_BLUE    = "\033[38;2;100;180;255m"  # Light Sky Blue (Sub-sections / Labels)
CLR_STEEL   = "\033[38;2;74;128;168m"   # Slate Blue (Step Banners)
CLR_CYAN    = "\033[38;2;110;225;245m"  # Soft Electric Cyan (Highlights)
CLR_MAGENTA = "\033[38;2;180;140;255m"  # Soft Lavender (Hunk Headers)

NON_INTERACTIVE = False


def ask_user_choice(prompt_msg, options):
    """
    Prompts the user with a question and a map/list of valid choices.
    Options format: [('y', 'Yes, delete tag'), ('n', 'No, abort release')]
    Returns the chosen key string.
    """
    if NON_INTERACTIVE:
        print(f"    ⚠️  {CLR_YELLOW}[NON-INTERACTIVE] Defaulting to first option: '{options[0][0]}'{CLR_RESET}")
        return options[0][0]

    valid_keys = [opt[0].lower() for opt in options]
    print(f"\n    ❓ {CLR_YELLOW}{CLR_BOLD}{prompt_msg}{CLR_RESET}")
    for key, description in options:
        print(f"       {CLR_CYAN}[{key.upper()}]{CLR_RESET} {description}")

    while True:
        try:
            choice = input(f"    👉 Select choice [{'/'.join(valid_keys)}]: ").strip().lower()
            if choice in valid_keys:
                return choice
            print(f"    ❌ {CLR_RED}Invalid choice '{choice}'. Please select one of: {', '.join(valid_keys)}{CLR_RESET}")
        except (KeyboardInterrupt, EOFError):
            print(f"\n    ❌ {CLR_RED}Aborted by user.{CLR_RESET}")
            sys.exit(1)


def run_cmd(cmd, dry_run=False, capture_output=False, check=True):
    """Runs a shell command with consistent logging and error handling."""
    cmd_str = " ".join(cmd)
    if dry_run:
        print(f"    {CLR_YELLOW}[DRY-RUN EXEC]{CLR_RESET} {cmd_str}")
        return subprocess.CompletedProcess(cmd, 0, stdout="", stderr="")

    print(f"    {CLR_GREEN}[EXEC]{CLR_RESET} {cmd_str}")
    res = subprocess.run(
        cmd,
        text=True,
        stdout=subprocess.PIPE if capture_output else None,
        stderr=subprocess.PIPE if capture_output else None,
        check=False
    )
    if check and res.returncode != 0:
        err_msg = res.stderr.strip() if res.stderr else "Command failed."
        print(f"    ❌ {CLR_RED}Command execution error ({res.returncode}): {err_msg}{CLR_RESET}")
        raise subprocess.CalledProcessError(res.returncode, cmd, output=res.stdout, stderr=res.stderr)
    return res


def format_diff_line(line):
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


def print_step(step_num, total_steps, title, dry_run=False):
    prefix = f"{CLR_YELLOW}[DRY-RUN]{CLR_RESET}" if dry_run else f"{CLR_GREEN}[EXEC]{CLR_RESET}"
    bar = "=" * 72
    print(f"\n{CLR_STEEL}{bar}{CLR_RESET}")
    print(f" {prefix} {CLR_BOLD}{CLR_STEEL}[Step {step_num}/{total_steps}]{CLR_RESET} {CLR_BOLD}{title}{CLR_RESET}")
    print(f"{CLR_STEEL}{bar}{CLR_RESET}")


# -----------------------------------------------------------------
# Pre-flight Checks & Resilience Handlers
# -----------------------------------------------------------------
def check_git_repository():
    """Ensures script is running inside a valid Git repository."""
    res = subprocess.run(["git", "rev-parse", "--is-inside-work-tree"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        print(f"❌ {CLR_RED}Fatal: Current directory is not a Git repository.{CLR_RESET}")
        sys.exit(1)


def check_dirty_working_tree(dry_run):
    """Handles dirty uncommitted working tree changes interactively."""
    status_res = run_cmd(["git", "status", "--porcelain"], dry_run=False, capture_output=True, check=False)
    if status_res.stdout and status_res.stdout.strip():
        print(f"⚠️  {CLR_YELLOW}Uncommitted local changes detected in repository.{CLR_RESET}")
        choice = ask_user_choice(
            "How would you like to handle uncommitted changes?",
            [
                ("s", "Stash changes automatically (`git stash`)"),
                ("c", "Continue anyway (risk committing uncommitted files)"),
                ("a", "Abort release process")
            ]
        )
        if choice == "a":
            sys.exit(1)
        elif choice == "s" and not dry_run:
            run_cmd(["git", "stash", "push", "-m", "Auto-stashed by release script"])


def verify_remote_exists(remote):
    """Verifies that the provided remote target exists."""
    remotes = subprocess.run(["git", "remote"], stdout=subprocess.PIPE, text=True).stdout.splitlines()
    if remote not in remotes:
        print(f"❌ {CLR_RED}Error: Git remote '{remote}' does not exist. Available remotes: {', '.join(remotes)}{CLR_RESET}")
        sys.exit(1)


def check_and_resolve_behind_remote(remote, dry_run):
    """Checks if local branch is behind remote and offers interactive rebase/pull."""
    print(f"    ℹ️  {CLR_BLUE}Fetching from remote '{remote}' to check synchronization state...{CLR_RESET}")
    if dry_run:
        return

    fetch_res = run_cmd(["git", "fetch", remote], capture_output=True, check=False)
    if fetch_res.returncode != 0:
        print(f"⚠️  {CLR_YELLOW}Warning: Could not fetch from remote '{remote}'. Network issue or invalid remote.{CLR_RESET}")
        return

    # Check rev-list count behind
    status = run_cmd(["git", "rev-list", "--count", "HEAD..@{u}"], capture_output=True, check=False)
    if status.returncode == 0 and status.stdout.strip().isdigit():
        behind_count = int(status.stdout.strip())
        if behind_count > 0:
            print(f"⚠️  {CLR_YELLOW}Your local branch is behind '{remote}' by {behind_count} commit(s).{CLR_RESET}")
            choice = ask_user_choice(
                "How would you like to update your local branch?",
                [
                    ("r", "Run `git pull --rebase` (Recommended)"),
                    ("p", "Run standard `git pull`"),
                    ("i", "Ignore and attempt to proceed anyway"),
                    ("a", "Abort script execution")
                ]
            )
            if choice == "a":
                sys.exit(1)
            elif choice == "r":
                run_cmd(["git", "pull", "--rebase", remote])
            elif choice == "p":
                run_cmd(["git", "pull", remote])


def check_and_resolve_tag_conflicts(tag_name, remote, dry_run):
    """Checks for tag collision locally and remotely, offering deletion or replacement."""
    if dry_run:
        return

    # 1. Local Tag Check
    local_tag_exists = subprocess.run(
        ["git", "rev-parse", "-q", "--verify", f"refs/tags/{tag_name}"],
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
    ).returncode == 0

    if local_tag_exists:
        print(f"⚠️  {CLR_YELLOW}Local tag '{tag_name}' already exists!{CLR_RESET}")
        choice = ask_user_choice(
            f"Local tag '{tag_name}' already exists. What should we do?",
            [
                ("d", f"Delete existing local tag '{tag_name}' and re-tag"),
                ("a", "Abort process")
            ]
        )
        if choice == "a":
            sys.exit(1)
        elif choice == "d":
            run_cmd(["git", "tag", "-d", tag_name])

    # 2. Remote Tag Check
    remote_tags = run_cmd(["git", "ls-remote", "--tags", remote, f"refs/tags/{tag_name}"], capture_output=True, check=False).stdout
    if tag_name in remote_tags:
        print(f"⚠️  {CLR_YELLOW}Remote tag '{tag_name}' already exists on '{remote}'!{CLR_RESET}")
        choice = ask_user_choice(
            f"Remote tag '{tag_name}' exists on remote '{remote}'. What should we do?",
            [
                ("d", f"Force-delete tag on remote (`git push {remote} --delete {tag_name}`)"),
                ("a", "Abort process")
            ]
        )
        if choice == "a":
            sys.exit(1)
        elif choice == "d":
            run_cmd(["git", "push", "--delete", remote, tag_name])


def push_with_retry(remote, tag_name, dry_run):
    """Executes git push and handles push rejection interactively."""
    while True:
        try:
            run_git_cmd(["git", "push", remote, "--follow-tags"], dry_run)
            break
        except subprocess.CalledProcessError as e:
            if dry_run:
                break
            print(f"❌ {CLR_RED}Git push rejected or failed!{CLR_RESET}")
            choice = ask_user_choice(
                "Push failed. How would you like to resolve this?",
                [
                    ("r", f"Rebase from '{remote}' (`git pull --rebase {remote}`) and retry push"),
                    ("f", "Force push (DANGER: `--force-with-lease`)"),
                    ("a", "Abort script")
                ]
            )
            if choice == "a":
                sys.exit(1)
            elif choice == "r":
                run_cmd(["git", "pull", "--rebase", remote])
            elif choice == "f":
                run_git_cmd(["git", "push", remote, "--follow-tags", "--force-with-lease"], dry_run)
                break


def run_git_cmd(cmd, dry_run=False):
    return run_cmd(cmd, dry_run=dry_run, check=True)


# -----------------------------------------------------------------
# Main Entry Point
# -----------------------------------------------------------------
def main():
    global NON_INTERACTIVE

    if os.name == 'nt':
        import ctypes
        kernel32 = ctypes.windll.kernel32
        kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)

    parser = argparse.ArgumentParser(description="Prepare, harden, and tag a new release.")
    parser.add_argument("tag", help="Release tag name (e.g. v99.99 or 99.99)")
    parser.add_argument("-r", "--remote", required=True, help="Git remote name (e.g. origin, upstream).")
    parser.add_argument("-n", "--dry-run", action="store_true", help="Perform a dry run without modifying files.")
    parser.add_argument("-y", "--non-interactive", action="store_true", help="Non-interactive mode (auto-accept defaults).")

    args = parser.parse_args()

    NON_INTERACTIVE = args.non_interactive
    remote = args.remote
    input_tag = args.tag.strip()

    # Parse version number out of input tag (extracts numeric string like "99.99" from "v99.99" or "99.99")
    version_match = re.search(r"(\d+\.\d+(?:\.\d+)?)", input_tag)
    if not version_match:
        print(f"❌ {CLR_RED}Error: Could not parse a valid version number from tag input '{input_tag}'. (Expected format e.g. v99.99 or 99.99){CLR_RESET}")
        sys.exit(1)

    raw_version = version_match.group(1)
    tag_name = f"v{raw_version}"
    bug_report_version = f"{raw_version}"
    today_str = datetime.now().strftime("%Y-%m-%d")
    dry_run = args.dry_run

    issue_template_path = ".github/ISSUE_TEMPLATE/bug_report.md"
    changelog_path = "CHANGELOG.md"
    cmakelists_path = "CMakeLists.txt"

    print(f"\n🚀 {CLR_BOLD}{CLR_CYAN}STARTING RELEASE PROCESS FOR {tag_name} ({today_str}) -> Remote: {remote}{CLR_RESET}")
    if dry_run:
        print(f"⚠️  {CLR_YELLOW}RUNNING IN DRY-RUN MODE — No files will be modified, no git commands executed.{CLR_RESET}")

    # --- PRE-FLIGHT CHECKS ---
    check_git_repository()
    verify_remote_exists(remote)
    check_dirty_working_tree(dry_run)
    check_and_resolve_behind_remote(remote, dry_run)
    check_and_resolve_tag_conflicts(tag_name, remote, dry_run)

    TOTAL_STEPS = 5

    # -----------------------------------------------------------------
    # Step 1: Update CHANGELOG.md
    # -----------------------------------------------------------------
    print_step(1, TOTAL_STEPS, "Parsing CHANGELOG.md & Updating Release Header", dry_run)
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
    else:
        with open(changelog_path, "w", encoding="utf-8") as f:
            f.write(updated_changelog)
        print(f"    ✅ {CLR_GREEN}Updated {changelog_path}{CLR_RESET}")

    # -----------------------------------------------------------------
    # Step 2: Update Issue Template & CMakeLists.txt
    # -----------------------------------------------------------------
    print_step(2, TOTAL_STEPS, "Updating Project Version Files", dry_run)

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
    print_step(3, TOTAL_STEPS, f"Creating Commit and Tag '{tag_name}'", dry_run)
    
    staged_files = [f for f in [issue_template_path, changelog_path, cmakelists_path] if os.path.exists(f)]
    run_git_cmd(["git", "add"] + staged_files, dry_run)
    run_git_cmd(["git", "commit", "-m", f"Release {tag_name}"], dry_run)
    run_git_cmd(["git", "tag", "-a", tag_name, "-m", f'"{release_notes}"'], dry_run)

    # -----------------------------------------------------------------
    # Step 4: Push Commit & Tag to Remote with Error Recovery
    # -----------------------------------------------------------------
    print_step(4, TOTAL_STEPS, f"Pushing Release Commit & Tag to '{remote}'", dry_run)
    push_with_retry(remote, tag_name, dry_run)

    # -----------------------------------------------------------------
    # Step 5: Prepare Next Development Cycle in CHANGELOG.md
    # -----------------------------------------------------------------
    print_step(5, TOTAL_STEPS, f"Setting up CHANGELOG.md for Next Cycle ({raw_version}+ (???))", dry_run)
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
    run_git_cmd(["git", "commit", "-m", f'"Start v{raw_version}+"'], dry_run)
    push_with_retry(remote, tag_name, dry_run)

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