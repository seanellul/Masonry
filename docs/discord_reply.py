#!/usr/bin/env python3
"""Reply to a Discord message in a channel.

Usage:
    DISCORD_TOKEN=xxx python3 docs/discord_reply.py <channel_id> <message_id> <reply_text>
"""

import json
import os
import sys
import urllib.request
import urllib.error

TOKEN = os.environ["DISCORD_TOKEN"]
API_BASE = "https://discord.com/api/v10"
HEADERS = {
    "Authorization": TOKEN,
    "User-Agent": "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)",
    "Content-Type": "application/json",
}


def reply_to_message(channel_id, message_id, content):
    url = f"{API_BASE}/channels/{channel_id}/messages"
    payload = json.dumps({
        "content": content,
        "message_reference": {
            "message_id": message_id,
        },
    }).encode("utf-8")

    req = urllib.request.Request(url, data=payload, headers=HEADERS, method="POST")
    try:
        with urllib.request.urlopen(req) as resp:
            data = json.loads(resp.read().decode())
            print(f"Replied to message {message_id}")
            print(f"Reply ID: {data['id']}")
            return data
    except urllib.error.HTTPError as e:
        body = e.read().decode()
        print(f"HTTP {e.code}: {body}")
        raise


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: DISCORD_TOKEN=xxx python3 docs/discord_reply.py <channel_id> <message_id> <reply_text>")
        sys.exit(1)

    channel_id = sys.argv[1]
    message_id = sys.argv[2]
    content = " ".join(sys.argv[3:])
    reply_to_message(channel_id, message_id, content)
