#!/usr/bin/env python3
"""Export all messages from a Discord channel to JSON."""

import json
import os
import sys
import time
import urllib.request
import urllib.error

CHANNEL_ID = "396381546988306443"
TOKEN = os.environ["DISCORD_TOKEN"]
OUTPUT_FILE = os.path.join(os.path.dirname(__file__), "suggestions", "suggestions_raw.json")

API_BASE = "https://discord.com/api/v10"
HEADERS = {
    "Authorization": TOKEN,
    "User-Agent": "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)",
}


def fetch_messages(before=None, limit=100):
    url = f"{API_BASE}/channels/{CHANNEL_ID}/messages?limit={limit}"
    if before:
        url += f"&before={before}"

    req = urllib.request.Request(url, headers=HEADERS)
    try:
        with urllib.request.urlopen(req) as resp:
            data = json.loads(resp.read().decode())
            return data
    except urllib.error.HTTPError as e:
        body = e.read().decode()
        print(f"HTTP {e.code}: {body}")
        if e.code == 429:
            retry = json.loads(body).get("retry_after", 5)
            print(f"Rate limited, waiting {retry}s...")
            time.sleep(retry + 0.5)
            return fetch_messages(before, limit)
        raise


def main():
    os.makedirs(os.path.dirname(OUTPUT_FILE), exist_ok=True)

    all_messages = []
    before = None
    batch = 0

    while True:
        batch += 1
        messages = fetch_messages(before=before)

        if not messages:
            break

        all_messages.extend(messages)
        before = messages[-1]["id"]
        print(f"Batch {batch}: fetched {len(messages)} messages (total: {len(all_messages)})")

        if len(messages) < 100:
            break

        time.sleep(0.5)  # Be nice to the API

    # Sort chronologically (oldest first)
    all_messages.sort(key=lambda m: m["id"])

    with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
        json.dump(all_messages, f, indent=2, ensure_ascii=False)

    print(f"\nDone! Exported {len(all_messages)} messages to {OUTPUT_FILE}")


if __name__ == "__main__":
    main()
