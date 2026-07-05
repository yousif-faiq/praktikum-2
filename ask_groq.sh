#!/usr/bin/env bash
#   Send a C source file to the Groq API and ask the LLM to predict
#   the runtime output — WITHOUT explaining its reasoning.
#
#   Prompt is structured to make analysis harder:
#     • Strips comments before submission
#     • Shuffles top-level blocks (non-sequential presentation)
#     • Injects a noise preamble
#     • Asks only for final stdout, no intermediate steps
#
# USAGE
#   export GROQ_API_KEY=
#   chmod +x ask_llm.sh
#   ./ask_llm.sh deep_chain.c
#
# REQUIREMENTS
#   bash, curl, python3

set -euo pipefail

# Config
# Available Groq models:
#   llama-3.3-70b-versatile
#   llama3-70b-8192
#   mixtral-8x7b-32768
#   gemma2-9b-it
MODEL="openai/gpt-oss-120b"
MAX_TOKENS=5000
API_URL="https://api.groq.com/openai/v1/chat/completions"

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <source.c> [source2.c ...]"
    exit 1
fi

if [[ -z "${GROQ_API_KEY:-}" ]]; then
    echo "ERROR: GROQ_API_KEY is not set."
    echo "  Get a free key at: https://console.groq.com"
    echo "  Then run: export GROQ_API_KEY=\"gsk_...\""
    exit 1
fi

# Step 1 — Strip C comments (removes inline hints)
strip_comments() {
    local file="$1"
    python3 - "$file" <<'PYEOF'
import sys, re

with open(sys.argv[1], "r") as f:
    src = f.read()

src = re.sub(r'/\*.*?\*/', '', src, flags=re.DOTALL)  # block comments
src = re.sub(r'//[^\n]*', '', src)                     # line comments
src = re.sub(r'\n{3,}', '\n\n', src)                   # collapse blank lines
print(src.strip())
PYEOF
}

# Step 2 — Shuffle top-level blocks (non-sequential presentation)
shuffle_blocks() {
    local file="$1"
    python3 - "$file" <<'PYEOF'
import sys, re, random

with open(sys.argv[1], "r") as f:
    src = f.read()

blocks = re.split(r'\n{2,}(?=\S)', src)

# Keep first 2 blocks (#include / #define) in place, shuffle the rest
if len(blocks) > 2:
    header = blocks[:2]
    body   = blocks[2:]
    random.shuffle(body)
    blocks = header + body

print('\n\n'.join(blocks))
PYEOF
}

# Step 3 — Noise preamble
noise_preamble() {
    cat <<'NOISE'
The following is an extract from a larger multi-module system.
Some declarations originate in other translation units.
Assume standard C99, hosted environment, little-endian architecture,
sizeof(int)==4, sizeof(uint32_t)==4, CHAR_BIT==8.
The program is compiled with gcc -O0 -std=c99.
Treat all arithmetic as wrapping (no UB). Treat all shifts as logical.
NOISE
}

# Step 4 — JSON-escape 
json_escape() {
    python3 -c "import sys,json; print(json.dumps(sys.stdin.read()))"
}

# Step 5 — Assemble prompt
build_prompt() {
    local file="$1"
    {
        noise_preamble
        echo ""
        echo "=== BEGIN C SOURCE ==="
        cat "$file"
        echo "=== END C SOURCE ==="
        echo ""
        echo "State ONLY the exact characters printed to stdout when this program runs."
        echo "Do not explain. Do not show working. One line per output line."
        echo "If output cannot be determined statically, write: UNKNOWN"
    }
}

# Main loop
for SRC in "$@"; do
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  File  : $SRC"
    echo "  Model : $MODEL"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    TMP_STRIPPED=$(mktemp /tmp/groq_strip_XXXX.c)
    TMP_SHUFFLED=$(mktemp /tmp/groq_shuf_XXXX.c)
    trap 'rm -f "$TMP_STRIPPED" "$TMP_SHUFFLED"' EXIT

    strip_comments "$SRC"          > "$TMP_STRIPPED"
    shuffle_blocks "$TMP_STRIPPED" > "$TMP_SHUFFLED"

    PROMPT=$(build_prompt "$TMP_SHUFFLED" | json_escape)

    PAYLOAD=$(cat <<JSON
{
  "model": "${MODEL}",
  "max_tokens": ${MAX_TOKENS},
  "temperature": 0,
  "messages": [
    {
      "role": "system",
      "content": "You are a C runtime emulator. Output  what the program prints to stdout. do a basic explanation for the code."
    },
    {
      "role": "user",
      "content": ${PROMPT}
    }
  ]
}
JSON
)

    echo "[*] Sending to Groq API…"

    RESPONSE=$(curl -s -X POST "$API_URL" \
        -H "Content-Type: application/json" \
        -H "Authorization: Bearer ${GROQ_API_KEY}" \
        -d "$PAYLOAD")

    LLM_OUTPUT=$(echo "$RESPONSE" | python3 -c "
import sys, json
try:
    d = json.load(sys.stdin)
    # Check for API errors
    if 'error' in d:
        print(f'[API Error] {d[\"error\"].get(\"message\", d[\"error\"])}')
    else:
        text = d['choices'][0]['message']['content']
        print(text)
except Exception as e:
    print(f'[Parse error: {e}]')
    print(sys.stdin.read())
")

    echo ""
    echo "LLM predicted output:"
    echo "$LLM_OUTPUT"
    echo ""

    rm -f "$TMP_STRIPPED" "$TMP_SHUFFLED"
    trap - EXIT
done