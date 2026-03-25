#!/usr/bin/env python3
"""
Discord Reply Queue Manager

Maps fixes/features to original Discord messages and manages draft replies.
Run with subcommands:
    python3 docs/discord_reply_queue.py search "thirst crash"     # Find related messages
    python3 docs/discord_reply_queue.py list                      # Show all queued replies
    python3 docs/discord_reply_queue.py stats                     # Show stats
    python3 docs/discord_reply_queue.py send                      # Send all approved replies (requires DISCORD_TOKEN)
    python3 docs/discord_reply_queue.py send --dry-run             # Preview without sending
"""

import json
import os
import re
import sys
import time
import urllib.request
import urllib.error

QUEUE_FILE = os.path.join(os.path.dirname(__file__), "discord_reply_queue.json")
INDEX_FILE = os.path.join(os.path.dirname(__file__), "discord_message_index.json")

CHANNEL_NAMES = {
    "451291891401490443": "#bug-reports",
    "396381546988306443": "#suggestions",
    "378630008060641283": "#dev-discussion",
}


def load_index():
    with open(INDEX_FILE) as f:
        return json.load(f)


def load_queue():
    if os.path.exists(QUEUE_FILE):
        with open(QUEUE_FILE) as f:
            return json.load(f)
    return {"replies": [], "sent": []}


def save_queue(queue):
    with open(QUEUE_FILE, "w", encoding="utf-8") as f:
        json.dump(queue, f, indent=2, ensure_ascii=False)


def search_messages(query, limit=20):
    """Search the message index for relevant messages."""
    index = load_index()
    words = query.lower().split()

    scored = []
    for msg in index:
        content_lower = msg["content"].lower()
        score = sum(2 for w in words if w in content_lower)
        # Bonus for exact phrase
        if query.lower() in content_lower:
            score += 5
        # Bonus for reactions
        score += msg["reactions"] * 0.5
        # Bonus for longer messages (more substantive)
        score += min(len(msg["content"]) / 200, 2)

        if score > 0:
            scored.append((score, msg))

    scored.sort(key=lambda x: -x[0])
    return scored[:limit]


def cmd_search(args):
    query = " ".join(args)
    results = search_messages(query)
    if not results:
        print(f"No messages found for: {query}")
        return

    print(f"Top matches for '{query}':\n")
    for i, (score, msg) in enumerate(results):
        channel = CHANNEL_NAMES.get(msg["channel_id"], msg["channel_id"])
        content = msg["content"][:200].replace("\n", " ")
        print(f"{i+1}. [{score:.1f}] {channel} | {msg['date']} | {msg['author']}")
        print(f"   ID: {msg['id']}")
        print(f"   {content}")
        print()


def cmd_list(args):
    queue = load_queue()
    if not queue["replies"]:
        print("No replies queued.")
        return

    # Group by feature/fix
    by_feature = {}
    for r in queue["replies"]:
        key = r["feature"]
        by_feature.setdefault(key, []).append(r)

    print(f"=== {len(queue['replies'])} replies queued across {len(by_feature)} features ===\n")
    for feature, replies in by_feature.items():
        status = replies[0].get("status", "draft")
        print(f"[{status.upper()}] {feature} ({len(replies)} replies)")
        for r in replies:
            channel = CHANNEL_NAMES.get(r["channel_id"], r["channel_id"])
            print(f"  → {channel} | {r['original_author']} ({r['original_date']})")
            print(f"    Draft: {r['reply_text'][:100]}")
        print()

    sent = queue.get("sent", [])
    if sent:
        print(f"\n=== {len(sent)} replies already sent ===")


def cmd_stats(args):
    queue = load_queue()
    replies = queue["replies"]
    sent = queue.get("sent", [])

    statuses = {}
    for r in replies:
        s = r.get("status", "draft")
        statuses[s] = statuses.get(s, 0) + 1

    features = set(r["feature"] for r in replies)
    channels = set(r["channel_id"] for r in replies)

    print(f"Queued replies: {len(replies)}")
    print(f"Features mapped: {len(features)}")
    print(f"Channels: {len(channels)}")
    print(f"Already sent: {len(sent)}")
    print(f"\nBy status:")
    for s, c in sorted(statuses.items()):
        print(f"  {s}: {c}")


def cmd_send(args):
    dry_run = "--dry-run" in args
    token = os.environ.get("DISCORD_TOKEN")
    if not token and not dry_run:
        print("Error: DISCORD_TOKEN not set. Use --dry-run to preview.")
        sys.exit(1)

    queue = load_queue()
    approved = [r for r in queue["replies"] if r.get("status") == "approved"]

    if not approved:
        print("No approved replies to send. Approve replies first by setting status to 'approved'.")
        return

    print(f"{'[DRY RUN] ' if dry_run else ''}Sending {len(approved)} replies...\n")

    for r in approved:
        channel = CHANNEL_NAMES.get(r["channel_id"], r["channel_id"])
        print(f"→ {channel} | reply to {r['original_author']} ({r['original_date']})")
        print(f"  {r['reply_text'][:150]}")

        if not dry_run:
            try:
                url = f"https://discord.com/api/v10/channels/{r['channel_id']}/messages"
                payload = json.dumps({
                    "content": r["reply_text"],
                    "message_reference": {"message_id": r["message_id"]},
                }).encode("utf-8")
                headers = {
                    "Authorization": token,
                    "User-Agent": "Mozilla/5.0",
                    "Content-Type": "application/json",
                }
                req = urllib.request.Request(url, data=payload, headers=headers, method="POST")
                with urllib.request.urlopen(req) as resp:
                    data = json.loads(resp.read().decode())
                    r["sent_id"] = data["id"]
                    r["sent_at"] = data["timestamp"]
                    print(f"  ✓ Sent (ID: {data['id']})")
            except urllib.error.HTTPError as e:
                body = e.read().decode()
                print(f"  ✗ Failed: HTTP {e.code}: {body}")
                if e.code == 429:
                    retry = json.loads(body).get("retry_after", 5)
                    print(f"  Rate limited, waiting {retry}s...")
                    time.sleep(retry + 1)
                continue

            # Move to sent
            queue["sent"].append(r)
            queue["replies"].remove(r)
            save_queue(queue)
            time.sleep(1)  # Be nice to the API

        print()

    if dry_run:
        print(f"\n[DRY RUN] Would have sent {len(approved)} replies. Run without --dry-run to send.")
    else:
        print(f"\nSent {len(approved)} replies.")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    cmd = sys.argv[1]
    args = sys.argv[2:]

    if cmd == "search":
        cmd_search(args)
    elif cmd == "list":
        cmd_list(args)
    elif cmd == "stats":
        cmd_stats(args)
    elif cmd == "send":
        cmd_send(args)
    else:
        print(f"Unknown command: {cmd}")
        print(__doc__)
