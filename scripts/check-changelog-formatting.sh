#!/bin/sh

# Run the script and capture the output
output=$(node scripts/format-changelog.js CHANGELOG.md)

# Check if node script ran successfully
if [ $? -eq 0 ]; then
    echo "Script ran successfully. Running diff..."
    
    # Pass $output via stdin (-) to git diff
    printf '%s\n' "$output" | git diff --color --no-index - CHANGELOG.md
    
    if [ $? -ne 0 ]; then
        printf "\nDifferences found. You can apply the changes locally with:\n"
        printf "  node scripts/format-changelog.js CHANGELOG.md | git diff --no-index - CHANGELOG.md | git apply\n"
        exit 1
    else
        echo "No differences found. Exiting with status 0."
        exit 0
    fi
else
    echo "Script encountered an error:"
    printf '%s\n' "$output"
    exit 1
fi