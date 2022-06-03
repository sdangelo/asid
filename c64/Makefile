asid.prg: asid.asm boot.prg barmapping.dat cutoffmapping.dat lfoscaling.dat bitmap.dat colormap1.dat colormap2.dat colormap3.dat colormap4.dat
	acme asid.asm

boot.prg: boot.bas
	bas2prg < boot.bas > boot.prg

%.dat: ../octave/%.m
	octave $<
