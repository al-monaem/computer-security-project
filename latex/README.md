# LaTeX build of the final report

A two-column, conference-paper rendering of the project report
(*From Stack Smashing to Code Reuse*). Same content as the top-level
`Fall2026_Interim Project Report_From Stack Smashsing to Code Reuse.pdf`,
in an IEEE-style layout.

It uses the stock `article` class with a custom preamble (Times via
`newtxtext`, `booktabs`, `listings`, `hyperref`), so **IEEEtran is not
required** — it builds on a standard TeX Live.

## Build

```bash
make            # -> report.pdf  (runs pdflatex twice for refs/labels)
# or:
pdflatex report.tex && pdflatex report.tex
# or:
latexmk -pdf report.tex
```

Output: `report.pdf` (4 pages).

```bash
make clean      # remove *.aux/*.log/… intermediates
make purge      # also remove report.pdf
```

## Files

| File         | What it is                                             |
|--------------|--------------------------------------------------------|
| `report.tex` | The paper source.                                      |
| `Makefile`   | `make` / `make clean` / `make purge`.                  |
| `report.pdf` | Pre-built PDF (regenerate with `make`).                |

> Stage 2 (NX protection + return-to-libc) is written up as a placeholder:
> its design is final, but the captured demo evidence is still being added.
