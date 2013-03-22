GPL = $(sort $(wildcard *.gpl))
EPS=$(GPL:.gpl=.ps)
PDF=$(EPS:.ps=.pdf)

%.ps: %.gpl $(GPL)
	gnuplot $<

%.pdf: %.ps $(EPS)
	epstopdf $<

all: $(PDF) epsclean

epsclean:
	rm *.ps

clean:
	rm *.pdf 
