"""Summarize opt-in UI-thread capture; swap return is not monitor presentation."""
import csv
import json
import math
import sys
from collections import defaultdict

rows = list(csv.DictReader(open(sys.argv[1], encoding='utf-8')))
groups = defaultdict(list)
for row in rows:
    groups[row['event']].append(float(row['duration_ms']))
def stats(values):
    values = sorted(values)
    return dict(n=len(values), mean_ms=sum(values)/len(values),
        p95_ms=values[math.ceil(len(values)*.95)-1], max_ms=values[-1])
result = {event: stats(values) for event, values in groups.items()}
first_present = {}
for row in rows:
    seq = int(row['input_sequence'])
    if row['event'] == 'render' and seq > 0 and seq not in first_present:
        first_present[seq] = float(row['input_age_ms'])
if first_present:
    result['input_to_swap_return'] = stats(list(first_present.values()))
print(json.dumps(result, indent=2))
