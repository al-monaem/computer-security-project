# Build the LaTeX report. Requires a TeX Live with pdflatex (no IEEEtran needed).
#
#   make        -> report.pdf  (two pdflatex passes for refs/labels)
#   make clean  -> remove LaTeX build intermediates
#   make purge  -> also remove the built PDF

TEX    := report.tex
PDF    := report.pdf
LATEX  := pdflatex -interaction=nonstopmode -halt-on-error

.PHONY: all clean purge

all: $(PDF)

$(PDF): $(TEX)
	$(LATEX) $(TEX)
	$(LATEX) $(TEX)

clean:
	rm -f *.aux *.log *.out *.toc *.lof *.lot *.fls *.fdb_latexmk pg-*.jpg

purge: clean
	rm -f $(PDF)
