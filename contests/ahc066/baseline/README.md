# Baseline 27055

This directory stores the current safe baseline source.

- Source: `baseline/main_27055.cpp`
- SHA-256: `6dfa2b56d851019e9ced857702c52c1805edbd2f46a5a1e5bf2d7c1d0982869d`
- Build command: `g++ -std=gnu++17 -O2 -Wall -Wextra -o /private/tmp/ahc066_baseline baseline/main_27055.cpp`
- Verification command: `python3 -B tools/evaluate.py --solver baseline=/private/tmp/ahc066_baseline --cases 'in/*.txt' --timeout 3`

Verified on 2026-05-31:

- 100-case absolute score: `27055`
- first50 absolute score: `12630`
- `bad = 0`
- max `output_len - T`: `-337`
- max `expanded_len - T`: `-290`
- max runtime in local run: `1486.380 ms` on `0000`

Use this as the first solver in every local experiment comparison.
