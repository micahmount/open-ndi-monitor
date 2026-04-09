#!/bin/bash
# Auto-increment version script
# Usage: ./scripts/increment-version.sh

set -e

CMAKE_FILE="CMakeLists.txt"

# Read current version
VERSION=$(grep -oP 'VERSION \K[0-9]+\.[0-9]+\.[0-9]+' "$CMAKE_FILE")

# Split into components
MAJOR=$(echo "$VERSION" | cut -d. -f1)
MINOR=$(echo "$VERSION" | cut -d. -f2)
PATCH=$(echo "$VERSION" | cut -d. -f3)

# Increment patch
PATCH=$((PATCH + 1))

NEW_VERSION="${MAJOR}.${MINOR}.${PATCH}"

# Update CMakeLists.txt
sed -i "s/VERSION [0-9]\+\.[0-9]\+\.[0-9]\+/VERSION ${NEW_VERSION}/" "$CMAKE_FILE"

echo "Version bumped: $VERSION -> $NEW_VERSION"