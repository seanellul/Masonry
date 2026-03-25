#!/usr/bin/env python3
"""Process Discord suggestions: group conversations, extract suggestions, categorize."""

import json
import re
from collections import defaultdict
from datetime import datetime, timedelta

with open("docs/suggestions/suggestions_cleaned.json") as f:
    msgs = json.load(f)

# --- Step 1: Group into conversation threads (gap > 30 min = new thread) ---
threads = []
current_thread = []
for m in msgs:
    ts = datetime.fromisoformat(m["timestamp"].replace("+00:00", ""))
    m["_ts"] = ts
    if current_thread:
        gap = (ts - current_thread[-1]["_ts"]).total_seconds()
        if gap > 1800:  # 30 min gap
            threads.append(current_thread)
            current_thread = []
    current_thread.append(m)
if current_thread:
    threads.append(current_thread)

print(f"Grouped {len(msgs)} messages into {len(threads)} conversation threads")

# --- Step 2: For each thread, extract the core suggestion(s) ---
# Heuristics: longer messages, first messages, messages with suggestion keywords
SUGGESTION_WORDS = re.compile(
    r"\b(suggest|idea|should|could|would be nice|what about|how about|maybe|"
    r"please|add|implement|feature|option|allow|enable|ability|wish|want|need|"
    r"request|propose|thought|improve|better|instead of|it.d be|can we|why not)\b",
    re.IGNORECASE,
)

# Known dev usernames (their responses are context, not suggestions)
DEVS = {".roest", "roest"}

CATEGORY_PATTERNS = {
    "UI/UX": re.compile(r"\b(ui|ux|menu|button|screen|window|interface|scroll|click|tooltip|display|panel|hud|font|icon)\b", re.I),
    "Combat/Military": re.compile(r"\b(combat|military|fight|attack|defend|weapon|armor|squad|patrol|soldier|guard|war|battle|enemy|goblin|mant)\b", re.I),
    "Farming/Agriculture": re.compile(r"\b(farm|crop|plant|grow|harvest|seed|field|grove|tree|agriculture|food|cook|brew|mushroom)\b", re.I),
    "Mining/Resources": re.compile(r"\b(mine|mining|ore|metal|stone|dig|resource|vein|gem|coal|iron|copper|gold|silver|lava)\b", re.I),
    "Crafting/Workshop": re.compile(r"\b(craft|workshop|forge|smelt|anvil|loom|saw|tool|recipe|produce|create|build|construct)\b", re.I),
    "Gnome Behavior/AI": re.compile(r"\b(pathfind|ai|behavior|schedule|sleep|eat|drink|mood|happy|emotion|idle|job|task|priority|trapped)\b", re.I),
    "Magic/Spells": re.compile(r"\b(magic|spell|enchant|mana|rune|arcane|wizard|mage|heal|curse|ritual|totem|potion)\b", re.I),
    "Animals/Creatures": re.compile(r"\b(animal|creature|pet|tame|breed|yak|chicken|dog|cat|mount|beast|livestock|pasture)\b", re.I),
    "World/Environment": re.compile(r"\b(world|map|biome|season|weather|rain|snow|temperature|river|lake|ocean|terrain|surface)\b", re.I),
    "Modding/Customization": re.compile(r"\b(mod|modding|custom|lua|script|config|json|xml|tweak|adjust)\b", re.I),
    "Stockpile/Storage": re.compile(r"\b(stockpile|storage|store|container|barrel|crate|chest|warehouse|inventory)\b", re.I),
    "Trading/Economy": re.compile(r"\b(trade|trader|merchant|buy|sell|value|worth|economy|market|gold|money|coin|price)\b", re.I),
    "Building/Architecture": re.compile(r"\b(wall|floor|door|window|roof|stair|ramp|bridge|furniture|bed|chair|table|room|house)\b", re.I),
    "Automation/Mechanics": re.compile(r"\b(automat|mechanism|gear|axle|lever|pump|pipe|piston|machine|engine|power|steam|water)\b", re.I),
    "Social/Religion": re.compile(r"\b(social|religion|god|worship|temple|priest|pray|faith|friend|relation|marriage|family|child)\b", re.I),
    "Performance/Technical": re.compile(r"\b(performance|fps|lag|crash|bug|memory|load|save|multithre|optimi|render)\b", re.I),
}


def score_suggestion(msg):
    """Score how likely a message is an actual suggestion vs conversation."""
    score = 0
    content = msg["content"]
    # Length bonus
    score += min(len(content) / 50, 5)
    # Suggestion keywords
    score += len(SUGGESTION_WORDS.findall(content)) * 2
    # Question marks (often suggestions phrased as questions)
    score += content.count("?") * 0.5
    # Not from dev
    if msg["author"] not in DEVS:
        score += 2
    # Has reactions
    score += msg["reactions"] * 3
    # Penalty for very short
    if len(content) < 30:
        score -= 3
    # Penalty for messages that are just banter
    if re.search(r"(lol|lmao|haha|😂|🤣|xd)", content, re.I):
        score -= 2
    # Penalty for links without text
    if re.match(r"^https?://", content) and len(content) < 100:
        score -= 3
    return score


def categorize(text):
    """Return matching categories for a text."""
    cats = []
    for cat, pattern in CATEGORY_PATTERNS.items():
        if pattern.search(text):
            cats.append(cat)
    return cats if cats else ["General/Other"]


# Extract suggestions from threads
extracted = []
for thread in threads:
    # Combine all text for categorization
    combined_text = " ".join(m["content"] for m in thread)

    # Score each message
    scored = [(score_suggestion(m), m) for m in thread]
    scored.sort(key=lambda x: x[0], reverse=True)

    # Take the best-scoring message(s) as the "suggestion"
    best_score, best_msg = scored[0]
    if best_score < 2:
        continue  # Skip threads that don't look like suggestions

    # Also collect supporting context from other high-scoring messages
    supporting = []
    for s, m in scored[1:5]:
        if s >= 2 and m["id"] != best_msg["id"]:
            supporting.append(m["content"][:150])

    categories = categorize(combined_text)

    extracted.append({
        "suggestion": best_msg["content"],
        "author": best_msg["author"],
        "date": best_msg["timestamp"],
        "reactions": best_msg["reactions"],
        "categories": categories,
        "score": round(best_score, 1),
        "thread_size": len(thread),
        "thread_date": thread[0]["timestamp"],
        "supporting_context": supporting[:3],
        "message_id": best_msg["id"],
    })

# Sort by score descending
extracted.sort(key=lambda x: x["score"], reverse=True)

print(f"Extracted {len(extracted)} suggestion threads")

# Category stats
cat_counts = defaultdict(int)
for s in extracted:
    for c in s["categories"]:
        cat_counts[c] += 1
print("\nCategory breakdown:")
for cat, count in sorted(cat_counts.items(), key=lambda x: -x[1]):
    print(f"  {cat}: {count}")

# Save
with open("docs/suggestions/suggestions_extracted.json", "w", encoding="utf-8") as f:
    json.dump(extracted, f, indent=2, ensure_ascii=False)

print(f"\nSaved to docs/suggestions/suggestions_extracted.json")

# Also generate a readable markdown summary
with open("docs/suggestions/suggestions_summary.md", "w", encoding="utf-8") as f:
    f.write("# Ingnomia Community Suggestions\n\n")
    f.write(f"Extracted from Discord #suggestions channel ({msgs[0]['timestamp'][:10]} to {msgs[-1]['timestamp'][:10]})\n\n")
    f.write(f"**{len(extracted)} suggestion threads** from {len(threads)} conversations\n\n")
    f.write("## Category Breakdown\n\n")
    f.write("| Category | Count |\n|----------|-------|\n")
    for cat, count in sorted(cat_counts.items(), key=lambda x: -x[1]):
        f.write(f"| {cat} | {count} |\n")
    f.write("\n---\n\n")

    # Group by category
    by_cat = defaultdict(list)
    for s in extracted:
        for c in s["categories"]:
            by_cat[c].append(s)

    for cat in sorted(by_cat.keys()):
        suggestions = sorted(by_cat[cat], key=lambda x: -x["score"])
        f.write(f"## {cat}\n\n")
        for s in suggestions:
            reactions = f" [{s['reactions']}⭐]" if s['reactions'] > 0 else ""
            content = s["suggestion"][:300].replace("\n", " ")
            f.write(f"- **{s['author']}** ({s['date'][:10]}){reactions}: {content}\n")
            if s["supporting_context"]:
                for ctx in s["supporting_context"]:
                    f.write(f"  - _Context: {ctx[:150].replace(chr(10), ' ')}_\n")
        f.write("\n")

print("Generated docs/suggestions/suggestions_summary.md")
